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

#include <functional>

#include <boost/variant.hpp>

#include <osg/Geometry> //need this for containers.

#include <application/application.h>
#include <application/mainwindow.h>
#include <project/project.h>
#include <feature/base.h>
#include <command/featuretosystem.h>
#include <command/systemtofeature.h>
#include <command/featuretodragger.h>
#include <command/draggertofeature.h>
#include <command/checkgeometry.h>
#include <command/editcolor.h>
#include <command/featurerename.h>
#include <command/blend.h>
#include <command/extract.h>
#include <command/featurereposition.h>
#include <command/squash.h>
#include <command/strip.h>
#include <command/nest.h>
#include <command/dieset.h>
#include <command/quote.h>
#include <command/isolate.h>
#include <command/measurelinear.h>
#include <command/refine.h>
#include <command/instancelinear.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <selection/message.h>
#include <selection/container.h>
#include <selection/definitions.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <viewer/message.h>
#include <viewer/widget.h>
#include <command/manager.h>

using namespace cmd;

Manager& cmd::manager()
{
  static Manager localManager;
  return localManager;
}

Manager::Manager()
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "cmd::Manager";
  setupDispatcher();
  
  setupEditFunctionMap();
  
  selectionMask = slc::AllEnabled;
}

void Manager::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Request | msg::Command | msg::Cancel;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::cancelCommandDispatched, this, _1)));
  
  mask = msg::Request | msg::Command | msg::Clear;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::cancelCommandDispatched, this, _1)));
  
  mask = msg::Request | msg::Command | msg::Done;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::doneCommandDispatched, this, _1)));
  
  mask = msg::Response | msg::Selection | msg::SetMask;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::selectionMaskDispatched, this, _1)));
  
  mask = msg::Request | msg::FeatureToSystem;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::featureToSystemDispatched, this, _1)));
  
  mask = msg::Request | msg::SystemToFeature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::systemToFeatureDispatched, this, _1)));
  
  mask = msg::Request | msg::FeatureToDragger;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::featureToDraggerDispatched, this, _1)));
  
  mask = msg::Request | msg::DraggerToFeature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::draggerToFeatureDispatched, this, _1)));
  
  mask = msg::Request | msg::FeatureReposition;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::featureRepositionDispatched, this, _1)));
  
  mask = msg::Request | msg::CheckGeometry;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::checkGeometryDispatched, this, _1)));
  
  mask = msg::Request | msg::Edit | msg::Feature |msg::Color;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::editColorDispatched, this, _1)));
  
  mask = msg::Request | msg::Edit | msg::Feature | msg::Name;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::featureRenameDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Blend;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::constructBlendDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Extract;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::constructExtractDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Squash;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::constructSquashDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Strip;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::constructStripDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Nest;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::constructNestDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::DieSet;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::constructDieSetDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Quote;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::constructQuoteDispatched, this, _1)));
  
  mask = msg::Request | msg::Edit | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::editFeatureDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::ThreeD | msg::Isolate;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::viewIsolateDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Overlay | msg::Isolate;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::viewIsolateDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::ThreeD | msg::Overlay | msg::Isolate;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::viewIsolateDispatched, this, _1)));
  
  mask = msg::Request | msg::LinearMeasure;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::measureLinearDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Refine;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::constructRefineDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::InstanceLinear;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::constructInstanceLinearDispatched, this, _1)));
}

void Manager::cancelCommandDispatched(const msg::Message &)
{
  //we might not want the update triggered inside done slot
  //from this handler? but leave for now.
  doneSlot();
}

void Manager::doneCommandDispatched(const msg::Message&)
{
  //same as above for now, but might be different in the future.
  doneSlot();
}

void Manager::clearCommandDispatched(const msg::Message &)
{
  while(!stack.empty())
    doneSlot();
}

void Manager::addCommand(BasePtr pointerIn)
{
  //preselection will only work if the command stack is empty.
  if (!stack.empty())
  {
    stack.top()->deactivate();
    clearSelection();
  }
  stack.push(pointerIn);
  activateTop();
}

void Manager::doneSlot()
{
  //only active command should trigger it is done.
  if (!stack.empty())
  {
    stack.top()->deactivate();
    stack.pop();
  }
  clearSelection();
  observer->outBlocked(msg::buildStatusMessage(""));
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
  {
    observer->outBlocked(msg::Mask(msg::Request | msg::Project | msg::Update));
  }
  
  if (!stack.empty())
    activateTop();
  else
  {
    sendCommandMessage("Active command count: 0");
    observer->outBlocked(msg::buildSelectionMask(selectionMask));
  }
}

void Manager::activateTop()
{
  observer->outBlocked(msg::buildStatusMessage(stack.top()->getStatusMessage()));
  
  std::ostringstream stream;
  stream << 
    "Active command count: " << stack.size() << std::endl <<
    "Command: " << stack.top()->getCommandName();
  sendCommandMessage(stream.str());
  
  stack.top()->activate();
}

void Manager::sendCommandMessage(const std::string& messageIn)
{
  msg::Message statusMessage(msg::Request | msg::Command | msg::Text);
  vwr::Message statusVMessage;
  statusVMessage.text = messageIn;
  statusMessage.payload = statusVMessage;
  observer->outBlocked(statusMessage);
}

