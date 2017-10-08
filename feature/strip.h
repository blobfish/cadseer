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
    QString picturePath;
  };
  
  class Strip : public Base
  {
  public:
    constexpr static const char *part = "Part";
    constexpr static const char *blank = "Blank";
    constexpr static const char *nest = "Nest";
    
    Strip();
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Strip;}
    virtual const std::string& getTypeString() const override {return toString(Type::Strip);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
//     void serialRead(const prj::srl::FeatureStrip &);
    
    void setLabelColors(const osg::Vec4&);
    void setAutoCalc(bool acIn){autoCalc = acIn;}
    bool isAutoCalc(){return autoCalc;}
    
    StripData stripData;
    
  protected:
    std::shared_ptr<Parameter> pitch;
    std::shared_ptr<Parameter> width;
    std::shared_ptr<Parameter> widthOffset;
    std::shared_ptr<Parameter> gap;
    osg::ref_ptr<lbr::PLabel> pitchLabel;
    osg::ref_ptr<lbr::PLabel> widthLabel;
    osg::ref_ptr<lbr::PLabel> widthOffsetLabel; //!< centerline of die.
    osg::ref_ptr<lbr::PLabel> gapLabel;
    std::vector<osg::ref_ptr<osg::MatrixTransform>> stationLabels;
    
    osg::Vec3d feedDirection; //!< eventually a parameter.
    bool autoCalc = true; //!< eventually a parameter.
    
  private:
    static QIcon icon;
    void exportSheet();
  };
}

#endif // FTR_STRIP_H
