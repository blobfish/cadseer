/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_TRIM_H
#define FTR_TRIM_H

#include <osg/ref_ptr>

#include <feature/base.h>

class TopoDS_Solid;

namespace lbr{class PLabel;}
namespace ann{class SeerShape; class IntersectionMapper;}
namespace prj{namespace srl{class FeatureTrim;}}

namespace ftr
{
  /**
  * @todo write docs
  */
  class Trim : public Base
  {
    public:
      Trim();
      virtual ~Trim() override;
      
      virtual void updateModel(const UpdatePayload&) override;
      virtual Type getType() const override {return Type::Trim;}
      virtual const std::string& getTypeString() const override {return toString(Type::Trim);}
      virtual const QIcon& getIcon() const override {return icon;}
      virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
      
      virtual void serialWrite(const QDir&) override;
      void serialRead(const prj::srl::FeatureTrim&);
      
    protected:
      std::unique_ptr<prm::Parameter> reversed;
      std::unique_ptr<ann::SeerShape> sShape;
      std::unique_ptr<ann::IntersectionMapper> iMapper;
      osg::ref_ptr<lbr::PLabel> reversedLabel;
      
      TopoDS_Solid makeHalfSpace(const ann::SeerShape &seerShapeIn, const TopoDS_Shape &shapeIn);
      
    private:
      static QIcon icon;
  };
}

#endif // FTR_TRIM_H
