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

#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS.hxx>

#include <osg/Matrixd>

#include <tools/infotools.h>
#include <feature/seershape.h>
#include <feature/seershapeinfo.h>

namespace ftr
{
    class SeerShapeInfo::FunctionMapper
    {
        public:
            typedef std::function<void (QTextStream&, const TopoDS_Shape&)> InfoFunction;
            typedef std::map<TopAbs_ShapeEnum, InfoFunction> FunctionMap;
            FunctionMap functionMap;
    };
}

using namespace ftr;

SeerShapeInfo::SeerShapeInfo(const SeerShape &shapeIn) : seerShape(shapeIn)
{
    functionMapper = std::move(std::unique_ptr<FunctionMapper>(new FunctionMapper()));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_COMPOUND, std::bind(&SeerShapeInfo::compInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_COMPSOLID, std::bind(&SeerShapeInfo::compSolidInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_SOLID, std::bind(&SeerShapeInfo::solidInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_SHELL, std::bind(&SeerShapeInfo::shellInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_FACE, std::bind(&SeerShapeInfo::faceInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_WIRE, std::bind(&SeerShapeInfo::wireInfo, this, std::placeholders::_1, std::placeholders::_2)));
    functionMapper->functionMap.insert(std::make_pair(TopAbs_EDGE, std::bind(&SeerShapeInfo::edgeInfo, this, std::placeholders::_1, std::placeholders::_2)));
    
    //points handled at factory.
}
SeerShapeInfo::~SeerShapeInfo(){}

QTextStream& SeerShapeInfo::getShapeInfo(QTextStream &streamIn, const boost::uuids::uuid &idIn )
{
    assert(functionMapper);
    assert(seerShape.hasShapeIdRecord(idIn));
    const TopoDS_Shape &shape = seerShape.findShapeIdRecord(idIn).shape;
    
    functionMapper->functionMap.at(shape.ShapeType())(streamIn, shape);
    //common to all shapes.
    std::ostringstream locationStream;
    shape.Location().ShallowDump(locationStream);
    streamIn << "orientation: " << ((shape.Orientation() == TopAbs_FORWARD) ? ("Forward") : ("Reversed")) << endl
        << "hash code: " << ShapeIdKeyHash()(shape) << endl;
    
    return streamIn;
}

void SeerShapeInfo::compInfo(QTextStream &streamIn, const TopoDS_Shape&)
{
    streamIn << "Shape type: compound" << endl;
}

void SeerShapeInfo::compSolidInfo(QTextStream &streamIn, const TopoDS_Shape&)
{
    streamIn << "Shape type: compound solid" << endl;
}

void SeerShapeInfo::solidInfo(QTextStream &streamIn, const TopoDS_Shape&)
{
    streamIn << "Shape type: solid" << endl;
}

void SeerShapeInfo::shellInfo(QTextStream &streamIn, const TopoDS_Shape &shapeIn)
{
    streamIn << "Shape type: shell." << endl
        << "Closed is: " << ((shapeIn.Closed()) ? ("true") : ("false")) << endl;
}

void SeerShapeInfo::faceInfo(QTextStream &streamIn, const TopoDS_Shape &shapeIn)
{
    assert(shapeIn.ShapeType() == TopAbs_FACE);
    BRepAdaptor_Surface surfaceAdaptor(TopoDS::Face(shapeIn));
    
    streamIn << "Shape type: face" << endl
        << "Surface type: " << gu::surfaceTypeStrings.at(surfaceAdaptor.GetType()) << endl;
}

void SeerShapeInfo::wireInfo(QTextStream &streamIn, const TopoDS_Shape&)
{
    streamIn << "Shape type: wire" << endl;
}

void SeerShapeInfo::edgeInfo(QTextStream &streamIn, const TopoDS_Shape &shapeIn)
{
    assert(shapeIn.ShapeType() == TopAbs_EDGE);
    BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shapeIn));
    
    streamIn << "Shape type: edge" << endl
        << "Curve type: " << gu::curveTypeStrings.at(curveAdaptor.GetType()) << endl;
}
