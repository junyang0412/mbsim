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

#include <config.h>
#include "observer.h"
#include "basic_properties.h"
#include "ombv_properties.h"
#include "objectfactory.h"
#include "mainwindow.h"
#include "embed.h"

using namespace std;
using namespace MBXMLUtils;
using namespace xercesc;

namespace MBSimGUI {

  extern MainWindow *mw;

  Observer::Observer(const string &str, Element *parent) : Element(str, parent) {
  }

  Observer* Observer::readXMLFile(const string &filename, Element *parent) {
    shared_ptr<DOMDocument> doc=mw->parser->parse(filename);
    DOMElement *e=doc->getDocumentElement();
//    Observer *observer=ObjectFactory::getInstance()->createObserver(e, parent);
    Observer *observer=Embed<Observer>::createAndInit(e,parent);
    if(observer) {
//      observer->initializeUsingXML(e);
      observer->initialize();
    }
    return observer;
  }

  KinematicCoordinatesObserver::KinematicCoordinatesObserver(const string &str, Element *parent) : Observer(str, parent), frameOfReference(0,false), position(0,false), velocity(0,false), acceleration(0,false) {

    frame.setProperty(new FrameOfReferenceProperty("",this,MBSIM%"frame"));

    frameOfReference.setProperty(new FrameOfReferenceProperty("",this,MBSIM%"frameOfReference"));

    position.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVPosition",getID(),true));

    velocity.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVVelocity",getID(),true));

    acceleration.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVAcceleration",getID(),true));
  }

  void KinematicCoordinatesObserver::initialize() {
    Observer::initialize();
    frame.initialize();
    frameOfReference.initialize();
  }

  DOMElement* KinematicCoordinatesObserver::initializeUsingXML(DOMElement *element) {
    Observer::initializeUsingXML(element);
    frame.initializeUsingXML(element);
    frameOfReference.initializeUsingXML(element);
    position.initializeUsingXML(element);
    velocity.initializeUsingXML(element);
    acceleration.initializeUsingXML(element);
    return element;
  }

  DOMElement* KinematicCoordinatesObserver::writeXMLFile(DOMNode *parent) {
    DOMElement *ele0 = Observer::writeXMLFile(parent);
    frame.writeXMLFile(ele0);
    frameOfReference.writeXMLFile(ele0);
    position.writeXMLFile(ele0);
    velocity.writeXMLFile(ele0);
    acceleration.writeXMLFile(ele0);
    return ele0;
  }

  KinematicsObserver::KinematicsObserver(const string &str, Element *parent) : Observer(str, parent), position(0,false), velocity(0,false), angularVelocity(0,false), acceleration(0,false), angularAcceleration(0,false) {

    frame.setProperty(new FrameOfReferenceProperty("",this,MBSIM%"frame"));

    position.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVPosition",getID(),true));

    velocity.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVVelocity",getID(),true));

    angularVelocity.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVAngularVelocity",getID(),true));

    acceleration.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVAcceleration",getID(),true));

    angularAcceleration.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVAngularAcceleration",getID(),true));
  }

  void KinematicsObserver::initialize() {
    Observer::initialize();
    frame.initialize();
  }

  DOMElement* KinematicsObserver::initializeUsingXML(DOMElement *element) {
    Observer::initializeUsingXML(element);
    frame.initializeUsingXML(element);
    position.initializeUsingXML(element);
    velocity.initializeUsingXML(element);
    angularVelocity.initializeUsingXML(element);
    acceleration.initializeUsingXML(element);
    angularAcceleration.initializeUsingXML(element);
    return element;
  }

  DOMElement* KinematicsObserver::writeXMLFile(DOMNode *parent) {

    DOMElement *ele0 = Observer::writeXMLFile(parent);
    frame.writeXMLFile(ele0);
    position.writeXMLFile(ele0);
    velocity.writeXMLFile(ele0);
    angularVelocity.writeXMLFile(ele0);
    acceleration.writeXMLFile(ele0);
    angularAcceleration.writeXMLFile(ele0);
    return ele0;
  }

