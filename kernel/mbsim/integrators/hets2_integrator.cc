/* Copyright (C) 2004-2014 MBSim Development Team
 *
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
 * Contact: thorsten.schindler@mytum.de
 */

#include <config.h>
#include "hets2_integrator.h"
#include <mbsim/dynamic_system_solver.h>

#include <ctime>
#include <boost/iostreams/tee.hpp>
#include <boost/iostreams/stream.hpp>

#ifndef NO_ISO_14882
using namespace std;
#endif

namespace bio = boost::iostreams;
using bio::tee_device;
using bio::stream;

using namespace fmatvec;
using namespace MBSim;
using namespace MBXMLUtils;
using namespace xercesc;

namespace MBSimIntegrator {

  MBSIM_OBJECTFACTORY_REGISTERCLASS(MBSIMINT, HETS2Integrator)

  void HETS2Integrator::preIntegrate() {
    debugInit();

    // set the time
    assert(dtPlot >= dt);
    system->setTime(tStart);

    // define initial state
    if(z0.size()) {
      if(z0.size() != system->getzSize())
        throwError("(HETS2Integrator::integrate): size of z0 does not match, must be " + toStr(system->getzSize()));
      system->setState(z0);
    }
    else
      system->evalz0();

    system->setUseOldla(false);
    system->setGeneralizedForceTolerance(1e-10/dt); // adaptation from impulse
    system->setGeneralizedRelativeAccelerationTolerance(1e-10/dt); // as we use local velocities to express accelerations within solveConstraints

    // prepare plotting
    if(plotIntegrationData) {
      integPlot.open((name + ".plt").c_str());
      integPlot << "time" << " " << "time step-size" << " " <<  "constraint iterations" << " " << "calculation time" << " " << "size of constraint system" << endl;
    }

    assert(fabs(((int) (dtPlot/dt + 0.5))*dt - dtPlot) < dt*dt);

    // start timing
    s0 = clock();
  }

  void HETS2Integrator::subIntegrate(double tStop) {

    while(system->getTime()<tStop) { // time loop
      system->resetUpToDate();

      // increase integration step counter
      integrationSteps++;

      /* LEFT INTERVAL END EVALUATIONS = ZERO STAGE EVALUATIONS */
      // update until the Jacobian matrices, especially also the active set
      evaluateStage();

      // save values
      Vec qStage0 = system->getq().copy();
      Vec uStage0 = system->getu().copy();
      Mat TStage0 = system->evalT().copy();
      SymMat LLMStage0 = system->evalLLM().copy();
      Vec hStage0 = system->evalh().copy();

      // plot
      if(system->getTime() >= tPlot) {
        system->setUpdatela(false);
        system->setUpdateLa(false);
        system->plot();
        double s1 = clock();
        time += (s1-s0)/CLOCKS_PER_SEC;
        s0 = s1; 
        if(plotIntegrationData) integPlot << system->getTime() << " " << dtInfo << " " <<  system->getIterC() << " " << time << " "<< system->getlaSize() << endl;
        if(msgAct(Status)) msg(Status) << "   t = " << system->getTime() << ",\tdt = "<< dtInfo << ",\titer = " << setw(5) << setiosflags(ios::left) << system->getIterC() << flush;
        tPlot += dtPlot;
      }
      /*****************************************/

      /* CHECK IMPACTS */
      // first stage position and time update (velocity is unknown and has to be calculated with constraints/impacts)
      system->getq() += system->evalT()*system->getu()*dt;
      system->getTime() += dt;

      system->resetUpToDate();

      // update until the Jacobian matrices, especially also the active set
      bool impact = evaluateStage(); // TODO: this also updates the active set!

      /* CALCULATE CONSTRAINT FORCES ON VELOCITY LEVEL AND FIRST STAGE */
      if (not impact) {

        // increase integration step counter for constraints
        integrationStepsConstraint++;

        // adapt last time step-size
        dtInfo = dt;

        system->getbc(false) << system->evalgd()/dt + system->evalW().T()*slvLLFac(LLMStage0,hStage0);
        system->setUpdatebc(false);

        // save values
        Vec laStage0 = system->evalla().copy();
        Mat VStage0 = system->evalV().copy();

        if(system->getIterC()>maxIter) maxIter = system->getIterC();
        sumIter += system->getIterC();

        // first stage velocity update
        system->getu() += slvLLFac(LLMStage0,hStage0+VStage0*laStage0)*dt;
        /*****************************************/

        /* CALCULATE CONSTRAINT FORCES ON VELOCITY LEVEL AND OUTPUT STAGE */
        // output stage position update
        system->getq() = qStage0 + (system->evalT()*system->getu()+TStage0*uStage0)*dt*0.5; // T-matrix in the sense of Brasey1994a, velocity is unknown and has to be calculated with constraint forces

        system->resetUpToDate();

        // update until the Jacobian matrices, especially also the active set
        evaluateStage();

        // output stage velocity update without constraint force
        system->getu() = uStage0 + (slvLLFac(LLMStage0,hStage0+VStage0*laStage0) + slvLLFac(system->evalLLM(),system->evalh()))*dt*0.5;

        system->resetUpToDate();

        // update until the Jacobian matrices, especially also the active set
        evaluateStage();

        system->getbc(false) << 2.*system->evalgd()/dt;
        system->setUpdatebc(false);

        // output stage velocity update
        system->getu() += slvLLFac(system->evalLLM(),system->evalV()*system->evalla())*dt*0.5;

        if(system->getIterC()>maxIter) maxIter = system->getIterC();
        sumIter += system->getIterC();

        system->resetUpToDate();
      }
      /*****************************************/

      /* CALCULATE IMPACTS ON VELOCITY LEVEL AND OUTPUT STAGE */
      else {

        // increase integration step counter for impacts
        integrationStepsImpact++;

        // adapt last time step-size
        dtInfo = dtImpulsive;

        // first stage position and time update (velocity is unknown and has to be calculated with constraints/impacts)
        system->getq() = qStage0 + system->evalT()*system->getu()*dtImpulsive;
        system->getTime() += dtImpulsive-dt;

        // first stage velocity update
        system->getu() += slvLLFac(LLMStage0,hStage0)*dtImpulsive;

        // output stage position update
        system->getq() = qStage0 + (system->getT()*system->getu()+TStage0*uStage0)*dtImpulsive*0.5; // T-matrix in the sense of Brasey1994a, velocity is unknown and has to be calculated with impacts

        system->resetUpToDate();

        // update until the Jacobian matrices, especially also the active set
        evaluateStage();

        // output stage velocity update without impact
        system->getu() = uStage0 + (slvLLFac(LLMStage0,hStage0) + slvLLFac(system->evalLLM(),system->evalh()))*dtImpulsive*0.5;

        system->resetUpToDate();

        // update until the Jacobian matrices, especially also the active set
        evaluateStage();

        // output stage velocity update
        system->getu() += slvLLFac(system->evalLLM(),system->evalV()*system->evalLa());

        if(system->getIterI()>maxIter)
          maxIter = system->getIterI();
        sumIter += system->getIterI();
      }
      /*****************************************/
    }
  }

