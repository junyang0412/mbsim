/*
   MBSimGUI - A fronted for MBSim.
   Copyright (C) 2016 Martin Förg

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
#include "flexible_body_ffr.h"
#include "objectfactory.h"
#include "frame.h"
#include "contour.h"
#include "group.h"
#include "basic_properties.h"
#include "kinematics_properties.h"
#include "ombv_properties.h"
#include "special_properties.h"
#include "kinematic_functions_properties.h"
#include "function_properties.h"
#include "function_property_factory.h"
#include "embed.h"

using namespace std;
using namespace MBXMLUtils;
using namespace xercesc;

namespace MBSimGUI {

  FlexibleBodyFFR::FlexibleBodyFFR(const string &str, Element *parent) : Body(str,parent), De(0,false), beta(0,false), Knl1(0,false), Knl2(0,false), ksigma0(0,false), ksigma1(0,false), K0t(0,false), K0r(0,false), K0om(0,false), r(0,false), A(0,false), Phi(0,false), sigmahel(0,false), sigmahen(0,false), sigma0(0,false), K0F(0,false), K0M(0,false), translation(0,false), rotation(0,false), translationDependentRotation(0,false), coordinateTransformationForRotation(0,false), ombvEditor(0,true) {
    InternalFrame *K = new InternalFrame("K",this,"plotFeatureFrameK");
    addFrame(K);

    mass.setProperty(new ChoiceProperty2(new ScalarPropertyFactory("1",MBSIMFLEX%"mass",vector<string>(2,"kg")),"",4));

    pdm.setProperty(new ChoiceProperty2(new VecPropertyFactory(3,MBSIMFLEX%"positionIntegral",vector<string>(3,"")),"",4));

    ppdm.setProperty(new ChoiceProperty2(new MatPropertyFactory(getEye<string>(3,3,"0","0"),MBSIMFLEX%"positionPositionIntegral",vector<string>(3,"kg*m^2")),"",4));

    Pdm.setProperty(new ChoiceProperty2(new MatPropertyFactory(getMat<string>(3,1,"0"),MBSIMFLEX%"shapeFunctionIntegral",vector<string>(3,"")),"",4));

    //rPdm.setProperty(new OneDimMatArrayProperty(3,3,1,MBSIMFLEX%"positionShapeFunctionIntegral"));
    rPdm.setProperty(new ChoiceProperty2(new OneDimMatArrayPropertyFactory(3,3,1),MBSIMFLEX%"positionShapeFunctionIntegral",5));

    //PPdm.setProperty(new TwoDimMatArrayProperty(3,1,1,MBSIMFLEX%"shapeFunctionShapeFunctionIntegral"));
    PPdm.setProperty(new ChoiceProperty2(new TwoDimMatArrayPropertyFactory(3,1,1),MBSIMFLEX%"shapeFunctionShapeFunctionIntegral",5));

    Ke.setProperty(new ChoiceProperty2(new MatPropertyFactory(getMat<string>(1,1,"0"),MBSIMFLEX%"stiffnessMatrix",vector<string>(3,"")),"",4));

    De.setProperty(new ChoiceProperty2(new MatPropertyFactory(getMat<string>(1,1,"0"),MBSIMFLEX%"dampingMatrix",vector<string>(3,"")),"",4));

    beta.setProperty(new ChoiceProperty2(new VecPropertyFactory(2,MBSIMFLEX%"proportionalDamping",vector<string>(3,"")),"",4));

    //Knl1.setProperty(new OneDimMatArrayProperty(1,1,1,MBSIMFLEX%"nonlinearStiffnessMatrixOfFirstOrder",true));
    Knl1.setProperty(new ChoiceProperty2(new OneDimMatArrayPropertyFactory(1,1,1,"",true),MBSIMFLEX%"nonlinearStiffnessMatrixOfFirstOrder",5));

    //Knl2.setProperty(new TwoDimMatArrayProperty(1,1,1,MBSIMFLEX%"nonlinearStiffnessMatrixOfSecondOrder",true));
    Knl2.setProperty(new ChoiceProperty2(new TwoDimMatArrayPropertyFactory(1,1,1,"",true),MBSIMFLEX%"nonlinearStiffnessMatrixOfSecondOrder",5));

    ksigma0.setProperty(new ChoiceProperty2(new VecPropertyFactory(1,MBSIMFLEX%"initialStressIntegral",vector<string>(3,"")),"",4));

    ksigma1.setProperty(new ChoiceProperty2(new MatPropertyFactory(getMat<string>(1,1,"0"),MBSIMFLEX%"nonlinearInitialStressIntegral",vector<string>(3,"")),"",4));

    //K0t.setProperty(new OneDimMatArrayProperty(3,1,1,MBSIMFLEX%"geometricStiffnessMatrixDueToAcceleration"));
    K0t.setProperty(new ChoiceProperty2(new OneDimMatArrayPropertyFactory(3,1,1),MBSIMFLEX%"geometricStiffnessMatrixDueToAcceleration",5));

    //K0r.setProperty(new OneDimMatArrayProperty(3,1,1,MBSIMFLEX%"geometricStiffnessMatrixDueToAngularAcceleration"));
    K0r.setProperty(new ChoiceProperty2(new OneDimMatArrayPropertyFactory(3,1,1),MBSIMFLEX%"geometricStiffnessMatrixDueToAngularAcceleration",5));

    //K0om.setProperty(new OneDimMatArrayProperty(3,1,1,MBSIMFLEX%"geometricStiffnessMatrixDueToAngularVelocity"));
    K0om.setProperty(new ChoiceProperty2(new OneDimMatArrayPropertyFactory(3,1,1),MBSIMFLEX%"geometricStiffnessMatrixDueToAngularVelocity",5));

    //r.setProperty(new ChoiceProperty2(new VecPropertyFactory(3,MBSIMFLEX%"relativeNodalPosition",vector<string>(3,"")),"",4));
    r.setProperty(new ChoiceProperty2(new OneDimVecArrayPropertyFactory(1,3,"",true),MBSIMFLEX%"nodalRelativePosition",5));

    //A.setProperty(new ChoiceProperty2(new MatPropertyFactory(getEye<string>(3,3,"1","0"),MBSIMFLEX%"relativeNodalOrientation",vector<string>(3,"")),"",4));
    A.setProperty(new ChoiceProperty2(new OneDimMatArrayPropertyFactory(3,3,3,"",true),MBSIMFLEX%"nodalRelativeOrientation",5));

    //Phi.setProperty(new ChoiceProperty2(new MatPropertyFactory(getMat<string>(3,1,"0"),MBSIMFLEX%"shapeMatrixOfTranslation",vector<string>(3,"")),"",4));
    Phi.setProperty(new ChoiceProperty2(new OneDimMatArrayPropertyFactory(3,1,1,"",true),MBSIMFLEX%"nodalShapeMatrixOfTranslation",5));

    //Psi.setProperty(new ChoiceProperty2(new MatPropertyFactory(getMat<string>(3,1,"0"),MBSIMFLEX%"shapeMatrixOfRotation",vector<string>(3,"")),"",4));
    Psi.setProperty(new ChoiceProperty2(new OneDimMatArrayPropertyFactory(3,1,1,"",true),MBSIMFLEX%"nodalShapeMatrixOfRotation",5));

    //sigmahel.setProperty(new ChoiceProperty2(new MatPropertyFactory(getMat<string>(6,1,"0"),MBSIMFLEX%"stressMatrix",vector<string>(3,"")),"",4));
    sigmahel.setProperty(new ChoiceProperty2(new OneDimMatArrayPropertyFactory(6,1,1,"",true),MBSIMFLEX%"nodalStressMatrix",5));

    sigmahen.setProperty(new ChoiceProperty2(new TwoDimMatArrayPropertyFactory(6,1,1,"",true),MBSIMFLEX%"nodalNonlinearStressMatrix",5));

    sigma0.setProperty(new ChoiceProperty2(new OneDimVecArrayPropertyFactory(1,6,"",true),MBSIMFLEX%"nodalInitialStress",5));

    K0F.setProperty(new ChoiceProperty2(new TwoDimMatArrayPropertyFactory(1,1,1,"",true),MBSIMFLEX%"nodalGeometricStiffnessMatrixDueToForce",5));

    K0M.setProperty(new ChoiceProperty2(new TwoDimMatArrayPropertyFactory(1,1,1,"",true),MBSIMFLEX%"nodalGeometricStiffnessMatrixDueToMoment",5));

    translation.setProperty(new ChoiceProperty2(new TranslationPropertyFactory4(this,MBSIMFLEX),"",3));

    rotation.setProperty(new ChoiceProperty2(new RotationPropertyFactory4(this,MBSIMFLEX),"",3));

    vector<PhysicalVariableProperty> input;
    input.push_back(PhysicalVariableProperty(new ScalarProperty("0"),"",MBSIMFLEX%"translationDependentRotation"));
    translationDependentRotation.setProperty(new ExtPhysicalVarProperty(input)); 
    input.clear();
    input.push_back(PhysicalVariableProperty(new ScalarProperty("0"),"",MBSIMFLEX%"coordinateTransformationForRotation"));
    coordinateTransformationForRotation.setProperty(new ExtPhysicalVarProperty(input)); 

    ombvEditor.setProperty(new FlexibleBodyFFRMBSOMBVProperty("NOTSET",MBSIMFLEX%"enableOpenMBV",getID()));
  }

  int FlexibleBodyFFR::getqRelSize() const {
     int nqT=0, nqR=0;
    if(translation.isActive()) {
      const ExtProperty *extProperty = static_cast<const ExtProperty*>(static_cast<const ChoiceProperty2*>(translation.getProperty())->getProperty());
      const ChoiceProperty2 *trans = static_cast<const ChoiceProperty2*>(extProperty->getProperty());
      nqT = static_cast<Function*>(trans->getProperty())->getArg1Size();
    }
    if(rotation.isActive()) {
      const ExtProperty *extProperty = static_cast<const ExtProperty*>(static_cast<const ChoiceProperty2*>(rotation.getProperty())->getProperty());
      const ChoiceProperty2 *rot = static_cast<const ChoiceProperty2*>(extProperty->getProperty());
      nqR = static_cast<Function*>(rot->getProperty())->getArg1Size();
    }
    int nq = nqT + nqR;
    return nq;
  }

  int FlexibleBodyFFR::getuRelSize() const {
    return getqRelSize();
  }

  int FlexibleBodyFFR::getqElSize() const {
    return static_cast<PhysicalVariableProperty*>(static_cast<const ChoiceProperty2*>(Pdm.getProperty())->getProperty())->cols();
  }

  void FlexibleBodyFFR::initialize() {
    Body::initialize();

    for(size_t i=0; i<frame.size(); i++)
      frame[i]->initialize();
    for(size_t i=0; i<contour.size(); i++)
      contour[i]->initialize();
  }

  DOMElement* FlexibleBodyFFR::initializeUsingXML(DOMElement *element) {
    DOMElement *e;
    Body::initializeUsingXML(element);

    // frames
    e=E(element)->getFirstElementChildNamed(MBSIMFLEX%"frames")->getFirstElementChild();
    Frame *f;
    while(e) {
      f = Embed<Frame>::createAndInit(e,this);
      if(f) addFrame(f);
      e=e->getNextElementSibling();
    }

    // contours
    e=E(element)->getFirstElementChildNamed(MBSIMFLEX%"contours")->getFirstElementChild();
    Contour *c;
    while(e) {
      c = Embed<Contour>::createAndInit(e,this);
      if(c) addContour(c);
      e=e->getNextElementSibling();
    }

    mass.initializeUsingXML(element);
    pdm.initializeUsingXML(element);
    ppdm.initializeUsingXML(element);
    Pdm.initializeUsingXML(element);
    rPdm.initializeUsingXML(element);
    PPdm.initializeUsingXML(element);
    Ke.initializeUsingXML(element);
    De.initializeUsingXML(element);
    beta.initializeUsingXML(element);
    Knl1.initializeUsingXML(element);
    Knl2.initializeUsingXML(element);
    ksigma0.initializeUsingXML(element);
    ksigma1.initializeUsingXML(element);
    K0t.initializeUsingXML(element);
    K0r.initializeUsingXML(element);
    K0om.initializeUsingXML(element);
    r.initializeUsingXML(element);
    A.initializeUsingXML(element);
    Phi.initializeUsingXML(element);
    Psi.initializeUsingXML(element);
    sigmahel.initializeUsingXML(element);
    sigmahen.initializeUsingXML(element);
    sigma0.initializeUsingXML(element);
    K0F.initializeUsingXML(element);
    K0M.initializeUsingXML(element);

    translation.initializeUsingXML(element);
    rotation.initializeUsingXML(element);
    translationDependentRotation.initializeUsingXML(element);
    coordinateTransformationForRotation.initializeUsingXML(element);

    ombvEditor.initializeUsingXML(element);

    return element;
  }

}
