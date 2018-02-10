/* Copyright (C) 2004-2006  Martin Förg
 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *  
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 *  
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

 *
 * Contact:
 *   martin.o.foerg@googlemail.com
 *
 */

#include <config.h>
#include <mbsim/dynamic_system_solver.h>
#include "fortran/fortran_wrapper.h"
#include "radau5_integrator.h"
#include <ctime>
#include <fstream>

#ifndef NO_ISO_14882
using namespace std;
#endif

using namespace fmatvec;
using namespace MBSim;
using namespace MBXMLUtils;
using namespace xercesc;

namespace MBSimIntegrator {

  MBSIM_OBJECTFACTORY_REGISTERCLASS(MBSIMINT, RADAU5Integrator)

  void RADAU5Integrator::fzdotODE(int* zSize, double* t, double* z_, double* zd_, double* rpar, int* ipar) {
    auto self=*reinterpret_cast<RADAU5Integrator**>(&ipar[0]);
    Vec zd(*zSize, zd_);
    self->getSystem()->setTime(*t);
    self->getSystem()->setState(Vec(*zSize, z_));
    self->getSystem()->resetUpToDate();
    zd = self->getSystem()->evalzd();
  }

  void RADAU5Integrator::fzdotDAE(int* neq, double* t, double* y_, double* yd_, double* rpar, int* ipar) {
    auto self=*reinterpret_cast<RADAU5Integrator**>(&ipar[0]);
    int nq = self->system->getqSize();
    int nu = self->system->getuSize();
    int nx = self->system->getxSize();
    Vec y(*neq, y_);
    Vec yd(*neq, yd_);
    self->getSystem()->setTime(*t);
    self->getSystem()->setState(y(0,nq+nu+nx-1));
    self->getSystem()->resetUpToDate();
    yd(0,nq-1) = self->system->evaldq();
    yd(nq,nq+nu-1) = slvLLFac(self->system->evalLLM(), self->system->evalh() + self->system->evalV()*y(nq+nu+nx,*neq-1));
    yd(nq+nu,nq+nu+nx-1) = self->system->evaldx();
    yd(nq+nu+nx,*neq-1) = self->system->evalgd();
  }

  void RADAU5Integrator::mass(int* zSize, double* m_, int* lmas, double* rpar, int* ipar) {
    auto self=*reinterpret_cast<RADAU5Integrator**>(&ipar[0]);
    Mat M(*lmas,*zSize, m_);
    for(int i=0; i<self->system->getzSize(); i++) M(0,i) = 1;
  }

  void  RADAU5Integrator::plot(int* nr, double* told, double* t, double* z, double* cont, int* lrc, int* n, double* rpar, int* ipar, int* irtrn) {
    auto self=*reinterpret_cast<RADAU5Integrator**>(&ipar[0]);

    while(*t >= self->tPlot) {
      self->getSystem()->setTime(self->tPlot);
      for(int i=1; i<=self->system->getzSize(); i++)
	self->getSystem()->getState()(i-1) = CONTR5(&i,&self->tPlot,cont,lrc);
      self->getSystem()->resetUpToDate();
      self->getSystem()->plot();
      if(self->output)
	cout << "   t = " <<  self->tPlot << ",\tdt = "<< *t-*told << "\r"<<flush;

      double s1 = clock();
      self->time += (s1-self->s0)/CLOCKS_PER_SEC;
      self->s0 = s1; 

      if(self->plotIntegrationData) self->integPlot<< self->tPlot << " " << *t-*told << " " << self->time << endl;
      self->tPlot += self->dtOut;
    }
  }

