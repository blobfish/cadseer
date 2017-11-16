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

#ifndef FTR_EXTRACT_H
#define FTR_EXTRACT_H

#include <osg/ref_ptr>

#include <tools/idtools.h>
#include <library/plabel.h>
#include <feature/parameter.h>
#include <feature/pick.h>
#include <feature/base.h>

namespace prj{namespace srl{class FeatureExtract;}}
namespace ann{class SeerShape;}

namespace ftr
{
  class Extract : public Base
  {
  public:
    struct AccruePick
    {
      boost::uuids::uuid id = gu::createRandomId(); //!< just used for runtime sync with dialog. no serial etc...
      Picks picks; //!< seeds for accrue
      AccrueType accrueType;
      std::shared_ptr<prm::Parameter> parameter; //!< degrees for tangent tolerance.
      osg::ref_ptr<lbr::PLabel> label; //!< graphic icon
    };
    typedef std::vector<AccruePick> AccruePicks;
    
    static std::shared_ptr<prm::Parameter> buildAngleParameter(double deg = 0.0); //set up default.
    
    Extract();
    virtual ~Extract() override;
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Extract;}
    virtual const std::string& getTypeString() const override {return toString(Type::Extract);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureExtract &);

    const Picks& getPicks(){return picks;}
    const AccruePicks& getAccruePicks(){return accruePicks;}
    void sync(const Picks&);
    void sync(const AccruePicks&);

  private:
    static QIcon icon;
    Picks picks; //!< 1 to 1 geometry copies.
    AccruePicks accruePicks; //!< collections of geometries.
    
    std::unique_ptr<ann::SeerShape> sShape;
  };
}

#endif // FTR_EXTRACT_H
