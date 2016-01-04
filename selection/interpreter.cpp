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

#include <cassert>

#include <nodemaskdefs.h>
#include <globalutilities.h>
#include <feature/base.h>
#include <application/application.h>
#include <project/project.h>
#include <modelviz/shapegeometry.h>
#include <modelviz/connector.h>
#include <selection/visitors.h>
#include <selection/interpreter.h>

using namespace slc;
using boost::uuids::uuid;
using boost::uuids::nil_generator;

//We need to make the containers a set so we don't put multiple items in.

//helper function
static uuid getFeatureId(mdv::ShapeGeometry *geometryIn)
{
  ParentMaskVisitor visitor(NodeMaskDef::object);
  geometryIn->accept(visitor);
  osg::Node *featureRoot = visitor.out;
  assert(featureRoot);
  return gu::getId(featureRoot);
}

//helper function
static const mdv::Connector& getConnector(const uuid &idIn)
{
  const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(idIn);
  assert(feature);
  return feature->getConnector();
}

Interpreter::Interpreter(const osgUtil::LineSegmentIntersector::Intersections& intersectionsIn, std::size_t selectionMaskIn) :
  intersections(intersectionsIn), selectionMask(selectionMaskIn)
{
  go();
}

void Interpreter::go()
{
  for (auto const &intersection : intersections)
  {
    mdv::ShapeGeometry *shapeGeometry = dynamic_cast<mdv::ShapeGeometry *>(intersection.drawable.get());
    if (!shapeGeometry)
	continue;
    int localNodeMask = intersection.nodePath.back()->getNodeMask();
    uuid selectedId = shapeGeometry->getId(intersection.primitiveIndex);
    uuid featureId = getFeatureId(shapeGeometry);
    const mdv::Connector &connector = getConnector(featureId);
    
    Container container;
    container.featureId = featureId;
    if (localNodeMask == NodeMaskDef::edge)
    {
      if (canSelectPoints(selectionMask))
      {
	osg::Vec3d iPoint = intersection.getWorldIntersectPoint();
	osg::Vec3d snapPoint;
	double distance = std::numeric_limits<double>::max();
	slc::Type sType = slc::Type::None;
	
	auto updateSnaps = [&](const std::vector<osg::Vec3d> &vecIn) -> bool
	{
	  bool out = false;
	  for (const auto& point : vecIn)
	  {
	    double tempDistance = (iPoint - point).length2();
	    if (tempDistance < distance)
	    {
	      snapPoint = point;
	      distance = tempDistance;
	      out = true;
	    }
	  }
	  return out;
	};
	
	if (canSelectEndPoints(selectionMask))
	{
	  std::vector<osg::Vec3d> endPoints = connector.useGetEndPoints(selectedId);
	  if (endPoints.size() > 0)
	  {
	    std::vector<osg::Vec3d> startPoint;
	    startPoint.push_back(endPoints.at(0));
	    if (updateSnaps(startPoint))
	      sType = slc::Type::StartPoint;
	  }
	  if (endPoints.size() > 1)
	  {
	    std::vector<osg::Vec3d> endPoint;
	    endPoint.push_back(endPoints.at(1));
	    if (updateSnaps(endPoints))
	      sType = slc::Type::EndPoint;
	  }
	}
	
	if (canSelectMidPoints(selectionMask))
	{
	  std::vector<osg::Vec3d> midPoints = connector.useGetMidPoint(selectedId);
	  if (updateSnaps(midPoints))
	    sType = slc::Type::MidPoint;
	}
	
	if (canSelectCenterPoints(selectionMask))
	{
	  std::vector<osg::Vec3d> centerPoints = connector.useGetCenterPoint(selectedId);
	  if (updateSnaps(centerPoints))
	    sType = slc::Type::CenterPoint;
	}
	
	if (canSelectQuadrantPoints(selectionMask))
	{
	  std::vector<osg::Vec3d> quadrantPoints = connector.useGetQuadrantPoints(selectedId);
	  if (updateSnaps(quadrantPoints))
	    sType = slc::Type::QuadrantPoint;
	}
	
	if (canSelectNearestPoints(selectionMask))
	{
	  std::vector<osg::Vec3d> nearestPoints = connector.useGetNearestPoint(selectedId, intersection.getWorldIntersectPoint());
	  if (updateSnaps(nearestPoints))
	    sType = slc::Type::NearestPoint;
	}
	container.selectionType = sType;
	container.shapeId = selectedId;
	container.pointLocation = snapPoint;
	add(containersOut,container);
      }
      if (canSelectEdges(selectionMask))
      {
	container.selectionType = Type::Edge;
	container.shapeId = selectedId;
	container.selectionIds.push_back(selectedId);
	container.pointLocation = intersection.getWorldIntersectPoint();
	add(containersOut, container);
      }
      if (canSelectWires(selectionMask))
      {
	//only select 'faceless' wires through an edge.
	//if the wire has a face it will be selected through
	//face intersection.
	
	uuid edgeId = selectedId;
	std::vector<uuid> wireIds = connector.useGetFacelessWires(edgeId);
	if (!wireIds.empty())
	{
	  container.selectionType = Type::Wire;
	  container.selectionIds = connector.useGetChildrenOfType(wireIds.front(), TopAbs_EDGE);
	  container.shapeId = wireIds.front();
	  container.pointLocation = intersection.getWorldIntersectPoint();
	  add(containersOut, container);
	}
      }
    }
    else if(localNodeMask == NodeMaskDef::face)
    {
      if (canSelectWires(selectionMask))
      {
	uuid wire = connector.useGetClosestWire(selectedId, intersection.getWorldIntersectPoint());
	if (!wire.is_nil())
	{
	  container.selectionIds = connector.useGetChildrenOfType(wire, TopAbs_EDGE);
	  container.selectionType = Type::Wire;
	  container.shapeId = wire;
	  container.pointLocation = intersection.getWorldIntersectPoint();
	  add(containersOut, container);
	}
      }
      if (canSelectFaces(selectionMask))
      {
	container.selectionType = Type::Face;
	container.shapeId = selectedId;
	container.selectionIds.push_back(selectedId);
	container.pointLocation = intersection.getWorldIntersectPoint();
	add(containersOut,container);
      }
      if (canSelectShells(selectionMask))
      {
	std::vector<uuid> shells = connector.useGetParentsOfType(selectedId, TopAbs_SHELL);
	if (shells.size() == 1)
	{
	  container.selectionType = Type::Shell;
	  container.shapeId = shells.at(0);
	  if (!has(containersOut, container)) //don't run again
	  {
	    container.selectionIds = connector.useGetChildrenOfType(shells.at(0), TopAbs_FACE);
	    container.pointLocation = intersection.getWorldIntersectPoint();
	    add(containersOut, container);
	  }
	}
      }
      if (canSelectSolids(selectionMask))
      {
	std::vector<uuid> solids = connector.useGetParentsOfType(selectedId, TopAbs_SOLID);
	//should be only 1 solid
	if (solids.size() == 1)
	{
	  container.selectionType = Type::Solid;
	  container.shapeId = solids.at(0);
	  if (!has(containersOut, container)) //don't run again
	  {
	    container.selectionIds = connector.useGetChildrenOfType(solids.at(0), TopAbs_FACE);
	    container.pointLocation = intersection.getWorldIntersectPoint();
	    add(containersOut, container);
	  }
	}
      }
      if (canSelectObjects(selectionMask))
      {
	uuid object = connector.useGetRoot();
	if (!object.is_nil())
	{
	  container.selectionType = Type::Object;
	  container.shapeId = nil_generator()();
	  if (!has(containersOut, container)) //don't run again
	  {
	    container.selectionIds = connector.useGetChildrenOfType(object, TopAbs_FACE);
	    container.pointLocation = intersection.getWorldIntersectPoint();
	    add(containersOut, container);
	  }
	}
      }
    }
  }
}
