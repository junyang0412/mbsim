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
 * Contact: martin.o.foerg@googlemail.com
 */

#include <config.h>
#include "eigenanalysis.h"
#include "mbsim/dynamic_system_solver.h"
#include "mbsim/utils/eps.h"
#include "fmatvec/linear_algebra_complex.h"
#include "mbsim/utils/nonlinear_algebra.h"
#include <iostream>

using namespace std;
using namespace fmatvec;

// TODO: remove the following two functions and provide an uniform concept to
// cast between complex and double
Vector<Ref, complex<double> > toComplex(const Vector<Ref, double> &x) {
  Vector<Ref, complex<double> > y(x.size(),NONINIT);
  for(int i=0; i<x.size(); i++)
    y(i) = complex<double>(x(i),0);
  return y;
}
const Vector<Ref, double> fromComplex(const Vector<Ref, complex<double> > &x) {
  Vector<Ref, double> y(x.size(),NONINIT);
  for(int i=0; i<x.size(); i++)
    y(i) = x(i).real();
  return y;
}

namespace MBSim {

  DynamicSystemSolver * Eigenanalysis::system = 0;

  Eigenanalysis::Residuum::Residuum(DynamicSystemSolver *sys_, double t_) : sys(sys_), t(t_) {}

  Vec Eigenanalysis::Residuum::operator()(const Vec &z) {
      Vec res;
      res = sys->zdot(z,t);
      return res;
    } 

  void Eigenanalysis::analyse(DynamicSystemSolver& system_) {
    system = &system_;

    double t=tStart;
    int zSize=system->getzSize();
    Vec z(zSize);
    if(z0.size())
      z = z0;
    else
      system->initz(z);          

    if(not(zEq.size())) {
      Residuum f(system,t);
      MultiDimNewtonMethod newton(&f);
      newton.setLinearAlgebra(1);
      zEq = newton.solve(z);
      if(newton.getInfo() != 0)
        throw MBSimError("ERROR in Eigenanalysis: computation of equilibrium position failed!");
    }

    double delta = epsroot();
    SqrMat A(zEq.size());
    Vec zd, zdOld;
    zdOld = system->zdot(zEq,t);
    for (int i=0; i<z.size(); i++) {
      double ztmp = zEq(i);
      zEq(i) += delta;
      zd = system->zdot(zEq,t);
      A.col(i) = (zd - zdOld) / delta;
      zEq(i) = ztmp;
    }
    eigvec(A,V,w);
    computeEigenfrequencies();
    saveEigenanalyis(fileName.empty()?system->getName()+".eigenanalysis.mat":fileName);
  }

  void Eigenanalysis::computeEigenfrequencies() {
    for (int i=0; i<w.size(); i++) {
      if((i < w.size()-1) and (w(i+1)==conj(w(i)))) {
        f.push_back(pair<double,int>(imag(w(i))/2/M_PI,i));
        i++;
      }
    }
    std::sort(f.begin(), f.end());
    freq.resize(f.size(),NONINIT);
    for(int i=0; i<freq.size(); i++)
      freq.e(i) = f[i].first;
  }

  bool Eigenanalysis::saveEigenanalyis(const string& fileName) {
    ofstream os(fileName.c_str());
    if(os.is_open()) {
      os << "# name: lambda" << endl;
      os << "# type: complex matrix" << endl;
      os << "# rows: " << w.size() << endl;
      os << "# columns: " << 1 << endl;
      for(int i=0; i < w.size(); ++i)
        os << setw(26) << w.e(i) << endl;
      os << endl;
      os << "# name: V" << endl;
      os << "# type: complex matrix" << endl;
      os << "# rows: " << V.rows() << endl;
      os << "# columns: " << V.cols() << endl;
      for(int i=0; i < V.rows(); ++i) {
        for(int j=0; j < V.cols(); ++j) 
          os << setw(26) << V.e(i,j);
        os << endl;
      }
      os << endl;
      os << "# name: z" << endl;
      os << "# type: matrix" << endl;
      os << "# rows: " << freq.size() << endl;
      os << "# columns: " << 1 << endl;
      for(int i=0; i < zEq.size(); ++i)
        os << setw(26) << zEq.e(i) << endl;
      os << endl;
      os << "# name: f" << endl;
      os << "# type: matrix" << endl;
      os << "# rows: " << freq.size() << endl;
      os << "# columns: " << 1 << endl;
      for(int i=0; i < freq.size(); ++i)
        os << setw(26) << freq.e(i) << endl;
      os.close();
      return true;
    }
    return false;
  }

