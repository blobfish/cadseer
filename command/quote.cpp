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

#include <application/mainwindow.h>
#include <project/project.h>
#include <message/observer.h>
#include <selection/eventhandler.h>
#include <feature/quote.h>
#include <dialogs/quote.h>
#include <command/quote.h>

using namespace cmd;

using boost::uuids::uuid;

Quote::Quote() : Base(){}

Quote::~Quote()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Quote::getStatusMessage()
{
  return QObject::tr("Select features for strip and dieset").toStdString();
}

void Quote::activate()
{
  isActive = true;
  
  if (!dialog)
    go();
  
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void Quote::deactivate()
{
  isActive = false;
  
  dialog->hide();
}

void Quote::go()
{
  uuid stripId = gu::createNilId();
  uuid diesetId = gu::createNilId();
  
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    if (c.featureType == ftr::Type::Strip)
      stripId = c.featureId;
    else if (c.featureType == ftr::Type::DieSet)
      diesetId = c.featureId;
  }
  
  if (stripId.is_nil() || diesetId.is_nil())
  {
    auto ids = project->getAllFeatureIds();
    for (const auto &id : ids)
    {
      ftr::Base *bf = project->findFeature(id);
      if ((bf->getType() == ftr::Type::Strip) && (stripId.is_nil()))
        stripId = id;
      if ((bf->getType() == ftr::Type::DieSet) && (diesetId.is_nil()))
        diesetId = id;
    }
  }
  
  std::shared_ptr<ftr::Quote> quote(new ftr::Quote());
  project->addFeature(quote);
  
  //this should trick the dagview into updating so it isn't screwed up
  //while dialog is running. only dagview responds to this message
  //as of git hash a530460.
  observer->outBlocked(msg::Response | msg::Post | msg::UpdateModel);
  
  dialog = new dlg::Quote(quote.get(), mainWindow);
  dialog->setStripId(stripId);
  dialog->setDieSetId(diesetId);
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

QuoteEdit::QuoteEdit(ftr::Base *feature) : Base()
{
  quote = dynamic_cast<ftr::Quote*>(feature);
  assert(quote);
}

QuoteEdit::~QuoteEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string QuoteEdit::getStatusMessage()
{
  return "Editing Quote Feature";
}

void QuoteEdit::activate()
{
  if (!dialog)
    dialog = new dlg::Quote(quote, mainWindow);
  
  isActive = true;
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void QuoteEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}
