/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef SUPPORT_H
#define SUPPORT_H

#include <tuple>

#include <osg/Group>
#include <osg/Geometry>
#include <osgDB/WriteFile>

#include "mesh.h"

namespace sqs
{
  std::string getFileExtension(const std::string &); //!< empty string if unrecognized.
  Mesh readFile(const std::string&);
  
  
  struct OsgInfo
  {
  public:
    OsgInfo
    (
      const Mesh &meshIn,
      const osg::Vec4 faceColorIn = osg::Vec4(1.0, 0.0, 0.0, 1.0),
      const osg::Vec4 edgeColorIn = osg::Vec4(0.0, 0.0, 0.0, 1.0),
      bool facesIn = true,
      bool edgesIn = true
    ): mesh(meshIn), faceColor(faceColorIn), edgeColor(edgeColorIn), faces(facesIn), edges(edgesIn)
    {}
    const Mesh &mesh;
    osg::Vec4 faceColor;
    osg::Vec4 edgeColor;
    bool faces;
    bool edges;
  };
  
  osg::Group* createOsg(const OsgInfo&);
  osg::Geometry* createPoints(osg::Vec3Array*, osg::Vec4 color = osg::Vec4(1.0, 1.0, 1.0, 1.0));
  osg::Geometry* createLines(osg::Vec3Array*, osg::Vec4 color = osg::Vec4(1.0, 1.0, 0.0, 1.0));
  osg::Geometry* createTriangles(osg::Vec3Array*, osg::Vec4 color = osg::Vec4(1.0, 0.0, 0.0, 0.5));
  osg::Vec3Array* createArray(const Mesh&, const Vertices&);
  osg::Vec3Array* createArray(const Mesh&, const Edges&);
  osg::Vec3Array* createArray(const Mesh&, const HalfEdges&);
  osg::Vec3Array* createArray(const Mesh&, const Faces&);
  osg::Geometry* createBoundingCircle(const osg::BoundingSphered&);
  void assignDepth(osg::Geometry*, double, double);
  void assignBlend(osg::Geometry*);
  void assignWidth(osg::Geometry*, double);
  void assignPointSize(osg::Geometry*, double);
  osg::Group* generateInput(const Mesh&);
  osg::Group* generateFilled(const Mesh&);
  osg::Group* generateRefined(const Mesh&);
  osg::Group* generateBoundary(const Mesh&, const Faces&, const Vertices&);
  osg::Group* generateObtuse(const Mesh&, const Faces&);
  
  
  
  
  //! copy a face from one mesh to another. used for debugging.
  Face copyFace(const Mesh &sourceIn, const Face &faceIn, Mesh &target);
  
  //! copy a edge from one mesh to another. used for debugging.
  Edge copyEdge(const Mesh &sourceIn, const Edge &edgeIn, Mesh &target);
  
  //! copy a half edge from one mesh to another. used for debugging.
  HalfEdge copyHalfEdge(const Mesh &sourceIn, const HalfEdge &halfEdgeIn, Mesh &target);
  
  //! get all vertex connected adjacent faces. output doesn't include input face.
  Faces adjacentFaces(const Mesh&, const Face&);
  
  //! get all vertex connected adjacent faces.
  Faces adjacentFaces(const Mesh&, const Vertex&);
  
  //! get all vertex connected adjacent edges.
  Edges adjacentEdges(const Mesh&, const Vertex&);
  
  //! get all vertices connected vertex. 1 degree. output doesn't include input vertex.
  Vertices adjacentVertices(const Mesh&, const Vertex&);
  
  /*! given a region of faces return the region(internal) edges, passed in faces boundary edges, and
   * passed in faces boundary edges that are also the mesh border. respectively.
   * input faces expected to be 1 contigous region.
   */
  std::tuple<Edges, Edges, Edges> classifyEdges(const Mesh&, const Faces&);
  
  Edges getBoundaryEdges(const Mesh&);
  
  //! get each mesh boundary. largest boundary will be the last
  std::vector<HalfEdges> getBoundaries(const Mesh &mIn);
  
  void getBSphere(const Mesh&, BSphere&);
  
  //! get the bounding sphere radius of the half edges.
  double getBSphereRadius(const Mesh &mIn, const HalfEdges &hesIn);
  
  double getFaceArea(const Mesh&, const Face&);
  double getMeshFaceArea(const Mesh&);
  double getEdgeLength(const Mesh&, const Edge&);
  double getMeshEdgeLength(const Mesh&);
  
  Faces obtuse(const Mesh&, double);
  
  //make a vector contain unique items.
  template<typename T>
  void uniquefy(T &t)
  {
    std::sort(t.begin(), t.end());
    auto last = std::unique(t.begin(), t.end());
    t.erase(last, t.end());
  }
}

#endif // SUPPORT_H
