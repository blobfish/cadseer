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

#include <QLabel>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <project/libgit2pp/src/signature.hpp>
#include <project/libgit2pp/src/oid.hpp>

#include <dialogs/commitwidget.h>

using namespace dlg;

CommitWidget::CommitWidget(QWidget *parent):
QWidget(parent)
{
  setContentsMargins(0, 0, 0, 0);
  buildGui();
}

CommitWidget::~CommitWidget() {}

void CommitWidget::setCommit(const git2::Commit &cIn)
{
  commit = cIn;
  update();
}

void CommitWidget::clear()
{
  shaLabel->clear();
  authorNameLabel->clear();
  authorEMailLabel->clear();
  dateTimeEdit->clear();
  textEdit->clear();
}

void CommitWidget::update()
{
  if (commit.isNull())
  {
    clear();
    return;
  }
  
  updateSha();
  git2::Signature author = commit.author();
  authorNameLabel->setText(QString::fromStdString(author.name()));
  authorEMailLabel->setText(QString::fromStdString(author.email()));
  dateTimeEdit->setDateTime(QDateTime::fromTime_t(author.when(), Qt::LocalTime, author.when_offset()));
  textEdit->setText(QString::fromStdString(commit.message()));
}

void CommitWidget::updateSha()
{
  if (commit.isNull())
    return;
  std::string idString = commit.oid().format();
  if (idString.size() > shaLength)
    idString = idString.substr(0, shaLength);
  shaLabel->setText(QString::fromStdString(idString));
}

void CommitWidget::setShaLength(std::size_t l)
{
  shaLength = l;
  updateSha();
}

void CommitWidget::buildGui()
{
  //just the visual labels
  QLabel *sha = new QLabel(tr("Sha:"), this);
  QLabel *authorName = new QLabel(tr("Author Name:"), this);
  QLabel *authorEMail = new QLabel(tr("Author EMail:"), this);
  QLabel *dateTime = new QLabel(tr("Date Time:"), this);
  
  //actual value labels
  shaLabel = new QLabel(this);
  authorNameLabel = new QLabel(this);
  authorEMailLabel = new QLabel(this);
  dateTimeEdit = new QDateTimeEdit(this); dateTimeEdit->setReadOnly(true);
  
  QGridLayout *gl = new QGridLayout();
  gl->addWidget(sha, 0, 0, Qt::AlignRight); gl->addWidget(shaLabel, 0, 1, Qt::AlignLeft);
  gl->addWidget(authorName, 1, 0, Qt::AlignRight); gl->addWidget(authorNameLabel, 1, 1, Qt::AlignLeft);
  gl->addWidget(authorEMail, 2, 0, Qt::AlignRight); gl->addWidget(authorEMailLabel, 2, 1, Qt::AlignLeft);
  gl->addWidget(dateTime, 3, 0, Qt::AlignRight); gl->addWidget(dateTimeEdit, 3, 1, Qt::AlignLeft);
  
  QHBoxLayout *labelLayout = new QHBoxLayout();
  labelLayout->addStretch();
  labelLayout->addLayout(gl);
  
  textEdit = new QTextEdit(this); textEdit->setReadOnly(true);
  textEdit->setWhatsThis(tr("A descriptive message of the commit changes"));
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addLayout(labelLayout);
  mainLayout->addWidget(textEdit);
  this->setLayout(mainLayout);
}
