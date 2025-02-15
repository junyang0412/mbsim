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
#include <mbsim/utils/eps.h>
#include "fortran/fortran_wrapper.h"
#include "lsode_integrator.h"
#include <time.h>

#ifndef NO_ISO_14882
using namespace std;
#endif

using namespace fmatvec;
using namespace MBSim;
using namespace MBXMLUtils;
using namespace xercesc;

namespace MBSim {

  MBSIM_OBJECTFACTORY_REGISTERCLASS(MBSIM, LSODEIntegrator)

  bool odePackInUse = false;

  void LSODEIntegrator::fzdot(int* neq, double* t, double* z_, double* zd_) {
    auto self=*reinterpret_cast<LSODEIntegrator**>(&neq[1]);
    Vec zd(neq[0], zd_);
    self->getSystem()->setTime(*t);
    self->getSystem()->resetUpToDate();
    zd = self->getSystem()->evalzd();
  }

  void LSODEIntegrator::integrate() {
    if(method==unknown)
      throwError("(LSODEIntegrator::integrate): method unknown");

    debugInit();

    if(odePackInUse)
      throwError("Only one integration with LSODARIntegrator, LSODERIntegrator and LSODEIntegrator at a time is possible.");
    odePackInUse = true;

    int zSize=system->getzSize();

    if(not zSize)
      throwError("(LSODEIntegrator::integrate): dimension of the system must be at least 1");

    int neq[1+sizeof(void*)/sizeof(int)+1];
    neq[0]=zSize;
    LSODEIntegrator *self=this;
    memcpy(&neq[1], &self, sizeof(void*));

    if(z0.size()) {
      if(z0.size() != zSize+system->getisSize())
        throwError("(LSODEIntegrator::integrate): size of z0 does not match, must be " + to_string(zSize+system->getisSize()));
      system->setState(z0(RangeV(0,zSize-1)));
      system->setcuris(z0(RangeV(zSize,z0.size()-1)));
    }
    else
      system->evalz0();

    double t = tStart;
    double tPlot = t + dtPlot;

    if(aTol.size() == 0)
      aTol.resize(1,INIT,1e-6);
    if(rTol.size() == 0)
      rTol.resize(1,INIT,1e-6);

    int iTol;
    if(rTol.size() == 1) {
      if(aTol.size() == 1)
        iTol = 1;
      else {
        iTol = 2;
        if(aTol.size() != zSize)
          throwError("(LSODEIntegrator::integrate): size of aTol does not match, must be " + to_string(zSize));
      }
    }
    else {
      if(aTol.size() == 1)
        iTol = 3;
      else {
        iTol = 4;
        if(aTol.size() != zSize)
          throwError("(LSODEIntegrator::integrate): size of aTol does not match, must be " + to_string(zSize));
      }
      if(rTol.size() != zSize)
        throwError("(LSODEIntegrator::integrate): size of rTol does not match, must be " + to_string(zSize));
    }

    int itask=2, iopt=1, istate=1;
    int lrWork = 2*(22+9*zSize+zSize*zSize);
    Vec rWork(lrWork);
    rWork(4) = dt0;
    rWork(5) = dtMax;
    rWork(6) = dtMin;
    int liWork = 2*(20+zSize);
    VecInt iWork(liWork);
    iWork(5) = maxSteps;

    system->setTime(t);
    system->resetUpToDate();
    system->computeInitialCondition();
    system->plot();
    svLast <<= system->evalsv();

    double s0 = clock();
    double time = 0;

    int MF = method;

    int zero = 0;
    int iflag;

    while(t<tEnd-epsroot) {
      DLSODE(fzdot, neq, system->getState()(), &t, &tEnd, &iTol, rTol(), aTol(),
          &itask, &istate, &iopt, rWork(), &lrWork, iWork(), &liWork, nullptr, &MF);
      if(istate==2 or istate==1) {
        double curTimeAndState = -1;
        double tRoot = t;

        // root-finding
        if(getSystem()->getsvSize()) {
          getSystem()->setTime(t);
          curTimeAndState = t;
          getSystem()->resetUpToDate();
          shift = signChangedWRTsvLast(getSystem()->evalsv());
          // if a root exists in the current step ...
          double dt = rWork(10);
          if(shift) {
            // ... search the first root and set step.second to this time
            while(dt>dtRoot) {
              dt/=2;
              double tCheck = tRoot-dt;
              curTimeAndState = tCheck;
              DINTDY(&tCheck, &zero, &rWork(20), neq, system->getState()(), &iflag);
              getSystem()->setTime(tCheck);
              getSystem()->resetUpToDate();
              if(signChangedWRTsvLast(getSystem()->evalsv()))
                tRoot = tCheck;
            }
            if(curTimeAndState != tRoot) {
              curTimeAndState = tRoot;
              DINTDY(&tRoot, &zero, &rWork(20), neq, system->getState()(), &iflag);
              getSystem()->setTime(tRoot);
            }
            getSystem()->resetUpToDate();
            auto &sv = getSystem()->evalsv();
            auto &jsv = getSystem()->getjsv();
            for(int i=0; i<sv.size(); ++i)
              jsv(i)=svLast(i)*sv(i)<0;
          }
        }

        while(tRoot>=tPlot and tPlot<=tEnd+epsroot) {
          if(curTimeAndState != tPlot) {
            curTimeAndState = tPlot;
            DINTDY(&tPlot, &zero, &rWork(20), neq, system->getState()(), &iflag);
            getSystem()->setTime(tPlot);
          }
          getSystem()->resetUpToDate();
          getSystem()->plot();
          if(msgAct(Status))
            msg(Status) << "   t = " <<  tPlot << ",\tdt = "<< rWork(10) << flush;

          getSystem()->updateInternalState();

          double s1 = clock();
          time += (s1-s0)/CLOCKS_PER_SEC;
          s0 = s1;

          tPlot += dtPlot;
        }

        if(shift) {
          // shift the system
          if(curTimeAndState != tRoot) {
            DINTDY(&tRoot, &zero, &rWork(20), neq, system->getState()(), &iflag);
            getSystem()->setTime(tRoot);
          }
          if(plotOnRoot) {
            getSystem()->resetUpToDate();
            getSystem()->plot();
          }
          getSystem()->resetUpToDate();
          getSystem()->shift();
          if(plotOnRoot) {
            getSystem()->resetUpToDate();
            getSystem()->plot();
          }
          istate=1;
        }
        else {
          // check drift
          bool projVel = true;
          if(gMax>=0) {
            getSystem()->setTime(t);
            getSystem()->resetUpToDate();
            if(getSystem()->positionDriftCompensationNeeded(gMax)) { // project both, first positions and then velocities
              getSystem()->projectGeneralizedPositions(3);
              getSystem()->projectGeneralizedVelocities(3);
              projVel = false;
              istate=1;
            }
          }
          if(gdMax>=0 and projVel) {
            getSystem()->setTime(t);
            getSystem()->resetUpToDate();
            if(getSystem()->velocityDriftCompensationNeeded(gdMax)) { // project velicities
              getSystem()->projectGeneralizedVelocities(3);
              istate=1;
            }
          }
          getSystem()->updateStopVectorParameters();
        }
        if(istate==1) {
          if(shift) {
            system->resetUpToDate();
            svLast = system->evalsv();
          }
          t = system->getTime();
        }

        getSystem()->updateInternalState();
      }
      else if(istate<0) throwError("Integrator LSODE failed with istate = "+to_string(istate));
    }

    odePackInUse = false;
  }

