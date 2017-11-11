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
 * Contact: martin.o.foerg@gmail.com
 */

#ifndef _MODULO_FUNCTION_H_
#define _MODULO_FUNCTION_H_

#include "mbsim/functions/function.h"

namespace MBSim {

  template<typename Sig> class ModuloFunction;

  template<typename Ret, typename Arg>
  class ModuloFunction<Ret(Arg)> : public Function<Ret(Arg)> {
    private:
      double denom;
    public:
      void setDenominator(double denom_) { denom = denom_; }
      int getArgSize() const { return 1; }
      std::pair<int, int> getRetSize() const { return std::make_pair(1,1); }
      Ret operator()(const Arg &x_) {
        double x = ToDouble<Arg>::cast(x_);
        return FromDouble<Ret>::cast(x-denom*floor(x/denom));
      }
      void initializeUsingXML(xercesc::DOMElement *element) {
        xercesc::DOMElement *e;
        e=MBXMLUtils::E(element)->getFirstElementChildNamed(MBSIM%"denominator");
        denom=MBXMLUtils::E(e)->getText<double>();
      }
  };

}

#endif
