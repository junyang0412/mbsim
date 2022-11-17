/*
    MBSimGUI - A fronted for MBSim.
    Copyright (C) 2022 Martin Förg

  This library is free software; you can redistribute it and/or 
  modify it under the terms of the GNU Lesser General Public 
  License as published by the Free Software Foundation; either 
  version 2.1 of the License, or (at your option) any later version. 
   
  This library is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
  Lesser General Public License for more details. 
   
  You should have received a copy of the GNU Lesser General Public 
  License along with this library; if not, write to the Free Software 
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/

#include <config.h>
#include "wizards.h"
#include "basic_widgets.h"
#include "variable_widgets.h"
#include "special_widgets.h"

using namespace std;
using namespace fmatvec;

namespace MBSimGUI {

  double (*Ni[20])(double x, double y, double z, double xi, double yi, double zi);
  double (*dNidq[20][3])(double x, double y, double z, double xi, double yi, double zi);

  double N1(double x, double y, double z, double xi, double yi, double zi) {
    return 1./8*(1+xi*x)*(1+yi*y)*(1+zi*z)*(xi*x+yi*y+zi*z-2);
  }

  double N2(double x, double y, double z, double xi, double yi, double zi) {
    return 1./4*(1-x*x)*(1+yi*y)*(1+zi*z);
  }

  double N3(double x, double y, double z, double xi, double yi, double zi) {
    return 1./4*(1-y*y)*(1+zi*z)*(1+xi*x);
  }

  double N4(double x, double y, double z, double xi, double yi, double zi) {
    return 1./4*(1-z*z)*(1+xi*x)*(1+yi*y);
  }

  double dN1dx(double x, double y, double z, double xi, double yi, double zi) {
    return 1./8*xi*(1+yi*y)*(1+zi*z)*(2*xi*x+yi*y+zi*z-1);
  }

  double dN1dy(double x, double y, double z, double xi, double yi, double zi) {
    return 1./8*yi*(1+zi*z)*(1+xi*x)*(2*yi*y+xi*x+zi*z-1);
  }

  double dN1dz(double x, double y, double z, double xi, double yi, double zi) {
    return 1./8*zi*(1+xi*x)*(1+yi*y)*(2*zi*z+xi*x+yi*y-1);
  }

  double dN2dx(double x, double y, double z, double xi, double yi, double zi) {
    return -1./2*x*(1+yi*y)*(1+zi*z);
  }

  double dN2dy(double x, double y, double z, double xi, double yi, double zi) {
    return 1./4*yi*(1-x*x)*(1+zi*z);
  }

  double dN2dz(double x, double y, double z, double xi, double yi, double zi) {
    return 1./4*zi*(1-x*x)*(1+yi*y);
  }

  double dN3dx(double x, double y, double z, double xi, double yi, double zi) {
    return 1./4*xi*(1-y*y)*(1+zi*z);
  }

  double dN3dy(double x, double y, double z, double xi, double yi, double zi) {
    return -1./2*y*(1+zi*z)*(1+xi*x);
  }

  double dN3dz(double x, double y, double z, double xi, double yi, double zi) {
    return 1./4*zi*(1-y*y)*(1+xi*x);
  }

  double dN4dx(double x, double y, double z, double xi, double yi, double zi) {
    return 1./4*xi*(1-z*z)*(1+yi*y);
  }

  double dN4dy(double x, double y, double z, double xi, double yi, double zi) {
    return 1./4*yi*(1-z*z)*(1+xi*x);
  }

  double dN4dz(double x, double y, double z, double xi, double yi, double zi) {
    return -1./2*z*(1+xi*x)*(1+yi*y);
  }

  double NL1(double x, double y, double z) {
    return 1-x-y-z;
  }

  double NL2(double x, double y, double z) {
    return x;
  }

  double NL3(double x, double y, double z) {
    return y;
  }

  double NL4(double x, double y, double z) {
    return z;
  }

  double NQ1(double x, double y, double z, double xi, double yi, double zi) {
    double NL = NL1(x,y,z);
    return NL*(2*NL-1);
  }

  double NQ2(double x, double y, double z, double xi, double yi, double zi) {
    double NL = NL2(x,y,z);
    return NL*(2*NL-1);
  }

  double NQ3(double x, double y, double z, double xi, double yi, double zi) {
    double NL = NL3(x,y,z);
    return NL*(2*NL-1);
  }

  double NQ4(double x, double y, double z, double xi, double yi, double zi) {
    double NL = NL4(x,y,z);
    return NL*(2*NL-1);
  }

  double NQ5(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL1(x,y,z)*NL2(x,y,z);
  }

  double NQ6(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL2(x,y,z)*NL3(x,y,z);
  }

  double NQ7(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL1(x,y,z)*NL3(x,y,z);
  }

  double NQ8(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL1(x,y,z)*NL4(x,y,z);
  }

  double NQ9(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL2(x,y,z)*NL4(x,y,z);
  }

  double NQ10(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL3(x,y,z)*NL4(x,y,z);
  }

  double dNQ1dx(double x, double y, double z, double xi, double yi, double zi) {
    return 1-4*NL1(x,y,z);
  }

  double dNQ1dy(double x, double y, double z, double xi, double yi, double zi) {
    return 1-4*NL1(x,y,z);
  }

  double dNQ1dz(double x, double y, double z, double xi, double yi, double zi) {
    return 1-4*NL1(x,y,z);
  }

  double dNQ2dx(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL2(x,y,z)-1;
  }

  double dNQ2dy(double x, double y, double z, double xi, double yi, double zi) {
    return 0;
  }

  double dNQ2dz(double x, double y, double z, double xi, double yi, double zi) {
    return 0;
  }

  double dNQ3dx(double x, double y, double z, double xi, double yi, double zi) {
    return 0;
  }

  double dNQ3dy(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL3(x,y,z)-1;
  }

  double dNQ3dz(double x, double y, double z, double xi, double yi, double zi) {
    return 0;
  }

  double dNQ4dx(double x, double y, double z, double xi, double yi, double zi) {
    return 0;
  }

  double dNQ4dy(double x, double y, double z, double xi, double yi, double zi) {
    return 0;
  }

  double dNQ4dz(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL4(x,y,z)-1;
  }

  double dNQ5dx(double x, double y, double z, double xi, double yi, double zi) {
    return 4*(NL1(x,y,z)-NL2(x,y,z));
  }

  double dNQ5dy(double x, double y, double z, double xi, double yi, double zi) {
    return -4*NL2(x,y,z);
  }

  double dNQ5dz(double x, double y, double z, double xi, double yi, double zi) {
    return -4*NL2(x,y,z);
  }

  double dNQ6dx(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL3(x,y,z);
  }

  double dNQ6dy(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL2(x,y,z);
  }

  double dNQ6dz(double x, double y, double z, double xi, double yi, double zi) {
    return 0;
  }

  double dNQ7dx(double x, double y, double z, double xi, double yi, double zi) {
    return -4*NL3(x,y,z);
  }

  double dNQ7dy(double x, double y, double z, double xi, double yi, double zi) {
    return 4*(NL1(x,y,z)-NL3(x,y,z));
  }

  double dNQ7dz(double x, double y, double z, double xi, double yi, double zi) {
    return -4*NL3(x,y,z);
  }

  double dNQ8dx(double x, double y, double z, double xi, double yi, double zi) {
    return -4*NL4(x,y,z);
  }

  double dNQ8dy(double x, double y, double z, double xi, double yi, double zi) {
    return -4*NL4(x,y,z);
  }

  double dNQ8dz(double x, double y, double z, double xi, double yi, double zi) {
    return 4*(NL1(x,y,z)-NL4(x,y,z));
  }

  double dNQ9dx(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL4(x,y,z);
  }

  double dNQ9dy(double x, double y, double z, double xi, double yi, double zi) {
    return 0;
  }

  double dNQ9dz(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL2(x,y,z);
  }

  double dNQ10dx(double x, double y, double z, double xi, double yi, double zi) {
    return 0;
  }

  double dNQ10dy(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL4(x,y,z);
  }

  double dNQ10dz(double x, double y, double z, double xi, double yi, double zi) {
    return 4*NL3(x,y,z);
  }

  void FlexibleBodyTool::C3D10() {
    npe = 10;
    Ni[0] = &NQ1;
    dNidq[0][0] = &dNQ1dx;
    dNidq[0][1] = &dNQ1dy;
    dNidq[0][2] = &dNQ1dz;
    Ni[1] = &NQ2;
    dNidq[1][0] = &dNQ2dx;
    dNidq[1][1] = &dNQ2dy;
    dNidq[1][2] = &dNQ2dz;
    Ni[2] = &NQ3;
    dNidq[2][0] = &dNQ3dx;
    dNidq[2][1] = &dNQ3dy;
    dNidq[2][2] = &dNQ3dz;
    Ni[3] = &NQ4;
    dNidq[3][0] = &dNQ4dx;
    dNidq[3][1] = &dNQ4dy;
    dNidq[3][2] = &dNQ4dz;
    Ni[4] = &NQ5;
    dNidq[4][0] = &dNQ5dx;
    dNidq[4][1] = &dNQ5dy;
    dNidq[4][2] = &dNQ5dz;
    Ni[5] = &NQ6;
    dNidq[5][0] = &dNQ6dx;
    dNidq[5][1] = &dNQ6dy;
    dNidq[5][2] = &dNQ6dz;
    Ni[6] = &NQ7;
    dNidq[6][0] = &dNQ7dx;
    dNidq[6][1] = &dNQ7dy;
    dNidq[6][2] = &dNQ7dz;
    Ni[7] = &NQ8;
    dNidq[7][0] = &dNQ8dx;
    dNidq[7][1] = &dNQ8dy;
    dNidq[7][2] = &dNQ8dz;
    Ni[8] = &NQ9;
    dNidq[8][0] = &dNQ9dx;
    dNidq[8][1] = &dNQ9dy;
    dNidq[8][2] = &dNQ9dz;
    Ni[9] = &NQ10;
    dNidq[9][0] = &dNQ10dx;
    dNidq[9][1] = &dNQ10dy;
    dNidq[9][2] = &dNQ10dz;

    rN.resize(10,NONINIT);

    xi.resize(4,NONINIT);
    wi.resize(4,NONINIT);
    double d = 0.138196601125011;
    double e = 0.585410196624968;
    double f = 0.041666666666667;

    xi(0,0) = d; xi(0,1) = d; xi(0,2) = d; wi(0,0) = f;
    xi(1,0) = e; xi(1,1) = d; xi(1,2) = d; wi(1,0) = f;
    xi(2,0) = d; xi(2,1) = e; xi(2,2) = d; wi(2,0) = f;
    xi(3,0) = d; xi(3,1) = d; xi(3,2) = e; wi(3,0) = f;
  }

  void FlexibleBodyTool::C3D20() {
    npe = 20;
    for(int i=0; i<8; i++) {
      Ni[i] = &N1;
      dNidq[i][0] = &dN1dx;
      dNidq[i][1] = &dN1dy;
      dNidq[i][2] = &dN1dz;
    }
    for(int i=8; i<15; i+=2) {
      Ni[i] = &N2;
      dNidq[i][0] = &dN2dx;
      dNidq[i][1] = &dN2dy;
      dNidq[i][2] = &dN2dz;
    }
    for(int i=9; i<16; i+=2) {
      Ni[i] = &N3;
      dNidq[i][0] = &dN3dx;
      dNidq[i][1] = &dN3dy;
      dNidq[i][2] = &dN3dz;
    }
    for(int i=16; i<20; i++) {
      Ni[i] = &N4;
      dNidq[i][0] = &dN4dx;
      dNidq[i][1] = &dN4dy;
      dNidq[i][2] = &dN4dz;
    }

    rN.resize(20,NONINIT);
    rN(0,0)  = -1;   rN(0,1)  = -1;  rN(0,2)  = -1;
    rN(1,0)  =  1;   rN(1,1)  = -1;  rN(1,2)  = -1;
    rN(2,0)  =  1;   rN(2,1)  =  1;  rN(2,2)  = -1;
    rN(3,0)  = -1;   rN(3,1)  =  1;  rN(3,2)  = -1;
    rN(4,0)  = -1;   rN(4,1)  = -1;  rN(4,2)  =  1;
    rN(5,0)  =  1;   rN(5,1)  = -1;  rN(5,2)  =  1;
    rN(6,0)  =  1;   rN(6,1)  =  1;  rN(6,2)  =  1;
    rN(7,0)  = -1;   rN(7,1)  =  1;  rN(7,2)  =  1;
    rN(8,0)  =  0;   rN(8,1)  = -1;  rN(8,2)  = -1;
    rN(9,0)  =  1;   rN(9,1)  =  0;  rN(9,2)  = -1;
    rN(10,0) =  0;   rN(10,1) =  1;  rN(10,2) = -1;
    rN(11,0) = -1;   rN(11,1) =  0;  rN(11,2) = -1;
    rN(12,0) =  0;   rN(12,1) = -1;  rN(12,2) =  1;
    rN(13,0) =  1;   rN(13,1) =  0;  rN(13,2) =  1;
    rN(14,0) =  0;   rN(14,1) =  1;  rN(14,2) =  1;
    rN(15,0) = -1;   rN(15,1) =  0;  rN(15,2) =  1;
    rN(16,0) = -1;   rN(16,1) = -1;  rN(16,2) =  0;
    rN(17,0) =  1;   rN(17,1) = -1;  rN(17,2) =  0;
    rN(18,0) =  1;   rN(18,1) =  1;  rN(18,2) =  0;
    rN(19,0) = -1;   rN(19,1) =  1;  rN(19,2) =  0;

    xi.resize(27,NONINIT);
    wi.resize(27,NONINIT);
    double d = sqrt(3./5);
    double e = 5./9;
    double f = 8./9;

    xi(0,0) = -d; xi(0,1) = -d; xi(0,2) = -d;   wi(0,0) = e*e*e;
    xi(1,0) = -d; xi(1,1) = -d; xi(1,2) = 0;    wi(1,0) = e*e*f;
    xi(2,0) = -d; xi(2,1) = -d; xi(2,2) = d;    wi(2,0) = e*e*e;
    xi(3,0) = -d; xi(3,1) = 0;  xi(3,2) = -d;   wi(3,0) = e*f*e;
    xi(4,0) = -d; xi(4,1) = 0;  xi(4,2) = 0;    wi(4,0) = e*f*f;
    xi(5,0) = -d; xi(5,1) = 0;  xi(5,2) = d;    wi(5,0) = e*f*e;
    xi(6,0) = -d; xi(6,1) = d;  xi(6,2) = -d;   wi(6,0) = e*e*e;
    xi(7,0) = -d; xi(7,1) = d;  xi(7,2) = 0;    wi(7,0) = e*e*f;
    xi(8,0) = -d; xi(8,1) = d;  xi(8,2) = d;    wi(8,0) = e*e*e;
    xi(9,0) =  0; xi(9,1) = -d; xi(9,2) = -d;   wi(9,0) = f*e*e;
    xi(10,0) = 0; xi(10,1) = -d; xi(10,2) = 0;  wi(10,0)= f*e*f;
    xi(11,0) = 0; xi(11,1) = -d; xi(11,2) = d;  wi(11,0)= f*e*e;
    xi(12,0) = 0; xi(12,1) = 0;  xi(12,2) = -d; wi(12,0)= f*f*e;
    xi(13,0) = 0; xi(13,1) = 0;  xi(13,2) = 0;  wi(13,0)= f*f*f;
    xi(14,0) = 0; xi(14,1) = 0;  xi(14,2) = d;  wi(14,0)= f*f*e;
    xi(15,0) = 0; xi(15,1) = d;  xi(15,2) = -d; wi(15,0) = f*e*e;
    xi(16,0) = 0; xi(16,1) = d;  xi(16,2) = 0;  wi(16,0) = f*e*f;
    xi(17,0) = 0; xi(17,1) = d;  xi(17,2) = d;  wi(17,0) = f*e*e;
    xi(18,0) = d; xi(18,1) = -d; xi(18,2) = -d; wi(18,0) = e*e*e;
    xi(19,0) = d; xi(19,1) = -d; xi(19,2) = 0;  wi(19,0) = e*e*f;
    xi(20,0) = d; xi(20,1) = -d; xi(20,2) = d;  wi(20,0) = e*e*e;
    xi(21,0) = d; xi(21,1) = 0;  xi(21,2) = -d; wi(21,0) = e*f*e;
    xi(22,0) = d; xi(22,1) = 0;  xi(22,2) = 0;  wi(22,0) = e*f*f;
    xi(23,0) = d; xi(23,1) = 0;  xi(23,2) = d;  wi(23,0) = e*f*e;
    xi(24,0) = d; xi(24,1) = d;  xi(24,2) = -d; wi(24,0) = e*e*e;
    xi(25,0) = d; xi(25,1) = d;  xi(25,2) = 0;  wi(25,0) = e*e*f;
    xi(26,0) = d; xi(26,1) = d;  xi(26,2) = d;  wi(26,0) = e*e*e;
  }

  void FlexibleBodyTool::C3D10ombv(const MatV &elei, int &oj) {
    for(int ee=0; ee<elei.rows(); ee++) {
      indices[oj++] = nodeTable[elei(ee,1)];
      indices[oj++] = nodeTable[elei(ee,2)];
      indices[oj++] = nodeTable[elei(ee,3)];
      indices[oj++] = -1;
      indices[oj++] = nodeTable[elei(ee,0)];
      indices[oj++] = nodeTable[elei(ee,1)];
      indices[oj++] = nodeTable[elei(ee,3)];
      indices[oj++] = -1;
      indices[oj++] = nodeTable[elei(ee,2)];
      indices[oj++] = nodeTable[elei(ee,0)];
      indices[oj++] = nodeTable[elei(ee,3)];
      indices[oj++] = -1;
      indices[oj++] = nodeTable[elei(ee,2)];
      indices[oj++] = nodeTable[elei(ee,0)];
      indices[oj++] = nodeTable[elei(ee,3)];
      indices[oj++] = -1;
      indices[oj++] = nodeTable[elei(ee,2)];
      indices[oj++] = nodeTable[elei(ee,1)];
      indices[oj++] = nodeTable[elei(ee,0)];
      indices[oj++] = -1;
    }
  }

  void FlexibleBodyTool::C3D20ombv(const MatV &elei, int &oj) {
      for(int ee=0; ee<elei.rows(); ee++) {
	indices[oj++] = nodeTable[elei(ee,3)];
	indices[oj++] = nodeTable[elei(ee,2)];
	indices[oj++] = nodeTable[elei(ee,1)];
	indices[oj++] = nodeTable[elei(ee,0)];
	indices[oj++] = -1;
	indices[oj++] = nodeTable[elei(ee,4)];
	indices[oj++] = nodeTable[elei(ee,5)];
	indices[oj++] = nodeTable[elei(ee,6)];
	indices[oj++] = nodeTable[elei(ee,7)];
	indices[oj++] = -1;
	indices[oj++] = nodeTable[elei(ee,1)];
	indices[oj++] = nodeTable[elei(ee,2)];
	indices[oj++] = nodeTable[elei(ee,6)];
	indices[oj++] = nodeTable[elei(ee,5)];
	indices[oj++] = -1;
	indices[oj++] = nodeTable[elei(ee,2)];
	indices[oj++] = nodeTable[elei(ee,3)];
	indices[oj++] = nodeTable[elei(ee,7)];
	indices[oj++] = nodeTable[elei(ee,6)];
	indices[oj++] = -1;
	indices[oj++] = nodeTable[elei(ee,4)];
	indices[oj++] = nodeTable[elei(ee,7)];
	indices[oj++] = nodeTable[elei(ee,3)];
	indices[oj++] = nodeTable[elei(ee,0)];
	indices[oj++] = -1;
	indices[oj++] = nodeTable[elei(ee,0)];
	indices[oj++] = nodeTable[elei(ee,1)];
	indices[oj++] = nodeTable[elei(ee,5)];
	indices[oj++] = nodeTable[elei(ee,4)];
	indices[oj++] = -1;
    }
  }

  void FlexibleBodyTool::fe() {
    MatV R;
    string str = static_cast<FileWidget*>(static_cast<FiniteElementsPage*>(page(PageFiniteElements))->nodes->getWidget())->getFile(true).toStdString();
    if(!str.empty())
      R <<= readMat(str);

    std::vector<fmatvec::MatVI> ele;
    auto *list = static_cast<ListWidget*>(static_cast<FiniteElementsPage*>(page(PageFiniteElements))->elements->getWidget());
    vector<string> type;
    for(int i=0; i<list->getSize(); i++) {
      auto type_ = static_cast<FiniteElementsDataWidget*>(list->getWidget(i))->getType().toStdString();
      type_ = type_.substr(1,type_.size()-2);
      int npe = stod(type_.substr(type_.find('D')+1,type_.size()-1));
      type.emplace_back(type_);
      str = static_cast<FiniteElementsDataWidget*>(list->getWidget(i))->getElementsFile().toStdString();
      if(!str.empty()) {
	auto ele_ = readMat(str);
	if(ele_.cols()==npe)
	  ele.emplace_back(ele_);
	else if(ele_.cols()==npe+1)
	  ele.emplace_back(ele_(RangeV(0,ele_.rows()-1),RangeV(1,ele_.cols()-1)));
      }
    }

    auto E = static_cast<PhysicalVariableWidget*>(static_cast<ChoiceWidget*>(static_cast<FiniteElementsPage*>(page(PageFiniteElements))->E->getWidget())->getWidget())->getEvalMat()[0][0].toDouble();

    auto nu = static_cast<PhysicalVariableWidget*>(static_cast<ChoiceWidget*>(static_cast<FiniteElementsPage*>(page(PageFiniteElements))->nu->getWidget())->getWidget())->getEvalMat()[0][0].toDouble();

    auto rho = static_cast<PhysicalVariableWidget*>(static_cast<ChoiceWidget*>(static_cast<FiniteElementsPage*>(page(PageFiniteElements))->rho->getWidget())->getWidget())->getEvalMat()[0][0].toDouble();

    int max = 0;
    if(R.cols()==3)
      max = R.rows();
    else if(R.cols()==4) {
      for(int i=0; i<R.rows(); i++) {
	if(R(i,0)>max)
	  max = R(i,0);
      }
    }

    int nE = 0;
    vector<int> nodeCount(max+1);
    for(size_t k=0; k<ele.size(); k++) {
      nE += ele[k].rows();
      for(int i=0; i<ele[k].rows(); i++) {
	for(int j=0; j<ele[k].cols(); j++)
	  nodeCount[ele[k](i,j)]++;
      }
    }

    nodeTable.resize(nodeCount.size());
    nN = 0;
    for(size_t i=0; i<nodeCount.size(); i++) {
      if(nodeCount[i])
	nodeTable[i] = nN++;
    }

    KrKP.resize(nN,Vec3(NONINIT));
    if(R.cols()==3) {
      for(int i=0; i<R.rows(); i++) {
	if(nodeCount[i+1])
	  KrKP[nodeTable[i+1]] = R.row(i).T();
      }
    }
    else {
      nodeNumbers.resize(nN);
      for(int i=0; i<R.rows(); i++) {
	if(nodeCount[R(i,0)]) {
	  KrKP[nodeTable[R(i,0)]] = R.row(i)(RangeV(1,3)).T();
	  nodeNumbers[nodeTable[R(i,0)]] = R(i,0);
	}
      }
    }

    ng = nN*3;
    nen = 3;

    rPdm.resize(3,Mat3xV(ng));
    Pdm.resize(ng);

    Phim.resize(nN,vector<map<int,double>>(3));
    for(int i=0; i<nN; i++) {
      Phim[i][0][3*i] = 1;
      Phim[i][1][3*i+1] = 1;
      Phim[i][2][3*i+2] = 1;
    }

    sigm.resize(nN,vector<map<int,double>>(6));
    MKm.resize(ng);
    PPm.resize(ng);

    m = 0;
    indices.resize(5*6*nE);
    SqrMat3 LUJ(NONINIT);
    double dK[3][3];
    double dsig[9];
    int oj = 0;
    for(size_t k=0; k<ele.size(); k++) {
      if(type[k]=="C3D10")
	C3D10();
      else if(type[k]=="C3D20")
	C3D20();

      MatVI &elei = ele[k];

      double N_[npe];
      Vec3 dN_[npe];
      for(int ee=0; ee<elei.rows(); ee++) {
	for(int ii=0; ii<xi.rows(); ii++) {
	  double x = xi(ii,0);
	  double y = xi(ii,1);
	  double z = xi(ii,2);
	  double wijk = wi(ii);
	  SqrMat3 J(3);
	  Vec3 r(3);
	  for(int ll=0; ll<npe; ll++) {
	    Vec3 r0 = KrKP[nodeTable[elei(ee,ll)]];
	    N_[ll] = (*Ni[ll])(x,y,z,rN(ll,0),rN(ll,1),rN(ll,2));
	    for(int mm=0; mm<3; mm++)
	      dN_[ll](mm) = (*dNidq[ll][mm])(x,y,z,rN(ll,0),rN(ll,1),rN(ll,2));
	    J += dN_[ll]*r0.T();
	    r += N_[ll]*r0;
	  }
	  Vector<Ref,int> ipiv(J.size(),NONINIT);
	  double detJ = J(0,0)*J(1,1)*J(2,2)+J(0,1)*J(1,2)*J(2,0)+J(0,2)*J(1,0)*J(2,1)-J(2,0)*J(1,1)*J(0,2)-J(2,1)*J(1,2)*J(0,0)-J(2,2)*J(1,0)*J(0,1);
	  LUJ(0,0) = J(2,2)*J(1,1)-J(2,1)*J(1,2);
	  LUJ(0,1) = J(2,1)*J(0,2)-J(2,2)*J(0,1);
	  LUJ(0,2) = J(1,2)*J(0,1)-J(1,1)*J(0,2);
	  LUJ(1,0) = J(2,0)*J(1,2)-J(2,2)*J(1,0);
	  LUJ(1,1) = J(2,2)*J(0,0)-J(2,0)*J(0,2);
	  LUJ(1,2) = J(1,0)*J(0,2)-J(1,2)*J(0,0);
	  LUJ(2,0) = J(2,1)*J(1,0)-J(2,0)*J(1,1);
	  LUJ(2,1) = J(2,0)*J(0,1)-J(2,1)*J(0,0);
	  LUJ(2,2) = J(1,1)*J(0,0)-J(1,0)*J(0,1);

	  double dm = rho*wijk*detJ;
	  double dk = E/(1+nu)*wijk*detJ;
	  m += dm;
	  rdm += dm*r;
	  rrdm += dm*JTJ(r.T());
	  for(int i=0; i<npe; i++) {
	    int u = nodeTable[elei(ee,i)];
	    double Ni_ = N_[i];
	    Vec3 dNi = LUJ*dN_[i]/detJ;
	    for(int i1=0; i1<3; i1++) {
	      Pdm(i1,u*3+i1) += dm*Ni_;
	      for(int j1=0; j1<3; j1++)
		rPdm[i1](j1,u*3+j1) += dm*r(i1)*Ni_;
	    }
	    for(int j=i; j<npe; j++) {
	      int v = nodeTable[elei(ee,j)];
	      double Nj_ = N_[j];
	      Vec3 dNj = LUJ*dN_[j]/detJ;
	      double dPPdm = dm*Ni_*Nj_;
	      dK[0][0] = dk*((1-nu)/(1-2*nu)*dNi(0)*dNj(0)+0.5*(dNi(1)*dNj(1)+dNi(2)*dNj(2)));
	      dK[0][1] = dk*(nu/(1-2*nu)*dNi(0)*dNj(1)+0.5*dNi(1)*dNj(0));
	      dK[0][2] = dk*(nu/(1-2*nu)*dNi(0)*dNj(2)+0.5*dNi(2)*dNj(0));
	      dK[1][0] = dk*(nu/(1-2*nu)*dNi(1)*dNj(0)+0.5*dNi(0)*dNj(1));
	      dK[1][1] = dk*((1-nu)/(1-2*nu)*dNi(1)*dNj(1)+0.5*(dNi(0)*dNj(0)+dNi(2)*dNj(2)));
	      dK[1][2] = dk*(nu/(1-2*nu)*dNi(1)*dNj(2)+0.5*dNi(2)*dNj(1));
	      dK[2][0] = dk*(nu/(1-2*nu)*dNi(2)*dNj(0)+0.5*dNi(0)*dNj(2));
	      dK[2][1] = dk*(nu/(1-2*nu)*dNi(2)*dNj(1)+0.5*dNi(1)*dNj(2));
	      dK[2][2] = dk*((1-nu)/(1-2*nu)*dNi(2)*dNj(2)+0.5*(dNi(0)*dNj(0)+dNi(1)*dNj(1)));
	      if(v>=u) {
		for(int iii=0; iii<3; iii++) {
		  auto d = MKm[u*3+iii][v*3+iii];
		  d[iii] += dPPdm;
		  d[3] += dK[iii][iii];
		}
		MKm[u*3][v*3+1][3] += dK[0][1];
		MKm[u*3][v*3+2][3] += dK[0][2];
		MKm[u*3+1][v*3+2][3] += dK[1][2];
		if(v!=u) {
		  MKm[u*3+1][v*3][3] += dK[1][0];
		  MKm[u*3+2][v*3][3] += dK[2][0];
		  MKm[u*3+2][v*3+1][3] += dK[2][1];
		}
	      }
	      else {
		for(int iii=0; iii<3; iii++) {
		  auto d = MKm[v*3+iii][u*3+iii];
		  d[iii] += dPPdm;
		  d[3] += dK[iii][iii];
		}
		MKm[v*3][u*3+1][3] += dK[1][0];
		MKm[v*3][u*3+2][3] += dK[2][0];
		MKm[v*3+1][u*3+2][3] += dK[2][1];
		MKm[v*3+1][u*3][3] += dK[0][1];
		MKm[v*3+2][u*3][3] += dK[0][2];
		MKm[v*3+2][u*3+1][3] += dK[1][2];
	      }
	      PPm[u*3][v*3+1][0] += dPPdm;
	      PPm[u*3][v*3+2][1] += dPPdm;
	      PPm[u*3+1][v*3+2][2] += dPPdm;
	      if(u!=v) {
		PPm[v*3][u*3+1][0] += dPPdm;
		PPm[v*3][u*3+2][1] += dPPdm;
		PPm[v*3+1][u*3+2][2] += dPPdm;
	      }
	    }
	  }
	}

	for(int k=0; k<npe; k++) {
	  double x = rN(k,0);
	  double y = rN(k,1);
	  double z = rN(k,2);
	  SqrMat3 J(3);
	  for(int ll=0; ll<npe; ll++) {
	    Vec3 r0 = KrKP[nodeTable[elei(ee,ll)]];
	    for(int mm=0; mm<3; mm++)
	      dN_[ll](mm) = (*dNidq[ll][mm])(x,y,z,rN(ll,0),rN(ll,1),rN(ll,2));
	    J += dN_[ll]*r0.T();
	  }
	  Vector<Ref,int> ipiv(J.size(),NONINIT);
	  double detJ = J(0,0)*J(1,1)*J(2,2)+J(0,1)*J(1,2)*J(2,0)+J(0,2)*J(1,0)*J(2,1)-J(2,0)*J(1,1)*J(0,2)-J(2,1)*J(1,2)*J(0,0)-J(2,2)*J(1,0)*J(0,1);
	  LUJ(0,0) = J(2,2)*J(1,1)-J(2,1)*J(1,2);
	  LUJ(0,1) = J(2,1)*J(0,2)-J(2,2)*J(0,1);
	  LUJ(0,2) = J(1,2)*J(0,1)-J(1,1)*J(0,2);
	  LUJ(1,0) = J(2,0)*J(1,2)-J(2,2)*J(1,0);
	  LUJ(1,1) = J(2,2)*J(0,0)-J(2,0)*J(0,2);
	  LUJ(1,2) = J(1,0)*J(0,2)-J(1,2)*J(0,0);
	  LUJ(2,0) = J(2,1)*J(1,0)-J(2,0)*J(1,1);
	  LUJ(2,1) = J(2,0)*J(0,1)-J(2,1)*J(0,0);
	  LUJ(2,2) = J(1,1)*J(0,0)-J(1,0)*J(0,1);
	  for(int i=0; i<npe; i++) {
	    Vec3 dNi = LUJ*dN_[i]/detJ;
	    int ku = nodeTable[elei(ee,k)];
	    int u = nodeTable[elei(ee,i)];
	    double al = E/(1+nu)/nodeCount[elei(ee,k)];
	    dsig[0] = al*(1-nu)/(1-2*nu)*dNi(0);
	    dsig[1] = al*nu/(1-2*nu)*dNi(1);
	    dsig[2] = al*nu/(1-2*nu)*dNi(2);
	    dsig[3] = al*nu/(1-2*nu)*dNi(0);
	    dsig[4] = al*(1-nu)/(1-2*nu)*dNi(1);
	    dsig[5] = al*(1-nu)/(1-2*nu)*dNi(2);
	    dsig[6] = al*0.5*dNi(1);
	    dsig[7] = al*0.5*dNi(0);
	    dsig[8] = al*0.5*dNi(2);
	    sigm[ku][0][u*3] += dsig[0];
	    sigm[ku][0][u*3+1] += dsig[1];
	    sigm[ku][0][u*3+2] += dsig[2];
	    sigm[ku][1][u*3] += dsig[3];
	    sigm[ku][1][u*3+1] += dsig[4];
	    sigm[ku][1][u*3+2] += dsig[2];
	    sigm[ku][2][u*3] += dsig[3];
	    sigm[ku][2][u*3+1] += dsig[1];
	    sigm[ku][2][u*3+2] += dsig[5];
	    sigm[ku][3][u*3] += dsig[6];
	    sigm[ku][3][u*3+1] += dsig[7];
	    sigm[ku][4][u*3+1] += dsig[8];
	    sigm[ku][4][u*3+2] += dsig[6];
	    sigm[ku][5][u*3] += dsig[8];
	    sigm[ku][5][u*3+2] += dsig[7];
	  }
	}
      }
      if(type[k]=="C3D10")
	C3D10ombv(elei,oj);
      else if(type[k]=="C3D20")
	C3D20ombv(elei,oj);
    }
  }
}
