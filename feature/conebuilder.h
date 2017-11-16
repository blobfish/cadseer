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

#ifndef FTR_CONEBUILDER_H
#define FTR_CONEBUILDER_H

#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>

namespace ftr
{
  class ConeBuilder
  {
  public:
    ConeBuilder(const double &radius1, const double &radius2, const double &height, gp_Ax2 axis2 = gp_Ax2());
    const TopoDS_Shape& getSolid() const {return solid;}
    const TopoDS_Shape& getShell() const {return shell;}
    const TopoDS_Shape& getFaceBottom() const {return faceBottom;}
    const TopoDS_Shape& getFaceConical() const {return faceConical;}
    const TopoDS_Shape& getFaceTop() const {return faceTop;}
    const TopoDS_Shape& getWireBottom() const {return wireBottom;}
    const TopoDS_Shape& getWireConical() const {return wireConical;}
    const TopoDS_Shape& getWireTop() const {return wireTop;}
    const TopoDS_Shape& getEdgeBottom() const {return edgeBottom;}
    const TopoDS_Shape& getEdgeConical() const {return edgeConical;}
    const TopoDS_Shape& getEdgeTop() const {return edgeTop;}
    const TopoDS_Shape& getVertexBottom() const {return vertexBottom;}
    const TopoDS_Shape& getVertexTop() const {return vertexTop;}
    
  private:
    TopoDS_Shape solid;
    TopoDS_Shape shell;
    TopoDS_Shape faceBottom;
    TopoDS_Shape faceConical;
    TopoDS_Shape faceTop;
    TopoDS_Shape wireBottom;
    TopoDS_Shape wireConical;
    TopoDS_Shape wireTop;
    TopoDS_Shape edgeBottom;
    TopoDS_Shape edgeConical;
    TopoDS_Shape edgeTop;
    TopoDS_Shape vertexBottom;
    TopoDS_Shape vertexTop;
  };
}

#endif // FTR_CONEBUILDER_H
