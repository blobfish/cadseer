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
#include <algorithm>
#include <cassert>

#include <gp_Ax1.hxx>
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
#include <TopTools_ListOfShape.hxx>
#include <Bnd_Box.hxx>


namespace occt
{
  template<typename T>
  void uniquefy(T &t)
  {
    auto lessThan = [](const typename T::value_type& t1, const typename T::value_type& t2) -> bool
    {
      return t1.HashCode(std::numeric_limits<int>::max()) < t2.HashCode(std::numeric_limits<int>::max());
    };
    
    auto equal = [](const typename T::value_type& t1, const typename T::value_type& t2) -> bool
    {
      return t1.HashCode(std::numeric_limits<int>::max()) == t2.HashCode(std::numeric_limits<int>::max());
    };
    
    std::sort(t.begin(), t.end(), lessThan);
    auto last = std::unique(t.begin(), t.end(), equal);
    t.erase(last, t.end());
  }
  
  /*! get occt hash */
  inline int getShapeHash(const TopoDS_Shape &shape)
  {
    return shape.HashCode(std::numeric_limits<int>::max());
  }
  
  /*! get the shape type as a string*/
  inline std::string getShapeTypeString(const TopoDS_Shape &shapeIn)
  {
    static const std::vector<std::string> strings = 
    {
      "Compound",
      "CompSolid",
      "Solid",
      "Shell",
      "Face",
      "Wire",
      "Edge",
      "Vertex",
      "Shape"
    };
    
    std::size_t index = static_cast<std::size_t>(shapeIn.ShapeType());
    assert(index < strings.size());
    return strings.at(index);
  };
  
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
    ShapeVectorCast(const TopTools_ListOfShape &listIn);
    
    //these will filter for appropriate types.
    operator ShapeVector() const;
    operator TopoDS_Compound() const;
    operator SolidVector() const;
    operator ShellVector() const;
    operator FaceVector() const;
    operator WireVector() const;
    operator EdgeVector() const;
    operator VertexVector() const;
    operator TopTools_MapOfShape() const;
    operator TopTools_IndexedMapOfShape() const;
    operator TopTools_ListOfShape() const;
    
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
   * @return A pair of wire vectors. first are the closed bounds, second is the open bounds.
   */
  std::pair<WireVector, WireVector> getBoundaryWires(const TopoDS_Shape &shapeIn);
  
  /*! @brief Get normal of a face
   * 
   * @param fIn face for normal calculation
   * @param u parameter in the u direction
   * @param v parameter in the v direction
   * @return A vector normal to face
   */
  gp_Vec getNormal(const TopoDS_Face &fIn, double u, double v);

  /*! @brief Derive a direction vector from shape.
   * 
   * @param shapeIn shape to infer
   * @param vec where on shape to infer direction 
   * @return A pair with unit vector and a boolean signaling success.
   */  
  std::pair<gp_Vec, bool> gleanVector(const TopoDS_Shape &shapeIn, const gp_Pnt &pIn);
  
  /*! @brief Derive an axis from input
   * 
   * @param sIn face or edge
   * @return pair with axis bool status if worked.
   * @note origin of axis will be center point of bounding box of shape projected onto axis.
   */
  std::pair<gp_Ax1, bool> gleanAxis(const TopoDS_Shape sIn);
  
  /*! @brief gets and edge parameter closest to a point.
   * 
   * @param eIn edge
   * @return pair with value and boolean signaling success.
   * @note takes the first solution of extrema
   */
  std::pair<double, bool> pointToParameter(const TopoDS_Edge &eIn, const gp_Pnt &pIn);
  
  /*! @brief gets the face parameter closest to a point.
   * 
   * @param fIn face
   * @return tuple consisting of parameters and boolean signal.
   * @note takes the first solution of extrema
   */
  std::tuple<double, double, bool> pointToParameter(const TopoDS_Face &fIn, const gp_Pnt &pIn);
  
  /*! @brief gets the point at the given edge parameter
   * 
   * @param eIn edge
   * @param u parameter
   * @return point
   */
  gp_Pnt parameterToPoint(const TopoDS_Edge &eIn, double u);
  
  /*! @brief gets the point at the given face parameters
   * 
   * @param fIn edge
   * @param u parameter
   * @param v parameter
   * @return point
   */
  gp_Pnt parameterToPoint(const TopoDS_Face &fIn, double u, double v);
  
  /*! @brief Copy shape at distance.
   * 
   * @param sIn shape to copy
   * @param dir unit direction to instance
   * @param distance distance of instance.
   */
  TopoDS_Shape instanceShape(const TopoDS_Shape sIn, const gp_Vec &dir, double distance);
  
  /*! @brief Move a shape by distance and direction
   * 
   * @param sIn shape to move
   * @param dir unit direction of movement.
   * @param distance distance to move.
   */
  void moveShape(TopoDS_Shape &sIn, const gp_Vec &dir, double distance);
  
  /*! @brief Accumalate unique child shapes.
   * 
   * @note this method ignores degenerate edges.
   * @param sIn shape to explore
   * @return vector of unique child shapes
   */
  ShapeVector mapShapes(const TopoDS_Shape &sIn);
  
  class BoundingBox
  {
  public:
    BoundingBox();
    BoundingBox(const TopoDS_Shape&);
    BoundingBox(const ShapeVector&);
    
    void add(const TopoDS_Shape&);
    void add(const ShapeVector&);
    
    const Bnd_Box& getOcctBox();
    double getLength();
    double getWidth();
    double getHeight();
    double getDiagonal();
    gp_Pnt getCenter();
    const std::vector<gp_Pnt>& getCorners();
    std::vector<int> getFaceIndexes(); //!< 24 referencing corners. Faces: x- x+ y- y+ z- z+
    
  private:
    void update();
    
    ShapeVector sv;
    Bnd_Box occtBox;
    
    gp_Pnt center;
    std::vector<gp_Pnt> corners;//!< 8 points. xMin, yMin, zMin is at(0). then clockwise around z- face.
    double length = 0.0;
    double width = 0.0;
    double height = 0.0;
    double diagonal = 0.0;
    
    bool dirty = true;
  };
}

inline std::ostream& operator<<(std::ostream &st, const TopoDS_Shape &sh)
{
  st << "Shape hash: " << occt::getShapeHash(sh)
    << "    Shape type: " << occt::getShapeTypeString(sh);
  return st;
}

#endif // GU_OCCTOOLS_H
