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

#ifndef SELECTIONSTATE_H
#define SELECTIONSTATE_H

#include <vector>
#include <string>

#include <TopAbs_ShapeEnum.hxx>

namespace slc
{
  static const std::size_t None = 0;
  static const std::size_t ObjectsEnabled = 1;
  static const std::size_t ObjectsSelectable = 1 << 1;
  static const std::size_t FeaturesEnabled = 1 << 2;
  static const std::size_t FeaturesSelectable = 1 << 3;
  static const std::size_t SolidsEnabled = 1 << 4;
  static const std::size_t SolidsSelectable = 1 << 5;
  static const std::size_t ShellsEnabled = 1 << 6;
  static const std::size_t ShellsSelectable = 1 << 7;
  static const std::size_t FacesEnabled = 1 << 8;
  static const std::size_t FacesSelectable = 1 << 9;
  static const std::size_t WiresEnabled = 1 << 10;
  static const std::size_t WiresSelectable = 1 << 11;
  static const std::size_t EdgesEnabled = 1 << 12;
  static const std::size_t EdgesSelectable = 1 << 13;
  static const std::size_t PointsEnabled = 1 << 14;
  static const std::size_t PointsSelectable = 1 << 15;
  static const std::size_t EndPointsEnabled = 1 << 16;
  static const std::size_t EndPointsSelectable = 1 << 17;
  static const std::size_t MidPointsEnabled = 1 << 18;
  static const std::size_t MidPointsSelectable = 1 << 19;
  static const std::size_t CenterPointsEnabled = 1 << 20;
  static const std::size_t CenterPointsSelectable = 1 << 21;
  static const std::size_t QuadrantPointsEnabled = 1 << 22;
  static const std::size_t QuadrantPointsSelectable = 1 << 23;
  static const std::size_t NearestPointsEnabled = 1 << 24;
  static const std::size_t NearestPointsSelectable = 1 << 25;
  static const std::size_t ScreenPointsEnabled = 1 << 24;
  static const std::size_t ScreenPointsSelectable = 1 << 26;
  static const std::size_t All = 0xffffffffu;

  enum class Type
  {
      None = 0,
      Object,
      Feature,
      Solid,
      Shell,
      Face,
      Wire,
      Edge,
      StartPoint,
      EndPoint,
      MidPoint,
      CenterPoint,
      QuadrantPoint,
      NearestPoint,
      ScreenPoint
  };

  inline std::string getNameOfType(Type theType)
  {
      static const std::vector<std::string> names({
						      "None",
						      "Object",
						      "Feature",
						      "Solid",
						      "Shell",
						      "Face",
						      "Wire",
						      "Edge",
						      "Start Point",
						      "End Point",
						      "Mid Point",
						      "Center Point",
						      "Quadrant Point",
						      "Nearest Point",
						      "Screen Point"
						  });
      return names.at(static_cast<std::size_t>(theType));
  }
  
  inline bool canSelectObjects(const std::size_t maskIn)
  {
    return
    (
      (maskIn & ObjectsEnabled) &&
      (maskIn & ObjectsSelectable)
    );
  }
  
  inline bool canSelectFeatures(const std::size_t maskIn)
  {
    return
    (
      (maskIn & FeaturesEnabled) &&
      (maskIn & FeaturesSelectable)
    );
  }
  
  inline bool canSelectSolids(const std::size_t maskIn)
  {
    return
    (
      (maskIn & SolidsEnabled) &&
      (maskIn & SolidsSelectable)
    );
  }
  
  inline bool canSelectShells(const std::size_t maskIn)
  {
    return
    (
      (maskIn & ShellsEnabled) &&
      (maskIn & ShellsSelectable)
    );
  }
  
  inline bool canSelectFaces(const std::size_t maskIn)
  {
    return
    (
      (maskIn & FacesEnabled) &&
      (maskIn & FacesSelectable)
    );
  }
  
  inline bool canSelectWires(const std::size_t maskIn)
  {
    return
    (
      (maskIn & WiresEnabled) &&
      (maskIn & WiresSelectable)
    );
  }
  
  inline bool canSelectEdges(const std::size_t maskIn)
  {
    return
    (
      (maskIn & EdgesEnabled) &&
      (maskIn & EdgesSelectable)
    );
  }
  
  inline bool canSelectEndPoints(const std::size_t maskIn)
  {
    return
    (
      (maskIn & EndPointsEnabled) &&
      (maskIn & EndPointsSelectable) &&
      (maskIn & PointsEnabled) &&
      (maskIn & PointsSelectable)
    );
  }
  
  inline bool canSelectMidPoints(const std::size_t maskIn)
  {
    return
    (
      (maskIn & MidPointsEnabled) &&
      (maskIn & MidPointsSelectable) &&
      (maskIn & PointsEnabled) &&
      (maskIn & PointsSelectable)
    );
  }
  
  inline bool canSelectCenterPoints(const std::size_t maskIn)
  {
    return
    (
      (maskIn & CenterPointsEnabled) &&
      (maskIn & CenterPointsSelectable) &&
      (maskIn & PointsEnabled) &&
      (maskIn & PointsSelectable)
    );
  }
  
  inline bool canSelectQuadrantPoints(const std::size_t maskIn)
  {
    return
    (
      (maskIn & QuadrantPointsEnabled) &&
      (maskIn & QuadrantPointsSelectable) &&
      (maskIn & PointsEnabled) &&
      (maskIn & PointsSelectable)
    );
  }
  
  inline bool canSelectNearestPoints(const std::size_t maskIn)
  {
    return
    (
      (maskIn & NearestPointsEnabled) &&
      (maskIn & NearestPointsSelectable) &&
      (maskIn & PointsEnabled) &&
      (maskIn & PointsSelectable)
    );
  }
  
  inline bool canSelectScreenPoints(const std::size_t maskIn)
  {
    return
    (
      (maskIn & ScreenPointsEnabled) &&
      (maskIn & ScreenPointsSelectable) &&
      (maskIn & PointsEnabled) &&
      (maskIn & PointsSelectable)
    );
  }
  
  inline bool canSelectPoints(const std::size_t maskIn)
  {
    return
    (
      canSelectEndPoints(maskIn) ||
      canSelectMidPoints(maskIn) ||
      canSelectCenterPoints(maskIn) ||
      canSelectQuadrantPoints(maskIn) ||
      canSelectNearestPoints(maskIn)
    );
  }
  
  inline bool isPointType(const Type typeIn)
  {
    return
    (
      (typeIn == Type::StartPoint) ||
      (typeIn == Type::EndPoint) ||
      (typeIn == Type::MidPoint) ||
      (typeIn == Type::CenterPoint) ||
      (typeIn == Type::QuadrantPoint) ||
      (typeIn == Type::NearestPoint)
    );
  }
  
  Type convert(TopAbs_ShapeEnum);
  TopAbs_ShapeEnum convert(Type);
}

#endif // SELECTIONSTATE_H
