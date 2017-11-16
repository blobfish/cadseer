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

#ifndef SLC_SELECTIONSTATE_H
#define SLC_SELECTIONSTATE_H

#include <vector>
#include <string>
#include <bitset>

#include <TopAbs_ShapeEnum.hxx>

namespace slc
{
  typedef std::bitset<32> Mask;
  static const Mask None; //default constructs all zeros.
  static const Mask ObjectsEnabled(Mask().set(                       1));
  static const Mask ObjectsSelectable(Mask().set(                    2));
  static const Mask FeaturesEnabled(Mask().set(                      3));
  static const Mask FeaturesSelectable(Mask().set(                   4));
  static const Mask SolidsEnabled(Mask().set(                        5));
  static const Mask SolidsSelectable(Mask().set(                     6));
  static const Mask ShellsEnabled(Mask().set(                        7));
  static const Mask ShellsSelectable(Mask().set(                     8));
  static const Mask FacesEnabled(Mask().set(                         9));
  static const Mask FacesSelectable(Mask().set(                     10));
  static const Mask WiresEnabled(Mask().set(                        11));
  static const Mask WiresSelectable(Mask().set(                     12));
  static const Mask EdgesEnabled(Mask().set(                        13));
  static const Mask EdgesSelectable(Mask().set(                     14));
  static const Mask PointsEnabled(Mask().set(                       15));
  static const Mask PointsSelectable(Mask().set(                    16));
  static const Mask EndPointsEnabled(Mask().set(                    17));
  static const Mask EndPointsSelectable(Mask().set(                 18));
  static const Mask MidPointsEnabled(Mask().set(                    19));
  static const Mask MidPointsSelectable(Mask().set(                 20));
  static const Mask CenterPointsEnabled(Mask().set(                 21));
  static const Mask CenterPointsSelectable(Mask().set(              22));
  static const Mask QuadrantPointsEnabled(Mask().set(               23));
  static const Mask QuadrantPointsSelectable(Mask().set(            24));
  static const Mask NearestPointsEnabled(Mask().set(                25));
  static const Mask NearestPointsSelectable(Mask().set(             26));
  static const Mask ScreenPointsEnabled(Mask().set(                 27));
  static const Mask ScreenPointsSelectable(Mask().set(              28));
  static const Mask All(Mask().set(                                   )); //set with no parameters sets all bits
  static const Mask AllEnabled(ObjectsEnabled | FeaturesEnabled | SolidsEnabled
    | ShellsEnabled | FacesEnabled | WiresEnabled | EdgesEnabled | PointsEnabled);

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
  
  inline bool canSelectObjects(Mask maskIn)
  {
    return (maskIn & (ObjectsEnabled | ObjectsSelectable)).count() == 2;
  }
  
  inline bool canSelectFeatures(Mask maskIn)
  {
    return (maskIn & (FeaturesEnabled | FeaturesSelectable)).count() == 2;
  }
  
  inline bool canSelectSolids(Mask maskIn)
  {
    return (maskIn & (SolidsEnabled | SolidsSelectable)).count() == 2;
  }
  
  inline bool canSelectShells(Mask maskIn)
  {
    return (maskIn & (ShellsEnabled | ShellsSelectable)).count() == 2;
  }
  
  inline bool canSelectFaces(Mask maskIn)
  {
    return (maskIn & (FacesEnabled | FacesSelectable)).count() == 2;
  }
  
  inline bool canSelectWires(Mask maskIn)
  {
    return (maskIn & (WiresEnabled | WiresSelectable)).count() == 2;
  }
  
  inline bool canSelectEdges(Mask maskIn)
  {
    return (maskIn & (EdgesEnabled | EdgesSelectable)).count() == 2;
  }
  
  inline bool canSelectEndPoints(Mask maskIn)
  {
    Mask temp = EndPointsEnabled | EndPointsSelectable | PointsEnabled | PointsSelectable;
    return (maskIn & temp).count() == 4;
  }
  
  inline bool canSelectMidPoints(Mask maskIn)
  {
    Mask temp = MidPointsEnabled | MidPointsSelectable | PointsEnabled | PointsSelectable;
    return (maskIn & temp).count() == 4;
  }
  
  inline bool canSelectCenterPoints(Mask maskIn)
  {
    Mask temp = CenterPointsEnabled | CenterPointsSelectable | PointsEnabled | PointsSelectable;
    return (maskIn & temp).count() == 4;
  }
  
  inline bool canSelectQuadrantPoints(Mask maskIn)
  {
    Mask temp = QuadrantPointsEnabled | QuadrantPointsSelectable | PointsEnabled | PointsSelectable;
    return (maskIn & temp).count() == 4;
  }
  
  inline bool canSelectNearestPoints(Mask maskIn)
  {
    Mask temp = NearestPointsEnabled | NearestPointsSelectable | PointsEnabled | PointsSelectable;
    return (maskIn & temp).count() == 4;
  }
  
  inline bool canSelectScreenPoints(Mask maskIn)
  {
    Mask temp = ScreenPointsEnabled | ScreenPointsSelectable | PointsEnabled | PointsSelectable;
    return (maskIn & temp).count() == 4;
  }
  
  inline bool canSelectPoints(Mask maskIn)
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

#endif // SLC_SELECTIONSTATE_H
