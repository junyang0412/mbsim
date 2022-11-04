/* Copyright (C) 2004-2021 MBSim Development Team
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
 * Contact: martin.o.foerg@gmail.com
 */

#include <config.h>
#include "mbsimControl/linear_system_analyzer.h"
#include "mbsimControl/namespace.h"
#include "mbsimControl/extern_signal_source.h"
#include "mbsimControl/extern_signal_sink.h"
#include "mbsim/dynamic_system_solver.h"
#include "mbsim/utils/eps.h"
#include "fmatvec/linear_algebra_complex.h"
#include "hdf5serie/simpledataset.h"

using namespace std;
using namespace fmatvec;
using namespace MBSim;
using namespace MBXMLUtils;
using namespace xercesc;

namespace MBSimControl {

  const Vector<Ref,double> fromComplex(const Vector<Ref,complex<double>> &x) {
    Vector<Ref,double> y(x.size(),NONINIT);
    for(int i=0; i<x.size(); i++)
      y(i) = x(i).real();
    return y;
  }

  MBSIM_OBJECTFACTORY_REGISTERCLASS(MBSIMCONTROL, LinearSystemAnalyzer)

  LinearSystemAnalyzer::~LinearSystemAnalyzer() {
    delete Amp;
  }

  void LinearSystemAnalyzer::execute() {
    vector<ExternSignalSource*> source;
    vector<ExternSignalSink*> sink;
    vector<Link*> links = system->getLinks();
    int nsource=0, nsink=0;
    for(size_t i=0; i<links.size(); i++) {
      if(dynamic_cast<ExternSignalSource*>(links[i])) {
	source.push_back(static_cast<ExternSignalSource*>(links[i]));
	nsource += source[source.size()-1]->getSignalSize();
      }
      if(dynamic_cast<ExternSignalSink*>(links[i])) {
	sink.push_back(static_cast<ExternSignalSink*>(links[i]));
	nsink += sink[sink.size()-1]->getSignalSize();
      }
    }

    if(not(z0.size()))
      zEq <<= system->evalz0();
    else {
      if(z0.size()!=system->getzSize()+system->getisSize())
	throwError(string("(LinearSystemAnalyzer::execute): size of z0 does not match, must be ") + to_string(system->getzSize()) +
	    ", but is " + to_string(z0.size()) + ".");
      zEq <<= z0(RangeV(0,system->getzSize()-1));
      system->setcuris(z0(RangeV(system->getzSize(),z0.size()-1)));
    }

    if(not(u0.size()))
      u0.resize(nsource);
    else if(u0.size()!=nsource)
      throwError(string("(LinearSystemAnalyzer::execute): size of u0 does not match, must be ") + to_string(nsource) +
	  ", but is " + to_string(u0.size()) + ".");

    if(fex.size()) {
      if(not Amp)
	throwError(string("(LinearSystemAnalyzer::execute): excitation amplitude function must be defined"));
      else if(Amp->getRetSize().first!=nsource)
	throwError(string("(LinearSystemAnalyzer::execute): size of excitation amplitude function does not match, must be ") + to_string(nsource) +
	    ", but is " + to_string(Amp->getRetSize().first) + ".");
      Amp->setParent(system);
      Amp->setName("ExcitationAmplitudeFunction");
      Amp->init(Element::preInit, InitConfigSet());
      if(not(phi.size()))
	phi.resize(nsource);
      else if(phi.size()!=nsource)
	throwError(string("(LinearSystemAnalyzer::execute): size of excitation phase vector does not match, must be ") + to_string(nsource) +
	    ", but is " + to_string(phi.size()) + ".");
    }

    SqrMat A(system->getzSize(),NONINIT);
    Mat B(system->getzSize(),nsource,NONINIT);
    Mat D(nsink,nsource,NONINIT);
    Mat C(nsink,system->getzSize(),NONINIT);

    system->setTime(t0);
    system->setState(zEq);
    int l=0;
    map<int,pair<int,int>> smap;
    for(size_t i=0; i<source.size(); i++) {
      Vec sigso(source[i]->getSignalSize());
      for(int k=0; k<source[i]->getSignalSize(); k++) {
	sigso(k) = u0(l);
	source[i]->setSignal(sigso);
	l++;
	smap[l] = make_pair(i,k);
      }
    }
    system->resetUpToDate();
    system->computeInitialCondition();
    zEq = system->getState();
    Vec zd0 = system->evalzd();
    Vec sigsi0(nsink), sigsi1(nsink);
    int j=0;
    for(size_t i=0; i<sink.size(); i++) {
      sigsi0.set(RangeV(j,j+sink[i]->getSignalSize()-1), sink[i]->evalSignal());
      j+=sink[i]->getSignalSize();
    }
    for(int i=0; i<system->getzSize(); i++) {
      double ztmp = system->getState()(i);
      system->getState()(i) += epsroot;
      system->resetUpToDate();
      Vec zd1 = system->evalzd();
      A.set(i, (zd1 - zd0) / epsroot);
      system->getState()(i) = ztmp;
    }

    l=0;
    for(size_t i=0; i<source.size(); i++) {
      Vec sigso(source[i]->getSignalSize());
      for(int k=0; k<source[i]->getSignalSize(); k++) {
	sigso(k) = u0(l) + epsroot;
	source[i]->setSignal(sigso);
	system->resetUpToDate();
	Vec zd1 = system->evalzd();
	B.set(l, (zd1 - zd0) / epsroot);
	j=0;
	for(size_t m=0; m<sink.size(); m++) {
	  sigsi1.set(RangeV(j,j+sink[m]->getSignalSize()-1), sink[m]->evalSignal());
	  j+=sink[m]->getSignalSize();
	}
	D.set(l, (sigsi1 - sigsi0) / epsroot);
	sigso(k) = u0(l);
	source[i]->setSignal(sigso);
	l++;
      }
    }
    for(int i=0; i<system->getzSize(); i++) {
      double ztmp = system->getState()(i);
      system->getState()(i) += epsroot;
      system->resetUpToDate();
      j=0;
      for(size_t k=0; k<sink.size(); k++) {
	sigsi1.set(RangeV(j,j+sink[k]->getSignalSize()-1), sink[k]->evalSignal());
	j+=sink[k]->getSignalSize();
      }
      C.set(i, (sigsi1 - sigsi0) / epsroot);
      system->getState()(i) = ztmp;
    }

    SquareMatrix<Ref,complex<double>> V;
    Vector<Ref,complex<double>> w;
    eigvec(A,V,w);
    vector<pair<double,int>> fna;
    for(int i=0; i<w.size()-1; i++) {
      if((abs(imag(w(i)))>=2*M_PI*fmin) and (abs(imag(w(i)))<=2*M_PI*fmax) and (w(i+1)==conj(w(i)))) {
        fna.push_back(pair<double,int>(imag(w(i))/2/M_PI,i));
        i++;
      }
    }
    sort(fna.begin(), fna.end());

    if(modeScale.size() and modeScale.size()!=(int)fna.size())
      fmatvec::Atom::msg(fmatvec::Atom::Warn) << "LinearSystemAnalyzer: size of mode scale does not match number of modes!" << std::endl;
    if(modeScale.size()<(int)fna.size()) {
      VecV modeScaleTmp = modeScale;
      modeScale.resize(fna.size(),INIT,1);
      modeScale.set(RangeV(0,modeScaleTmp.size()-1),modeScaleTmp);
    }

    Matrix<General,Ref,Ref,complex<double>> Zhna(system->getzSize(),fna.size(),NONINIT);
    Matrix<General,Ref,Ref,complex<double>> Yhna(nsink,fna.size(),NONINIT);
    for(size_t i=0; i<fna.size(); i++) {
      Vector<Ref,complex<double>> zh = modeScaleFactor*modeScale(i)*V.col(fna[i].second);
      Vector<Ref,complex<double>> yh = C*zh;
      Zhna.set(i,zh);
      Yhna.set(i,yh);
    }

    Matrix<General,Ref,Ref,complex<double>> Zhex(system->getzSize(),fex.size(),NONINIT);
    Matrix<General,Ref,Ref,complex<double>> Yhex(nsink,fex.size(),NONINIT);
    Vector<Ref,complex<double>> u(nsource);
    cout << phi << endl;
    for(int i=0; i<fex.size(); i++) {
      SquareMatrix<Ref,complex<double>> H = SquareMatrix<Ref,complex<double>>(A.size(),Eye(),complex<double>(0,2*M_PI*fex(i))) - A;
      for(int j=0; j<nsource; j++) {
	double A = (*Amp)(fex(i))(j);
	u(j).real(A*cos(phi(j)));
	u(j).imag(A*sin(phi(j)));
      }
      Vector<Ref,complex<double>> zh = slvLU(H, B*u);
      Vector<Ref,complex<double>> yh = C*zh + D*u;
      Zhex.set(i,zh);
      Yhex.set(i,yh);
    }

    ofstream os("inputtable.asc");
    if(os.is_open()) {
      for(size_t i=0; i<source.size(); i++) {
	for(int j=0; j<source[i]->getSignalSize(); j++)
	  os << source[i]->getPath() << " u " << j << endl;
      }
      os.close();
    }

    os.open("outputtable.asc");
    if(os.is_open()) {
      for(size_t i=0; i<sink.size(); i++) {
	for(int j=0; j<sink[i]->getSignalSize(); j++)
	  os << sink[i]->getPath() << " y " << j << endl;
      }
      os.close();
    }

    H5::File file("linear_system_analysis.h5", H5::File::write);

    auto group=file.createChildObject<H5::Group>("initial output")();
    auto vdata=group->createChildObject<H5::SimpleDataset<vector<double>>>("state (z)")(zEq.size());
    if(zEq.size()) vdata->write((vector<double>)zEq);
    vdata=group->createChildObject<H5::SimpleDataset<vector<double>>>("output (y)")(sigsi0.size());
    if(sigsi0.size()) vdata->write((vector<double>)sigsi0);

    group=file.createChildObject<H5::Group>("frequency response analysis")();
    vdata=group->createChildObject<H5::SimpleDataset<vector<double>>>("excitation frequencies")(fex.size());
    if(fex.size()) vdata->write((vector<double>)fex);
    auto cmdata=group->createChildObject<H5::SimpleDataset<vector<vector<complex<double>>>>>("state response")(Zhex.rows(),Zhex.cols());
    if(Zhex.rows() and Zhex.cols()) cmdata->write((vector<vector<complex<double>>>)Zhex);
    cmdata=group->createChildObject<H5::SimpleDataset<vector<vector<complex<double>>>>>("output response")(Yhex.rows(),Yhex.cols());
    if(Yhex.rows() and Yhex.cols()) cmdata->write((vector<vector<complex<double>>>)Yhex);

    group=file.createChildObject<H5::Group>("modal analysis")();
    vector<complex<double>> lambda(fna.size());
    for(size_t i=0; i<fna.size(); i++)
      lambda[i] = w(fna[i].second);
    auto cvdata=group->createChildObject<H5::SimpleDataset<vector<complex<double>>>>("eigenvalues")(lambda.size());
    if(lambda.size()) cvdata->write(lambda);
    cmdata=group->createChildObject<H5::SimpleDataset<vector<vector<complex<double>>>>>("state modes")(Zhna.rows(),Zhna.cols());
    if(Zhna.rows() and Zhna.cols()) cmdata->write((vector<vector<complex<double>>>)Zhna);
    cmdata=group->createChildObject<H5::SimpleDataset<vector<vector<complex<double>>>>>("output modes")(Yhna.rows(),Yhna.cols());
    if(Yhna.rows() and Yhna.cols()) cmdata->write((vector<vector<complex<double>>>)Yhna);

    group=file.createChildObject<H5::Group>("eigenanalysis")();
    cvdata=group->createChildObject<H5::SimpleDataset<vector<complex<double>>>>("eigenvalues")(w.size());
    if(w.size()) cvdata->write((vector<complex<double>>)w);
    cmdata=group->createChildObject<H5::SimpleDataset<vector<vector<complex<double>>>>>("eigenvectors")(V.rows(),V.cols());
    if(V.rows() and V.cols()) cmdata->write((vector<vector<complex<double>>>)V);

    group=file.createChildObject<H5::Group>("lti system")();
    auto data=group->createChildObject<H5::SimpleDataset<vector<vector<double>>>>("system matrix")(A.rows(),A.cols());
    if(A.rows() and A.cols()) data->write((vector<vector<double>>)A);
    data=group->createChildObject<H5::SimpleDataset<vector<vector<double>>>>("input matrix")(B.rows(),B.cols());
    if(B.rows() and B.cols()) data->write((vector<vector<double>>)B);
    data=group->createChildObject<H5::SimpleDataset<vector<vector<double>>>>("output matrix")(C.rows(),C.cols());
    if(C.rows() and C.cols()) data->write((vector<vector<double>>)C);
    data=group->createChildObject<H5::SimpleDataset<vector<vector<double>>>>("feedthrough matrix")(D.rows(),D.cols());
    if(D.rows() and D.cols()) data->write((vector<vector<double>>)D);
    vdata=group->createChildObject<H5::SimpleDataset<vector<double>>>("initial state")(z0.size());
    if(z0.size()) vdata->write((vector<double>)z0);

    t0 = 0;

    if(msv) {
      if(not modes.size()) {
	modes.resize(fna.size(),NONINIT);
	for(int i=0; i<modes.size(); i++)
	  modes(i) = i+1;
      }
      else if(min(modes)<1 or max(modes)>(int)fna.size())
	throwError(string("(LinearSystemAnalyzer::execute): mode numbers do not match, must be within the range [1,") + to_string(fna.size()) + "]");
      for(int k=0; k<modes.size(); k++) {
	int i = modes(k)-1;
	if(modeScale(i)>0) {
	  complex<double> iom(0,2*M_PI*fna[i].first);
	  double T = double(loops)/fna[i].first;
	  for(double t=t0; t<t0+T+dtPlot; t+=dtPlot) {
	    system->setTime(t);
	    system->setState(zEq + fromComplex(Zhna.col(i)*exp(iom*t)));
	    system->resetUpToDate();
	    system->plot();
	    system->checkExitRequest();
	    if(system->getIntegratorExitRequest())
	      break;
	  }
	  if(system->getIntegratorExitRequest())
	    break;
	  t0 += T+dtPlot;
	}
      }
    }

    if(frv) {
      if(not inputs.size()) {
	inputs.resize(nsource,NONINIT);
	for(int i=0; i<inputs.size(); i++)
	  inputs(i) = i+1;
      }
      else if(min(inputs)<1 or max(inputs)>nsource)
	throwError(string("(LinearSystemAnalyzer::execute): input numbers do not match, must be within the range [1,") + to_string(nsource) + "]");
      Vec sigso(source[j]->getSignalSize());
      for(int i=0; i<fex.size(); i++) {
	if(fex(i)>=fRange(0) and fex(i)<=fRange(1)) {
	  complex<double> iOm(0,2*M_PI*fex(i));
	  double T = double(loops)/fex(i);
	  for(double t=t0; t<t0+T+dtPlot; t+=dtPlot) {
	    system->setTime(t);
	    for(int l=0; l<inputs.size(); l++) {
	      int j = smap[inputs(l)].first;
	      int k = smap[inputs(l)].second;
	      sigso(k) = (*Amp)(fex(i))(l)*cos(iOm.imag()*t-phi(l));
	      source[j]->setSignal(sigso);
	    }
	    system->setState(zEq + fromComplex(Zhex.col(i)*exp(iOm*t)));
	    system->resetUpToDate();
	    system->plot();
	    system->checkExitRequest();
	    if(system->getIntegratorExitRequest())
	      break;
	  }
	  t0 += T+dtPlot;
	}
      }
    }
  }

