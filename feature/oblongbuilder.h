/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_OBLONGBUILDER_H
#define FTR_OBLONGBUILDER_H

#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>

namespace ftr
{
  /*! @brief helper class for oblong feature.
   * 
   * this allows introspection of the constructed oblong.
   */
  class OblongBuilder
  {
  public:
    OblongBuilder(double lengthIn, double widthIn, double heightIn, gp_Ax2 axis2 = gp_Ax2());
    
    const TopoDS_Shape& getSolid() const {return solid;}
    const TopoDS_Shape& getShell() const {return shell;}
    const TopoDS_Shape& getFaceXP() const {return faceXP;}
    const TopoDS_Shape& getFaceXN() const {return faceXN;}
    const TopoDS_Shape& getFaceYP() const {return faceYP;}
    const TopoDS_Shape& getFaceYN() const {return faceYN;}
    const TopoDS_Shape& getFaceZP() const {return faceZP;}
    const TopoDS_Shape& getFaceZN() const {return faceZN;}
    const TopoDS_Shape& getWireXP() const {return wireXP;}
    const TopoDS_Shape& getWireXN() const {return wireXN;}
    const TopoDS_Shape& getWireYP() const {return wireYP;}
    const TopoDS_Shape& getWireYN() const {return wireYN;}
    const TopoDS_Shape& getWireZP() const {return wireZP;}
    const TopoDS_Shape& getWireZN() const {return wireZN;}
    
    const TopoDS_Shape& getEdgeXPYP() const {return edgeXPYP;}
    const TopoDS_Shape& getEdgeXPZP() const {return edgeXPZP;}
    const TopoDS_Shape& getEdgeXPYN() const {return edgeXPYN;}
    const TopoDS_Shape& getEdgeXPZN() const {return edgeXPZN;}
    const TopoDS_Shape& getEdgeXNYN() const {return edgeXNYN;}
    const TopoDS_Shape& getEdgeXNZP() const {return edgeXNZP;}
    const TopoDS_Shape& getEdgeXNYP() const {return edgeXNYP;}
    const TopoDS_Shape& getEdgeXNZN() const {return edgeXNZN;}
    const TopoDS_Shape& getEdgeYPZP() const {return edgeYPZP;}
    const TopoDS_Shape& getEdgeYPZN() const {return edgeYPZN;}
    const TopoDS_Shape& getEdgeYNZP() const {return edgeYNZP;}
    const TopoDS_Shape& getEdgeYNZN() const {return edgeYNZN;}
    
    const TopoDS_Shape& getVertexXPYPZP() const {return vertexXPYPZP;}
    const TopoDS_Shape& getVertexXPYNZP() const {return vertexXPYNZP;}
    const TopoDS_Shape& getVertexXPYNZN() const {return vertexXPYNZN;}
    const TopoDS_Shape& getVertexXPYPZN() const {return vertexXPYPZN;}
    const TopoDS_Shape& getVertexXNYNZP() const {return vertexXNYNZP;}
    const TopoDS_Shape& getVertexXNYPZP() const {return vertexXNYPZP;}
    const TopoDS_Shape& getVertexXNYPZN() const {return vertexXNYPZN;}
    const TopoDS_Shape& getVertexXNYNZN() const {return vertexXNYNZN;}
    
  private:
    TopoDS_Shape solid;
    TopoDS_Shape shell;
    TopoDS_Shape faceXP;
    TopoDS_Shape faceXN;
    TopoDS_Shape faceYP;
    TopoDS_Shape faceYN;
    TopoDS_Shape faceZP;
    TopoDS_Shape faceZN;
    TopoDS_Shape wireXP;
    TopoDS_Shape wireXN;
    TopoDS_Shape wireYP;
    TopoDS_Shape wireYN;
    TopoDS_Shape wireZP;
    TopoDS_Shape wireZN;
    TopoDS_Shape edgeXPYP;
    TopoDS_Shape edgeXPZP;
    TopoDS_Shape edgeXPYN;
    TopoDS_Shape edgeXPZN;
    TopoDS_Shape edgeXNYN;
    TopoDS_Shape edgeXNZP;
    TopoDS_Shape edgeXNYP;
    TopoDS_Shape edgeXNZN;
    TopoDS_Shape edgeYPZP;
    TopoDS_Shape edgeYPZN;
    TopoDS_Shape edgeYNZP;
    TopoDS_Shape edgeYNZN;
    TopoDS_Shape vertexXPYPZP;
    TopoDS_Shape vertexXPYNZP;
    TopoDS_Shape vertexXPYNZN;
    TopoDS_Shape vertexXPYPZN;
    TopoDS_Shape vertexXNYNZP;
    TopoDS_Shape vertexXNYPZP;
    TopoDS_Shape vertexXNYPZN;
    TopoDS_Shape vertexXNYNZN;
  };
}

#endif // FTR_OBLONGBUILDER_H
