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

#ifndef MDV_SHAPEGEOMETRY_H
#define MDV_SHAPEGEOMETRY_H

#include <memory>

#include <osg/BoundingSphere>

#include <TopoDS_Shape.hxx>
#include <Bnd_Box.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>

#include <modelviz/base.h>

namespace boost{namespace uuids{class uuid;}}
class TopoDS_Face; class TopoDS_Edge;
namespace osg{class Switch; class Depth; class LineWidth;}
namespace ann{class ShapeIdHelper;}

namespace mdv
{
  class IdPSetWrapper;
  class PSetPrimitiveWrapper;
  
  class ShapeGeometry : public Base
  {
  public:
    ShapeGeometry();
    ShapeGeometry(const ShapeGeometry &rhs, const osg::CopyOp& copyOperation = osg::CopyOp::SHALLOW_COPY);
    
    META_Node(mdv, ShapeGeometry)

    void setIdPSetWrapper(std::shared_ptr<IdPSetWrapper> &);
    void setPSetPrimitiveWrapper(std::shared_ptr<PSetPrimitiveWrapper> &);
    boost::uuids::uuid getId(std::size_t) const;
    std::size_t getPSetFromPrimitive(std::size_t) const;
    
    virtual void setColor(const osg::Vec4 &colorIn) override;
    
    void setToColor(const boost::uuids::uuid&); //!< set to color of primitive index.
    void setToPreHighlight(const boost::uuids::uuid&); //!< set to prehighlight of primitive index.
    void setToHighlight(const boost::uuids::uuid&); //!< set to highlight of primitive index.
    
    //need these for serialize
    const IdPSetWrapper& getIdPSetWrapper() const;
    const PSetPrimitiveWrapper& getPSetPrimitiveWrapper() const;
    
  protected:
    void setColor(const boost::uuids::uuid&, const osg::Vec4&); //set color of primitive index.
    
    std::shared_ptr<IdPSetWrapper> idPSetWrapper;
    std::shared_ptr<PSetPrimitiveWrapper> pSetVertexWrapper;
  };
  
  class ShapeGeometryBuilder
  {
  public:
    ShapeGeometryBuilder() = delete;
    ShapeGeometryBuilder(const TopoDS_Shape&, const ann::ShapeIdHelper&);
    ~ShapeGeometryBuilder();
    void go(double, double);
    void buildFaces(bool in){shouldBuildFaces = in;}
    void buildEdges(bool in){shouldBuildEdges = in;}
    void buildVertices(bool in){shouldBuildVertices = in;}
    osg::ref_ptr<osg::Switch> out;
    bool success = false;
    const std::vector<std::string>& getWarnings(){return warnings;}
    const std::vector<std::string>& getErrors(){return errors;}
  private:
    void initialize();
    void recursiveConstruct(const TopoDS_Shape &shapeIn);
    void edgeConstruct(const TopoDS_Edge &edgeIn);
    void faceConstruct(const TopoDS_Face &faceIn);
    TopoDS_Shape copiedShape;
    const ann::ShapeIdHelper &shapeIdHelper;
    Bnd_Box bound;
    TopTools_IndexedDataMapOfShapeListOfShape edgeToFace;
    TopTools_MapOfShape processed;
    std::shared_ptr<IdPSetWrapper> idPSetWrapperFace; //shared among all generated with this object
    std::shared_ptr<IdPSetWrapper> idPSetWrapperEdge; //shared among all generated with this object
    std::shared_ptr<PSetPrimitiveWrapper> pSetPrimitiveWrapperFace; //unique to each generation.
    std::shared_ptr<PSetPrimitiveWrapper> pSetPrimitiveWrapperEdge; //unique to each generation.
    bool shouldBuildFaces = true;
    bool shouldBuildEdges = true;
    bool shouldBuildVertices = false;
    osg::ref_ptr<ShapeGeometry> faceGeometry;
    osg::ref_ptr<ShapeGeometry> edgeGeometry;
    osg::ref_ptr<ShapeGeometry> vertexGeometry;
    osg::ref_ptr<osg::Depth> faceDepth;
    osg::ref_ptr<osg::LineWidth> lineWidth;
    std::size_t primitiveCountFace;
    std::size_t primitiveCountEdge;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
  };
}

#endif // MDV_SHAPEGEOMETRY_H
