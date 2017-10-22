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

#include <QTextStream>

#include <osg/Matrixd>

#include <globalutilities.h>
#include <tools/infotools.h>


QString ms(const double &number)
{
  return QString::number(number, 'f', 12);
};

QTextStream& gu::osgMatrixOut(QTextStream &streamIn, const osg::Matrixd &m)
{
    streamIn << "Coordinate system: "
    << "["
    << "["
    << ms(gu::getXVector(m).x()) << ", "
    << ms(gu::getXVector(m).y()) << ", "
    << ms(gu::getXVector(m).z())
    << "], ["
    << ms(gu::getYVector(m).x()) << ", "
    << ms(gu::getYVector(m).y()) << ", "
    << ms(gu::getYVector(m).z())
    << "], ["
    << ms(gu::getZVector(m).x()) << ", "
    << ms(gu::getZVector(m).y()) << ", "
    << ms(gu::getZVector(m).z())
    << "], ["
    << ms(m.getTrans().x()) << ", "
    << ms(m.getTrans().y()) << ", "
    << ms(m.getTrans().z())
    << "]"
    << "]" << endl;
    
    return streamIn;
}

QTextStream& gu::osgQuatOut(QTextStream &s, const osg::Quat &qIn)
{
  s << "[" << ms(qIn.x()) << ", " << ms(qIn.y()) << ", " << ms(qIn.z()) << ", " << ms(qIn.w()) << "]";
}

QTextStream& gu::osgVectorOut(QTextStream &s, const osg::Vec3d &vIn)
{
  s << "[" << ms(vIn.x()) << ", " << ms(vIn.y()) << ", " << ms(vIn.z()) << "]";
}

QTextStream& gu::gpPntOut(QTextStream &sIn, const gp_Pnt &pIn)
{
  sIn << "[" << pIn.X() << ", " << pIn.Y() << ", " << pIn.Z() << "]";
  
  return sIn;
}
