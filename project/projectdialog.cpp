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

#include <QDoubleValidator>
#include <QMessageBox>
#include <QFileDialog>

#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <project/projectdialog.h>
#include <ui_projectdialog.h> //in build directory

using namespace prj;

Dialog::Dialog(QWidget *parent) : QDialog(parent), ui(new Ui::projectDialog)
{
  ui->setupUi(this);
  populateRecentList();
  
  connect(ui->newButton, SIGNAL(clicked()), this, SLOT(goNewSlot()));
  connect(ui->openButton, SIGNAL(clicked()), this, SLOT(goOpenSlot()));
  connect(ui->recentTableWidget, SIGNAL(cellClicked(int,int)), this, SLOT(goRecentSlot(int,int)));
}

Dialog::~Dialog()
{
  delete ui;
}

void Dialog::populateRecentList()
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

bool Dialog::validateDir(const QDir& dir)
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

void Dialog::goNewSlot()
{
  QString browseStart = QString::fromStdString(prf::manager().rootPtr->project().basePath());
  QDir browseStartDir(browseStart);
  if (!browseStartDir.exists() || browseStart.isEmpty())
    browseStart = QDir::homePath();
  QString freshDirectory = QFileDialog::getExistingDirectory(this, tr("Browse to new project directory"), browseStart);
  if (freshDirectory.isEmpty())
    return;
  
  result = Result::New;
  directory = QDir(freshDirectory);
  addToRecentList();
  
  this->accept();
}

void Dialog::goOpenSlot()
{
  QString browseStart = QString::fromStdString(prf::manager().rootPtr->project().basePath());
  QDir browseStartDir(browseStart);
  if (!browseStartDir.exists() || browseStart.isEmpty())
    browseStart = QDir::homePath();
  QString freshDirectory = QFileDialog::getExistingDirectory(this, tr("Browse to existing project directory"), browseStart);
  if (freshDirectory.isEmpty())
    return;
  
  result = Result::Open;
  directory = QDir(freshDirectory);
  addToRecentList();
  
  this->accept();
}

void Dialog::goRecentSlot(int rowIn, int)
{
  QTableWidgetItem *widget = ui->recentTableWidget->item(rowIn, 1);
  result = Result::Recent;
  directory = QDir(widget->text());
  addToRecentList();
  
  this->accept();
}

void Dialog::addToRecentList()
{
  std::string freshEntry = directory.absolutePath().toStdString();
  auto &recent = prf::manager().rootPtr->project().recentProjects().Entry();
  auto it = std::find(recent.begin(), recent.end(), freshEntry);
  if (it != recent.end())
    recent.erase(it);
  recent.insert(recent.begin(), freshEntry);
  prf::manager().saveConfig();
}
