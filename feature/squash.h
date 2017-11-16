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

#ifndef FTR_SQUASH_H
#define FTR_SQUASH_H

#include <feature/pick.h>
#include <feature/parameter.h>
#include <library/plabel.h>
#include <feature/base.h>

namespace prj{namespace srl{class FeatureSquash;}}
namespace ann{class SeerShape;}

namespace ftr
{
  /*! @brief Flattening of a sheet
   * 
   * This is usually a slow updating feature.
   * So user can set the granularity to zero
   * and that will effectly 'freeze' the update.
   */
  class Squash : public Base
  {
  public:
    Squash();
    virtual ~Squash() override;
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Squash;}
    virtual const std::string& getTypeString() const override {return toString(Type::Squash);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureSquash &);
    
    void setPicks(const Picks &psIn){picks = psIn;}
    
    int getGranularity();
    void setGranularity(int);
  private:
    static QIcon icon;
    Picks picks;
    boost::uuids::uuid faceId; //!< id of the generated face.
    boost::uuids::uuid wireId; //!< outer wire of face.
    std::shared_ptr<prm::Parameter> granularity; //!< 0 means no update.
    osg::ref_ptr<lbr::PLabel> label;
    std::unique_ptr<ann::SeerShape> sShape;
  };
}

#endif // FTR_SQUASH_H
