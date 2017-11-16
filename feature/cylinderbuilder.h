/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef FTR_CYLINDERBUILDER_H
#define FTR_CYLINDERBUILDER_H

#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>

namespace ftr
{
  class CylinderBuilder
  {
  public:
    CylinderBuilder(const double &radiusIn, const double &heightIn, gp_Ax2 axis2 = gp_Ax2());
    const TopoDS_Shape& getSolid() const {return solid;}
    const TopoDS_Shape& getShell() const {return shell;}
    const TopoDS_Shape& getFaceBottom() const {return faceBottom;}
    const TopoDS_Shape& getFaceCylindrical() const {return faceCylindrical;}
    const TopoDS_Shape& getFaceTop() const {return faceTop;}
    const TopoDS_Shape& getWireBottom() const {return wireBottom;}
    const TopoDS_Shape& getWireCylindrical() const {return wireCylindrical;}
    const TopoDS_Shape& getWireTop() const {return wireTop;}
    const TopoDS_Shape& getEdgeBottom() const {return edgeBottom;}
    const TopoDS_Shape& getEdgeCylindrical() const {return edgeCylindrical;}
    const TopoDS_Shape& getEdgeTop() const {return edgeTop;}
    const TopoDS_Shape& getVertexBottom() const {return vertexBottom;}
    const TopoDS_Shape& getVertexTop() const {return vertexTop;}
    
  private:
    TopoDS_Shape solid;
    TopoDS_Shape shell;
    TopoDS_Shape faceBottom;
    TopoDS_Shape faceCylindrical;
    TopoDS_Shape faceTop;
    TopoDS_Shape wireBottom;
    TopoDS_Shape wireCylindrical;
    TopoDS_Shape wireTop;
    TopoDS_Shape edgeBottom;
    TopoDS_Shape edgeCylindrical;
    TopoDS_Shape edgeTop;
    TopoDS_Shape vertexBottom;
    TopoDS_Shape vertexTop;
  };
}

#endif // FTR_CYLINDERBUILDER_H
