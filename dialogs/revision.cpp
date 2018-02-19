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

#include <QMessageBox>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QAction>
#include <QCloseEvent>

#include <boost/variant/variant.hpp>

#include <message/message.h>
#include <message/observer.h>
#include <application/application.h>
#include <project/project.h>
#include <project/gitmanager.h>
#include <project/message.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/commitwidget.h>
#include <dialogs/tagwidget.h>
#include <application/splitterdecorated.h>
#include <dialogs/revision.h>

namespace dlg
{
  struct UndoPage::Data
  {
    std::vector<git2::Commit> commits;
  };
  
  struct AdvancedPage::Data
  {
    std::vector<git2::Tag> tags;
    git2::Commit currentHead;
    QIcon headIcon;
  };
}

using namespace dlg;

UndoPage::UndoPage(QWidget *parent) :
QWidget(parent),
observer(new msg::Observer()),
data(new UndoPage::Data())
{
  observer->name = "dlg::UndoPage";
  
  buildGui();
  fillInCommitList();
  if (!data->commits.empty())
  {
    commitList->setCurrentRow(0);
    commitList->item(0)->setSelected(true);
  }
}

UndoPage::~UndoPage(){}

void UndoPage::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  
  SplitterDecorated *splitter = new SplitterDecorated(this);
  splitter->setChildrenCollapsible(false);
  splitter->setOrientation(Qt::Horizontal);
  
  commitList = new QListWidget(this);
  commitList->setSelectionMode(QAbstractItemView::SingleSelection);
  commitList->setContextMenuPolicy(Qt::ActionsContextMenu);
  QAction *resetAction = new QAction(tr("Reset"), this);
  commitList->addAction(resetAction);
  commitList->setWhatsThis(tr("A list of commits to current branch. List is in descending order. Top is the latest"));
  splitter->addWidget(commitList);
  
  commitWidget = new CommitWidget(this);
  splitter->addWidget(commitWidget);
  
  mainLayout->addWidget(splitter);
  
  this->setLayout(mainLayout);
  
  splitter->restoreSettings("dlg::UndoPage");
  
  connect(commitList, &QListWidget::currentRowChanged, this, &UndoPage::commitRowChangedSlot);
  connect(resetAction, &QAction::triggered, this, &UndoPage::resetActionSlot);
}

void UndoPage::fillInCommitList()
{
  prj::Project *p = static_cast<app::Application*>(qApp)->getProject();
  assert(p);
  prj::GitManager &gm = p->getGitManager();
  data->commits = gm.getCommitsHeadToNamed("main");
  for (const auto &c : data->commits)
  {
    std::string idString = c.oid().format();
    commitList->addItem(QString::fromStdString(idString.substr(0, 12)));
  }
}

void UndoPage::commitRowChangedSlot(int r)
{
  assert(static_cast<std::size_t>(r) < data->commits.size());
  commitWidget->setCommit(data->commits.at(r));
}

void UndoPage::resetActionSlot()
{
  int r = commitList->currentRow();
  assert(static_cast<std::size_t>(r) < data->commits.size());
  
  app::Application *application = static_cast<app::Application*>(qApp);
  std::string pdir = application->getProject()->getSaveDirectory();
  observer->out(msg::Mask(msg::Request | msg::Close | msg::Project));
  
  /* keep in mind the project is closed, so the git manager in the project is gone.
   * when the project opens, the project will build a git manager. So here we have the potential
   * of having 2 git managers alive at the same time. Going to ensure that is not the case
   * with a unique ptr.
   */
  std::unique_ptr<prj::GitManager> localManager(new prj::GitManager());
  localManager->open(pdir);
  localManager->resetHard(data->commits.at(r).oid().format());
  localManager.release();
  
  prj::Message pMessage;
  pMessage.directory = pdir;
  observer->out(msg::Message(msg::Mask(msg::Request | msg::Open | msg::Project), pMessage));
  
  //addTab reparents so the constructor argument is not really the parent.
  QWidget *parentDialog = this->parentWidget()->parentWidget()->parentWidget();
  application->postEvent(parentDialog, new QCloseEvent());
}



AdvancedPage::AdvancedPage(QWidget *parent) :
QWidget(parent),
observer(new msg::Observer()),
data(new AdvancedPage::Data())
{
  observer->name = "dlg::AdvancedPage";
  data->headIcon = QIcon(":/resources/images/trafficGreen.svg");
  
  buildGui();
  init();
}

AdvancedPage::~AdvancedPage(){}

void AdvancedPage::init()
{
  tagList->clear();
  tagWidget->clear();
  data->tags.clear();
  
  setCurrentHead();
  fillInTagList();
  if (!data->tags.empty())
  {
    tagList->setCurrentRow(0);
    tagList->item(0)->setSelected(true);
  }
}

