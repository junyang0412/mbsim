/* Copyright (C) 2004-2009 MBSim Development Team
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

#ifndef _LINEAR_TRANSFER_SYSTEM_
#define _LINEAR_TRANSFER_SYSTEM_

#include "mbsimControl/signal_.h"

namespace MBSimControl {

  /*!
   * \brief Linear tansfer system
   * \author Martin Foerg
   */
  class LinearTransferSystem : public Signal {

    public:   
      LinearTransferSystem(const std::string& name="") : Signal(name), inputSignal(NULL) { }

      void initializeUsingXML(xercesc::DOMElement * element);
      
      void calcxSize() { xSize = A.rows(); }
      
      void init(InitStage stage, const MBSim::InitConfigSet &config);

      void updateSignal();
      void updatexd();
      
      void setSystemMatrix(fmatvec::SqrMatV A_) { A = A_; }
      void setInputMatrix(fmatvec::MatV B_) { B = B_; }
      void setOutputMatrix(fmatvec::MatV C_) { C = C_; }
      void setFeedthroughMatrix(fmatvec::SqrMatV D_) { D = D_; }

      void setInputSignal(Signal * inputSignal_) { inputSignal = inputSignal_; }
      int getSignalSize() const { return inputSignal->getSignalSize(); }

    protected:
      Signal* inputSignal;
      std::string inputSignalString;
      fmatvec::SqrMatV A, D;
      fmatvec::MatV B, C;
  };

}

#endif
