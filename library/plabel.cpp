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
  std::string operator()(const boost::filesystem::path &p) const {return p.string();}
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
    stream << parameter->getName().toStdString() << " = "
    << boost::apply_visitor(TextVisitor(), parameter->getVariant());
  text->setText(stream.str());
}

void PLabel::setTextColor(const osg::Vec4 &cIn)
{
  text->setColor(cIn);
}

void PLabel::valueHasChanged()
{
  assert(parameter);
  setText();
}

void PLabel::constantHasChanged()
{
  //not doing anything with this yet.
  //maybe color reflects linked status?
  assert(parameter);
}