  bool Eigenanalysis::loadEigenanalyis(const string& fileName) {
    ifstream is(fileName.c_str());
    if(is.is_open()) {
      char str[100];
      string n;
      is.getline(str,100);
      is.getline(str,100);
      is >> n >> n >> n;
      w.resize(atoi(n.c_str()));
      V.resize(w.size());
      zEq.resize(w.size());
      is >> n;
      is.getline(str,100);
      for(int i=0; i<w.size(); i++)
        is >> w.e(i);
      is.getline(str,100);
      is.getline(str,100);
      is.getline(str,100);
      is.getline(str,100);
      is.getline(str,100);
      is.getline(str,100);
      for(int i=0; i<V.size(); i++)
        for(int j=0; j<V.size(); j++)
          is >> V.e(i,j);
      is.getline(str,100);
      is.getline(str,100);
      is.getline(str,100);
      is.getline(str,100);
      is.getline(str,100);
      is.getline(str,100);
      for(int i=0; i<zEq.size(); i++)
        is >> zEq.e(i);
      is.close();
      computeEigenfrequencies();
      return true;
    }
    return false;
  }

  void Eigenanalysis::eigenmode(int i, DynamicSystemSolver& system_) {
    system = &system_;
    if(not(w.size()) and not(loadEigenanalyis(fileName.empty()?system->getName()+".eigenanalysis.mat":fileName)))
      analyse(system_);
//      throw MBSimError("ERROR in Eigenanalysis: eigenanalysis not yet performed!");
    if(i<1 or i>f.size())
      throw MBSimError("ERROR in Eigenanalysis: frequency number out of range!");
    Vector<Ref, complex<double> > c(w.size());
    Vector<Ref, complex<double> > deltaz(w.size(),NONINIT);

    Vec z;
    c(f[i-1].second) = complex<double>(0,1);
    c(f[i-1].second+1) = complex<double>(0,-1);
    double T=1./f[i-1].first;
    for(double t=tStart; t<tStart+T+dtPlot; t+=dtPlot) {
      deltaz.init(0);
      for(int i=0; i<w.size(); i++)
        deltaz += c(i)*V.col(i)*exp(w(i)*t); 
      z = zEq + fromComplex(deltaz);
      system->plot(z,t);
    }
  }

  void Eigenanalysis::eigenmodes(DynamicSystemSolver& system_) {
    system = &system_;
    if(not(w.size()) and not(loadEigenanalyis(fileName.empty()?system->getName()+".eigenanalysis.mat":fileName)))
      analyse(system_);
    Vector<Ref, complex<double> > c(w.size());
    Vector<Ref, complex<double> > deltaz(w.size(),NONINIT);

    Vec z;
    double t0 = tStart;
    for(int j=0; j<f.size(); j++) {
      c(f[j].second) = complex<double>(0,1);
      c(f[j].second+1) = complex<double>(0,-1);
      double T=10*1./f[j].first;
      for(double t=t0; t<t0+T+dtPlot; t+=dtPlot) {
        deltaz.init(0);
        for(int i=0; i<w.size(); i++)
          deltaz += c(i)*V.col(i)*exp(w(i)*t); 
        z = zEq + fromComplex(deltaz);
        system->plot(z,t);
      }
      t0 += T+dtPlot;
      c(f[j].second) = complex<double>(0,0);
      c(f[j].second+1) = complex<double>(0,0);
    }
  }

  void Eigenanalysis::eigenmotion(DynamicSystemSolver& system_) {
    system = &system_;
    if(not(w.size()) and not(loadEigenanalyis(fileName.empty()?system->getName()+".eigenanalysis.mat":fileName)))
      analyse(system_);
    Vector<Ref, complex<double> > deltaz(w.size(),NONINIT);

    Vec z;
    if(deltaz0.size()==0)
      deltaz0.resize(w.size());
    Vector<Ref, complex<double> > c = slvLU(V,toComplex(deltaz0));

    for(double t=tStart; t<tEnd+dtPlot; t+=dtPlot) {
      deltaz.init(0);
      for(int i=0; i<w.size(); i++)
        deltaz += c(i)*V.col(i)*exp(w(i)*t); 
      z = zEq + fromComplex(deltaz);
      system->plot(z,t);
    }
  }

}

