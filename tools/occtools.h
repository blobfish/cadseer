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

#ifndef GU_OCCTOOLS_H
#define GU_OCCTOOLS_H

#include <vector>
#include <stack>
#include <algorithm>

#include <Standard_StdAllocator.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_MapOfShape.hxx>

namespace occt
{
  template<typename T>
  void uniquefy(T &t)
  {
    struct Predicate
    {
      bool operator()(const typename T::value_type& t1, const typename T::value_type& t2)
      {
        return t1.HashCode(std::numeric_limits<int>::max()) < t2.HashCode(std::numeric_limits<int>::max());
      }
    };
    Predicate p;
    std::sort(t.begin(), t.end(), p);
    auto last = std::unique(t.begin(), t.end(), p);
    t.erase(last, t.end());
  }
  
  typedef std::vector<TopoDS_Shape, Standard_StdAllocator<TopoDS_Shape> > ShapeVector;
  typedef std::vector<TopoDS_Solid, Standard_StdAllocator<TopoDS_Solid> > SolidVector;
  typedef std::vector<TopoDS_Shell, Standard_StdAllocator<TopoDS_Shell> > ShellVector;
  typedef std::vector<TopoDS_Face, Standard_StdAllocator<TopoDS_Face> > FaceVector;
  typedef std::vector<TopoDS_Wire, Standard_StdAllocator<TopoDS_Wire> > WireVector;
  typedef std::vector<TopoDS_Edge, Standard_StdAllocator<TopoDS_Edge> > EdgeVector;
  typedef std::vector<TopoDS_Vertex, Standard_StdAllocator<TopoDS_Vertex> > VertexVector;
  
  class ShapeVectorCast
  {
  public:
    ShapeVectorCast(const ShapeVector &shapeVectorIn);
    ShapeVectorCast(const TopoDS_Compound &compoundIn);
    ShapeVectorCast(const SolidVector &solidVectorIn);
    ShapeVectorCast(const ShellVector &shellVectorIn);
    ShapeVectorCast(const FaceVector &faceVectorIn);
    ShapeVectorCast(const WireVector &wireVectorIn);
    ShapeVectorCast(const EdgeVector &edgeVectorIn);
    ShapeVectorCast(const VertexVector &vertexVectorIn);
    ShapeVectorCast(const TopTools_MapOfShape &mapIn);
    ShapeVectorCast(const TopTools_IndexedMapOfShape &mapIn);
    
    //these will filter for appropriate types.
    operator ShapeVector() const;
    operator TopoDS_Compound() const;
    operator SolidVector() const;
    operator ShellVector() const;
    operator FaceVector() const;
    operator WireVector() const;
    operator EdgeVector() const;
    operator VertexVector() const;
    
  private:
    ShapeVector shapeVector;
  };
  
  /*! @brief collect all faces inside shape that is tangent to face within degree tolerance*/
  FaceVector walkTangentFaces(const TopoDS_Shape&, const TopoDS_Face&, double t = 0.0);
  
  /*! @brief Find the first shape that isn't a compound.
   * 
   * @param shapeIn shape to be searched
   * @return a first "non compound" shape. Maybe NULL.
   * @note may loose shape if last compound contains more than 1 shape.
   */
  TopoDS_Shape getFirstNonCompound(const TopoDS_Shape &shapeIn);
  
  /*! @brief Get the last compound in a chain of compounds.
   * 
   * @param compoundIn shape to be searched.
   * @return last unique compound. Can be null if not unique compound is NOT found.
   * @note A unique compound is a compound that contains something
   * other than just another compound.
   */
  TopoDS_Compound getLastUniqueCompound(const TopoDS_Compound &compoundIn);
  
  /*! @brief Get the boundary wires of a shape.
   * 
   * @param shapeIn shape to be searched.
   * @return A vector wires. can be empty.
   */
  WireVector getBoundaryWires(const TopoDS_Shape &shapeIn);
}

#endif // GU_OCCTOOLS_H
