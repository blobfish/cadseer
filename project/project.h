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

#ifndef PROJECT_H
#define PROJECT_H

#include <map>

#include <boost/signals2.hpp>

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
    Feature::Base* findFeature(const boost::uuids::uuid &idIn);
    void update();
    void updateVisual();
    void writeGraphViz(const std::string &fileName);
    void setAllVisualDirty();
    
    void addFeature(std::shared_ptr<Feature::Base> feature, osg::Group *root);
    void connect(const boost::uuids::uuid &parentIn, const boost::uuids::uuid &childIn, Feature::InputTypes type);
    
    typedef boost::signals2::signal<void (std::shared_ptr<Feature::Base> feature)> FeatureAddedSignal;
    boost::signals2::connection connectFeatureAdded(const FeatureAddedSignal::slot_type &subscriber)
    {
      return featureAddedSignal.connect(subscriber);
    }
    
    typedef boost::signals2::signal<void ()> ProjectUpdatedSignal;
    boost::signals2::connection connectProjectUpdated(const ProjectUpdatedSignal::slot_type &subscriber)
    {
      return projectUpdatedSignal.connect(subscriber);
    }
    
    typedef boost::signals2::signal<void (const boost::uuids::uuid&, const boost::uuids::uuid&, Feature::InputTypes)> ConnectionAddedSignal;
    boost::signals2::connection connectConnectionAdded(const ConnectionAddedSignal::slot_type &subscriber)
    {
      return connectionAddedSignal.connect(subscriber);
    }
    
    void stateChangedSlot(const boost::uuids::uuid &featureIdIn, std::size_t stateIn); //!< received from each feature.
    
private:
    ProjectGraph::Edge connectVertices(ProjectGraph::Vertex parent, ProjectGraph::Vertex child, Feature::InputTypes type);
    ProjectGraph::Vertex findVertex(const boost::uuids::uuid &idIn);
    
    IdVertexMap map;
    ProjectGraph::Graph projectGraph;
    
    FeatureAddedSignal featureAddedSignal;
    ProjectUpdatedSignal projectUpdatedSignal;
    ConnectionAddedSignal connectionAddedSignal;
};

#endif // PROJECT_H
