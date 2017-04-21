/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_CHAMFER_H
#define FTR_CHAMFER_H

#include <map>

#include <library/plabel.h>
#include <feature/base.h>

class BRepFilletAPI_MakeChamfer;
namespace prj{namespace srl{class FeatureChamfer;}}

namespace ftr
{
  class SeerShape;
  struct ChamferPick
  {
    boost::uuids::uuid edgeId; //!< id of picked edge or maybe face?
    double u; //!< u parameter on picked object
    double v;//!< v parameter on picked object
    boost::uuids::uuid faceId; //!< reference face. set with referenceFaceId.
  };
  
  struct SymChamfer
  {
    std::vector<ChamferPick> picks;
    std::shared_ptr<Parameter> distance; //!< parameter containing distance.
    osg::ref_ptr<lbr::PLabel> label; //!< graphic icon
  };

  class Chamfer : public Base
  {
  public:
    Chamfer();
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Chamfer;}
    virtual const std::string& getTypeString() const override {return toString(Type::Chamfer);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureChamfer &);
    
    static std::shared_ptr<Parameter> buildSymParameter();
    static boost::uuids::uuid referenceFaceId(const SeerShape&, const boost::uuids::uuid&);
    static double calculateUParameter(const TopoDS_Edge&, const osg::Vec3d&);
    static osg::Vec3d calculateUPoint(const TopoDS_Edge&, double);
    
    void addSymChamfer(const SymChamfer &);
  private:
    void generatedMatch(BRepFilletAPI_MakeChamfer&, const SeerShape &);
    std::vector<SymChamfer> symChamfers;
    std::map<boost::uuids::uuid, boost::uuids::uuid> shapeMap; //!< map edges or vertices to faces
    
    static QIcon icon;
  };
}

#endif // CHAMFER_H
