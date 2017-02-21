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

#ifndef _OBSERVER__H_
#define _OBSERVER__H_

#include "element.h"
#include "extended_properties.h"

namespace MBSimGUI {

  class Observer : public Element {
    public:
      Observer(const std::string &str, Element *parent);
      static Observer* readXMLFile(const std::string &filename, Element *parent);
      virtual int getxSize() {return 0;}
  };

  class KinematicCoordinatesObserver : public Observer {
    friend class KinematicCoordinatesObserverPropertyDialog;
    public:
    KinematicCoordinatesObserver(const std::string &str, Element *parent);
    std::string getType() const { return "KinematicCoordinatesObserver"; }
    virtual xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element);
    virtual xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *element);
    void initialize();
    ElementPropertyDialog* createPropertyDialog() {return new KinematicCoordinatesObserverPropertyDialog(this);}
    protected:
    ExtProperty frame, frameOfReference, position, velocity, acceleration;
  };

  class RelativeKinematicsObserver : public Observer {
    friend class RelativeKinematicsObserverPropertyDialog;
    public:
    RelativeKinematicsObserver(const std::string &str, Element *parent);
    std::string getType() const { return "RelativeKinematicsObserver"; }
    virtual xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element);
    virtual xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *element);
    void initialize();
    ElementPropertyDialog* createPropertyDialog() {return new RelativeKinematicsObserverPropertyDialog(this);}
    protected:
    ExtProperty frame, refFrame, position, velocity, angularVelocity, acceleration, angularAcceleration;
  };

  class MechanicalLinkObserver : public Observer {
    friend class MechanicalLinkObserverPropertyDialog;
    public:
    MechanicalLinkObserver(const std::string &str, Element *parent);
    std::string getType() const { return "MechanicalLinkObserver"; }
    virtual xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element);
    virtual xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *element);
    void initialize();
    protected:
    ExtProperty link, forceArrow, momentArrow;
  };

  class MechanicalConstraintObserver : public Observer {
    friend class MechanicalConstraintObserverPropertyDialog;
    public:
    MechanicalConstraintObserver(const std::string &str, Element *parent);
    std::string getType() const { return "MechanicalConstraintObserver"; }
    virtual xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element);
    virtual xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *element);
    void initialize();
    ElementPropertyDialog* createPropertyDialog() {return new MechanicalConstraintObserverPropertyDialog(this);}
    protected:
    ExtProperty constraint, forceArrow, momentArrow;
  };

  class ContactObserver : public Observer {
    friend class ContactObserverPropertyDialog;
    public:
    ContactObserver(const std::string &str, Element *parent);
    std::string getType() const { return "ContactObserver"; }
    virtual xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element);
    virtual xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *element);
    void initialize();
    ElementPropertyDialog* createPropertyDialog() {return new ContactObserverPropertyDialog(this);}
    protected:
    ExtProperty link, forceArrow, momentArrow, contactPoints, normalForceArrow, frictionArrow;
  };

  class FrameObserver : public Observer {
    friend class FrameObserverPropertyDialog;
    public:
    FrameObserver(const std::string &str, Element *parent);
    std::string getType() const { return "FrameObserver"; }
    virtual xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element);
    virtual xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *element);
    void initialize();
    ElementPropertyDialog* createPropertyDialog() {return new FrameObserverPropertyDialog(this);}
    protected:
    ExtProperty frame, position, velocity, angularVelocity, acceleration, angularAcceleration;
  };

  class RigidBodyObserver : public Observer {
    friend class RigidBodyObserverPropertyDialog;
    public:
    RigidBodyObserver(const std::string &str, Element *parent);
    std::string getType() const { return "RigidBodyObserver"; }
    virtual xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element);
    virtual xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *element);
    void initialize();
    ElementPropertyDialog* createPropertyDialog() {return new RigidBodyObserverPropertyDialog(this);}
    protected:
    ExtProperty body, weight, jointForce, jointMoment, axisOfRotation;
  };
}

#endif
