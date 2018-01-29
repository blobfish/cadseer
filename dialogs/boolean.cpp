/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QSettings>
#include <QGridLayout>
#include <QButtonGroup>
#include <QLabel>
#include <QDialogButtonBox>
#include <QTimer>

#include <globalutilities.h>
#include <tools/idtools.h>
#include <application/application.h>
#include <application/mainwindow.h>
#include <project/project.h>
#include <message/observer.h>
#include <selection/message.h>
#include <annex/seershape.h>
#include <tools/featuretools.h>
#include <feature/intersect.h>
#include <feature/subtract.h>
#include <feature/union.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/selectionbutton.h>
#include <dialogs/boolean.h>

using boost::uuids::uuid;

using namespace dlg;

Boolean::Boolean(ftr::Intersect *i, QWidget *parent) : QDialog(parent), observer(new msg::Observer())
{
  intersect = i;
  booleanId = intersect->getId();
  init();
}

Boolean::Boolean(ftr::Subtract *s, QWidget *parent) : QDialog(parent), observer(new msg::Observer())
{
  subtract = s;
  booleanId = subtract->getId();
  init();
}

Boolean::Boolean(ftr::Union *u, QWidget *parent) : QDialog(parent), observer(new msg::Observer())
{
  onion = u;
  booleanId = onion->getId();
  init();
}

Boolean::~Boolean()
{
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("dlg::Boolean");

  settings.endGroup();
}

void Boolean::init()
{
  observer->name = "dlg::Boolean";
  
  buildGui();
  
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("dlg::Boolean");

  settings.endGroup();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Boolean");
  this->installEventFilter(filter);
  
  QTimer::singleShot(0, targetButton, &QPushButton::click);
}

void Boolean::setEditDialog()
{
  isEditDialog =  true;
  
  prj::Project *project = static_cast<app::Application*>(qApp)->getProject();
  assert(project);
  
  ftr::UpdatePayload::UpdateMap editMap = project->getParentMap(booleanId);
  std::vector<const ftr::Base*> targets = ftr::UpdatePayload::getFeatures(editMap, ftr::InputType::target);
  std::vector<const ftr::Base*> tools = ftr::UpdatePayload::getFeatures(editMap, ftr::InputType::tool);
  std::vector<ftr::Pick> targetPicks;
  std::vector<ftr::Pick> toolPicks;
  if (intersect)
  {
    booleanId = intersect->getId();
    targetPicks = intersect->getTargetPicks();
    toolPicks = intersect->getToolPicks();
  }
  else if (subtract)
  {
    booleanId = subtract->getId();
    targetPicks = subtract->getTargetPicks();
    toolPicks = subtract->getToolPicks();
  }
  else if (onion)
  {
    booleanId = onion->getId();
    targetPicks = onion->getTargetPicks();
    toolPicks = onion->getToolPicks();
  }
  
  auto resolvedTargets = tls::resolvePicks(targets, targetPicks, project->getShapeHistory());
  auto resolvedTools = tls::resolvePicks(tools, toolPicks, project->getShapeHistory());
  
  auto isFeatureValid = [](const ftr::Base *f) -> bool
  {
    if (!f->hasAnnex(ann::Type::SeerShape))
      return false;
    const ann::SeerShape &targetSeerShape = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (targetSeerShape.isNull())
      return false;
    
    return true;
  };
  
  //need reference capture to capture the isFeatureValid lambda
  auto createsMessages = [&](const auto &Resolves) -> slc::Messages
  {
    slc::Messages out;
    
    for (const auto &resolved : Resolves)
    {
      const ftr::Base* f = resolved.feature;
      uuid shapeId = resolved.resultId;
      if (!isFeatureValid(f)) //this should filter invalid picks.
        continue;
      const ann::SeerShape &seerShape = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      if (!shapeId.is_nil())
      {
        assert(seerShape.hasShapeIdRecord(shapeId)); //want to know about this.
        if (!seerShape.hasShapeIdRecord(shapeId))
          continue;
      }
      slc::Message cm; //current message.
      if (shapeId.is_nil())
        cm.type = slc::Type::Object;
      else
        cm.type = slc::Type::Solid;
      cm.featureType = ftr::Type::Intersect;
      cm.featureId = f->getId();
      cm.shapeId = shapeId;
      
      out.push_back(cm);
    }
    
    return out;
  };
    
  slc::Messages targetMessages = createsMessages(resolvedTargets);
  slc::Messages toolMessages = createsMessages(resolvedTools);
  
  targetButton->setMessages(targetMessages);
  toolButton->setMessages(toolMessages);
  
  if ((!targetMessages.empty()) && (!targetMessages.front().shapeId.is_nil()))
      targetButton->mask = slc::None | slc::ObjectsEnabled | slc::SolidsEnabled | slc::SolidsSelectable;
  if ((!toolMessages.empty()) && (!toolMessages.front().shapeId.is_nil()))
      toolButton->mask = slc::None | slc::ObjectsEnabled | slc::SolidsEnabled | slc::SolidsSelectable;
  
  leafChildren.clear();
  for (const ftr::Base *f : targets)
  {
    auto lc = project->getLeafChildren(f->getId());
    std::copy(lc.begin(), lc.end(), std::back_inserter(leafChildren));
  }
  for (const ftr::Base *f : tools)
  {
    auto lc = project->getLeafChildren(f->getId());
    std::copy(lc.begin(), lc.end(), std::back_inserter(leafChildren));
  }
  gu::uniquefy(leafChildren);
  
  for (const ftr::Base *f : targets)
    project->setCurrentLeaf(f->getId());
  for (const ftr::Base *f : tools)
    project->setCurrentLeaf(f->getId());
}

