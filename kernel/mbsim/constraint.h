/* Copyright (C) 2004-2009 MBSim Development Team
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

#ifndef _CONSTRAINT_H
#define _CONSTRAINT_H

#include "mbsim/element.h"
#include "functions/auxiliary_functions.h"

#ifdef HAVE_OPENMBVCPPINTERFACE
#include "mbsim/utils/boost_parameters.h"
#include "mbsim/utils/openmbv_utils.h"
#endif

namespace MBSim {

  class RigidBody;
  class Frame;

  /** 
   * \brief Class for constraints between generalized coordinates of objects
   * \author Martin Foerg
   */
  class Constraint : public Element {
    private:

    public:
      Constraint(const std::string &name);
      virtual void updateGeneralizedCoordinates(double t) {}
      virtual void updateGeneralizedJacobians(double t, int j=0) {}
      virtual void updatedx(double t, double dt) {}
      virtual void updatexd(double t) {}
      virtual void calcxSize() { xSize = 0; }
      virtual const fmatvec::Vec& getx() const { return x; }
      virtual fmatvec::Vec& getx() { return x; }
      virtual void setxInd(int xInd_) { xInd = xInd_; };
      virtual int getxSize() const { return xSize; }
      virtual void updatexRef(const fmatvec::Vec& ref);
      virtual void updatexdRef(const fmatvec::Vec& ref);
      virtual void init(InitStage stage);
      virtual void initz();
      virtual void writez(H5::GroupBase *group);
      virtual void readz0(H5::GroupBase *group);
      std::string getType() const { return "Constraint"; }
      virtual void plot(double t, double dt = 1);
      virtual void closePlot();
      virtual void setUpInverseKinetics() {}
#ifdef HAVE_OPENMBVCPPINTERFACE
      virtual boost::shared_ptr<OpenMBV::Group> getOpenMBVGrp() {return boost::shared_ptr<OpenMBV::Group>();}
#endif
      bool updateGeneralizedCoordinates() const { return updGC; }
      bool updateGeneralizedJacobians() const { return updGJ; }
      void resetUpToDate() { updGC = true; updGJ = true; }

    protected:
      /** 
       * \brief order one parameters
       */
      fmatvec::Vec x;

      /** 
       * \brief differentiated order one parameters 
       */
      fmatvec::Vec xd;

      /**
       * \brief order one initial value
       */
      fmatvec::Vec x0;

      /**
       * \brief size  and local index of order one parameters
       */
      int xSize, xInd;
      bool updGC, updGJ;
  };

  struct Transmission {
    Transmission(RigidBody *body_, double ratio_) : body(body_), ratio(ratio_) { }
    RigidBody *body;
    double ratio;
  };

  class GearConstraint : public Constraint {

    public:
      GearConstraint(const std::string &name="");

      void addTransmission(const Transmission &transmission);

      void init(InitStage stage);

      void setDependentBody(RigidBody* body_) {bd=body_; }

      void updateGeneralizedCoordinates(double t);
      void updateGeneralizedJacobians(double t, int j=0); 
      void setUpInverseKinetics();

      void initializeUsingXML(xercesc::DOMElement * element);

      virtual std::string getType() const { return "GearConstraint"; }
    
#ifdef HAVE_OPENMBVCPPINTERFACE
      /** \brief Visualize a force arrow acting on frame2 */
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVForce, tag, (optional (scaleLength,(double),1)(scaleSize,(double),1)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::toPoint)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVArrow ombv(diffuseColor,transparency,OpenMBV::Arrow::toHead,referencePoint,scaleLength,scaleSize);
        FArrow=ombv.createOpenMBV();
      }

      /** \brief Visualize a moment arrow */
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVMoment, tag, (optional (scaleLength,(double),1)(scaleSize,(double),1)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::toPoint)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVArrow ombv(diffuseColor,transparency,OpenMBV::Arrow::toDoubleHead,referencePoint,scaleLength,scaleSize);
        MArrow=ombv.createOpenMBV();
      }
#endif

    private:
      std::vector<RigidBody*> bi;
      RigidBody *bd;
      std::vector<double> ratio;

      std::string saved_DependentBody;
      std::vector<std::string> saved_IndependentBody;

#ifdef HAVE_OPENMBVCPPINTERFACE
      boost::shared_ptr<OpenMBV::Arrow> FArrow, MArrow;