  void LinearSystemAnalyzer::initializeUsingXML(DOMElement *element) {
    DOMElement *e;
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"initialTime");
    if(e) setInitialTime(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"initialState");
    if(e) setInitialState(E(e)->getText<Vec>());
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"initialInput");
    if(e) setInitialInput(E(e)->getText<fmatvec::Vec>());
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"minimumNaturalFrequency");
    if(e) setMinimumNaturalFrequency(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"maximumNaturalFrequency");
    if(e) setMaximumNaturalFrequency(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"normalModeScaleFactor");
    if(e) setNormalModeScaleFactor(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"normalModeScale");
    if(e) setNormalModeScale(E(e)->getText<VecV>());
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"excitationFrequencies");
    if(e) setExcitationFrequencies(E(e)->getText<Vec>());
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"excitationAmplitudeFunction");
    if(e) setExcitationAmplitudeFunction(ObjectFactory::createAndInit<MBSim::Function<VecV(double)>>(e->getFirstElementChild()));
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"excitationPhaseShift");
    if(e) setExcitationPhaseShift(E(e)->getText<Vec>());
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"visualizeNormalModes");
    if(e) {
      msv = true;
      DOMElement *ee=E(e)->getFirstElementChildNamed(MBSIMCONTROL%"modeNumbers");
      if(ee) modes <<= E(ee)->getText<VecVI>();
    }
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"visualizeFrequencyResponse");
    if(e) {
      frv = true;
      DOMElement *ee=E(e)->getFirstElementChildNamed(MBSIMCONTROL%"inputNumbers");
      if(ee) inputs <<= E(ee)->getText<VecVI>();
      ee=E(e)->getFirstElementChildNamed(MBSIMCONTROL%"frequencyRange");
      if(ee) fRange = E(ee)->getText<Vec2>();
    }
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"plotStepSize");
    if(e) setPlotStepSize(E(e)->getText<double>());
    e=E(element)->getFirstElementChildNamed(MBSIMCONTROL%"loops");
    if(e) setLoops(E(e)->getText<double>());
  }

}
