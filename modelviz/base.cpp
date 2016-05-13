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

#include <modelviz/base.h>

using namespace mdv;

Base::Base()
{

}

Base::Base(const Base &rhs, const osg::CopyOp& copyOperation):
  osg::Geometry(rhs, copyOperation)
{

}

void Base::setColor(const osg::Vec4& colorIn)
{
  color = colorIn;
}

void Base::setPreHighlightColor(const osg::Vec4& colorIn)
{
  colorPreHighlight = colorIn;
}

void Base::setHighlightColor(const osg::Vec4& colorIn)
{
  colorHighlight = colorIn;
}