#endif
  };

  class KinematicConstraint : public Constraint {

    public:
      KinematicConstraint(const std::string &name="");

      void setDependentBody(RigidBody* body) {bd=body; }

      void init(InitStage stage);

      void initializeUsingXML(xercesc::DOMElement * element);

#ifdef HAVE_OPENMBVCPPINTERFACE
      /** \brief Visualize a force arrow acting on frame2 */
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVForce, tag, (optional (scaleLength,(double),1)(scaleSize,(double),1)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::toPoint)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) {
        OpenMBVArrow ombv(diffuseColor,transparency,OpenMBV::Arrow::toHead,referencePoint,scaleLength,scaleSize);
        FArrow=ombv.createOpenMBV();
      }

      /** \brief Visualize a moment arrow */
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVMoment, tag, (optional (scaleLength,(double),1)(scaleSize,(double),1)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::toPoint)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) {
        OpenMBVArrow ombv(diffuseColor,transparency,OpenMBV::Arrow::toDoubleHead,referencePoint,scaleLength,scaleSize);
        MArrow=ombv.createOpenMBV();
      }
#endif

    protected:
      RigidBody *bd;

      std::string saved_DependentBody;

#ifdef HAVE_OPENMBVCPPINTERFACE
      boost::shared_ptr<OpenMBV::Arrow> FArrow, MArrow;
#endif
  };

  class GeneralizedPositionConstraint : public KinematicConstraint {

    public:
      GeneralizedPositionConstraint(const std::string &name="") : KinematicConstraint(name), f(NULL) {}
      ~GeneralizedPositionConstraint() { delete f; }

      void init(Element::InitStage stage);

      void setConstraintFunction(Function<fmatvec::VecV(double)>* f_) {
        f = f_;
        f->setParent(this);
        f->setName("Constraint");
      }

      void setUpInverseKinetics();

      void updateGeneralizedCoordinates(double t);
      void updateGeneralizedJacobians(double t, int j=0);

      void initializeUsingXML(xercesc::DOMElement * element);

      virtual std::string getType() const { return "GeneralizedPositionConstraint"; }

    private:
      Function<fmatvec::VecV(double)> *f;
  };

  class GeneralizedVelocityConstraint : public KinematicConstraint {

    public:
      GeneralizedVelocityConstraint(const std::string &name="") : KinematicConstraint(name), f(NULL) {}
      ~GeneralizedVelocityConstraint() { delete f; }

      void init(InitStage stage);

      void calcxSize();

      // NOTE: we can not use a overloaded setConstraintFunction here due to restrictions in XML but define them for convinience in c++
      void setGeneralConstraintFunction(Function<fmatvec::VecV(fmatvec::VecV,double)>* f_) {
        f = f_;
        f->setParent(this);
        f->setName("Constraint");
      }
      void setTimeDependentConstraintFunction(Function<fmatvec::VecV(double)>* f_) {
        setGeneralConstraintFunction(new TimeDependentFunction<fmatvec::VecV>(f_));
      }
      void setStateDependentConstraintFunction(Function<fmatvec::VecV(fmatvec::VecV)>* f_) {
        setGeneralConstraintFunction(new StateDependentFunction<fmatvec::VecV>(f_));
      }
      void setConstraintFunction(Function<fmatvec::VecV(fmatvec::VecV,double)>* f_) { setGeneralConstraintFunction(f_); }
      void setConstraintFunction(Function<fmatvec::VecV(double)>* f_) { setTimeDependentConstraintFunction(f_); }
      void setConstraintFunction(Function<fmatvec::VecV(fmatvec::VecV)>* f_) { setStateDependentConstraintFunction(f_); }

      virtual void setUpInverseKinetics();

      void updatexd(double t);
      void updateGeneralizedCoordinates(double t);
      void updateGeneralizedJacobians(double t, int j=0);

      void initializeUsingXML(xercesc::DOMElement * element);

      virtual std::string getType() const { return "GeneralizedVelocityConstraint"; }

    private:
      Function<fmatvec::VecV(fmatvec::VecV,double)> *f;
  };

  class GeneralizedAccelerationConstraint : public KinematicConstraint {

    public:
      GeneralizedAccelerationConstraint(const std::string &name="") : KinematicConstraint(name) {}
      ~GeneralizedAccelerationConstraint() { delete f; }

      void init(InitStage stage);

      void calcxSize();

      // NOTE: we can not use a overloaded setConstraintFunction here due to restrictions in XML but define them for convinience in c++
      void setGeneralConstraintFunction(Function<fmatvec::VecV(fmatvec::VecV,double)>* f_) {
        f = f_;
        f->setParent(this);
        f->setName("Constraint");
      }
      void setTimeDependentConstraintFunction(Function<fmatvec::VecV(double)>* f_) {
        setGeneralConstraintFunction(new TimeDependentFunction<fmatvec::VecV>(f_));
      }
      void setStateDependentConstraintFunction(Function<fmatvec::VecV(fmatvec::VecV)>* f_) {
        setGeneralConstraintFunction( new StateDependentFunction<fmatvec::VecV>(f_));
      }
      void setConstraintFunction(Function<fmatvec::VecV(fmatvec::VecV,double)>* f_) { setGeneralConstraintFunction(f_); }
      void setConstraintFunction(Function<fmatvec::VecV(double)>* f_) { setTimeDependentConstraintFunction(f_); }
      void setConstraintFunction(Function<fmatvec::VecV(fmatvec::VecV)>* f_) { setStateDependentConstraintFunction(f_); }

      virtual void setUpInverseKinetics();

      void updatexd(double t);
      void updateGeneralizedCoordinates(double t);
      void updateGeneralizedJacobians(double t, int j=0);

      void initializeUsingXML(xercesc::DOMElement * element);

      virtual std::string getType() const { return "GeneralizedAccelerationConstraint"; }

    private:
      Function<fmatvec::VecV(fmatvec::VecV,double)> *f;
  };

  /** 
   * \brief Joint contraint 
   * \author Martin Foerg
   * 2011-08-04 XML Interface added (Markus Schneider)
   */
  class JointConstraint : public Constraint {
    public:
      JointConstraint(const std::string &name="");

      void init(InitStage stage);
      void initz();

      void connect(Frame* frame1, Frame* frame2);
      void setDependentBodiesFirstSide(std::vector<RigidBody*> bd);
      void setDependentBodiesSecondSide(std::vector<RigidBody*> bd);
      void setIndependentBody(RigidBody* bi);
      void setSecondIndependentBody(RigidBody* bi2);

      virtual void setUpInverseKinetics();
      void setForceDirection(const fmatvec::Mat3xV& d_);
      void setMomentDirection(const fmatvec::Mat3xV& d_);

      /** \brief The frame of reference ID for the force/moment direction vectors.
       * If ID=0 (default) the first frame, if ID=1 the second frame is used.
       */
      void setFrameOfReferenceID(int ID) { refFrameID=ID; }

      fmatvec::Vec res(const fmatvec::Vec& q, const double& t);
      void updateGeneralizedCoordinates(double t); 
      void updateGeneralizedJacobians(double t, int j=0); 
      virtual void initializeUsingXML(xercesc::DOMElement *element);
      virtual xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *element);

      virtual std::string getType() const { return "JointConstraint"; }

      void setInitialGuess(const fmatvec::VecV &q0_) { q0 = q0_; }

