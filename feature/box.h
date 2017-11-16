/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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


#ifndef BOX_H
#define BOX_H

#include <osg/ref_ptr>

#include <feature/base.h>

namespace lbr{class IPGroup;}
namespace prj{namespace srl{class FeatureBox;}}
namespace ann{class CSysDragger; class SeerShape;}

namespace ftr
{
  
class BoxBuilder;

class Box : public Base
{
public:
  Box();
  virtual ~Box() override;
  void setLength(const double &lengthIn);
  void setWidth(const double &widthIn);
  void setHeight(const double &heightIn);
  void setParameters(const double &lengthIn, const double &widthIn, const double &heightIn);
  void setCSys(const osg::Matrixd&);
  double getLength() const {return static_cast<double>(length);}
  double getWidth() const {return static_cast<double>(width);}
  double getHeight() const {return static_cast<double>(height);}
  void getParameters (double &lengthOut, double &widthOut, double &heightOut) const;
  osg::Matrixd getCSys() const {return static_cast<osg::Matrixd>(csys);}
  
  virtual void updateModel(const UpdatePayload&) override;
  virtual Type getType() const override {return Type::Box;}
  virtual const std::string& getTypeString() const override {return toString(Type::Box);}
  virtual const QIcon& getIcon() const override {return icon;}
  virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
  virtual void serialWrite(const QDir&) override; //!< write xml file. not const, might reset a modified flag.
  void serialRead(const prj::srl::FeatureBox &sBox); //!<initializes this from sBox. not virtual, type already known.
  
protected:
  prm::Parameter length;
  prm::Parameter width;
  prm::Parameter height;
  prm::Parameter csys;
  
  std::unique_ptr<ann::CSysDragger> csysDragger;
  std::unique_ptr<ann::SeerShape> sShape;
  
  osg::ref_ptr<lbr::IPGroup> lengthIP;
  osg::ref_ptr<lbr::IPGroup> widthIP;
  osg::ref_ptr<lbr::IPGroup> heightIP;
  
  void initializeMaps();
  void updateResult(const BoxBuilder&);
  void setupIPGroup();
  void updateIPGroup();
  
  
private:
  static QIcon icon;
};
}

#endif // BOX_H