void Boolean::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Boolean::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Boolean::finishDialog()
{
  auto finishCommand = [&]()
  {
    msg::Message mOut(msg::Mask(msg::Request | msg::Command | msg::Done));
    QMetaObject::invokeMethod(qApp, "messageSlot", Qt::QueuedConnection, Q_ARG(msg::Message, mOut));
  };
  
  prj::Project *p = static_cast<app::Application *>(qApp)->getProject();
  if (isAccepted)
  {
    const slc::Messages &tms = targetButton->getMessages(); //target messages
    if (tms.empty())
    {
      finishCommand();
      return;
    }
    ftr::Picks tps; //target picks
    std::vector<uuid> targetIds;
    for (const auto &m : tms)
    {
      targetIds.push_back(m.featureId);
      ftr::Pick tp; //target pick
      tp.id = m.shapeId;
      if (!tp.id.is_nil())
      {
        tp.shapeHistory = p->getShapeHistory().createDevolveHistory(tp.id);
        tps.push_back(tp);
      }
    }
    
    const slc::Messages &tlms = toolButton->getMessages(); //tool messages.
    if (tlms.empty())
    {
      finishCommand();
      return;
    }
    ftr::Picks tlps; //tool picks
    std::vector<uuid> toolIds;
    for (const auto &m : tlms)
    {
      toolIds.push_back(m.featureId);
      ftr::Pick pk;
      pk.id = m.shapeId;
      if (!pk.id.is_nil())
      {
        pk.shapeHistory = p->getShapeHistory().createDevolveHistory(pk.id);
        tlps.push_back(pk);
      }
    }
    gu::uniquefy(toolIds);
    
    osg::Vec4 targetColor = p->findFeature(targetIds.front())->getColor();
    if (intersect)
    {
      booleanId = intersect->getId();
      intersect->setTargetPicks(tps);
      intersect->setToolPicks(tlps);
      intersect->setColor(targetColor);
      intersect->setModelDirty();
    }
    else if (subtract)
    {
      booleanId = subtract->getId();
      subtract->setTargetPicks(tps);
      subtract->setToolPicks(tlps);
      subtract->setColor(targetColor);
      subtract->setModelDirty();
    }
    else if (onion)
    {
      booleanId = onion->getId();
      onion->setTargetPicks(tps);
      onion->setToolPicks(tlps);
      onion->setColor(targetColor);
      onion->setModelDirty();
    }
    
    if (isEditDialog)
    {
      p->clearAllInputs(booleanId);
    }
    for (const auto &tid : targetIds)
    {
      p->connect(tid, booleanId, {ftr::InputType::target});
      observer->outBlocked(msg::buildHideThreeD(tid));
      observer->outBlocked(msg::buildHideOverlay(tid));
    }
    for (const auto &tid : toolIds)
    {
      p->connect(tid, booleanId, {ftr::InputType::tool});
      observer->outBlocked(msg::buildHideThreeD(tid));
      observer->outBlocked(msg::buildHideOverlay(tid));
    }
  }
  else //rejected dialog
  {
    if (!isEditDialog)
    {
      if (intersect)
        p->removeFeature(intersect->getId());
      else if (subtract)
        p->removeFeature(subtract->getId());
      else if (onion)
        p->removeFeature(onion->getId());
    }
  }
  
  if (isEditDialog)
  {
    for (const auto &id : leafChildren)
      p->setCurrentLeaf(id);
  }
  
  finishCommand();
}

void Boolean::buildGui()
{
  QGridLayout *gl = new QGridLayout();
  bGroup = new QButtonGroup(this);
  
  QPixmap pmap = QPixmap(":resources/images/cursor.svg").scaled(32, 32, Qt::KeepAspectRatio);
  
  QLabel *targetLabel = new QLabel(tr("Target:"), this);
  targetButton = new SelectionButton(pmap, QString(), this);
  targetButton->isSingleSelection = true;
  targetButton->mask = slc::None | slc::ObjectsEnabled | slc::ObjectsSelectable | slc::SolidsEnabled;
  targetButton->statusPrompt = tr("Select Target");
  gl->addWidget(targetLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(targetButton, 0, 1, Qt::AlignVCenter | Qt::AlignCenter);
  connect(targetButton, &SelectionButton::advance, this, &Boolean::advanceSlot);
  bGroup->addButton(targetButton);
  
  QLabel *toolLabel = new QLabel(tr("Tools:"), this);
  toolButton = new SelectionButton(pmap, QString(), this);
  toolButton->isSingleSelection = true;
  toolButton->mask = slc::None | slc::ObjectsEnabled | slc::ObjectsSelectable | slc::SolidsEnabled;
  toolButton->statusPrompt = tr("Select Tools");
  gl->addWidget(toolLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(toolButton, 1, 1, Qt::AlignVCenter | Qt::AlignCenter);
  bGroup->addButton(toolButton);
  
  QDialogButtonBox *buttons = new QDialogButtonBox
    (QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  QHBoxLayout *hl = new QHBoxLayout();
  hl->addLayout(gl);
  hl->addStretch();
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(hl);
  vl->addSpacing(10);
  vl->addLayout(buttonLayout);
  vl->addStretch();
  
  this->setLayout(vl);
}

void Boolean::advanceSlot()
{
  QAbstractButton *cb = bGroup->checkedButton();
  if (!cb)
    return;
  if (cb == targetButton)
    toolButton->setChecked(true);
}
