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

#include <dialogs/tagwidget.h>

using namespace dlg;

TagWidget::TagWidget(QWidget *parent):
QWidget(parent)
{
  setContentsMargins(0, 0, 0, 0);
  buildGui();
}

TagWidget::~TagWidget(){}

void TagWidget::setTag(const git2::Tag &tagIn)
{
  tag = tagIn;
  update();
}

void TagWidget::clear()
{
  nameLabel->clear();
  shaTargetLabel->clear();
  authorNameLabel->clear();
  authorEMailLabel->clear();
  dateTimeEdit->clear();
  textEdit->clear();
}

void TagWidget::update()
{
  if (tag.isNull())
  {
    clear();
    return;
  }
  
  nameLabel->setText(QString::fromStdString(tag.name()));
  updateSha();
  
  git2::Signature author = tag.tagger();
  authorNameLabel->setText(QString::fromStdString(author.name()));
  authorEMailLabel->setText(QString::fromStdString(author.email()));
  dateTimeEdit->setDateTime(QDateTime::fromTime_t(author.when(), Qt::LocalTime, author.when_offset()));
  textEdit->setText(QString::fromStdString(tag.message()));
}

void TagWidget::updateSha()
{
  if (tag.isNull())
    return;
  std::string idString = tag.peel().oid().format();
  if (idString.size() > shaLength)
    idString = idString.substr(0, shaLength);
  shaTargetLabel->setText(QString::fromStdString(idString));
}

void TagWidget::setShaLength(std::size_t l)
{
  shaLength = l;
  updateSha();
}

void TagWidget::buildGui()
{
  //just the visual labels
  QLabel *name = new QLabel(tr("Name:"), this);
  QLabel *shaTarget = new QLabel(tr("Sha Target:"), this);
  QLabel *authorName = new QLabel(tr("Author Name:"), this);
  QLabel *authorEMail = new QLabel(tr("Author EMail:"), this);
  QLabel *dateTime = new QLabel(tr("Date Time:"), this);
  
    //actual value labels
  nameLabel = new QLabel(this);
  shaTargetLabel = new QLabel(this);
  authorNameLabel = new QLabel(this);
  authorEMailLabel = new QLabel(this);
  dateTimeEdit = new QDateTimeEdit(this); dateTimeEdit->setReadOnly(true);
  textEdit = new QTextEdit(this); textEdit->setReadOnly(true);
  textEdit->setWhatsThis(tr("A tag message"));
  
  QGridLayout *gl = new QGridLayout();
  gl->addWidget(name, 0, 0, Qt::AlignRight); gl->addWidget(nameLabel, 0, 1, Qt::AlignLeft);
  gl->addWidget(shaTarget, 1, 0, Qt::AlignRight); gl->addWidget(shaTargetLabel, 1, 1, Qt::AlignLeft);
  gl->addWidget(authorName, 2, 0, Qt::AlignRight); gl->addWidget(authorNameLabel, 2, 1, Qt::AlignLeft);
  gl->addWidget(authorEMail, 3, 0, Qt::AlignRight); gl->addWidget(authorEMailLabel, 3, 1, Qt::AlignLeft);
  gl->addWidget(dateTime, 4, 0, Qt::AlignRight); gl->addWidget(dateTimeEdit, 4, 1, Qt::AlignLeft);
  
  QHBoxLayout *hgl = new QHBoxLayout();
  hgl->setContentsMargins(0, 0, 0, 0);
  hgl->addStretch();
  hgl->addLayout(gl);
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addLayout(hgl);
  mainLayout->addWidget(textEdit);
  
  this->setLayout(mainLayout);
}