  void HETS2Integrator::postIntegrate() {
    if(plotIntegrationData) integPlot.close();

    if(writeIntegrationSummary) {
      typedef tee_device<ostream, ofstream> TeeDevice;
      typedef stream<TeeDevice> TeeStream;
      ofstream integSum((name + ".sum").c_str());
      TeeDevice hets2_tee(msg(Info), integSum);
      TeeStream hets2_split(hets2_tee);

      hets2_split << endl << endl << "******************************" << endl;
      hets2_split << "INTEGRATION SUMMARY: " << endl;
      hets2_split << "End time [s]: " << tEnd << endl;
      hets2_split << "Integration time [s]: " << time << endl;
      hets2_split << "Integration steps: " << integrationSteps << endl;
      hets2_split << "Fraction of impulsive integration steps: " << double(integrationStepsImpact)/integrationSteps << endl;
      hets2_split << "Maximum number of iterations: " << maxIter << endl;
      hets2_split << "Average number of iterations: " << double(sumIter)/integrationSteps << endl;
      hets2_split << "******************************" << endl;
      hets2_split.flush();
      hets2_split.close();
    }
  }

  void HETS2Integrator::integrate() {
    system->setUseConstraintSolverForPlot(true);
    preIntegrate();
    subIntegrate(tEnd);
    postIntegrate();
  }

  void HETS2Integrator::initializeUsingXML(DOMElement *element) {
    Integrator::initializeUsingXML(element);
    DOMElement *e;
    e=E(element)->getFirstElementChildNamed(MBSIMINT%"stepSize");
    if(e) setStepSize(E(e)->getText<double>());
  }

  bool HETS2Integrator::evaluateStage() {
    system->checkActive(1);

    bool impact = system->detectImpact();

    // adapt size of constraint system on velocity level
    if (system->gActiveChanged()) {
      system->calcgdSize(2); // contacts which stay closed
      system->calclaSize(2); // contacts which stay closed
      system->calcrFactorSize(2); // contacts which stay closed

      system->updateWRef(system->getWParent(0)(RangeV(0, system->getuSize() - 1), RangeV(0, system->getlaSize() - 1)));
      system->updateVRef(system->getVParent(0)(RangeV(0, system->getuSize() - 1), RangeV(0, system->getlaSize() - 1)));
      system->updatelaRef(system->getlaParent()(0, system->getlaSize() - 1));
      system->updateLaRef(system->getLaParent()(0, system->getlaSize() - 1));
      system->updategdRef(system->getgdParent()(0, system->getgdSize() - 1));
      system->updaterFactorRef(system->getrFactorParent()(0, system->getrFactorSize() - 1));
    }

    return impact; 
  }

}
