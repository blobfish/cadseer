/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QDir>

#include <osg/Geometry>
#include <osg/MatrixTransform>

#include <nodemaskdefs.h>
#include <modelviz/datumplane.h>
#include <feature/datumplane.h>

using namespace ftr;

QIcon DatumPlane::icon;

DatumPlane::DatumPlane() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumPlane.svg");
  
  name = QObject::tr("Datum Plane");
  
  display = new mdv::DatumPlane();
  
  transform = new osg::MatrixTransform();
  transform->addChild(display.get());
  
  mainSwitch->addChild(transform.get());
  updateGeometry();
}

DatumPlane::~DatumPlane()
{

}

void DatumPlane::updateModel(const UpdateMap &mapIn)
{
  //temp for visual work.
  updateGeometry();
  setModelClean();
}

void DatumPlane::updateVisual()
{
  setVisualClean();
}

void DatumPlane::serialWrite(const QDir &directoryIn)
{
//   ftr::Base::serialWrite(directoryIn);
}

void DatumPlane::updateGeometry()
{
  
  
//   geometry->dirtyDisplayList();
}
