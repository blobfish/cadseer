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

#include <osg/Geometry>

#include <TopoDS_Shape.hxx>
#include <Bnd_Box.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>

class TopoDS_Face; class TopoDS_Edge;
namespace osg{class Switch; class Depth; class LineWidth;}
namespace ftr{class ResultContainerWrapper;}

namespace mdv
{
  class IdPSetWrapper;
  class PSetVertexWrapper;
  
  class ShapeGeometry : public osg::Geometry
  {
  public:
    ShapeGeometry();
    ShapeGeometry(const ShapeGeometry &rhs, const osg::CopyOp& copyOperation = osg::CopyOp::SHALLOW_COPY);

    virtual osg::Object* cloneType() const override {return new ShapeGeometry();}
    virtual osg::Object* clone(const osg::CopyOp& copyOperation) const override {return new ShapeGeometry(*this, copyOperation);}
    virtual bool isSameKindAs(const osg::Object* obj) const override {return dynamic_cast<const ShapeGeometry*>(obj)!=NULL;}
    /* using same libray name and class name as parent let the geometry be included
     * in the exported osg file. Don't know the implications */
    virtual const char* libraryName() const override {return "osg";}
    virtual const char* className() const override {return "Geometry";}
    
    void setIdPSetWrapper(std::shared_ptr<IdPSetWrapper> &);
    void setPSetVertexWrapper(std::shared_ptr<PSetVertexWrapper> &);
    boost::uuids::uuid getId(std::size_t primitiveIndexIn) const;
    std::size_t getPSetFromVertex(std::size_t) const;
    
    void setColor(const osg::Vec4 &colorIn);
    osg::Vec4 getColor() const {return color;}
    void setPreHighlightColor(const osg::Vec4 &colorIn);
    void setHighlightColor(const osg::Vec4 &colorIn);
    
    void setToColor(const boost::uuids::uuid&); //!< set to color of primitive index.
    void setToPreHighlight(const boost::uuids::uuid&); //!< set to prehighlight of primitive index.
    void setToHighlight(const boost::uuids::uuid&); //!< set to highlight of primitive index.
    
  protected:
    void setColor(const boost::uuids::uuid&, const osg::Vec4&); //set color of primitive index.
    
    std::shared_ptr<IdPSetWrapper> idPSetWrapper;
    std::shared_ptr<PSetVertexWrapper> pSetVertexWrapper;
    osg::Vec4 color = osg::Vec4(.1f, .7f, .1f, .5f);
    osg::Vec4 colorPreHighlight = osg::Vec4(1.0, 1.0, 0.0, 1.0);
    osg::Vec4 colorHighlight = osg::Vec4(1.0, 1.0, 1.0, 1.0);
  };
  
  class ShapeGeometryBuilder
  {
  public:
    ShapeGeometryBuilder(const TopoDS_Shape &, const ftr::ResultContainerWrapper &);
    ~ShapeGeometryBuilder();
    void go(double, double);
    void buildFaces(bool in){shouldBuildFaces = in;}
    void buildEdges(bool in){shouldBuildEdges = in;}
    void buildVertices(bool in){shouldBuildVertices = in;}
    osg::ref_ptr<osg::Switch> out;
    bool success = false;
  private:
    void initialize();
    void recursiveConstruct(const TopoDS_Shape &shapeIn);
    void edgeConstruct(const TopoDS_Edge &edgeIn);
    void faceConstruct(const TopoDS_Face &faceIn);
    const TopoDS_Shape &originalShape;
    TopoDS_Shape copiedShape;
    const ftr::ResultContainerWrapper &resultWrapper;
    Bnd_Box bound;
    TopTools_IndexedDataMapOfShapeListOfShape edgeToFace;
    TopTools_MapOfShape processed;
    std::shared_ptr<IdPSetWrapper> idPSetWrapperFace; //shared among all generated with this object
    std::shared_ptr<IdPSetWrapper> idPSetWrapperEdge; //shared among all generated with this object
    std::shared_ptr<PSetVertexWrapper> pSetTriangleWrapper; //unique to each generation.
    bool shouldBuildFaces = true;
    bool shouldBuildEdges = true;
    bool shouldBuildVertices = false;
    osg::ref_ptr<ShapeGeometry> faceGeometry;
    osg::ref_ptr<ShapeGeometry> edgeGeometry;
    osg::ref_ptr<ShapeGeometry> vertexGeometry;
    osg::ref_ptr<osg::Depth> faceDepth;
    osg::ref_ptr<osg::Depth> edgeDepth;
    osg::ref_ptr<osg::LineWidth> lineWidth;
  };
}

#endif // MDV_SHAPEGEOMETRY_H
