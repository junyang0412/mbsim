/* Copyright (C) 2004-2016 MBSim Development Team
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
 * Contact: martin.o.foerg@googlemail.com
 */

#include <config.h> 
#include "point_spatialcontour.h"
#include "mbsim/frames/contour_frame.h"
#include "mbsim/contours/point.h"
#include "mbsim/functions/contact/funcpair_spatialcontour_point.h"
#include "mbsim/utils/spatial_contact_search.h"

using namespace fmatvec;
using namespace std;

namespace MBSim {

  ContactKinematicsPointSpatialContour::~ContactKinematicsPointSpatialContour() {
    delete func;
  }

  void ContactKinematicsPointSpatialContour::assignContours(const vector<Contour*> &contour) {
    if(dynamic_cast<Point*>(contour[0])) {
      ipoint = 0;
      ispatialcontour = 1;
      point = static_cast<Point*>(contour[0]);
      spatialcontour = static_cast<Contour*>(contour[1]);
    }
    else {
      ipoint = 1;
      ispatialcontour = 0;
      point = static_cast<Point*>(contour[1]);
      spatialcontour = static_cast<Contour*>(contour[0]);
    }
    func = new FuncPairSpatialContourPoint(point,spatialcontour);
    if(zeta0() == NULL)
      zeta0.resize(2);
    else if(zeta0.size() != 2)
      MBSimError("(ContactKinematicsPointSpatialContour::assignContours): size of zeta0 does not match");
  }

  void ContactKinematicsPointSpatialContour::updateg(double &g, vector<ContourFrame*> &cFrame, int index) {

    SpatialContactSearch search(func);

    if ((spatialcontour->getEtaNodes().size() != 0) && (spatialcontour->getXiNodes().size() != 0))
      search.setNodes(spatialcontour->getEtaNodes(), spatialcontour->getXiNodes());
    else
      search.setEqualSpacing(10, 10, 0, 0, 0.1, 0.1);

    if (searchAllCP==false) {
      search.setInitialValue(zeta0);
    }
    else {
      search.setSearchAll(true);
      searchAllCP = false;
    }

    zeta0 = search.slv();
    cFrame[ispatialcontour]->setZeta(zeta0);

    cFrame[ispatialcontour]->getOrientation(false).set(0, spatialcontour->evalWn(cFrame[ispatialcontour]->getZeta()));
    cFrame[ispatialcontour]->getOrientation(false).set(1, spatialcontour->evalWu(cFrame[ispatialcontour]->getZeta()));
    cFrame[ispatialcontour]->getOrientation(false).set(2, spatialcontour->evalWv(cFrame[ispatialcontour]->getZeta()));

    cFrame[ipoint]->getOrientation(false).set(0, -cFrame[ispatialcontour]->getOrientation(false).col(0));
    cFrame[ipoint]->getOrientation(false).set(1, -cFrame[ispatialcontour]->getOrientation(false).col(1));
    cFrame[ipoint]->getOrientation(false).set(2, cFrame[ispatialcontour]->getOrientation(false).col(2));

    cFrame[ispatialcontour]->setPosition(spatialcontour->evalPosition(cFrame[ispatialcontour]->getZeta()));
    cFrame[ipoint]->setPosition(point->getFrame()->evalPosition());

    if(spatialcontour->isZetaOutside(cFrame[ispatialcontour]->getZeta()))
      g = 1;
    else
      g = cFrame[ispatialcontour]->getOrientation(false).col(0).T() * (cFrame[ipoint]->getPosition(false) - cFrame[ispatialcontour]->getPosition(false));
    if(g < -spatialcontour->getThickness()) g = 1;
  }

  void ContactKinematicsPointSpatialContour::updatewb(Vec &wb, double g, vector<ContourFrame*> &cFrame) {
    throw MBSim::MBSimError("(ContactKinematicsPointSpatialContour:updatewb): Not implemented!");
  }

}
