/*
    MBSimGUI - A fronted for MBSim.
    Copyright (C) 2012 Martin Förg

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _SOLVER__H_
#define _SOLVER__H_

#include "solver_property_dialog.h"
#include "namespace.h"

namespace XERCES_CPP_NAMESPACE {
  class DOMElement;
  class DOMNode;
}

namespace MBSimGUI {

  class Parameter;

  class Solver {
    protected:
      QString name, value;
      xercesc::DOMElement *element;
    public:
      Solver();
      virtual ~Solver();
      xercesc::DOMElement* getXMLElement() { return element; }
      virtual void removeXMLElements();
      virtual xercesc::DOMElement* createXMLElement(xercesc::DOMNode *parent);
      virtual void initializeUsingXML(xercesc::DOMElement *element);
      virtual QString getType() const { return "Solver"; }
      const QString& getName() const { return name; }
      const QString& getValue() const { return value; }
      virtual MBXMLUtils::NamespaceURI getNameSpace() const = 0;
      virtual SolverPropertyDialog* createPropertyDialog() {return new SolverPropertyDialog(this);}
      void addParameter(Parameter *param) { }
  };

}

#endif
