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

#include <iostream>
#include <sstream>
#include <assert.h>

#include <boost/uuid/uuid_io.hpp>

#include <QApplication>

#include <osg/Switch>
#include <osg/PolygonMode>
#include <osgViewer/GraphicsWindow>
#include <osgText/Text>
#include <osgQt/QFontImplementation>

#include <selection/definitions.h>
#include <selection/message.h>
#include "textcamera.h"

ResizeEventHandler::ResizeEventHandler(const osg::observer_ptr< osg::Camera > slaveCameraIn) :
                                       osgGA::GUIEventHandler(), slaveCamera(slaveCameraIn)
{

}

bool ResizeEventHandler::handle(const osgGA::GUIEventAdapter& eventAdapter, osgGA::GUIActionAdapter& actionAdapter,
                                osg::Object* object, osg::NodeVisitor* nodeVistor)
{
  if (eventAdapter.getEventType() != osgGA::GUIEventAdapter::RESIZE)
    return false;
  
  if (!slaveCamera.valid())
    return false;
  
  osg::Viewport *viewPort = slaveCamera->getViewport();
  slaveCamera->setProjectionMatrix(osg::Matrixd::ortho2D(viewPort->x(), viewPort->width(), viewPort->y(), viewPort->height()));
  
  positionSelection();
  
  //other cameras, views etc.. might want to respond to this event.
  return false;
}

void ResizeEventHandler::positionSelection()
{
  //look for selection geode.
  osg::Switch *infoSwitch = slaveCamera->getChild(0)->asSwitch();
  osg::Geode *selectionGeode = nullptr;
  for (unsigned int index = 0; index < infoSwitch->getNumChildren(); ++index)
  {
    if (infoSwitch->getChild(index)->getName() != "selection")
      continue;
    selectionGeode = infoSwitch->getChild(index)->asGeode();
    break;
  }
  
  assert(selectionGeode);
  osgText::Text *label = dynamic_cast<osgText::Text*>(selectionGeode->getChild(0));
  assert(label);
  
  osg::Vec3 pos;
  osg::BoundingBox::value_type padding = label->getCharacterHeight() / 2.0;
  pos.x() = slaveCamera->getViewport()->width() - padding;
  pos.y() = slaveCamera->getViewport()->height() - padding;
  label->setPosition(pos);
}

TextCamera::TextCamera(osgViewer::GraphicsWindow *windowIn) : osg::Camera()
{
  setGraphicsContext(windowIn);
  setProjectionMatrix(osg::Matrix::ortho2D(0, windowIn->getTraits()->width, 0, windowIn->getTraits()->width));
  setReferenceFrame(osg::Transform::ABSOLUTE_RF);
  setRenderOrder(osg::Camera::POST_RENDER);
  setAllowEventFocus(false);
  setViewMatrix(osg::Matrix::identity());
  setClearMask(0);
  
  infoSwitch = new osg::Switch();
  osg::StateSet* stateset = infoSwitch->getOrCreateStateSet();
  stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
  stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
  stateset->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
  stateset->setAttribute(new osg::PolygonMode(), osg::StateAttribute::PROTECTED);
  addChild(infoSwitch);
  
  //set up on screen text.
  osg::Vec3 pos(0.0, 0.0f, 0.0f);
  osg::Vec4 color(0.0f,0.0f,0.0f,1.0f);
  osg::Geode* geode = new osg::Geode();
  geode->setName("selection");
  infoSwitch->addChild(geode, true);
  
  selectionLabel = new osgText::Text();
  geode->addDrawable( selectionLabel.get() );
  osg::ref_ptr<osgQt::QFontImplementation> fontImplement(new osgQt::QFontImplementation(qApp->font()));
  osg::ref_ptr<osgText::Font> textFont(new osgText::Font(fontImplement.get()));
  selectionLabel->setFont(textFont);
  selectionLabel->setColor(color);
  selectionLabel->setBackdropType(osgText::Text::OUTLINE);
  selectionLabel->setBackdropColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
//   label->setFont("/usr/share/fonts/truetype/msttcorefonts/arial.ttf");
  selectionLabel->setCharacterSize(qApp->font().pointSizeF()); //this is 9.0 here.
  selectionLabel->setPosition(pos);
  selectionLabel->setAlignment(osgText::TextBase::RIGHT_TOP);
  selectionLabel->setText("testing text\n and yet even multiline");
  
  infoSwitch->setAllChildrenOn();
}

void TextCamera::selectionChangedSlot(const Selection::Message &messageIn)
{
  if (messageIn.type == Selection::Message::Type::Preselection)
  {
    preselectionText.clear();
    std::ostringstream preselectStream;
    preselectStream << "Pre-Selection:" << std::endl;
    if (messageIn.action == Selection::Message::Action::Addition)
    {
      preselectStream << "object type: " << Selection::getNameOfType(messageIn.objectType) << std::endl <<
        "Feature id: " << messageIn.featureId << std::endl <<
        "Shape id: " << messageIn.shapeId << std::endl;
    }
    preselectionText = preselectStream.str();
  }
  if (messageIn.type == Selection::Message::Type::Selection)
  {
    auto key = std::make_pair(messageIn.featureId, messageIn.shapeId);
    if (messageIn.action == Selection::Message::Action::Addition)
    {
      //snap points will have duplicates. what to do?
//       assert(selectionMap.count(key) == 0);
      selectionMap.insert(std::make_pair(key, messageIn));
    }
    if (messageIn.action == Selection::Message::Action::Subtraction)
    {
      //snap points will have duplicates. what to do?
//       assert(selectionMap.count(key) == 1);
      selectionMap.erase(key);
    }
  }
  
  updateSelectionLabel();
}

void TextCamera::updateSelectionLabel()
{
  std::ostringstream labelStream;
  labelStream << preselectionText << std::endl <<
    "Selection:";
  for (SelectionMap::const_iterator it = selectionMap.begin(); it != selectionMap.end(); ++it)
  {
    labelStream << std::endl <<
      "object type: " << Selection::getNameOfType(it->second.objectType) << std::endl <<
      "Feature id: " << it->second.featureId << std::endl <<
      "Shape id: " << it->second.shapeId << std::endl;
  }
  
  selectionLabel->setText(labelStream.str());
}
