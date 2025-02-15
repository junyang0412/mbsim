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
#include "extended_widgets.h"

using namespace std;
using namespace fmatvec;

namespace MBSimGUI {

 void FlexibleBodyTool::ombv() {
    if(static_cast<OpenMBVPage*>(page(PageOMBV))->ombvIndices->isActive()) {
      string str = static_cast<FileWidget*>(static_cast<OpenMBVPage*>(page(PageOMBV))->ombvIndices->getWidget())->getFile(true).toStdString();
      MatV ombvIndices;
      if(!str.empty())
	ombvIndices <<= readMat(str);
      indices.resize(ombvIndices.rows());
      for(int i=0; i<ombvIndices.rows(); i++)
	indices[i] = ombvIndices(i,0)-1;
    }
  }

}
