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

#include <QDialogButtonBox>
#include <QListWidget>
#include <QTabWidget>
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
#include <application/splitterdecorated.h>
#include <dialogs/revision.h>

namespace dlg
{
  struct Revision::Data
  {
    std::vector<git2::Commit> commits;
  };
  struct UndoPage::Data
  {
    std::vector<git2::Commit> commits;
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

Revision::Revision(QWidget *parent) :
QDialog(parent),
observer(new msg::Observer()),
data(new Revision::Data())
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
  mainLayout->addWidget(tabWidget);
  
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
  mainLayout->addWidget(buttonBox);
  
  this->setLayout(mainLayout);
  
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
}
