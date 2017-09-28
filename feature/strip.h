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

#ifndef FTR_STRIP_H
#define FTR_STRIP_H

#include <feature/parameter.h>
#include <library/plabel.h>
#include <feature/base.h>

namespace ftr
{
  struct StripData
  {
    int quoteNumber;
    QString customerName;
    boost::uuids::uuid customerId; //not used yet.
    QString partName;
    QString partNumber;
    QString partSetup;
    QString partRevision;
    QString materialType;
    double materialThickness;
    QString processType;
    int annualVolume;
    std::vector<QString> stations;
  };
  
  class Strip : public Base
  {
  public:
    constexpr static const char *part = "Part";
    constexpr static const char *blank = "Blank";
    
    Strip();
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Strip;}
    virtual const std::string& getTypeString() const override {return toString(Type::Strip);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
//     void serialRead(const prj::srl::FeatureStrip &);
    
    StripData stripData;
    
  protected:
    std::shared_ptr<Parameter> pitch; //!< 0 means auto update.
    osg::ref_ptr<lbr::PLabel> pitchLabel;
    
  private:
    static QIcon icon;
    void exportSheet();
  };
}

#endif // FTR_STRIP_H
