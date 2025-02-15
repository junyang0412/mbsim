/* Copyright (C) 2004-2015 MBSim Development Team
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
 * Contact: thorsten.schindler@mytum.de
 */

#ifndef FLEXIBLEBODY2S13_H_
#define FLEXIBLEBODY2S13_H_

#include "mbsimFlexibleBody/flexible_body/flexible_body_2s.h"

namespace MBSimFlexibleBody {

  class NurbsDisk2s;

  /*!
   * \brief condenses rows of matrix concerning index
   * \param input matrix
   * \param indices to be condensed
   * \return condensed matrix
   */
  fmatvec::Mat condenseMatrixRows(const fmatvec::Mat &A, const fmatvec::RangeV &I);

  /*!
   * \brief condenses cols of matrix concerning index
   * \param input matrix
   * \param indices to be condensed
   * \return condensed matrix
   */
  fmatvec::Mat condenseMatrixCols(const fmatvec::Mat &A, const fmatvec::RangeV &I);

  /*!
   * \brief condenses symmetric matrix concerning index
   * \param input matrix
   * \param indices to be condensed
   * \return condensed matrix
   */
  fmatvec::SymMat condenseMatrix(const fmatvec::SymMat &A,const fmatvec::RangeV &I);

  /*!
   * \brief generates an output for a matrix for the input in maple - just for testing
   * \param matrix for the output
   */
  void MapleOutput(const fmatvec::Mat &A, const std::string &MatName, const std::string &file);

  /*!
   * \brief generates an output for a matrix for the input in maple - just for testing
   * \param matrix for the output
   */
  void MapleOutput(const fmatvec::SymMat &C, const std::string &MatName, const std::string &file);

  /*!
   * \brief plate according to Reissner-Mindlin with moving frame of reference
   * \author Kilian Grundl
   * \author Thorsten Schindler
   * \date 2010-04-23 initial commit (Schindler / Grundl)
   * \date 2010-08-12 revision (Schindler)
   *
   * The plate lies in the xy-plane of its reference frame. The z-vector is its "normal".
   * Thus the radial and the azimuthal component give the x- and y- coordinates where the z-coordinate is defined by the thickness parameterization. (neglecting the flexible dofs)
   */
  class FlexibleBody2s13 : public FlexibleBody2s {

    public:
      /**
       * \brief condensation setting for clamping to rigid body motion
       */
      enum LockType { innerring,outerring };

      /**
       * \brief constructor
       * \param name of body
       */
      FlexibleBody2s13(const std::string &name);

      /**
       * \brief destructor
       */
       ~FlexibleBody2s13() override = default;

      /* INHERITED INTERFACE OF OBJECTINTERFACE */
       void updateh(int j=0) override;
       void updatedhdz() override;
      /******************************************/

      /* INHERITED INTERFACE OF OBJECT */
       void updateM() override;
       void updateLLM() override { }

      /* INHERITED INTERFACE OF ELEMENT */
       void plot() override;
      /***************************************************/

      /* GETTER/SETTER */
      void setRadius(double Ri_,double Ra_) { Ri = Ri_; Ra = Ra_; }
      void setEModul(double E_) { E = E_; }
      void setPoissonRatio(double nu_) { nu = nu_; }
      void setThickness(const fmatvec::Vec &d_) { d = d_; }
      fmatvec::Vec getThickness() const { return d; }
      void setDensity(double rho_) { rho = rho_; }
      int getReferenceDegreesOfFreedom() const { return RefDofs; }
      int getRadialNumberOfElements() const { return nr; }
      int getAzimuthalNumberOfElements() const { return nj; }
      double getInnerRadius() const { return Ri; }
      double getOuterRadius() const { return Ra; }
      double getAzimuthalDegree() const { return degU; }
      double getRadialDegree() const { return degV; }
      fmatvec::SqrMat3& evalA() { if(updAG) updateAG(); return A; }
      fmatvec::SqrMat3& evalG() { if(updAG) updateAG(); return G; }
      void setReferenceInertia(double m0_, fmatvec::SymMat3 J0_) { m0 = m0_; J0 = J0_; }
      void setLockType(LockType LT_) { LType = LT_; }
      /***************************************************/

      /*!
       * \brief set number of elements in radial and azimuthal direction
       * \param radial number of elements
       * \param azimuthal number of elements
       */
      void setNumberElements(int nr_,int nj_);

      /*!
       * \return potential energy
       */
      double computePotentialEnergy() override { return 0.5*q.T()*K*q; }

