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

#ifndef FTR_DIESET_H
#define FTR_DIESET_H

#include <memory>

#include <osg/ref_ptr>

#include <feature/parameter.h>
#include <feature/base.h>

namespace lbr{class PLabel;}
namespace prj{namespace srl{class FeatureDieSet;}}

namespace ftr
{
  /*! should this inherit from csysbase? for now, location
   * is calculated from the inputs reqardless of autoCalc value
   */
  class DieSet : public Base
  {
  public:
    constexpr static const char *strip = "Strip";
    
    DieSet();
    virtual ~DieSet() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::DieSet;}
    virtual const std::string& getTypeString() const override {return toString(Type::DieSet);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureDieSet &);
    
    double getLength() const;
    double getWidth() const;
    
  protected:
    std::shared_ptr<Parameter> length; //!< only used if autoCalc is false.
    std::shared_ptr<Parameter> width; //!< only used if autoCalc is false.
    std::shared_ptr<Parameter> lengthPadding; //!< only used if autoCalc is true.
    std::shared_ptr<Parameter> widthPadding; //!< only used if autoCalc is true.
    bool autoCalc = true; //!< eventually a parameter.
    
    osg::ref_ptr<lbr::PLabel> lengthLabel;
    osg::ref_ptr<lbr::PLabel> widthLabel;
    osg::ref_ptr<lbr::PLabel> lengthPaddingLabel;
    osg::ref_ptr<lbr::PLabel> widthPaddingLabel;
    
  private:
    static QIcon icon;
  };

}

#endif // FTR_DIESET_H