void AdvancedPage::buildGui()
{
  QLabel *revisionLabel = new QLabel(tr("Revisions:"), this);
  QHBoxLayout *rhl = new QHBoxLayout();
  rhl->addWidget(revisionLabel);
  rhl->addStretch();
  
  tagList = new QListWidget(this);
  tagList->setSelectionMode(QAbstractItemView::SingleSelection);
  tagList->setContextMenuPolicy(Qt::ActionsContextMenu);
  tagList->setWhatsThis
  (
    tr
    (
      "List of project revisions. "
      "Traffic signal labels a revision that is current. "
      "Can not have identical revisions"
    )
  );
  QVBoxLayout *revisionLayout = new QVBoxLayout();
  revisionLayout->setContentsMargins(0, 0, 0, 0);
  revisionLayout->addLayout(rhl);
  revisionLayout->addWidget(tagList);
  
  //splitter will only take a widget.
  QWidget *dummy = new QWidget(this);
  dummy->setContentsMargins(0, 0, 0, 0);
  dummy->setLayout(revisionLayout);
  
  tagWidget = new TagWidget(this);
  
  SplitterDecorated *splitter = new SplitterDecorated(this);
  splitter->setChildrenCollapsible(false);
  splitter->setOrientation(Qt::Horizontal);
  splitter->addWidget(dummy);
  splitter->addWidget(tagWidget);
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addWidget(splitter);
  
  this->setLayout(mainLayout);
  
  splitter->restoreSettings("dlg::AdvancedPage");
  
  createTagAction = new QAction(tr("Create New Revision"), this);
  tagList->addAction(createTagAction);
  connect(createTagAction, &QAction::triggered, this, &AdvancedPage::createTagSlot);
  
  checkoutTagAction = new QAction(tr("Checkout Revision"), this);
  tagList->addAction(checkoutTagAction);
  connect(checkoutTagAction, &QAction::triggered, this, &AdvancedPage::checkoutTagSlot);
  
  destroyTagAction = new QAction(tr("Destroy Selected Revision"), this);
  tagList->addAction(destroyTagAction);
  connect(destroyTagAction, &QAction::triggered, this, &AdvancedPage::destroyTagSlot);
  
  connect(tagList, &QListWidget::currentRowChanged, this, &AdvancedPage::tagRowChangedSlot);
}

void AdvancedPage::tagRowChangedSlot(int r)
{
  if (r == -1)
  {
    tagWidget->clear();
    destroyTagAction->setDisabled(true);
    checkoutTagAction->setDisabled(true);
    return;
  }
  assert(static_cast<std::size_t>(r) < data->tags.size());
  tagWidget->setTag(data->tags.at(r));
  if (data->tags.at(r).targetOid() == data->currentHead.oid())
    checkoutTagAction->setDisabled(true);
  else
    checkoutTagAction->setEnabled(true);
  destroyTagAction->setEnabled(true);
}

void AdvancedPage::createTagSlot()
{
  //hack together a dialog.
  std::unique_ptr<QDialog> ntDialog(new QDialog(this));
  ntDialog->setWindowTitle(tr("Create New Revision"));
  QVBoxLayout *mainLayout = new QVBoxLayout();
  
  QLabel *nameLabel = new QLabel(tr("Name:"), ntDialog.get());
  QLineEdit *nameEdit = new QLineEdit(ntDialog.get());
  nameEdit->setWhatsThis(tr("Name For The New Revision"));
  nameEdit->setPlaceholderText(tr("Name For The New Revision"));
  QHBoxLayout *nameLayout = new QHBoxLayout();
  nameLayout->addWidget(nameLabel);
  nameLayout->addWidget(nameEdit);
  mainLayout->addLayout(nameLayout);
  
  QTextEdit *messageEdit = new QTextEdit(ntDialog.get());
  messageEdit->setWhatsThis(tr("Message For The New Revision"));
  messageEdit->setPlaceholderText(tr("Message For The New Revision"));
  mainLayout->addWidget(messageEdit);
  
  QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, ntDialog.get());
  mainLayout->addWidget(bb);
  connect(bb, &QDialogButtonBox::accepted, ntDialog.get(), &QDialog::accept);
  connect(bb, &QDialogButtonBox::rejected, ntDialog.get(), &QDialog::reject);
  
  ntDialog->setLayout(mainLayout);
  
  if (ntDialog->exec() != QDialog::Accepted)
    return;
  
  //make sure we don't already have a tag with that name
  std::string name = nameEdit->text().toStdString();
  for (const auto &t : data->tags)
  {
    if (t.name() == name)
    {
      QMessageBox::critical(ntDialog.get(), tr("Error"), tr("Name already exists"));
      return;
    }
  }
  
  prj::Project *p = static_cast<app::Application*>(qApp)->getProject();
  assert(p);
  prj::GitManager &gm = p->getGitManager();
  gm.createTag(name, messageEdit->toPlainText().toStdString());
  init();
}

