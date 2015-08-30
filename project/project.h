#ifndef PROJECT_H
#define PROJECT_H

#include <map>

#include "projectgraph.h"

class TopoDS_Shape;
namespace osg{class Group;}

typedef std::map<boost::uuids::uuid, ProjectGraph::Vertex> IdVertexMap;

class Project
{
public:
    Project();
    void readOCC(const std::string &fileName, osg::Group *root);
    void addOCCShape(const TopoDS_Shape &shapeIn, osg::Group *root);
    const Feature::Base* findFeature(const boost::uuids::uuid &idIn);
    void update();
    void updateVisual();
    void writeGraphViz(const std::string &fileName);
    
    void addFeature(std::shared_ptr<Feature::Base> feature, osg::Group *root);
    void connect(const boost::uuids::uuid &parentIn, const boost::uuids::uuid &childIn, Feature::InputTypes type);
    
private:
    ProjectGraph::Edge connectVertices(ProjectGraph::Vertex parent, ProjectGraph::Vertex child, Feature::InputTypes type);
    
    IdVertexMap map;
    ProjectGraph::Graph projectGraph;
};

#endif // PROJECT_H
