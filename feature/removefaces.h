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

#ifndef FTR_REMOVEFACES_H
#define FTR_REMOVEFACES_H

#include <feature/pick.h>
#include <feature/base.h>

namespace prj{namespace srl{class FeatureRemoveFaces;}}

namespace ftr
{
  /**
  * @brief remove faces from a solid.
  */
  class RemoveFaces : public Base
  {
    public:
      RemoveFaces();
      virtual ~RemoveFaces() override;
      
      virtual void updateModel(const UpdatePayload&) override;
      virtual Type getType() const override {return Type::RemoveFaces;}
      virtual const std::string& getTypeString() const override {return toString(Type::RemoveFaces);}
      virtual const QIcon& getIcon() const override {return icon;}
      virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
      
      virtual void serialWrite(const QDir&) override;
      void serialRead(const prj::srl::FeatureRemoveFaces&);
      
      void setPicks(const Picks&);
      Picks getPicks(){return picks;}
      
    protected:
      std::unique_ptr<ann::SeerShape> sShape;
      Picks picks;
      
    private:
      static QIcon icon;
  };
}

#endif // FTR_REMOVEFACES_H
