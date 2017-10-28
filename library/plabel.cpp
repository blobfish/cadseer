/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <cassert>
#include <sstream>
#include <iomanip>

#include <boost/signals2.hpp>

#include <application/application.h>

#include <osg/AutoTransform>
#include <osgQt/QFontImplementation>
#include <osgText/Text>

#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <feature/parameter.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <library/plabel.h>

using namespace lbr;

class TextVisitor : public boost::static_visitor<std::string>
{
public:
  std::string operator()(double d) const
  {
    std::ostringstream stream;
    stream << std::setprecision(3) << std::fixed << d << std::endl;
    return stream.str();
  }
  std::string operator()(int i) const {return std::to_string(i);}
  std::string operator()(bool b) const
  {
    if (b)
      return "True";
    return "False";
  }
  std::string operator()(const std::string &s) const {return s;}
  std::string operator()(const boost::filesystem::path &p) const
  {
    if (p.has_filename())
      return p.filename().string();
    else
      return p.end()->string();
  }
  std::string operator()(const osg::Vec3d &vIn) const
  {
    std::ostringstream stream;
    stream << std::setprecision(3) << std::fixed
    << vIn.x() << ", " << vIn.y() << ", " << vIn.z();
    return stream.str();
  }
  std::string operator()(const osg::Quat &qIn) const
  {
    std::ostringstream stream;
    stream << std::setprecision(3) << std::fixed
    << qIn.x() << ", " << qIn.y() << ", " << qIn.z() << ", " << qIn.w();
    return stream.str();
  }
  std::string operator()(const osg::Matrixd&) const
  {
    return "some matrix";
  }
};

PLabel::PLabel() : osg::MatrixTransform()
{
  assert(0); //don't use default constructor.
}

PLabel::PLabel(const PLabel& copy, const osg::CopyOp& copyOp) : osg::MatrixTransform(copy, copyOp)
{
  assert(0); //don't use copy.
}

PLabel::PLabel(ftr::prm::Parameter* parameterIn) : osg::MatrixTransform(), parameter(parameterIn)
{
  getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  build();
  
  valueConnection = parameter->connectValue(boost::bind(&PLabel::valueHasChanged, this));
  constantConnection = parameter->connectConstant(boost::bind(&PLabel::constantHasChanged, this));
}

void PLabel::build()
{
  const prf::InteractiveParameter& iPref = prf::manager().rootPtr->interactiveParameter();
  
  autoTransform = new osg::AutoTransform();
  autoTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
  autoTransform->setAutoScaleToScreen(true);
  this->addChild(autoTransform.get());
  
  osg::MatrixTransform *textScale = new osg::MatrixTransform();
  textScale->setMatrix(osg::Matrixd::scale(75.0, 75.0, 75.0));
  autoTransform->addChild(textScale);
  
  text = new osgText::Text();
  text->setName("PLabel");
  osg::ref_ptr<osgQt::QFontImplementation> fontImplement(new osgQt::QFontImplementation(qApp->font()));
  osg::ref_ptr<osgText::Font> textFont(new osgText::Font(fontImplement.get()));
  text->setFont(textFont);
  text->setColor(osg::Vec4(0.0, 0.0, 1.0, 1.0));
  text->setBackdropType(osgText::Text::OUTLINE);
  text->setBackdropColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  text->setCharacterSize(iPref.characterSize());
  text->setAlignment(osgText::Text::CENTER_CENTER);
  textScale->addChild(text.get());
}

void PLabel::setText()
{
  std::ostringstream stream;
  if (showName)
    stream << parameter->getName().toStdString() << " = ";
  stream << boost::apply_visitor(TextVisitor(), parameter->getVariant());
  text->setText(stream.str());
}

void PLabel::setTextColor(const osg::Vec4 &cIn)
{
  text->setColor(cIn);
}

void PLabel::setTextColor()
{
  if (parameter->isConstant())
    text->setColor(osg::Vec4(0.0, 0.0, 1.0, 1.0)); //blue is constant
  else
    text->setColor(osg::Vec4(0.0, 1.0, 0.0, 1.0)); //green is linked.
}

void PLabel::valueHasChanged()
{
  assert(parameter);
  setText();
}

void PLabel::constantHasChanged()
{
  assert(parameter);
  setTextColor();
}

prj::srl::PLabel PLabel::serialOut() const
{
  const osg::Matrixd &m = this->getMatrix();
  prj::srl::Matrixd mOut
  (
    m(0,0), m(0,1), m(0,2), m(0,3),
    m(1,0), m(1,1), m(1,2), m(1,3),
    m(2,0), m(2,1), m(2,2), m(2,3),
    m(3,0), m(3,1), m(3,2), m(3,3)
  );
  
  const osg::Vec4 &c = text->getColor();
  prj::srl::Color cOut(c.x(), c.y(), c.z(), c.w());
  
  return prj::srl::PLabel(mOut, cOut);
}

void PLabel::serialIn(const prj::srl::PLabel &sIn)
{
  const auto &mIn = sIn.matrix();
  osg::Matrixd m
  (
    mIn.i0j0(), mIn.i0j1(), mIn.i0j2(), mIn.i0j3(),
    mIn.i1j0(), mIn.i1j1(), mIn.i1j2(), mIn.i1j3(),
    mIn.i2j0(), mIn.i2j1(), mIn.i2j2(), mIn.i2j3(),
    mIn.i3j0(), mIn.i3j1(), mIn.i3j2(), mIn.i3j3()
  );
  this->setMatrix(m);
  
  constantHasChanged();
  valueHasChanged();
  
  //label color maybe overridden by feature update.
  const auto &cIn = sIn.color();
  osg::Vec4 c(cIn.r(), cIn.g(), cIn.b(), cIn.a());
  setTextColor(c);
}
    
