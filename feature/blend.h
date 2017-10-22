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

#ifndef BLEND_H
#define BLEND_H

#include <memory>

#include <osg/ref_ptr>

#include <library/plabel.h>
#include <feature/parameter.h>
#include <feature/pick.h>
#include <feature/base.h>

class BRepFilletAPI_MakeFillet;
class TopoDS_Edge;

namespace prj{namespace srl{class FeatureBlend;}}

namespace ftr
{
  class SeerShape;
  class ShapeHistory;

struct SimpleBlend
{
  SimpleBlend();
  boost::uuids::uuid id; //!< just used for runtime sync with dialog. no serial etc...
  Picks picks; //!< vector of picked objects
  std::shared_ptr<prm::Parameter> radius; //!< parameter containing blend radius.
  osg::ref_ptr<lbr::PLabel> label; //!< graphic icon
};

struct VariableEntry
{
  Pick pick; //!< edge or vertex.
  std::shared_ptr<prm::Parameter> position; //!< parameter along edge 0 to 1. ignored if vertex. maybe invalid
  std::shared_ptr<prm::Parameter> radius; //!< value of blend.
  osg::ref_ptr<lbr::PLabel> label; //!< graphic icon
};

struct VariableBlend
{
  VariableBlend();
  boost::uuids::uuid id; //!< just used for runtime sync with dialog. no serial etc...
  Pick pick; //!< pick object.
  std::vector<VariableEntry> entries;
};
  
class Blend : public Base
{
  public:
    Blend();
    
    static std::shared_ptr<prm::Parameter> buildRadiusParameter();
    static std::shared_ptr<prm::Parameter> buildPositionParameter();
    static VariableBlend buildDefaultVariable(const SeerShape&, const Pick &, const ShapeHistory&);
    
    void addSimpleBlend(const SimpleBlend&);
    void addVariableBlend(const VariableBlend&);
    std::vector<SimpleBlend>& getSimpleBlends(){return simpleBlends;}
    std::vector<VariableBlend>& getVariableBlends(){return variableBlends;}
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Blend;}
    virtual const std::string& getTypeString() const override {return toString(Type::Blend);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
    virtual void serialWrite(const QDir&) override; //!< write xml file. not const, might reset a modified flag.
    void serialRead(const prj::srl::FeatureBlend &); //!<initializes this from sBox. not virtual, type already known.
  
  protected:
    std::vector<SimpleBlend> simpleBlends;
    std::vector<VariableBlend> variableBlends;
    
    /*! used to map the edges that are blended away to the face generated.
     * used to map new generated face to outer wire.
     */ 
    std::map<boost::uuids::uuid, boost::uuids::uuid> shapeMap; //!< map edges or vertices to faces
    
private:
    void generatedMatch(BRepFilletAPI_MakeFillet&, const SeerShape &);
    
    /*! now that we are 'resolving' picks we need to update the shapemap to ensure
     * consistant id output of generated faces.
     */
    void updateShapeMap(const boost::uuids::uuid&, const ShapeHistory &);
    void ensureNoFaceNils();
    void dumpInfo(BRepFilletAPI_MakeFillet&, const SeerShape&);
    
    static QIcon icon;
};
}

#endif // BLEND_H
