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

#ifndef GU_INFOTOOLS_H
#define GU_INFOTOOLS_H

class QTextStream;

namespace osg{class Matrixd;}
class gp_Pnt;

namespace gu
{
    QTextStream& osgMatrixOut(QTextStream&, const osg::Matrixd&);
    QTextStream& gpPntOut(QTextStream&, const gp_Pnt&);
    
    static const std::vector<QString> surfaceTypeStrings = 
    {
        ("GeomAbs_Plane"),
        ("GeomAbs_Cylinder"),	
        ("GeomAbs_Cone"),
        ("GeomAbs_Sphere"),
        ("GeomAbs_Torus"),
        ("GeomAbs_BezierSurface"),
        ("GeomAbs_BSplineSurface"),
        ("GeomAbs_SurfaceOfRevolution"),
        ("GeomAbs_SurfaceOfExtrusion"),
        ("GeomAbs_OffsetSurface"),
        ("GeomAbs_OtherSurface ")
    };
    
    static const std::vector<QString> curveTypeStrings = 
    {
        ("GeomAbs_Line"),
        ("GeomAbs_Circle "),
        ("GeomAbs_Ellipse"),
        ("GeomAbs_Hyperbola"),
        ("GeomAbs_Parabola"),
        ("GeomAbs_BezierCurve"),
        ("GeomAbs_BSplineCurve"),
        ("GeomAbs_OffsetCurve"),
        ("GeomAbs_OtherCurve")
    };
}

#endif // GU_INFOTOOLS_H