  RelativeKinematicsObserver::RelativeKinematicsObserver(const string &str, Element *parent) : KinematicsObserver(str, parent) {

    refFrame.setProperty(new FrameOfReferenceProperty("",this,MBSIM%"frameOfReference"));
  }

  void RelativeKinematicsObserver::initialize() {
    KinematicsObserver::initialize();
    refFrame.initialize();
  }

  DOMElement* RelativeKinematicsObserver::initializeUsingXML(DOMElement *element) {
    KinematicsObserver::initializeUsingXML(element);
    refFrame.initializeUsingXML(element);
    return element;
  }

  DOMElement* RelativeKinematicsObserver::writeXMLFile(DOMNode *parent) {

    DOMElement *ele0 = KinematicsObserver::writeXMLFile(parent);
    refFrame.writeXMLFile(ele0);
    return ele0;
  }

  MechanicalLinkObserver::MechanicalLinkObserver(const string &str, Element *parent) : Observer(str, parent), forceArrow(0,false), momentArrow(0,false) {

    link.setProperty(new LinkOfReferenceProperty("",this,MBSIM%"mechanicalLink"));

    forceArrow.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVForce",getID()));

    momentArrow.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVMoment",getID()));
  }

  void MechanicalLinkObserver::initialize() {
    Observer::initialize();
    link.initialize();
  }

  DOMElement* MechanicalLinkObserver::initializeUsingXML(DOMElement *element) {
    Observer::initializeUsingXML(element);
    link.initializeUsingXML(element);
    forceArrow.initializeUsingXML(element);
    momentArrow.initializeUsingXML(element);
    return element;
  }

  DOMElement* MechanicalLinkObserver::writeXMLFile(DOMNode *parent) {
    DOMElement *ele0 = Observer::writeXMLFile(parent);
    link.writeXMLFile(ele0);
    forceArrow.writeXMLFile(ele0);
    momentArrow.writeXMLFile(ele0);
    return ele0;
  }

  MechanicalConstraintObserver::MechanicalConstraintObserver(const string &str, Element *parent) : Observer(str, parent), forceArrow(0,false), momentArrow(0,false) {

    constraint.setProperty(new ConstraintOfReferenceProperty("",this,MBSIM%"mechanicalConstraint"));

    forceArrow.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVForce",getID()));

    momentArrow.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVMoment",getID()));
  }

  void MechanicalConstraintObserver::initialize() {
    Observer::initialize();
    constraint.initialize();
  }

  DOMElement* MechanicalConstraintObserver::initializeUsingXML(DOMElement *element) {
    Observer::initializeUsingXML(element);
    constraint.initializeUsingXML(element);
    forceArrow.initializeUsingXML(element);
    momentArrow.initializeUsingXML(element);
    return element;
  }

  DOMElement* MechanicalConstraintObserver::writeXMLFile(DOMNode *parent) {
    DOMElement *ele0 = Observer::writeXMLFile(parent);
    constraint.writeXMLFile(ele0);
    forceArrow.writeXMLFile(ele0);
    momentArrow.writeXMLFile(ele0);
    return ele0;
  }

