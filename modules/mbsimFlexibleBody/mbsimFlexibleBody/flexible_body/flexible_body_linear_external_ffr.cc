/* Copyright (C) 20004-2011 MBSim Development Team
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

#include<config.h>
#include "flexible_body_linear_external_ffr.h"
#include "mbsimFlexibleBody/contours/nurbs_disk_2s.h"
#include "mbsim/dynamic_system.h"
#include "mbsim/utils/eps.h"
#include <mbsim/utils/utils.h>
#include <mbsim/utils/rotarymatrices.h>
#include <mbsim/contours/sphere.h>
#include <mbsim/frames/fixed_relative_frame.h>
#include "mbsim/mbsim_event.h"

#include "openmbvcppinterface/nurbsdisk.h"

using namespace std;
using namespace fmatvec;
using namespace MBSim;

namespace MBSimFlexibleBody {
  
  FlexibleBodyLinearExternalFFR::FlexibleBodyLinearExternalFFR(const string &name) :
      FlexibleBodyContinuum<Vec>(name), nNodes(0), FFR(new Frame("FFR")), I_1(INIT, 0.0), I_kl(INIT, 0.0), S_bar(), I_kl_bar(), I_ThetaTheta_bar(INIT, 0.0), I_ThetaF_bar(), K(), G_bar(INIT, 0.0), G_bar_Dot(INIT, 0.0), A(INIT, 0), M_FF(), Qv(), phi(), nf(1), openStructure(true), updAG(true), updQv(true) {
    for (auto & i : S_kl_bar) {
      for (auto & j : i)
        j = SqrMat();
    }
    Body::addFrame(FFR);
//    FFR->setParent(this);
  }
  
  void FlexibleBodyLinearExternalFFR::init(InitStage stage, const InitConfigSet &config) {
    if (stage == preInit) {
      qSize = 6+nf;
      uSize[0] = qSize;
      uSize[1] = qSize; // TODO
      nn = nNodes;
      nodeNumbers.resize(nNodes);
      for(int i=0; i<nNodes; i++)
        nodeNumbers[i] = i+1;
    }
    else if (stage == unknownStage) {
      // only the modes can be used that are available
      if(nf > phiFull.cols()) {
        nf = phiFull.cols();
        msg(Warn) << "only able to use nf=" << nf << " modes" << endl;
      }

      phi.resize(3 * nNodes, nf, NONINIT);
      for(int col =0; col < nf; col++) {
        phi.set(col, phiFull.col(col));
      }

      I_ThetaF_bar.resize(3, nf, INIT, 0.);

      K.resize(6 + nf, INIT, 0);

      SymMat Kff(phi.T() * KFull * phi);

      for (int i = 6; i < 6 + nf; i++)
        for (int j = i; j < 6 + nf; j++)
          K(i, j) = Kff(i - 6, j - 6);
      // read the input data file: mode shape vector, u0, mij, K.
//      readFEMData();  // move to public function, has to be executed before the init().
      
      assert(nNodes > 1);
      // at least two azimuthal elements

      // create Elements and store them into discretization vector
      // the node index in abaqus strats from 1, but here starts from 0.
      for (int j = 0; j < nNodes; j++) {
        discretization.push_back(new FiniteElementLinearExternalLumpedNode(mij(j), u0[j], phi(RangeV(3 * j, 3 * j + 2), RangeV(0, nf - 1))));
      }

      // compute the inertia shape integrals
      computeShapeIntegrals();

      updateAGbarGbardot();

      // set mode shape vector for each element.
//      BuildElements();  //
      
      initM(); // calculate constant stiffness matrix and the constant parts of the mass-matrix
      
      initQv();
    }

    FlexibleBodyContinuum<Vec>::init(stage, config);
  }
  
  void FlexibleBodyLinearExternalFFR::calcqSize() {
    qSize = 6 + nf;
  }

  void FlexibleBodyLinearExternalFFR::calcuSize(int j) {
    uSize[j] = 6 + nf;
  }

  void FlexibleBodyLinearExternalFFR::readFEMData(const string &inFilePath, const bool millimeterUnits, bool output) {

    msg(Info) << "Reading FEM-Data" << endl;

    // if SI-milimeter units instead of SI units are used the can be converted to SI-meter (SI-standard) units
    // please refer to the Abaqus-manual for more information
    double power = 1;
    if (millimeterUnits)
      power = 1000;

    /* read u0 */
    ifstream u0Infile(inFilePath + "/u0.dat");
    if (!u0Infile.is_open()) {
      msg(Error) << "Can not open file " << inFilePath << "u0.dat" << endl;
      throw 1;
    }
    for (string s; getline(u0Infile, s);) {
      fmatvec::Vec u0Line(3);
      istringstream sin(s);
      int i = 0;
      for (double temp; sin >> temp; i++) {
        u0Line(i) = temp;
      }
      u0.emplace_back(u0Line / power);
    }

    // get the number of lumped nodes(nj) = number of numOfElements in the model
    nNodes = u0.size();
    msg(Info) << "The number of nodes is " << nNodes << endl;

    if (output) {
      msg(Info) << "... with the following initial positions" << endl;
      for (int i = 0; i < nNodes; i++) {
        msg(Info) << i << ". " << u0[i] << endl;
      }
    }

    mij.resize(nNodes, NONINIT);

    /* read mij */
    ifstream mijInfile(inFilePath + "/mij.dat");
    if (!mijInfile.is_open()) {
      msg(Error) << "Can not open file " << inFilePath << "mij.dat" << endl;
      throw 1;
    }

    double totalMass = 0;

    int i = 0;
    for (double a; mijInfile >> a; i++) {
      mij(i) = a * power;
      totalMass += mij(i);
    }

    msg(Info) << "The total mass is " << totalMass << " [kg]" <<  endl;

    
    if (output) {
      msg(Info) << "The lumped masses are: " << mij << endl;
    }
    
    // get the number of mode shapes(nf) used to describe the deformation
    ifstream phiInfile(inFilePath + "/modeShapeMatrix.dat");
    if (!phiInfile.is_open()) {
      msg(Error) << "Can not open file " << inFilePath << "modeShapeMatrix.dat" << endl;
      throw 1;
    }
    string s1;
    std::vector<double> phiLine;
    getline(phiInfile, s1);
    istringstream sin1(s1);
    for (double temp; sin1 >> temp;)
      phiLine.push_back(temp);
    int nfFull = phiLine.size();
    phiInfile.close();
    
    // read mode shape matrix
    phiFull.resize(3 * nNodes, nfFull, INIT, 0.0);
    phiInfile.open((inFilePath + "/modeShapeMatrix.dat").c_str());
    if (!phiInfile.is_open()) {
      msg(Error) << "Can not open file " << inFilePath << "modeShapeMatrix.dat" << endl;
      throw 1;
    }
    int row = 0;
    for (string s; getline(phiInfile, s); row++) {
      istringstream sin(s);  // TODO: use sin.clear() to avoid creating sin every time
      int i = 0;
      for (double temp; sin >> temp; i++)
        phiFull(row, i) = temp;
    }

    // read stiffness matrix
    KFull.resize(3 * nNodes, INIT, 0.0);
    ifstream KInfile(inFilePath + "/stiffnessMatrix.dat");
    if (!KInfile.is_open()) {
      msg(Error) << "Can not open file " << inFilePath << "stiffnessMatrix.dat" << endl;
      throw 1;
    }
    for (string s; getline(KInfile, s);) {
      std::vector<double> KLine;
      istringstream sin(s);
      for (string tempxx; getline(sin, tempxx, ',');) {
        KLine.push_back(boost::lexical_cast<double>(boost::algorithm::trim_copy(tempxx)));  // atof()change char to double
      }
      KFull(3 * KLine[0] + KLine[1] - 4, 3 * KLine[2] + KLine[3] - 4) = KLine[4] * power;
    }
    
    if (msgAct(Debug)) {

      msg(Debug).precision(6);
      msg(Debug) << "numOfElements = " << nNodes << endl;
      msg(Debug) << "nf = " << nfFull << endl;
      
      msg(Debug) << "phi" << phi << endl;
//      phi >> "phi.out";

//      msg(Debug) << "KFull" << KFull << endl;
//      msg(Debug) << "K" << KFull << endl;

      msg(Debug) << "mij 0: " << mij(0) << endl;
      msg(Debug) << "mij 1: " << mij(1) << endl;
      msg(Debug) << "mij 2: " << mij(2) << endl;
      msg(Debug) << "mij last one" << mij(nNodes - 1) << endl;

      msg(Debug) << "u0[0] = " << u0.at(0) << endl;
      msg(Debug) << "u0[1] = " << u0.at(1) << endl;
      msg(Debug) << "u0[2] = " << u0.at(2) << endl;
      msg(Debug) << "u0[" << nNodes - 3 << "] = " << u0.at(nNodes - 3) << endl;
      msg(Debug) << "u0[" << nNodes - 2 << "] = " << u0.at(nNodes - 2) << endl;
      msg(Debug) << "u0[" << nNodes - 1 << "] = " << u0.back() << endl;
      msg(Debug) << "u0.at(1)(2)" << u0.at(1)(2) << endl;

      msg(Debug) << "phi node0,1 = " << phiFull(RangeV(0, 0), RangeV(0, nfFull - 1)) << endl;
      msg(Debug) << "phi node0,2 = " << phiFull(RangeV(1, 1), RangeV(0, nfFull - 1)) << endl;
      msg(Debug) << "phi node0,3 = " << phiFull(RangeV(2, 2), RangeV(0, nfFull - 1)) << endl;
      msg(Debug) << "phi node" << nNodes << ",1 = " << phiFull(RangeV(3 * nNodes - 3, 3 * nNodes - 3), RangeV(0, nfFull - 1)) << endl;
      msg(Debug) << "phi node" << nNodes << ",2 = " << phiFull(RangeV(3 * nNodes - 2, 3 * nNodes - 2), RangeV(0, nfFull - 1)) << endl;
      msg(Debug) << "phi node" << nNodes << ",3 = " << phiFull(RangeV(3 * nNodes - 1, 3 * nNodes - 1), RangeV(0, nfFull - 1)) << endl;
    }

  }
  
  void FlexibleBodyLinearExternalFFR::enableFramePlot(double size, VecInt numbers) {
    if (numbers.size() == 0) { //take all nodes
      numbers.resize(nNodes, NONINIT);
      for (int i = 1; i <= nNodes; i++) {
        numbers(i - 1) = i;
      }
    }

    for (int i = 0; i < numbers.size(); i++) {
      int nodeNumber = numbers(i);
      auto * refFrame = new NodeFrame("RefFrame" + toString(nodeNumber), nodeNumber);
      addFrame(refFrame);
      refFrame->enableOpenMBV(size);
    }
  }

  void FlexibleBodyLinearExternalFFR::initM() {
    
    for (int j = 0; j < nNodes; j++)
      M_RR += static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getMij();

    // M_FF: nf*nf constant during the simulation;
    M_FF.resize(nf, INIT, 0.0);
    M_FF = S_kl_bar[0][0] + S_kl_bar[1][1] + S_kl_bar[2][2];
  }
  
  void FlexibleBodyLinearExternalFFR::updateM() {
    // update M_RTheta, M_ThetaTheta, M_RF, M_ThetaF;  M_RR and M_FF is constant
    
    const fmatvec::Vec& qf = q(RangeV(6, 5 + nf));
    
    // M_RTheta: 3*3
    fmatvec::Vec S_bar_t = I_1 + S_bar * qf;
    fmatvec::SqrMat M_RTheta((-evalA()) * tilde(S_bar_t) * evalG_bar());
    
    // M_ThetaTheta: 3*3
    
    double u11 = I_kl(0, 0) + 2. * I_kl_bar[0][0] * qf + qf.T() * S_kl_bar[0][0] * qf;
    double u22 = I_kl(1, 1) + 2. * I_kl_bar[1][1] * qf + qf.T() * S_kl_bar[1][1] * qf;
    double u33 = I_kl(2, 2) + 2. * I_kl_bar[2][2] * qf + qf.T() * S_kl_bar[2][2] * qf;
    double u21 = I_kl(1, 0) + (I_kl_bar[1][0] + I_kl_bar[0][1]) * qf + qf.T() * S_kl_bar[1][0] * qf;
    double u31 = I_kl(2, 0) + (I_kl_bar[2][0] + I_kl_bar[0][2]) * qf + qf.T() * S_kl_bar[2][0] * qf;
    double u32 = I_kl(2, 1) + (I_kl_bar[2][1] + I_kl_bar[1][2]) * qf + qf.T() * S_kl_bar[2][1] * qf;
    
    I_ThetaTheta_bar(0, 0) = u22 + u33;
    I_ThetaTheta_bar(0, 1) = -u21;
    I_ThetaTheta_bar(0, 2) = -u31;
    I_ThetaTheta_bar(1, 1) = u11 + u33;
    I_ThetaTheta_bar(1, 2) = -u32;
    I_ThetaTheta_bar(2, 2) = u11 + u22;
    
    fmatvec::SymMat3 M_ThetaTheta(G_bar.T() * I_ThetaTheta_bar * G_bar);
    
    // M_RF: 3*nf
    fmatvec::Mat M_RF = Mat(3, nf, INIT, 0.);
    M_RF = A * S_bar;
    
    I_ThetaF_bar.set(RangeV(0, 0), RangeV(0, nf - 1), qf.T() * (S_kl_bar[1][2] - S_kl_bar[2][1]) + (I_kl_bar[1][2] - I_kl_bar[2][1]));
    I_ThetaF_bar.set(RangeV(1, 1), RangeV(0, nf - 1), qf.T() * (S_kl_bar[2][0] - S_kl_bar[0][2]) + (I_kl_bar[2][0] - I_kl_bar[0][2]));
    I_ThetaF_bar.set(RangeV(2, 2), RangeV(0, nf - 1), qf.T() * (S_kl_bar[0][1] - S_kl_bar[1][0]) + (I_kl_bar[0][1] - I_kl_bar[1][0]));
    
    fmatvec::Mat M_ThetaF(G_bar.T() * I_ThetaF_bar);
    
    // update M by the submatrix M_RTheta, M_ThetaTheta, M_RF, M_ThetaF
    M.set(RangeV(0, 2), RangeV(3, 5), M_RTheta);
    M.set(RangeV(0, 2), RangeV(6, nf + 5), M_RF);

    M(0, 0) = M_RR;
    M(1, 1) = M_RR;
    M(2, 2) = M_RR;

    // copy M_FF to M in this way as the limitation of the submatrix opertor for symmetric matrix.
    for (int i = 6; i < nf + 6; i++)
      for (int j = i; j < nf + 6; j++)
        M(i, j) = M_FF(i - 6, j - 6);

    // copy M_ThetaTheta to M in this way as the limitation of the submatrix opertor for symmetric matrix.
    for (int i = 3; i < 6; i++)
      for (int j = i; j < 6; j++)
        M(i, j) = M_ThetaTheta(i - 3, j - 3);

    M.set(RangeV(3, 5), RangeV(6, nf + 5), M_ThetaF);
    
    if (msgAct(Debug)) {

      msg(Debug) << "q " << q << endl;
      msg(Debug) << "u " << u << endl;
      msg(Debug) << "I_kl(1,1) " << I_kl(0, 0) << endl;
      msg(Debug) << "I_kl_bar(1,1) " << I_kl_bar[0][0] << endl;
      msg(Debug) << "S_kl_bar(1,1) " << S_kl_bar[0][0] << endl;
      msg(Debug) << "M_RTheta " << M_RTheta << endl;
      msg(Debug) << "I_ThetaTheta_bar " << I_ThetaTheta_bar << endl;
      msg(Debug) << "M_ThetaTheta " << M_ThetaTheta << endl;
      msg(Debug) << "M_RF " << M_RF << endl;
      msg(Debug) << "I_ThetaF_bar " << I_ThetaF_bar << endl;
      msg(Debug) << "M_ThetaF " << M_ThetaF << endl;

      msg(Debug) << "S1" << S_kl_bar[1][2] - S_kl_bar[2][1] << endl;
      msg(Debug) << "S2" << S_kl_bar[2][0] - S_kl_bar[0][2] << endl;
      msg(Debug) << "S3" << S_kl_bar[0][1] - S_kl_bar[1][0] << endl;
      msg(Debug) << "I1 " << I_kl_bar[1][2] - I_kl_bar[2][1] << endl;
      msg(Debug) << "I2 " << I_kl_bar[2][0] - I_kl_bar[0][2] << endl;
      msg(Debug) << "I3 " << I_kl_bar[0][1] - I_kl_bar[1][0] << endl;
      msg(Debug) << "qf " << qf << endl;
    }

  }
  
  void FlexibleBodyLinearExternalFFR::initQv() {
    // initQv has to be called after initM
    Qv.resize(nf + 6, NONINIT);
    updateQv();
  }
  
  void FlexibleBodyLinearExternalFFR::updateQv() {
    
    fmatvec::Vec3 uTheta = u(RangeV(3, 5));
    fmatvec::Vec uf = u(RangeV(6, 5 + nf));
    fmatvec::Vec qf = q(RangeV(6, 5 + nf));
    fmatvec::SqrMat omega_bar_skew(tilde(evalG_bar() * uTheta));
    
    Qv.init(0.); // Qv has to be init every time step as for calculating Qv(6, nf + 5), the operator "+=" is used.

    Qv.set(RangeV(0, 2), (-A) * (omega_bar_skew * omega_bar_skew * (I_1 + S_bar * qf) + 2. * omega_bar_skew * S_bar * uf));
    Qv.set(RangeV(3, 5), -2. * G_bar_Dot.T() * I_ThetaTheta_bar * (G_bar * uTheta) - 2. * G_bar_Dot.T() * I_ThetaF_bar * uf - G_bar.T() * I_ThetaTheta_bar * (G_bar * uTheta));
    for (int j = 0; j < nNodes; j++) {
      auto* node = static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j]);
      Qv.add(RangeV(6, nf + 5), -node->getMij() * (node->getModeShape().T() * (omega_bar_skew * omega_bar_skew * (node->getU0() + node->getModeShape() * qf) + 2. * omega_bar_skew * node->getModeShape() * uf)));
    }

    if (msgAct(Debug)) {
      msg(Debug) << "Qv" << Qv << endl;
      msg(Debug) << "A" << A << endl;
      msg(Debug) << "uThe" << uTheta << endl;
      msg(Debug) << "qf" << qf << endl;
      msg(Debug) << "uf" << uf << endl;
      msg(Debug) << "omtild" << omega_bar_skew << endl;
      msg(Debug) << "G_bar" << G_bar << endl;
    }
    updQv = false;
  }
  
  void FlexibleBodyLinearExternalFFR::updateh(int k) {
    h[k] = evalQv() - K * q;

    if (msgAct(Debug)) {
      ofstream f;
      f.open("Variables.txt", fstream::in | fstream::out | fstream::trunc);
      f << "t" << getTime() << endl;
      f << "q" << q << endl;
      f << "K" << K << endl;
      f << "Qv" << Qv << endl;
      f << "h[" << k << "]" << h[k] << endl;
      f << "M[" << k << "]" << M << endl;
      f.close();
    }

  }

  void FlexibleBodyLinearExternalFFR::computeShapeIntegrals() {
    // all shape integrals are constant, need to be calculated one time only.
//    if (msgAct(Debug)){
//      for (int j = 0; j < Elements; j++) {
//           msg(Debug) << "phi entering computeShapeIntegrals" << static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getModeShape() << endl;
//      }
//    }
    //I_1  3*1,
    for (int j = 0; j < nNodes; j++) {
      I_1 += static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getMij() * static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getU0();
    }
    
    // I_kl: each element is a scalar, and k,l =1,2,3  need define a matrix to store these nine scalar
    // as I_kl is symmetric, the second loop starts at l=k.
    for (int k = 0; k < 3; k++) {
      for (int l = k; l < 3; l++) {
        for (int j = 0; j < nNodes; j++) {
          I_kl(k, l) += static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getMij() * static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getU0()(k) * static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getU0()(l);
        }
      }
    }

    // S_bar:  3*nf
    S_bar.resize(3, nf, INIT, 0.);
    for (int j = 0; j < nNodes; j++) {
      S_bar += static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getMij() * static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getModeShape();
    }
    
    //S_kl_bar each element is a nf*nf matrix and k,l = 1,2,3  need define a matrix to store these nine pointers of the nf*nf matrices
    for (int k = 0; k < 3; k++) {
      for (int l = 0; l < 3; l++) {
        S_kl_bar[k][l].resize(nf, INIT, 0.0);
        for (int j = 0; j < nNodes; j++) {
          S_kl_bar[k][l] += static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getMij() * trans(static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getModeShape().row(k)) * static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getModeShape().row(l);
        }
      }
    }
    
    //I_kl_bar  each element is a 1*nf row vector, and k,l =1,2,3  need define a matrix to store these nine vector

    for (int k = 0; k < 3; k++) {
      for (int l = 0; l < 3; l++) {
        I_kl_bar[k][l].resize(nf, INIT, 0.0);
        for (int j = 0; j < nNodes; j++) {
          I_kl_bar[k][l] += static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getMij() * static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getU0()(k) * static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[j])->getModeShape().row(l); // getModeShape() return matrix
        }
      }
    }
    
    if (msgAct(Debug)) {
      msg(Debug) << "I_1" << I_1 << endl << endl;
      msg(Debug) << "I_kl" << I_kl << endl << endl;
      msg(Debug) << "S_bar" << S_bar << endl << endl;
      msg(Debug) << "S_11_bar" << S_kl_bar[0][0] << endl << endl;
      msg(Debug) << "S_22_bar" << S_kl_bar[1][1] << endl << endl;
      msg(Debug) << "S_33_bar" << S_kl_bar[2][2] << endl << endl;
      msg(Debug) << "S_23_bar" << S_kl_bar[1][2] << endl << endl;
      msg(Debug) << "S_32_bar" << S_kl_bar[2][1] << endl << endl;
      msg(Debug) << "I_13_bar" << I_kl_bar[0][2] << endl << endl;
      msg(Debug) << "I_23_bar" << I_kl_bar[1][2] << endl << endl;
    }

  }
  
  void FlexibleBodyLinearExternalFFR::updateAGbarGbardot() {
    // use CardanPtr angle->computA() to compute A  and G, Gbar....

    double sinalpha = sin(q(3));
    double cosalpha = cos(q(3));
    double sinbeta = sin(q(4));
    double cosbeta = cos(q(4));
    double singamma = sin(q(5));
    double cosgamma = cos(q(5));
    double betadot = u(4);
    double gammadot = u(5);
    
    A(0, 0) = cosbeta * cosgamma;
    A(0, 1) = -cosbeta * singamma;
    A(0, 2) = sinbeta;
    
    A(1, 0) = cosalpha * singamma + sinalpha * sinbeta * cosgamma;
    A(1, 1) = cosalpha * cosgamma - sinalpha * sinbeta * singamma;
    A(1, 2) = -sinalpha * cosbeta;
    
    A(2, 0) = sinalpha * singamma - cosalpha * sinbeta * cosgamma;
    A(2, 1) = sinalpha * cosgamma + cosalpha * sinbeta * singamma;
    A(2, 2) = cosalpha * cosbeta;
    
    G_bar(0, 0) = cosbeta * cosgamma;
    G_bar(0, 1) = singamma;
    G_bar(0, 2) = 0.;
    
    G_bar(1, 0) = -cosbeta * singamma;
    G_bar(1, 1) = cosgamma;
    G_bar(1, 2) = 0.;
    
    G_bar(2, 0) = sinbeta;
    G_bar(2, 1) = 0.;
    G_bar(2, 2) = 1.;
    
    // update G_bar_Dot
    //TODO:: check whether the formula for G_bar_Dot is correct?
    G_bar_Dot(0, 0) = -sinbeta * cosgamma * betadot - cosbeta * singamma * gammadot;
    G_bar_Dot(0, 1) = cosgamma * gammadot;
    
    G_bar_Dot(1, 0) = sinbeta * singamma * betadot - cosbeta * cosgamma * gammadot;
    G_bar_Dot(1, 1) = -singamma * gammadot;
    
    G_bar_Dot(2, 0) = cosbeta * betadot;
    
    if (msgAct(Debug)) {

      msg(Debug) << "cosbeta" << cosbeta << endl << endl;
      msg(Debug) << "cosgamma" << cosgamma << endl << endl;
      msg(Debug) << "A" << A << endl << endl;
      msg(Debug) << "G_bar" << G_bar << endl << endl;
      msg(Debug) << "G_bar_Dot" << G_bar_Dot << endl << endl;

    }
    updAG = false;
  }

  void FlexibleBodyLinearExternalFFR::updatePositions(Frame *frame) {
    if (nrm2(R->evalVelocity()) > epsroot) {
      throwError("Only absolute description of FFR-bodies possible (right now)!");
    }
    frame->setOrientation(R->evalOrientation() * evalA());
    frame->setPosition(R->getPosition() + R->getOrientation() * q(RangeV(0, 2))); // transformation from Reference Frame R into world frame
  }

  void FlexibleBodyLinearExternalFFR::updateVelocities(Frame *frame) {
    if (nrm2(R->evalVelocity()) > epsroot) {
      throwError("Only absolute description of FFR-bodies possible (right now)!");
    }
    // Update kinematics part
    frame->setVelocity(R->evalOrientation() * u(RangeV(0, 2)));

    Vec3 omega_ref = evalA() * evalG_bar() * u(RangeV(3, 5));
    frame->setAngularVelocity(R->getOrientation() * omega_ref);
  }

  void FlexibleBodyLinearExternalFFR::updateAccelerations(Frame *frame) {
    throwError("(FlexibleBodyLinearExternalFFR::updateAccelerations(): Not implemented!");
  }

  void FlexibleBodyLinearExternalFFR::updateJacobians(Frame *frame, int j) {

    // update Jacobians at Frame
    Mat3xV Jactmp_trans(6 + nf, INIT, 0.);
    Mat3xV Jactmp_rot(6 + nf, INIT, 0.);

    Jactmp_trans.set(RangeV(0, 2), RangeV(0, 2), Mat3x3(EYE));

    Jactmp_rot.set(RangeV(0, 2), RangeV(3, 5), evalA() * evalG_bar());

    frame->setJacobianOfTranslation(R->evalOrientation() * Jactmp_trans);
    frame->setJacobianOfRotation(R->getOrientation() * Jactmp_rot);
  }

  void FlexibleBodyLinearExternalFFR::updateGyroscopicAccelerations(Frame *frame) {
    throwError("(FlexibleBodyLinearExternalFFR::updateGyroscopicAccelerations(): Not implemented!");
  }

  void FlexibleBodyLinearExternalFFR::updatePositions(int nodeIndex) {

    auto* node = static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[nodeIndex]);

    Vec3 u_bar = node->getU0() + node->getModeShape() * q(RangeV(6, 5 + nf));
    //        u_bar = A * u_bar; // A*u_bar: transform local position vector expressed in FFR into Reference Frame R
    //        u_bar += q(0, 2); // r_p = R + A*U_bar: add the translational displacement of FFR (based on the reference frame R) to the local position vector expressed in Reference Frame R

    WrOP[nodeIndex] = R->evalPosition() + R->evalOrientation() * (q(RangeV(0, 2)) + evalA() * u_bar); // transformation from Reference Frame R into world frame
    // TODO:  why in cosserat is there not  plus R->getPosition() ?

    // TODO: interpolate the position of lumped node to get a smooth surface, and then get A from that curve.
    SqrMat3 wA(R->getOrientation() * A);
    AWK[nodeIndex].set(0, wA.col(0));
    AWK[nodeIndex].set(1, wA.col(1));
    AWK[nodeIndex].set(2, wA.col(2));

    updNodalPos[nodeIndex] = false;
 }

  void FlexibleBodyLinearExternalFFR::updateVelocities(int nodeIndex) {

    auto* node = static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[nodeIndex]);

    Vec3 A_S_qfDot = evalA() * node->getModeShape() * u(RangeV(6, 5 + nf));

    Vec3 u_bar = node->getU0() + node->getModeShape() * q(RangeV(6, 5 + nf));

    Vec3 u_ref_1 = -A * tilde(u_bar) * G_bar * u(RangeV(3, 5));

    WvP[nodeIndex] = R->evalOrientation() * (u(RangeV(0, 2)) + u_ref_1 + A_S_qfDot);  // Schabana 5.15

    // In reality, each point on the flexible body should has a different angular rotation velocity omega as the effect of deformation. But as we assume the deformation is small and thus linearizable, in calculating
    // deformation u_f, we neglect the effect of the change of orientation, simply set the deformation to be equal a weighted summation of different modes vectors.
    // For kinematics, it means that we assume that every point in the flexible body have the same angular velocity omega of the FFR, neglecting the effect of deformation on angular velocity.

    Vec omega_ref = A * G_bar * u(RangeV(3, 5));

    Wom[nodeIndex] = R->getOrientation() * omega_ref;

    updNodalVel[nodeIndex] = false;
  }

  void FlexibleBodyLinearExternalFFR::updateAccelerations(int nodeIndex) {
    throwError("(FlexibleBodyLinearExternalFFR::updateAccelerations): Not implemented.");
  }

  void FlexibleBodyLinearExternalFFR::updateJacobians(int nodeIndex, int j) {

    // Jacobian of element
    Mat Jactmp_trans(3, 6 + nf, INIT, 0.), Jactmp_rot(3, 6 + nf, INIT, 0.); // initializing Ref + 1 Node

    // translational DOFs (d/dR)
    Jactmp_trans.set(RangeV(0, 2), RangeV(0, 2), SqrMat(3, EYE)); // ref
    Vec3 u_bar = static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[nodeIndex])->getU0() + static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[nodeIndex])->getModeShape() * q(RangeV(6, 5 + nf));
    Jactmp_trans.set(RangeV(0, 2), RangeV(3, 5), -evalA() * tilde(u_bar) * evalG_bar());
    Jactmp_rot.set(RangeV(0, 2), RangeV(3, 5), A * G_bar);

    // elastic DOFs
    // translation (A*phi)
    Jactmp_trans.set(RangeV(0, 2), RangeV(6, 5 + nf), A * static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[nodeIndex])->getModeShape());

    // rotation part for elastic DOFs is zero.

    // transformation
    WJP[j][nodeIndex] = R->evalOrientation() * Jactmp_trans;
    WJR[j][nodeIndex] = R->getOrientation() * Jactmp_rot;

    updNodalJac[j][nodeIndex] = false;
 }

  void FlexibleBodyLinearExternalFFR::updateGyroscopicAccelerations(int nodeIndex) {
    throwError("(FlexibleBodyLinearExternalFFR::updateGyroscopicAccelerations): Not implemented.");
  }

  Vec3 FlexibleBodyLinearExternalFFR::evalLocalPosition(int i) {
    auto* node = static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[getNodeIndex(i)]);

    return node->getU0() + node->getModeShape() * q(RangeV(6, 5 + nf)); // don't need to transform to the system coordinates,  needed to be done in neutral contour when calculating the Jacobian matrix
  }

  const Vec3 FlexibleBodyLinearExternalFFR::getModeShapeVector(int node, int column) const {
    return static_cast<FiniteElementLinearExternalLumpedNode*>(discretization[getNodeIndex(node)])->getModeShape().col(column);
  }

  void FlexibleBodyLinearExternalFFR::BuildElements() {
//    throwError("(FlexibleBodyLinearExternalFFR::BuildElements(): Not implemented");
  }

  void FlexibleBodyLinearExternalFFR::GlobalVectorContribution(int CurrentElement, const fmatvec::Vec& locVec, fmatvec::Vec& gloVec) {
    throwError("(FlexibleBodyLinearExternalFFR::GlobalVectorContribution(): Not implemented!");
  }

  void FlexibleBodyLinearExternalFFR::GlobalMatrixContribution(int CurrentElement, const fmatvec::Mat& locMat, fmatvec::Mat& gloMat) {
    throwError("(FlexibleBodyLinearExternalFFR::GlobalMatrixContribution(): Not implemented!");
  }

  void FlexibleBodyLinearExternalFFR::GlobalMatrixContribution(int CurrentElement, const fmatvec::SymMat& locMat, fmatvec::SymMat& gloMat) {
    throwError("(FlexibleBodyLinearExternalFFR::GlobalMatrixContribution(): Not implemented!");
  }

  double FlexibleBodyLinearExternalFFR::getLength() const {
    throwError("(FlexibleBodyLinearExternalFFR::getLength(): Not implemented!");
  }

  void FlexibleBodyLinearExternalFFR::resetUpToDate() {
    FlexibleBodyContinuum<Vec>::resetUpToDate();
    updAG = true;
    updQv = true;
  }

}