void Manager::clearSelection()
{
  observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Manager::selectionMaskDispatched(const msg::Message &messageIn)
{
  if (!stack.empty()) //only when no commands 
    return;
  
  slc::Message sMsg = boost::get<slc::Message>(messageIn.payload);
  selectionMask = sMsg.selectionMask;
}

void Manager::featureToSystemDispatched(const msg::Message&)
{
  std::shared_ptr<cmd::FeatureToSystem> featureToSystem(new cmd::FeatureToSystem());
  addCommand(featureToSystem);
}

void Manager::systemToFeatureDispatched(const msg::Message&)
{
  std::shared_ptr<cmd::SystemToFeature> systemToFeature(new cmd::SystemToFeature());
  addCommand(systemToFeature);
}

void Manager::draggerToFeatureDispatched(const msg::Message&)
{
  std::shared_ptr<cmd::DraggerToFeature> draggerToFeature(new cmd::DraggerToFeature());
  addCommand(draggerToFeature);
}

void Manager::featureToDraggerDispatched(const msg::Message&)
{
  std::shared_ptr<cmd::FeatureToDragger> featureToDragger(new cmd::FeatureToDragger());
  addCommand(featureToDragger);
}

void Manager::checkGeometryDispatched(const msg::Message&)
{
  std::shared_ptr<CheckGeometry> checkGeometry(new CheckGeometry());
  addCommand(checkGeometry);
}

void Manager::editColorDispatched(const msg::Message&)
{
  std::shared_ptr<EditColor> editColor(new EditColor());
  addCommand(editColor);
}

void Manager::featureRenameDispatched(const msg::Message &mIn)
{
  std::shared_ptr<FeatureRename> featureRename(new FeatureRename());
  featureRename->setFromMessage(mIn);
  addCommand(featureRename);
}

void Manager::constructBlendDispatched(const msg::Message&)
{
  std::shared_ptr<Blend> blend(new Blend());
  addCommand(blend);
}

void Manager::constructExtractDispatched(const msg::Message&)
{
  std::shared_ptr<Extract> e(new Extract());
  addCommand(e);
}

void Manager::constructSquashDispatched(const msg::Message&)
{
  std::shared_ptr<Squash> s(new Squash());
  addCommand(s);
}

void Manager::constructStripDispatched(const msg::Message&)
{
  std::shared_ptr<Strip> s(new Strip());
  addCommand(s);
}

void Manager::constructNestDispatched(const msg::Message&)
{
  std::shared_ptr<Nest> n(new Nest());
  addCommand(n);
}

void Manager::constructDieSetDispatched(const msg::Message&)
{
  std::shared_ptr<DieSet> d(new DieSet());
  addCommand(d);
}

void Manager::constructQuoteDispatched(const msg::Message&)
{
  std::shared_ptr<Quote> q(new Quote());
  addCommand(q);
}

void Manager::viewIsolateDispatched(const msg::Message &mIn)
{
  std::shared_ptr<Isolate> i(new Isolate());
  i->setFromMessage(mIn);
  addCommand(i);
}

void Manager::featureRepositionDispatched(const msg::Message&)
{
  std::shared_ptr<FeatureReposition> fr(new FeatureReposition());
  addCommand(fr);
}

void Manager::measureLinearDispatched(const msg::Message&)
{
  std::shared_ptr<MeasureLinear> ml(new MeasureLinear());
  addCommand(ml);
}

void Manager::constructRefineDispatched(const msg::Message&)
{
  std::shared_ptr<Refine> r(new Refine());
  addCommand(r);
}

void Manager::constructInstanceLinearDispatched(const msg::Message&)
{
  std::shared_ptr<InstanceLinear> i(new InstanceLinear());
  addCommand(i);
}

void Manager::editFeatureDispatched(const msg::Message&)
{
  app::Application *application = static_cast<app::Application*>(qApp);
  const slc::Containers &selections = application->
    getMainWindow()->getViewer()->getSelections();
    
  //edit feature only works with 1 object pre-selection.
  if (selections.size() != 1)
  {
    observer->outBlocked(msg::buildStatusMessage("Select 1 object prior to edit feature command"));
    return;
  }
  
  if (selections.front().selectionType != slc::Type::Object)
  {
    observer->outBlocked(msg::buildStatusMessage("Wrong selection type for edit feature command"));
    return;
  }
  
  ftr::Base *feature = application->getProject()->findFeature(selections.front().featureId);
  
  auto it = editFunctionMap.find(feature->getType());
  if (it == editFunctionMap.end())
  {
    observer->outBlocked(msg::buildStatusMessage("Editing of feature type not implemented"));
    return;
  }
  
  addCommand(it->second(feature));
}

void Manager::setupEditFunctionMap()
{
  editFunctionMap.insert(std::make_pair(ftr::Type::Blend, std::bind(&Manager::editBlend, this, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Strip, std::bind(&Manager::editStrip, this, std::placeholders::_1)));
  editFunctionMap.insert(std::make_pair(ftr::Type::Quote, std::bind(&Manager::editQuote, this, std::placeholders::_1)));
}

BasePtr Manager::editBlend(ftr::Base *feature)
{
  std::shared_ptr<Base> command(new BlendEdit(feature));
  return command;
}

BasePtr Manager::editStrip(ftr::Base *feature)
{
  std::shared_ptr<Base> command(new StripEdit(feature));
  return command;
}

BasePtr Manager::editQuote(ftr::Base *feature)
{
  std::shared_ptr<Base> command(new QuoteEdit(feature));
  return command;
}