  void LSODEIntegrator::initializeUsingXML(DOMElement *element) {
    RootFindingIntegrator::initializeUsingXML(element);
    DOMElement *e;
    e=E(element)->getFirstElementChildNamed(MBSIM%"method");
    if(e) {
      string methodStr=string(X()%E(e)->getFirstTextChild()->getData()).substr(1,string(X()%E(e)->getFirstTextChild()->getData()).length()-2);
      if(methodStr=="nonstiff" or methodStr=="Adams") method=nonstiff;
      else if(methodStr=="stiff" or methodStr=="BDF") method=stiff;
      else method=unknown;
    }
    e=E(element)->getFirstElementChildNamed(MBSIM%"absoluteTolerance");
    if(e) setAbsoluteTolerance(E(e)->getText<Vec>());
    e=E(element)->getFirstElementChildNamed(MBSIM%"absoluteToleranceScalar");
    if(e) setAbsoluteTolerance(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIM%"relativeTolerance");
    if(e) setRelativeTolerance(E(e)->getText<Vec>());
    e=E(element)->getFirstElementChildNamed(MBSIM%"relativeToleranceScalar");
    if(e) setRelativeTolerance(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIM%"initialStepSize");
    if(e) setInitialStepSize(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIM%"maximumStepSize");
    if(e) setMaximumStepSize(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIM%"minimumStepSize");
    if(e) setMinimumStepSize(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIM%"stepLimit");
    if(e) setStepLimit(E(e)->getText<int>());
  }

}
