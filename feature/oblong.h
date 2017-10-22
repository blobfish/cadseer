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

#ifndef FTR_OBLONG_H
#define FTR_OBLONG_H

#include <osg/ref_ptr>

#include <feature/csysbase.h>

namespace lbr{class IPGroup;}
namespace prj{namespace srl{class FeatureOblong;}}

namespace ftr
{
  class OblongBuilder;
  
  class Oblong : public CSysBase
  {
  public:
    Oblong();
    virtual ~Oblong() override;
    void setLength(const double &lengthIn);
    void setWidth(const double &widthIn);
    void setHeight(const double &heightIn);
    void setParameters(const double &lengthIn, const double &widthIn, const double &heightIn);
    double getLength() const {return static_cast<double>(length);}
    double getWidth() const {return static_cast<double>(width);}
    double getHeight() const {return static_cast<double>(height);}
    void getParameters (double &lengthOut, double &widthOut, double &heightOut) const;
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Oblong;}
    virtual const std::string& getTypeString() const override {return toString(Type::Oblong);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override; //!< write xml file. not const, might reset a modified flag.
    void serialRead(const prj::srl::FeatureOblong &sOblong);
  protected:
    prm::Parameter length;
    prm::Parameter width;
    prm::Parameter height;
    
    osg::ref_ptr<lbr::IPGroup> lengthIP;
    osg::ref_ptr<lbr::IPGroup> widthIP;
    osg::ref_ptr<lbr::IPGroup> heightIP;
    
    void initializeMaps();
    void updateResult(const OblongBuilder&);
    void setupIPGroup();
    void updateIPGroup();
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_OBLONG_H