  void RADAU5Integrator::integrate() {
    debugInit();

    int zSize = system->getzSize();
    int neq = formalism?zSize+system->getgdSize():zSize;

    double t = tStart;

    Vec y(neq);
    Vec z = y(0,zSize-1);
    if(z0.size()) {
      if(z0.size() != zSize)
        throw MBSimError("(RADAU5Integrator::integrate): size of z0 does not match");
      y = z0;
    }
    else
      z = system->evalz0();

    if(aTol.size() == 0) 
      aTol.resize(1,INIT,1e-6);
    if(rTol.size() == 0) 
      rTol.resize(1,INIT,1e-6);

    assert(aTol.size() == rTol.size());

    int iTol;
    if(aTol.size() == 1) {
      iTol = 0; // Skalar
    } else {
      iTol = 1; // Vektor
      assert (aTol.size() >= zSize);
    }

    int out = 1; // subroutine is available for output

    double rPar;
    int iPar[sizeof(void*)/sizeof(int)+1];
    RADAU5Integrator *self=this;
    memcpy(&iPar[0], &self, sizeof(void*));

    int lWork = 2*(neq*(neq+neq+3*neq+12)+20);
    int liWork = 2*(3*neq+20);
    VecInt iWork(liWork);
    Vec work(lWork);
    if(dtMax>0)
      work(6) = dtMax; // maximum step size
    iWork(1) = maxSteps; // maximum number of steps
    if(formalism) {
      iWork(4) = zSize;
      iWork(5) = system->getgdSize();
    }

    int iJac = 0; // jacobian is computed internally by finite differences
    int mlJac = neq; // jacobian is a full matrix
    int muJac = mlJac; // need not to be defined if mlJac = neq
    int iMas = formalism; // mass-matrix
    int mlMas = 0; // lower bandwith of the mass-matrix
    int muMas = 0; // upper bandwith of the mass-matrix
    int idid;

    tPlot = t + dtPlot;
    dtOut = dtPlot;

    system->setStepSize(1);
    system->setTime(t);
    system->setState(z);
    system->resetUpToDate();
    system->plot();

    if(plotIntegrationData) {
      integPlot.open((name + ".plt").c_str());

      integPlot << "#1 t [s]:" << endl;
      integPlot << "#1 dt [s]:" << endl;
      integPlot << "#1 calculation time [s]:" << endl;
    }

    cout.setf(ios::scientific, ios::floatfield);

    s0 = clock();

    RADAU5(&neq,formalism?fzdotDAE:fzdotODE,&t,y(),&tEnd,&dt0,
	rTol(),aTol(),&iTol,
	nullptr,&iJac,&mlJac,&muJac,
	mass,&iMas,&mlMas,&muMas,
	plot,&out,
	work(),&lWork,iWork(),&liWork,&rPar,iPar,&idid);

    if(plotIntegrationData) integPlot.close();

    if(writeIntegrationSummary) {
      ofstream integSum((name + ".sum").c_str());
      integSum << "Integration time: " << time << endl;
      //integSum << "Integration steps: " << integrationSteps << endl;
      integSum.close();
    }

    cout.unsetf (ios::scientific);
    cout << endl;
  }

  void RADAU5Integrator::initializeUsingXML(DOMElement *element) {
    Integrator::initializeUsingXML(element);
    DOMElement *e;
    e=E(element)->getFirstElementChildNamed(MBSIMINT%"absoluteTolerance");
    if(e) setAbsoluteTolerance(E(e)->getText<Vec>());
    e=E(element)->getFirstElementChildNamed(MBSIMINT%"absoluteToleranceScalar");
    if(e) setAbsoluteTolerance(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIMINT%"relativeTolerance");
    if(e) setRelativeTolerance(E(e)->getText<Vec>());
    e=E(element)->getFirstElementChildNamed(MBSIMINT%"relativeToleranceScalar");
    if(e) setRelativeTolerance(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIMINT%"initialStepSize");
    if(e) setInitialStepSize(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIMINT%"maximumStepSize");
    if(e) setMaximumStepSize(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIMINT%"stepLimit");
    if(e) setStepLimit(E(e)->getText<int>());
    e=E(element)->getFirstElementChildNamed(MBSIMINT%"formalism");
    if(e) {
      string formalismStr=string(X()%E(e)->getFirstTextChild()->getData()).substr(1,string(X()%E(e)->getFirstTextChild()->getData()).length()-2);
      if(formalismStr=="ODE") formalism=ODE;
      else if(formalismStr=="DAE") formalism=DAE;
    }
  }

}
