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
 * GNU General Public License for more details.if (selectionType == slc::Type::Object)
 r eturn TopAbs_COMPOUND;*
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <map>

#include <selection/definitions.h>

using namespace slc;


/* selection points and vertices are not necessarily
 * a perfect match. i.e. no vertex for a center point etc..
 */

slc::Type slc::convert(TopAbs_ShapeEnum shapeType)
{
  typedef std::map<TopAbs_ShapeEnum, slc::Type> Map;
  static const Map map
  {
    {TopAbs_COMPOUND, slc::Type::Object},
    {TopAbs_COMPSOLID, slc::Type::Object},
    {TopAbs_SOLID, slc::Type::Solid},
    {TopAbs_SHELL, slc::Type::Shell},
    {TopAbs_FACE, slc::Type::Face},
    {TopAbs_WIRE, slc::Type::Wire},
    {TopAbs_EDGE, slc::Type::Edge},
    {TopAbs_VERTEX, slc::Type::StartPoint}, //note not a 1 to 1 mapping for vertices.
    {TopAbs_SHAPE, slc::Type::None}
  };
  
  return map.at(shapeType);
}

TopAbs_ShapeEnum slc::convert(slc::Type selectionType)
{
  typedef std::map<slc::Type, TopAbs_ShapeEnum> Map;
  static const Map map
  {
    {slc::Type::Object, TopAbs_COMPOUND},
    {slc::Type::Feature, TopAbs_COMPOUND},
    {slc::Type::Solid, TopAbs_SOLID},
    {slc::Type::Shell, TopAbs_SHELL},
    {slc::Type::Face, TopAbs_FACE},
    {slc::Type::Wire, TopAbs_WIRE},
    {slc::Type::Edge, TopAbs_EDGE},
    {slc::Type::StartPoint, TopAbs_VERTEX},
    {slc::Type::EndPoint, TopAbs_VERTEX},
    {slc::Type::MidPoint, TopAbs_VERTEX},
    {slc::Type::CenterPoint, TopAbs_VERTEX},
    {slc::Type::QuadrantPoint, TopAbs_VERTEX},
    {slc::Type::NearestPoint, TopAbs_VERTEX},
    {slc::Type::ScreenPoint, TopAbs_VERTEX}
  };
  
  return map.at(selectionType);
}
