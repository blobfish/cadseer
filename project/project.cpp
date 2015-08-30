#include <assert.h>
#include <iostream>

#include <boost/graph/topological_sort.hpp>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopoDS_Iterator.hxx>

#include <osg/ValueObject>

#include "project.h"
#include "../modelviz/graph.h"
#include "../modelviz/connector.h"
#include "../globalutilities.h"
#include "../feature/base.h"
#include "../feature/inert.h"

using namespace ProjectGraph;
using boost::uuids::uuid;

Project::Project()
{
}

void Project::update()
{
  writeGraphViz("/home/tanderson/temp/cadseer.dot");
  
  Path sorted;
  try
  {
    boost::topological_sort(projectGraph, std::back_inserter(sorted));
  }
  catch(const boost::not_a_dag &)
  {
    std::cout << std::endl << "Graph is not a dag exception in Project::update()" << std::endl << std::endl;
    return;
  }
  
  for (const auto &currentVertex : sorted)
  {
    if (projectGraph[currentVertex].feature->isClean())
      continue;
    
    Feature::UpdateMap updateMap;
    InEdgeIterator inEdgeIt, inEdgeItDone;
    boost::tie(inEdgeIt, inEdgeItDone) = boost::in_edges(currentVertex, projectGraph);
    for(;inEdgeIt != inEdgeItDone; ++inEdgeIt)
    {
      Vertex source = boost::source(*inEdgeIt, projectGraph);
      updateMap.insert(std::make_pair(projectGraph[*inEdgeIt].inputType, projectGraph[source].feature.get()));
    }
    projectGraph[currentVertex].feature->update(updateMap);
  }
}

void Project::updateVisual()
{
  Path sorted;
  try
  {
    boost::topological_sort(projectGraph, std::back_inserter(sorted));
  }
  catch(const boost::not_a_dag &)
  {
    std::cout << std::endl << "Graph is not a dag exception in Project::update()" << std::endl << std::endl;
    return;
  }
  
  for (const auto &currentVertex : sorted)
  {
    if (projectGraph[currentVertex].feature->isVisualClean())
      continue;
    projectGraph[currentVertex].feature->updateVisual();
  }
}

void Project::writeGraphViz(const std::string& fileName)
{
  outputGraphviz(projectGraph, fileName);
}


void Project::readOCC(const std::string &fileName, osg::Group *root)
{
    TopoDS_Shape base;
    BRep_Builder junk;
    std::fstream file(fileName.c_str());
    BRepTools::Read(base, file, junk);

    //structure understood to be a compound of compounds.
    //check base is a compound.
    if (base.ShapeType() != TopAbs_COMPOUND)
    {
        std::cout << "expected base compound in reading of OCC" << std::endl;
        return;
    }

    TopoDS_Iterator it;
    for (it.Initialize(base); it.More(); it.Next())
    {
        TopoDS_Shape current = it.Value();
        if (current.ShapeType() != TopAbs_COMPOUND)
        {
            std::cout << "expected compound in reading of OCC. Got " << current.ShapeType() << std::endl;
//            continue;
        }

        addOCCShape(current, root);
    }
}

const Feature::Base* Project::findFeature(const boost::uuids::uuid &idIn)
{
    IdVertexMap::const_iterator it;
    it = map.find(idIn);
    assert(it != map.end());
    return projectGraph[it->second].feature.get();
}

void Project::addOCCShape(const TopoDS_Shape &shapeIn, osg::Group *root)
{
  std::shared_ptr<Feature::Inert> inert(new Feature::Inert(shapeIn));
  addFeature(inert, root);
}

void Project::addFeature(std::shared_ptr<Feature::Base> feature, osg::Group *root)
{
  Vertex newVertex = boost::add_vertex(projectGraph);
  projectGraph[newVertex].feature = feature;
  
  map.insert(std::make_pair(feature->getId(), newVertex));
  root->addChild(feature->getMainSwitch());
}

void Project::connect(const boost::uuids::uuid& parentIn, const boost::uuids::uuid& childIn, Feature::InputTypes type)
{
  Vertex parent = map.at(parentIn);
  Vertex child = map.at(childIn);
  Edge edge = connectVertices(parent, child, type);
  projectGraph[edge].inputType = type;
}


Edge Project::connectVertices(Vertex parent, Vertex child, Feature::InputTypes type)
{
  bool results;
  Edge newEdge;
  boost::tie(newEdge, results) = boost::add_edge(parent, child, projectGraph);
  assert(results);
  projectGraph[newEdge].inputType = type;
  return newEdge;
}