      /*!
       * \brief transform Cartesian to cylinder system
       * \param Cartesian vector in world system of plate
       * \return cylindrical coordinates
       */
      virtual fmatvec::Vec transformCW(const fmatvec::Vec& WrPoint) = 0;

      void resetUpToDate() override;

      void updateExt();

      const fmatvec::Vec& evalqExt() { if(updExt) updateExt(); return qext; }
      const fmatvec::Vec& evaluExt() { if(updExt) updateExt(); return uext; }

      virtual fmatvec::Vec3 evalPosition() { return fmatvec::Vec3(); }
      virtual fmatvec::SqrMat3 evalOrientation() { return fmatvec::SqrMat3(fmatvec::EYE); }

    protected:
      /**
       * \brief total number of elements
       */
      int Elements;

      /**
       * \brief elastic dof per node
       */
      int NodeDofs;

      /**
       * \brief dof of moving frame of reference
       */
      int RefDofs;

      /**
       * \brief Young's modulus
       */
      double E;

      /**
       * \brief Poisson ratio
       */
      double nu;

      /**
       * \brief density
       */
      double rho;

      /**
       * \brief parameterization of thickness over radius function: d(0) + d(1)*r + d(2)*r*r
       *
       * \remark vector must have length 3
       */
      fmatvec::Vec d;

      /**
       * \brief inner and outer radius of disk
       */
      double Ri, Ra;

      /**
       * \brief radial and azimuthal length of an FE
       */
      double dr, dj;

      /**
       * \brief mass of the attached shaft
       */
      double m0;

      /**
       * \brief inertia of the attached shaft in local coordinates
       */
      fmatvec::SymMat3 J0;

      /**
       * \brief degree of surface interpolation in radial and azimuthal direction
       *
       * both degrees have to be smaller than 8
       */
      int degV, degU;

      /**
       * \brief number of points drawn between nodes
       */
      int drawDegree;

      /**
       * \brief vector of boundary data of the FE (r1,j1,r2,j2)
       */
      std::vector<fmatvec::Vec> ElementalNodes;

      /**
       * \brief number of element currently involved in contact calculations
       */
      int currentElement;

      /**
       * \brief constant part of the mass matrix
       */
      fmatvec::SymMat MConst;

      /**
       * \brief stiffness matrix
       */
      fmatvec::SymMat K;

      /**
       * \brief number of elements in radial and azimuthal direction, number of FE nodes
       */
      int nr, nj, Nodes;

      /**
       * \brief matrix mapping nodes and coordinates (size number of nodes x number of node coordinates)
       *
       * NodeCoordinates(GlobalNodeNumber,0) = radius (at the node)
       * NodeCoordinates(GlobalNodeNumber,1) = angle (at the node)
       */
      fmatvec::Mat NodeCoordinates;

      /**
       * \brief matrix mapping elements and nodes (size number of elements x number of nodes per elements)
       *
       * ElementNodeList(Element,LocalNodeNumber) = globalNodeNumber;
       */
      fmatvec::Matrix<fmatvec::General,fmatvec::Ref,fmatvec::Ref,int> ElementNodeList;

      /**
       * \brief total dof of disk with reference movement and elastic deformation but without including bearing
       */
      int Dofs;

      /**
       * \brief Dirichlet boundary condition concerning reference movement
       *
       * possible settings: innering/outerring
       */
      LockType LType;

      /**
       * \brief index of condensated dofs
       */
      fmatvec::RangeV ILocked;

      /**
       * \brief position and velocity with respect to Dofs
       */
      fmatvec::Vec qext, uext;

      /**
       * \brief transformation matrix of coordinates of the moving frame of reference into the reference frame
       */
      fmatvec::SqrMat3 A;

      /**
       * \brief transformation matrix of the time derivates of the angles into tho angular velocity in reference coordinates
       */
      fmatvec::SqrMat3 G;

      /*
       * \brief Jacobian for condensation with size Dofs x qSize
       */
      fmatvec::Mat Jext;

      /**
       * \brief contour for contact description
       */
      NurbsDisk2s *contour;

      /*!
       * \brief detect involved element for contact description
       * \param parametrisation vector (radial / azimuthal)
       */
      void BuildElement(const fmatvec::Vec &s);

      /*!
       * \brief calculate the matrices for the first time
       */
      virtual void initMatrices() = 0;

      /*!
       * \brief update the transformation matrices A and G
       */
      virtual void updateAG() = 0;

      /*!
       * \return thickness of disk at radial coordinate
       * \param radial coordinate
       */
      double computeThickness(const double &r_);

      bool updExt, updAG;
  };

}

#endif /* FLEXIBLEBODY2S13_H_ */