#ifdef HAVE_OPENMBVCPPINTERFACE
      /** \brief Visualize a force arrow acting on frame2 */
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVForce, tag, (optional (scaleLength,(double),1)(scaleSize,(double),1)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::toPoint)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVArrow ombv(diffuseColor,transparency,OpenMBV::Arrow::toHead,referencePoint,scaleLength,scaleSize);
        FArrow=ombv.createOpenMBV();
      }

      /** \brief Visualize a moment arrow */
      BOOST_PARAMETER_MEMBER_FUNCTION( (void), enableOpenMBVMoment, tag, (optional (scaleLength,(double),1)(scaleSize,(double),1)(referencePoint,(OpenMBV::Arrow::ReferencePoint),OpenMBV::Arrow::toPoint)(diffuseColor,(const fmatvec::Vec3&),"[-1;1;1]")(transparency,(double),0))) { 
        OpenMBVArrow ombv(diffuseColor,transparency,OpenMBV::Arrow::toDoubleHead,referencePoint,scaleLength,scaleSize);
        MArrow=ombv.createOpenMBV();
      }
#endif

    private:
      class Residuum : public Function<fmatvec::Vec(fmatvec::Vec)> {
        private:
          std::vector<RigidBody*> body1, body2;
          fmatvec::Mat3xV forceDir, momentDir;
          Frame *frame1, *frame2, *refFrame;
          double t;
          std::vector<Frame*> i1,i2;
        public:
          Residuum(std::vector<RigidBody*> body1_, std::vector<RigidBody*> body2_, const fmatvec::Mat3xV &forceDir_, const fmatvec::Mat3xV &momentDir_, Frame *frame1_, Frame *frame2_, Frame *refFrame, double t_, std::vector<Frame*> i1_, std::vector<Frame*> i2_);
          fmatvec::Vec operator()(const fmatvec::Vec &x);
      };
      std::vector<RigidBody*> bd1;
      std::vector<RigidBody*> bd2;
      RigidBody *bi, *bi2;
      std::vector<Frame*> if1;
      std::vector<Frame*> if2;

      Frame *frame1,*frame2;

      /**
       * \brief frame of reference the force is defined in
       */
      Frame *refFrame;
      int refFrameID;

      fmatvec::Mat3xV dT, dR, forceDir, momentDir;

      std::vector<fmatvec::Index> Iq1, Iq2, Iu1, Iu2, Ih1, Ih2;
      int nq, nu, nh;
      fmatvec::Vec q, q0;
      fmatvec::Mat JT, JR;

      std::string saved_ref1, saved_ref2;
      std::vector<std::string> saved_RigidBodyFirstSide, saved_RigidBodySecondSide;
      std::string saved_IndependentBody, saved_IndependentBody2;
#ifdef HAVE_OPENMBVCPPINTERFACE
      boost::shared_ptr<OpenMBV::Arrow> FArrow, MArrow;
#endif
  };

}

#endif