void AdvancedPage::destroyTagSlot()
{
  QList<QListWidgetItem*> items = tagList->selectedItems();
  if (items.isEmpty())
    return;
  int row = tagList->row(items.front());
  assert(row >= 0 && static_cast<std::size_t>(row) < data->tags.size());
  if (row < 0 || static_cast<std::size_t>(row) > data->tags.size())
    return;
  prj::Project *p = static_cast<app::Application*>(qApp)->getProject();
  assert(p);
  prj::GitManager &gm = p->getGitManager();
  gm.destroyTag(data->tags.at(static_cast<std::size_t>(row)).name());
  init();
}

void AdvancedPage::checkoutTagSlot()
{
  // we disable the checkout action when selected tag equals the current head
  // so we should be good to go here.
  
  QList<QListWidgetItem*> items = tagList->selectedItems();
  if (items.isEmpty())
    return;
  int row = tagList->row(items.front());
  assert(row >= 0 && static_cast<std::size_t>(row) < data->tags.size());
  if (row < 0 || static_cast<std::size_t>(row) > data->tags.size())
    return;
  
  //lets warn user if current head is not tagged.
  bool currentTagged = false;
  for (const auto &t : data->tags)
  {
    if (t.targetOid() == data->currentHead.oid())
      currentTagged = true;
  }
  if (!currentTagged)
  {
    if
    (
      QMessageBox::question
      (
        this,
        tr("Warning"),
        tr("No revision at current state. Work will be lost. Continue?")
      ) != QMessageBox::Yes
    )
    return;
  }
  
  /* 1) close the project
   * 2) update the git repo.
   * 3) reopen the project
   * see not in UndoPage::resetActionSlot() why we dynamicly allocate a new git manager.
   */
  app::Application *application = static_cast<app::Application*>(qApp);
  std::string pdir = application->getProject()->getSaveDirectory();
  observer->out(msg::Mask(msg::Request | msg::Close | msg::Project));
  
  std::unique_ptr<prj::GitManager> localManager(new prj::GitManager());
  localManager->open(pdir);
  localManager->checkoutTag(data->tags.at(static_cast<std::size_t>(row)));
  localManager.release();
  
  prj::Message pMessage;
  pMessage.directory = pdir;
  observer->out(msg::Message(msg::Mask(msg::Request | msg::Open | msg::Project), pMessage));
  
  //addTab reparents so the constructor argument is not really the parent.
  QWidget *parentDialog = this->parentWidget()->parentWidget()->parentWidget();
  application->postEvent(parentDialog, new QCloseEvent()); //close dialog
}

void AdvancedPage::fillInTagList()
{
  createTagAction->setEnabled(true);
  
  prj::Project *p = static_cast<app::Application*>(qApp)->getProject();
  assert(p);
  prj::GitManager &gm = p->getGitManager();
  data->tags = gm.getTags();
  for (const auto &t : data->tags)
  {
    if (t.targetOid() == data->currentHead.oid())
    {
      new QListWidgetItem(data->headIcon, QString::fromStdString(t.name()), tagList);
      createTagAction->setDisabled(true); //don't create more than 1 tag at any one commit.
    }
    else
      tagList->addItem(QString::fromStdString(t.name()));
  }
}

void AdvancedPage::setCurrentHead()
{
  prj::Project *p = static_cast<app::Application*>(qApp)->getProject();
  assert(p);
  prj::GitManager &gm = p->getGitManager();
  data->currentHead = gm.getCurrentHead();
}


Revision::Revision(QWidget *parent) :
QDialog(parent),
observer(new msg::Observer())
{
  this->setWindowTitle(tr("Revisions"));
  
  observer->name = "dlg::Revision";
  
  buildGui();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Revision");
  this->installEventFilter(filter);
}

Revision::~Revision() {}

void Revision::closeEvent(QCloseEvent *e)
{
  QDialog::closeEvent(e);
  observer->out(msg::Mask(msg::Request | msg::Command | msg::Done));
}

void Revision::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  
  tabWidget = new QTabWidget(this);
  tabWidget->addTab(new UndoPage(tabWidget), tr("Undo")); //addTab reparents.
  tabWidget->addTab(new AdvancedPage(tabWidget), tr("Advanced"));
  mainLayout->addWidget(tabWidget);
  
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
  mainLayout->addWidget(buttonBox);
  
  this->setLayout(mainLayout);
  
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
}
