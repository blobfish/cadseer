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

#ifndef FTR_REFINE_H
#define FTR_REFINE_H

#include <feature/base.h>

class BRepTools_History;

namespace ann{class SeerShape;}
namespace prj{namespace srl{class FeatureRefine;}}

namespace ftr
{
  /**
  * @todo write docs
  */
  class Refine : public Base
  {
  public:
    Refine();
    virtual ~Refine() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Refine;}
    virtual const std::string& getTypeString() const override {return toString(Type::Refine);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureRefine &);
    
  protected:
    std::unique_ptr<ann::SeerShape> sShape;
    /*! used to map new geometry to old geometry.
     */ 
    std::map<boost::uuids::uuid, boost::uuids::uuid> shapeMap;
    
    void historyMatch(const BRepTools_History& , const ann::SeerShape&);
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_REFINE_H
