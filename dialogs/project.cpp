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
#include <stdexcept>

#include <boost/filesystem.hpp>

#include <QDoubleValidator>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>

#include <application/application.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/project.h>
#include <ui_project.h> //in build directory

using namespace boost::filesystem;

using namespace dlg;

Project::Project(QWidget *parent) : QDialog(parent), ui(new Ui::projectDialog)
{
  ui->setupUi(this);
  populateRecentList();
  
  connect(ui->newButton, SIGNAL(clicked()), this, SLOT(goNewSlot()));
  connect(ui->openButton, SIGNAL(clicked()), this, SLOT(goOpenSlot()));
  connect(ui->recentTableWidget, SIGNAL(cellClicked(int,int)), this, SLOT(goRecentSlot(int,int)));
  
  dlg::WidgetGeometry *filter = new dlg::WidgetGeometry(this, "dlg::Project");
  this->installEventFilter(filter);
  
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("dlg::Project");
  settings.beginGroup("RecentTable");
  ui->recentTableWidget->horizontalHeader()->restoreState(settings.value("header").toByteArray());
  settings.endGroup();
  settings.endGroup();
  
  //test the default project directory.
  boost::filesystem::path p = prf::manager().rootPtr->project().basePath();
  if (!boost::filesystem::exists(p))
  {
    ui->newNameEdit->setPlaceholderText(QObject::tr("Default project location doesn't exist"));
    ui->newNameEdit->setDisabled(true);
  }
  else
    ui->newNameEdit->setFocus();
}

Project::~Project()
{
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("dlg::Project");
  settings.beginGroup("RecentTable");
  settings.setValue("header", ui->recentTableWidget->horizontalHeader()->saveState());
  settings.endGroup();
  settings.endGroup();
  
  delete ui;
}

void Project::populateRecentList()
{
  const auto &recent = prf::manager().rootPtr->project().recentProjects().Entry();
  prf::RecentProjects::EntrySequence reconcile;
  std::size_t row = 0;
  for (const auto &entry : recent)
  {
    QDir localDir(QString::fromStdString(entry));
    if (!validateDir(localDir)) //only valid directories.
      continue;
    reconcile.push_back(entry);
    QString name = localDir.dirName();
    QString path = localDir.absolutePath();
    
    ui->recentTableWidget->insertRow(row);
    QTableWidgetItem *nameItem = new QTableWidgetItem(name);
    ui->recentTableWidget->setItem(row, 0, nameItem);
    QTableWidgetItem *pathItem = new QTableWidgetItem(path);
    ui->recentTableWidget->setItem(row, 1, pathItem);
    
    row++;
  }
  
  prf::manager().rootPtr->project().recentProjects().Entry() = reconcile;
  prf::manager().saveConfig();
}

bool Project::validateDir(const QDir& dir)
{
  if (!dir.exists())
    return false;
  if (!dir.exists("project.prjt"))
    return false;
  if (!dir.exists("project.brep"))
    return false;
  
  QDir gitDir(dir.absolutePath() + QDir::separator() + ".git");
  if (!gitDir.exists())
    return false;
  
  return true;
}

void Project::goNewSlot()
{
  path basePath = prf::manager().rootPtr->project().basePath();
  QString newNameText = ui->newNameEdit->text();
  if (exists(basePath) && (!newNameText.isEmpty()))
  {
    path newProjectPath = basePath /= newNameText.toStdString();
    if (exists(newProjectPath))
    {
      QMessageBox::critical(this, tr("Error"), tr("Project with name already exists"));
      return;
    }
    if (!create_directory(newProjectPath))
    {
      QMessageBox::critical(this, tr("Error"), tr("Couldn't create directory"));
      return;
    }
    result = Result::New;
    directory = QDir(QString::fromStdString(newProjectPath.string()));
    addToRecentList();
  }
  else
  {
    //browse dialog.
    path browsePath = basePath;
    if (!exists(browsePath))
      browsePath = prf::manager().rootPtr->project().lastDirectory().get();
    if (!exists(browsePath))
    {
      const char *home = std::getenv("HOME");
      if (home == NULL)
      {
        QMessageBox::critical(this, tr("Error"), tr("REALLY!? no home directory"));
        return;
      }
      
      browsePath = home;
      if (!exists(browsePath))
      {
        QMessageBox::critical(this, tr("Error"), tr("home directory doesn't exist"));
        return;
      }
    }
    
    QString freshDirectory = QFileDialog::getExistingDirectory
    (
      this,
      tr("Browse to new project directory"),
      QString::fromStdString(browsePath.string())
    );
    if (freshDirectory.isEmpty())
      return;
    browsePath = freshDirectory.toStdString();
    if (!is_empty(browsePath))
    {
      QMessageBox::critical(this, tr("Error"), tr("Expecting an empty directory"));
      return;
    }
    prf::manager().rootPtr->project().lastDirectory() = browsePath.string();
    prf::manager().saveConfig();
    result = Result::New;
    directory = QDir(freshDirectory);
    addToRecentList();
  }
  
  this->accept();
}

void Project::goOpenSlot()
{
  QString browseStart = QString::fromStdString(prf::manager().rootPtr->project().basePath());
  QDir browseStartDir(browseStart);
  if (!browseStartDir.exists() || browseStart.isEmpty())
    browseStart = QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get());
  QString freshDirectory = QFileDialog::getExistingDirectory(this, tr("Browse to existing project directory"), browseStart);
  if (freshDirectory.isEmpty())
    return;
  
  boost::filesystem::path p = freshDirectory.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.string();
  prf::manager().saveConfig();
  
  result = Result::Open;
  directory = QDir(freshDirectory);
  addToRecentList();
  
  this->accept();
}

void Project::goRecentSlot(int rowIn, int)
{
  QTableWidgetItem *widget = ui->recentTableWidget->item(rowIn, 1);
  result = Result::Recent;
  directory = QDir(widget->text());
  addToRecentList();
  
  this->accept();
}

void Project::addToRecentList()
{
  std::string freshEntry = directory.absolutePath().toStdString();
  auto &recent = prf::manager().rootPtr->project().recentProjects().Entry();
  auto it = std::find(recent.begin(), recent.end(), freshEntry);
  if (it != recent.end())
    recent.erase(it);
  recent.insert(recent.begin(), freshEntry);
  prf::manager().saveConfig();
}
