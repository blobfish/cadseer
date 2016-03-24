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
#include <boost/signals2.hpp>

#include <QApplication>

#include <osg/Switch>
#include <osg/PolygonMode>
#include <osgViewer/GraphicsWindow>
#include <osgText/Text>
#include <osgQt/QFontImplementation>

#include <selection/definitions.h>
#include <message/dispatch.h>
#include <viewer/textcamera.h>

using namespace vwr;

ResizeEventHandler::ResizeEventHandler(const osg::observer_ptr< osg::Camera > slaveCameraIn) :
                                       osgGA::GUIEventHandler(), slaveCamera(slaveCameraIn)
{

}

bool ResizeEventHandler::handle(const osgGA::GUIEventAdapter& eventAdapter, osgGA::GUIActionAdapter&,
                                osg::Object*, osg::NodeVisitor*)
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
  osgText::Text *selectionLabel = nullptr;
  osgText::Text *statusLabel = nullptr;
  osgText::Text *commandLabel = nullptr;
  for (unsigned int index = 0; index < infoSwitch->getNumChildren(); ++index)
  {
    osg::Node *child = infoSwitch->getChild(index);
    if (child->getName() == "selection")
      selectionLabel = dynamic_cast<osgText::Text*>(child);
    if (child->getName() == "status")
      statusLabel = dynamic_cast<osgText::Text*>(child);
    if (child->getName() == "command")
      commandLabel = dynamic_cast<osgText::Text*>(child);
  }
  
  assert(selectionLabel);
  assert(statusLabel);
  assert(commandLabel);
  
  osg::Vec3 pos;
  osg::BoundingBox::value_type padding = selectionLabel->getCharacterHeight() / 2.0;
  pos.x() = slaveCamera->getViewport()->width() - padding;
  pos.y() = slaveCamera->getViewport()->height() - padding;
  selectionLabel->setPosition(pos);
  
  padding = statusLabel->getCharacterHeight() / 2.0; //redundent
  pos.x() = padding;
  pos.y() = slaveCamera->getViewport()->height() - padding; //redundent
  statusLabel->setPosition(pos);
  
  padding = statusLabel->getCharacterHeight() / 2.0; //redundent
  pos.x() = padding;
  pos.y() = padding;
  commandLabel->setPosition(pos);
}

TextCamera::TextCamera(osgViewer::GraphicsWindow *windowIn) : osg::Camera()
{
  setupDispatcher();
  setGraphicsContext(windowIn);
  setProjectionMatrix(osg::Matrix::ortho2D(0, windowIn->getTraits()->width, 0, windowIn->getTraits()->width));
  setReferenceFrame(osg::Transform::ABSOLUTE_RF);
  setRenderOrder(osg::Camera::POST_RENDER, 2);
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
  osg::Vec3 pos(0.0, 0.0f, 0.0f); //position gets updated in resize handler.
  osg::Vec4 color(0.0f,0.0f,0.0f,1.0f);
  
  selectionLabel = new osgText::Text();
  selectionLabel->setName("selection");
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
  infoSwitch->addChild(selectionLabel.get());
  
  statusLabel = new osgText::Text();
  statusLabel->setName("status");
  statusLabel->setFont(textFont);
  statusLabel->setColor(color);
  statusLabel->setBackdropType(osgText::Text::OUTLINE);
  statusLabel->setBackdropColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  statusLabel->setCharacterSize(qApp->font().pointSizeF()); //this is 9.0 here.
  statusLabel->setPosition(pos);
  statusLabel->setAlignment(osgText::TextBase::LEFT_TOP);
  statusLabel->setText("Status: ");
  infoSwitch->addChild(statusLabel.get());
  
  commandLabel = new osgText::Text();
  commandLabel->setName("command");
  commandLabel->setFont(textFont);
  commandLabel->setColor(color);
  commandLabel->setBackdropType(osgText::Text::OUTLINE);
  commandLabel->setBackdropColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  commandLabel->setCharacterSize(qApp->font().pointSizeF()); //this is 9.0 here.
  commandLabel->setPosition(pos);
  commandLabel->setAlignment(osgText::TextBase::LEFT_BOTTOM);
  commandLabel->setText("Active command count: 0");
  infoSwitch->addChild(commandLabel.get());
  
  infoSwitch->setAllChildrenOn();
}

TextCamera::~TextCamera() //for ref_ptr and forward declare.
{

}

void TextCamera::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Post | msg::Preselection | msg::Addition;
  dispatcher.insert(std::make_pair(mask, boost::bind(&TextCamera::preselectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Preselection | msg::Subtraction;
  dispatcher.insert(std::make_pair(mask, boost::bind(&TextCamera::preselectionSubtractionDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Selection | msg::Addition;
  dispatcher.insert(std::make_pair(mask, boost::bind(&TextCamera::selectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Selection | msg::Subtraction;
  dispatcher.insert(std::make_pair(mask, boost::bind(&TextCamera::selectionSubtractionDispatched, this, _1)));
  
  mask = msg::Request | msg::StatusText;
  dispatcher.insert(std::make_pair(mask, boost::bind(&TextCamera::statusTextDispatched, this, _1)));
  
  mask = msg::Request | msg::CommandText;
  dispatcher.insert(std::make_pair(mask, boost::bind(&TextCamera::commandTextDispatched, this, _1)));
}

void TextCamera::messageInSlot(const msg::Message &messageIn)
{
  msg::MessageDispatcher::iterator it = dispatcher.find(messageIn.mask);
  if (it == dispatcher.end())
    return;
  
  it->second(messageIn);
  updateSelectionLabel();
}

void TextCamera::preselectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  
  preselectionText.clear();
  std::ostringstream preselectStream;
  preselectStream << "Pre-Selection:" << std::endl;
  preselectStream << "object type: " << slc::getNameOfType(sMessage.type) << std::endl <<
    "Feature id: " << sMessage.featureId << std::endl <<
    "Shape id: " << sMessage.shapeId << std::endl;
  preselectionText = preselectStream.str();
}

void TextCamera::preselectionSubtractionDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  preselectionText.clear();
}

void TextCamera::selectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  assert(std::find(selections.begin(), selections.end(), sMessage) == selections.end());
  selections.push_back(sMessage);
}

void TextCamera::selectionSubtractionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  auto it = std::find(selections.begin(), selections.end(), sMessage); 
  assert(it != selections.end());
  selections.erase(it);
}

void TextCamera::statusTextDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  vwr::Message sMessage = boost::get<vwr::Message>(messageIn.payload);
  std::string display = "Status: ";
  if (!sMessage.text.empty())
    display = sMessage.text;
  statusLabel->setText(display);
}

void TextCamera::updateSelectionLabel()
{
  std::ostringstream labelStream;
  labelStream << preselectionText << std::endl <<
    "Selection:";
  for (auto it = selections.begin(); it != selections.end(); ++it)
  {
    labelStream << std::endl <<
      "object type: " << slc::getNameOfType(it->type) << std::endl <<
      "Feature id: " << it->featureId << std::endl <<
      "Shape id: " << it->shapeId << std::endl;
  }
  
  selectionLabel->setText(labelStream.str());
}

void TextCamera::commandTextDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  vwr::Message vMessage = boost::get<vwr::Message>(messageIn.payload);
  commandLabel->setText(vMessage.text);
}