  ContactObserver::ContactObserver(const string &str, Element *parent) : Observer(str, parent), forceArrow(0,false), momentArrow(0,false), contactPoints(0,false), normalForceArrow(0,false), frictionArrow(0,false) {

    link.setProperty(new LinkOfReferenceProperty("",this,MBSIM%"contact"));

    forceArrow.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVForce",getID()));

    momentArrow.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVMoment",getID()));

    contactPoints.setProperty(new FrameMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVContactPoints",getID()));

    normalForceArrow.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVNormalForce",getID()));

    frictionArrow.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVTangentialForce",getID()));
  }

  void ContactObserver::initialize() {
    Observer::initialize();
    link.initialize();
  }

  DOMElement* ContactObserver::initializeUsingXML(DOMElement *element) {
    Observer::initializeUsingXML(element);
    link.initializeUsingXML(element);
    forceArrow.initializeUsingXML(element);
    momentArrow.initializeUsingXML(element);
    contactPoints.initializeUsingXML(element);
    normalForceArrow.initializeUsingXML(element);
    frictionArrow.initializeUsingXML(element);
    return element;
  }

  DOMElement* ContactObserver::writeXMLFile(DOMNode *parent) {
    DOMElement *ele0 = Observer::writeXMLFile(parent);
    link.writeXMLFile(ele0);
    forceArrow.writeXMLFile(ele0);
    momentArrow.writeXMLFile(ele0);
    contactPoints.writeXMLFile(ele0);
    normalForceArrow.writeXMLFile(ele0);
    frictionArrow.writeXMLFile(ele0);
    return ele0;
  }

  FrameObserver::FrameObserver(const string &str, Element *parent) : Observer(str, parent), position(0,false), velocity(0,false), angularVelocity(0,false), acceleration(0,false), angularAcceleration(0,false) {

    frame.setProperty(new FrameOfReferenceProperty("",this,MBSIM%"frame"));

    position.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVPosition",getID(),true));

    velocity.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVVelocity",getID(),true));

    angularVelocity.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVAngularVelocity",getID(),true));

    acceleration.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVAcceleration",getID(),true));

    angularAcceleration.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVAngularAcceleration",getID(),true));
  }

  void FrameObserver::initialize() {
    Observer::initialize();
    frame.initialize();
  }

  DOMElement* FrameObserver::initializeUsingXML(DOMElement *element) {
    Observer::initializeUsingXML(element);
    frame.initializeUsingXML(element);
    position.initializeUsingXML(element);
    velocity.initializeUsingXML(element);
    angularVelocity.initializeUsingXML(element);
    acceleration.initializeUsingXML(element);
    angularAcceleration.initializeUsingXML(element);
    return element;
  }

  DOMElement* FrameObserver::writeXMLFile(DOMNode *parent) {

    DOMElement *ele0 = Observer::writeXMLFile(parent);
    frame.writeXMLFile(ele0);
    position.writeXMLFile(ele0);
    velocity.writeXMLFile(ele0);
    angularVelocity.writeXMLFile(ele0);
    acceleration.writeXMLFile(ele0);
    angularAcceleration.writeXMLFile(ele0);
    return ele0;
  }

  RigidBodyObserver::RigidBodyObserver(const string &str, Element *parent) : Observer(str, parent), weight(0,false), jointForce(0,false), jointMoment(0,false), axisOfRotation(0,false) {

    body.setProperty(new RigidBodyOfReferenceProperty("",this,MBSIM%"rigidBody"));

    weight.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVWeight",getID(),true));

    jointForce.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVJointForce",getID(),true));

    jointMoment.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVJointMoment",getID(),true));

    axisOfRotation.setProperty(new ArrowMBSOMBVProperty("NOTSET",MBSIM%"enableOpenMBVAxisOfRotation",getID(),true));
  }

  void RigidBodyObserver::initialize() {
    Observer::initialize();
    body.initialize();
  }

  DOMElement* RigidBodyObserver::initializeUsingXML(DOMElement *element) {
    Observer::initializeUsingXML(element);
    body.initializeUsingXML(element);
    weight.initializeUsingXML(element);
    jointForce.initializeUsingXML(element);
    jointMoment.initializeUsingXML(element);
    axisOfRotation.initializeUsingXML(element);
    return element;
  }

  DOMElement* RigidBodyObserver::writeXMLFile(DOMNode *parent) {

    DOMElement *ele0 = Observer::writeXMLFile(parent);
    body.writeXMLFile(ele0);
    weight.writeXMLFile(ele0);
    jointForce.writeXMLFile(ele0);
    jointMoment.writeXMLFile(ele0);
    axisOfRotation.writeXMLFile(ele0);
    return ele0;
  }

}
