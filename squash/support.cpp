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

#include <fstream>

#include <osg/Depth>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/io_utils>
#include <osgUtil/SmoothingVisitor>

#include "tools.h"
#include "support.h"

using namespace sqs;

std::string sqs::getFileExtension(const std::string &fn)
{
  std::string::size_type p = fn.rfind(".");
  if (p == std::string::npos)
    return std::string();
  return fn.substr(p+1);
}

Mesh sqs::readFile(const std::string &fileName)
{
  enum class ReadState
  {
    Start,
    Nodes,
    EndNodes,
    Elements,
    EndElements,
    Finish,
  };
  
  Mesh out;
  
  auto splitLine = [](const std::string &line) -> std::vector<std::string>
  {
    std::string buffer;
    std::vector<std::string> strings;
    std::istringstream stream(line);
    while(std::getline(stream, buffer, ' '))
    {
      if (!buffer.empty())
        strings.push_back(buffer);
    }
    return strings;
  };
  
  auto parseNode = [&](const std::string &line)
  {
    std::vector<std::string> strings = splitLine(line);
    if (strings.size() != 4)
      return; //first line has count only.
      
    out.add_vertex(Point(std::stod(strings.at(1)), std::stod(strings.at(2)), std::stod(strings.at(3))));
  };
  
  auto parseElement = [&](const std::string &line)
  {
    std::vector<std::string> strings = splitLine(line);
    if (strings.size() != 8)
      return; //first line has count only.
      
    //indexes in file are 1 based not zero.
    //assuming triangles
    Vertex u = static_cast<Vertex>(std::stoi(strings.at(5)) - 1);
    Vertex v = static_cast<Vertex>(std::stoi(strings.at(6)) - 1);
    Vertex w = static_cast<Vertex>(std::stoi(strings.at(7)) - 1);

    out.add_face(u,v,w);
  };
  
  std::string line;
  std::ifstream fileStream;
  fileStream.open(fileName.c_str(), std::ios::in);
  assert(fileStream.is_open());
  ReadState readState = ReadState::Start;
  while(std::getline(fileStream, line))
  {
    if (line.empty())
      continue;
    if (readState == ReadState::Start)
    {
      if (line.find("$Nodes") != std::string::npos)
        readState = ReadState::Nodes;
      continue;
    }
    if (readState == ReadState::Nodes)
    {
      if (line.find("$EndNodes") != std::string::npos)
        readState = ReadState::EndNodes;
      else
        parseNode(line);
      continue;
    }
    if (readState == ReadState::EndNodes)
    {
      if (line.find("$Elements") != std::string::npos)
        readState = ReadState::Elements;
      continue;
    }
    if (readState == ReadState::Elements)
    {
      if (line.find("$EndElements") != std::string::npos)
        readState = ReadState::EndElements;
      else
        parseElement(line);
      continue;
    }
  }
  
  fileStream.close();
  return out;
}











osg::Group* sqs::createOsg(const OsgInfo &osgInfo)
{
  osg::ref_ptr<osg::Group> root = new osg::Group();
  
  
  //faces
  if (osgInfo.faces)
  {
    osg::ref_ptr<osg::Depth> faceDepth = new osg::Depth();
    faceDepth->setRange(0.005, 1.005);
    
    osg::ref_ptr<osg::Geometry> faceGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> faceVertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> faceColors = new osg::Vec4Array();
    
    faceGeometry->setName("faces");
    faceGeometry->setDataVariance(osg::Object::STATIC);
    faceGeometry->setVertexArray(faceVertices.get());
    faceGeometry->setUseDisplayList(false);
    faceGeometry->setColorArray(faceColors.get());
    faceGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    faceGeometry->getOrCreateStateSet()->setAttribute(faceDepth.get());
    faceGeometry->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    faceGeometry->getOrCreateStateSet()->setMode(GL_BLEND, true);

    osg::ref_ptr<osg::DrawElementsUInt> triangles = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (Face face : osgInfo.mesh.faces())
    {
      for(Vertex vd : vertices_around_face(osgInfo.mesh.halfedge(face), osgInfo.mesh))
      {
        faceVertices->push_back(osg::Vec3(osgInfo.mesh.point(vd).x(), osgInfo.mesh.point(vd).y(), osgInfo.mesh.point(vd).z()));
        faceColors->push_back(osgInfo.faceColor);
        triangles->push_back(faceVertices->size() - 1);
      }
    }
    faceGeometry->addPrimitiveSet(triangles.get());
    osgUtil::SmoothingVisitor::smooth(*faceGeometry); //build normals
    root->addChild(faceGeometry.get());
  }
  
  
  //edges
  if (osgInfo.edges)
  {
    osg::ref_ptr<osg::Geometry> edgeGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> edgeVertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> edgeColors = new osg::Vec4Array();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(2.0f);
    edgeGeometry->setName("edges");
    edgeGeometry->setDataVariance(osg::Object::STATIC);
    edgeGeometry->setVertexArray(edgeVertices.get());
    edgeGeometry->setUseDisplayList(false);
    edgeGeometry->setColorArray(edgeColors.get());
    edgeGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    edgeGeometry->getOrCreateStateSet()->setAttribute(lineWidth.get());
    edgeGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    osg::ref_ptr<osg::DrawElementsUInt> edges = new osg::DrawElementsUInt(GL_LINES);
    for (Edge edge : osgInfo.mesh.edges())
    {
          Vertex start = osgInfo.mesh.vertex(edge, 0);
          Vertex end = osgInfo.mesh.vertex(edge, 1);
      edgeVertices->push_back(osg::Vec3(osgInfo.mesh.point(start).x(), osgInfo.mesh.point(start).y(), osgInfo.mesh.point(start).z()));
      edges->push_back(edgeVertices->size() - 1);
      edgeColors->push_back(osgInfo.edgeColor);

      edgeVertices->push_back(osg::Vec3(osgInfo.mesh.point(end).x(), osgInfo.mesh.point(end).y(), osgInfo.mesh.point(end).z()));
      edges->push_back(edgeVertices->size() - 1);
      edgeColors->push_back(osgInfo.edgeColor);
    }
    edgeGeometry->addPrimitiveSet(edges.get());
    assignDepth(edgeGeometry, 0.004, 1.004);
    root->addChild(edgeGeometry.get());
  }
  
  return root.release();
}

osg::Geometry* sqs::createTriangles(osg::Vec3Array *faceVertices, osg::Vec4 color)
{
  osg::ref_ptr<osg::Geometry> faceGeometry = new osg::Geometry();
  osg::ref_ptr<osg::Vec4Array> faceColors = new osg::Vec4Array();
  faceColors->push_back(color);
  
  faceGeometry->setName("faces");
  faceGeometry->setDataVariance(osg::Object::STATIC);
  faceGeometry->setVertexArray(faceVertices);
  faceGeometry->setUseDisplayList(false);
  faceGeometry->setColorArray(faceColors.get());
  faceGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  faceGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, faceVertices->size()));
  
  osgUtil::SmoothingVisitor::smooth(*faceGeometry); //build normals
  
  return faceGeometry.release();
}

osg::Geometry* sqs::createPoints(osg::Vec3Array *points, osg::Vec4 color)
{
  osg::Geometry *vertexGeometry = new osg::Geometry();
  vertexGeometry->setCullingActive(false);
  
  vertexGeometry->setVertexArray(points);
  
  osg::Vec4Array *colors = new osg::Vec4Array();
  colors->push_back(color);
  vertexGeometry->setColorArray(colors);
  vertexGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  
  vertexGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, points->size()));
  
  return vertexGeometry;
}

osg::Geometry* sqs::createLines(osg::Vec3Array *pIn, osg::Vec4 color)
{
  osg::Geometry *g = new osg::Geometry();
  g->setVertexArray(pIn);
  
  osg::Vec4Array *colors = new osg::Vec4Array();
  colors->push_back(color);
  g->setColorArray(colors);
  g->setColorBinding(osg::Geometry::BIND_OVERALL);
  
  g->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, pIn->size()));
  
  return g;
}

osg::Vec3Array* sqs::createArray(const Mesh &meshIn, const Vertices &verts)
{
  osg::Vec3Array *out = new osg::Vec3Array();
  for (const auto &v : verts)
    out->push_back(sqs::toOsg(meshIn.point(v)));
  
  return out;
}

osg::Vec3Array* sqs::createArray(const Mesh &meshIn, const Edges &eIn)
{
  osg::Vec3Array *out = new osg::Vec3Array();
  for (const auto &e : eIn)
  {
    out->push_back(sqs::toOsg(meshIn.point(meshIn.vertex(e, 0))));
    out->push_back(sqs::toOsg(meshIn.point(meshIn.vertex(e, 1))));
  }
  
  return out;
}

osg::Vec3Array* sqs::createArray(const Mesh &meshIn, const HalfEdges &hesIn)
{
  osg::Vec3Array *out = new osg::Vec3Array();
  for (const auto &he : hesIn)
  {
    out->push_back(sqs::toOsg(meshIn.point(meshIn.source(he))));
    out->push_back(sqs::toOsg(meshIn.point(meshIn.target(he))));
  }
  
  return out;
}

osg::Vec3Array* sqs::createArray(const Mesh &meshIn, const Faces &fsIn)
{
  osg::Vec3Array *out = new osg::Vec3Array();
  for (const auto &f : fsIn)
  {
    for(Vertex vd : vertices_around_face(meshIn.halfedge(f), meshIn))
    {
      out->push_back(sqs::toOsg(meshIn.point(vd)));
    }
  }
  
  return out;
}

osg::Geometry* sqs::createBoundingCircle(const osg::BoundingSphered &sIn)
{
  osg::Vec3Array* points = new osg::Vec3Array();
  
  osg::Quat rotation(2 * osg::PI / static_cast<double>(64), osg::Vec3d(0.0, 0.0, 1.0));
  osg::Vec3d startingPoint(sIn.radius(), 0.0, 0.0);
  points->push_back(startingPoint + sIn.center());
  osg::Vec3d lastPoint = startingPoint;
  std::size_t loopCount = 63;
  for (std::size_t index = 0; index < loopCount; ++index)
  {
    osg::Vec3d nextPoint = rotation * lastPoint;
    points->push_back(nextPoint + sIn.center());
    lastPoint = nextPoint;
  }
  
  osg::ref_ptr<osg::Geometry> out = new osg::Geometry();
  out->setUseDisplayList(false);
  out->setUseVertexBufferObjects(true);
  out->setVertexArray(points);
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, points->size()));
  
  return out.release();
}

void sqs::assignDepth(osg::Geometry *g, double l, double h)
{
  osg::ref_ptr<osg::Depth> faceDepth = new osg::Depth();
  faceDepth->setRange(l, h);
  g->getOrCreateStateSet()->setAttribute(faceDepth.get());
}

void sqs::assignBlend(osg::Geometry *g)
{
  g->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
  g->getOrCreateStateSet()->setMode(GL_BLEND, true);
}

void sqs::assignWidth(osg::Geometry *g, double w)
{
  g->getOrCreateStateSet()->setAttribute(new osg::LineWidth(w));
}

void sqs::assignPointSize(osg::Geometry *g, double s)
{
  g->getOrCreateStateSet()->setAttribute(new osg::Point(s));
}

osg::Group* sqs::generateInput(const Mesh &mIn)
{
  osg::ref_ptr<osg::Group> inputViz = new osg::Group();
  Faces bmf;
  for(const auto &f : mIn.faces())
    bmf.push_back(f);
  osg::Geometry *bmt = sqs::createTriangles(sqs::createArray(mIn, bmf), osg::Vec4(1.0, 1.0, 0.0, 0.3));
  sqs::assignDepth(bmt, 0.011, 1.011);
  sqs::assignBlend(bmt);
  inputViz->addChild(bmt);
  
  Edges bme;
  for(const auto &e : mIn.edges())
    bme.push_back(e);
  osg::Geometry *bml = sqs::createLines(sqs::createArray(mIn, bme), osg::Vec4(1.0, 1.0, 0.0, 1.0));
  sqs::assignWidth(bml, 1.0);
  sqs::assignDepth(bml, 0.01, 1.01);
  inputViz->addChild(bml);
  
  Edges border = sqs::getBoundaryEdges(mIn);
  osg::Geometry *boundary = sqs::createLines(sqs::createArray(mIn, border), osg::Vec4(1.0, 1.0, 1.0, 1.0));
  sqs::assignWidth(boundary, 5.0);
  inputViz->addChild(boundary);
  
  //show ends in boundary.
  Vertices tv;
  for (const auto &e : border)
  {
    tv.push_back(mIn.vertex(e, 0));
    tv.push_back(mIn.vertex(e, 1));
  }
  sqs::uniquefy(tv);
  Vertices bv;
  for (const auto &v : tv)
  {
    Edges te;
    for (const auto &he : CGAL::halfedges_around_target(v, mIn))
      te.push_back(mIn.edge(he));
    sqs::uniquefy(te);
    Edges ie; //intersection edges.
    std::set_intersection(border.begin(), border.end(), te.begin(), te.end(), std::back_inserter(ie));
    if (ie.size() != 2)
      bv.push_back(v);
  }
  if (!bv.empty())
  {
    osg::Geometry *bust = sqs::createPoints(sqs::createArray(mIn, bv), osg::Vec4(0.0, 0.0, 0.0, 1.0));
    sqs::assignDepth(bust, 0.007, 1.007);
    sqs::assignPointSize(bust, 5);
    inputViz->addChild(bust);
    std::cout << "found " << bv.size() << " breaks in boundary" << std::endl;
  }
  
  return inputViz.release();
}

osg::Group* sqs::generateFilled(const Mesh &mIn)
{
  osg::ref_ptr<osg::Group> filledViz = new osg::Group();
  Faces bmf;
  for(const auto &f : mIn.faces())
    bmf.push_back(f);
  osg::Geometry *bmt = sqs::createTriangles(sqs::createArray(mIn, bmf), osg::Vec4(1.0, 0.0, 0.0, 0.3));
  sqs::assignDepth(bmt, 0.01, 1.01);
  sqs::assignBlend(bmt);
  filledViz->addChild(bmt);
  Edges bme;
  for(const auto &e : mIn.edges())
    bme.push_back(e);
  osg::Geometry *bml = sqs::createLines(sqs::createArray(mIn, bme), osg::Vec4(1.0, 0.0, 0.0, 1.0));
  sqs::assignDepth(bml, 0.009, 1.009);
  sqs::assignWidth(bml, 1.0);
  filledViz->addChild(bml);
  
  return filledViz.release();
}

osg::Group* sqs::generateRefined(const Mesh &mIn)
{
  osg::ref_ptr<osg::Group> refinedViz = new osg::Group();
  Faces bmf;
  for(const auto &f : mIn.faces())
    bmf.push_back(f);
  osg::Geometry *bmt = sqs::createTriangles(sqs::createArray(mIn, bmf), osg::Vec4(1.0, 0.0, 0.0, 0.3));
  sqs::assignDepth(bmt, 0.01, 1.01);
  sqs::assignBlend(bmt);
  refinedViz->addChild(bmt);
  Edges bme;
  for(const auto &e : mIn.edges())
    bme.push_back(e);
  osg::Geometry *bml = sqs::createLines(sqs::createArray(mIn, bme), osg::Vec4(1.0, 0.0, 0.0, 1.0));
  sqs::assignDepth(bml, 0.009, 1.009);
  sqs::assignWidth(bml, 1.0);
  refinedViz->addChild(bml);
  
  return refinedViz.release();
}

osg::Group* sqs::generateBoundary(const Mesh &mIn, const Faces &baseFaces, const Vertices &baseVertices)
{
  osg::ref_ptr<osg::Group> boundaryViz = new osg::Group();
  Faces bmf;
  for(const auto &f : mIn.faces())
    bmf.push_back(f);
  osg::Geometry *bmt = sqs::createTriangles(sqs::createArray(mIn, bmf), osg::Vec4(1.0, 0.0, 0.0, 0.3));
  sqs::assignDepth(bmt, 0.01, 1.01);
  sqs::assignBlend(bmt);
  boundaryViz->addChild(bmt);
  
  Edges bme;
  for(const auto &e : mIn.edges())
    bme.push_back(e);
  osg::Geometry *bml = sqs::createLines(sqs::createArray(mIn, bme), osg::Vec4(1.0, 0.0, 0.0, 1.0));
  sqs::assignDepth(bml, 0.009, 1.009);
  sqs::assignWidth(bml, 1.0);
  boundaryViz->addChild(bml);
  
  Faces sf; //start faces
  Edges se; //start edges
  for (const auto &f : baseFaces)
  {
    sf.push_back(f);
    for (const auto &he : CGAL::halfedges_around_face(mIn.halfedge(f), mIn))
      se.push_back(mIn.edge(he));
  }
  sqs::uniquefy(sf);
  sqs::uniquefy(se);
  
  osg::Geometry *sfg = sqs::createTriangles(sqs::createArray(mIn, sf), osg::Vec4(1.0, 1.0, 0.0, 1.0));
  sqs::assignDepth(sfg, 0.01, 1.01);
  boundaryViz->addChild(sfg);
  
  osg::Geometry *seg = sqs::createLines(sqs::createArray(mIn, se), osg::Vec4(0.0, 1.0, 1.0, 1.0));
  sqs::assignWidth(seg, 5.0);
  sqs::assignDepth(seg, 0.009, 1.009);
  boundaryViz->addChild(seg);
  
  osg::Geometry *svg = sqs::createPoints(sqs::createArray(mIn, baseVertices), osg::Vec4(1.0, 1.0, 1.0, 1.0));
  sqs::assignDepth(svg, 0.008, 1.008);
  sqs::assignPointSize(svg, 10);
  boundaryViz->addChild(svg);
  
  return boundaryViz.release();
}

osg::Group* sqs::generateObtuse(const Mesh &mIn, const Faces &fsIn)
{
  osg::ref_ptr<osg::Group> fsInViz = new osg::Group();
  osg::Geometry *fsInfg = sqs::createTriangles(sqs::createArray(mIn, fsIn), osg::Vec4(1.0, 0.0, 0.0, 0.3));
  sqs::assignDepth(fsInfg, 0.008, 1.008);
  fsInViz->addChild(fsInfg);
  
  Vertices vs;
  for (const auto &f : fsIn)
  {
    for (const auto &v : CGAL::vertices_around_face(mIn.halfedge(f), mIn))
      vs.push_back(v);
  }
  osg::Geometry *fsInpg = sqs::createPoints(sqs::createArray(mIn, vs), osg::Vec4(0.0, 0.0, 1.0, 1.0));
  sqs::assignDepth(fsInpg, 0.007, 1.007);
  sqs::assignPointSize(fsInpg, 10);
  fsInViz->addChild(fsInpg);
  
  return fsInViz.release();
}

Face sqs::copyFace(const Mesh &sourceIn, const Face &faceIn, Mesh &target)
{
  Vertices newVertices;
  for(Vertex vd : vertices_around_face(sourceIn.halfedge(faceIn), sourceIn))
  {
    Point p = sourceIn.point(vd);
    newVertices.push_back(target.add_vertex(p));
  }
  assert(newVertices.size() == 3);
  
  Face newFace = target.add_face
  (
    newVertices.at(0),
    newVertices.at(1),
    newVertices.at(2)
  );
  
  return newFace;
}

Edge sqs::copyEdge(const Mesh &sourceIn, const Edge &edgeIn, Mesh &target)
{
  Point p1 = sourceIn.point(sourceIn.vertex(edgeIn, 0));
  Point p2 = sourceIn.point(sourceIn.vertex(edgeIn, 1));
  return target.edge(target.add_edge(target.add_vertex(p1), target.add_vertex(p2)));
}

HalfEdge sqs::copyHalfEdge(const Mesh &sourceIn, const HalfEdge &halfEdgeIn, Mesh &target)
{
  Point p1 = sourceIn.point(sourceIn.source(halfEdgeIn));
  Point p2 = sourceIn.point(sourceIn.target(halfEdgeIn));
  return target.add_edge(target.add_vertex(p1), target.add_vertex(p2));
}

//output doesn't include face passed in.
Faces sqs::adjacentFaces(const Mesh& mIn, const Face &fIn)
{
  assert(mIn.null_face() != fIn);
  Faces out;
  for (const auto &hEdge : CGAL::halfedges_around_face(mIn.halfedge(fIn), mIn))
  {
    for (const auto &face : CGAL::faces_around_target(hEdge, mIn))
    {
      if (mIn.null_face() == face)
        continue;
      if (fIn == face)
        continue;
      out.push_back(face);
    }
  }
  
  uniquefy(out);
  
  return out;
}

Faces sqs::adjacentFaces(const Mesh& mIn, const Vertex &vIn)
{
  Faces out;

  for (const auto &face : CGAL::faces_around_target(mIn.halfedge(vIn), mIn))
  {
    if (mIn.null_face() == face)
      continue;
    out.push_back(face);
  }
  
  uniquefy(out);
  
  return out;
}

Edges sqs::adjacentEdges(const Mesh &mIn, const Vertex &vIn)
{
  Edges out;
  for (const auto &he : CGAL::halfedges_around_target(vIn, mIn))
    out.push_back(mIn.edge(he));
  uniquefy(out);
  return out;
}

Vertices sqs::adjacentVertices(const Mesh &mIn, const Vertex &vIn)
{
  Vertices out;
  
  for (const auto &v : CGAL::vertices_around_target(mIn.halfedge(vIn), mIn))
    out.push_back(v);
  
  uniquefy(out); //so they are ordered.
  
  return out;
}

std::tuple<Edges, Edges, Edges> sqs::classifyEdges(const Mesh &mIn, const Faces &fsIn)
{
  Edges regionEdges; //output
  Edges boundaryEdges; //output
  Edges sharedBoundaryEdges; //output
  
  Edges tEdges; //temp edges.
  for (const auto &f : fsIn)
  {
    for (const auto &he : CGAL::halfedges_around_face(mIn.halfedge(f), mIn))
      tEdges.push_back(mIn.edge(he));
  }
  sqs::uniquefy(tEdges);
  for (const auto &e : tEdges)
  {
    HalfEdge he = mIn.halfedge(e);
    HalfEdge ohe = mIn.opposite(he);
    Face f = mIn.face(he);
    Face of = mIn.face(ohe);
    if (mIn.is_border(e))
    {
      sharedBoundaryEdges.push_back(e);
      continue;
    }
    if
    (
      (std::binary_search(fsIn.begin(), fsIn.end(), f))
      && (std::binary_search(fsIn.begin(), fsIn.end(), of))
    )
    {
      //any edge within the region faces that has 2 region faces is a region edge.
      regionEdges.push_back(e);
      continue;
    }
    //anything else should be a boundary edge.
    boundaryEdges.push_back(e);
  }
  sqs::uniquefy(regionEdges);
  sqs::uniquefy(boundaryEdges);
  sqs::uniquefy(sharedBoundaryEdges);
  
  return std::make_tuple(regionEdges, boundaryEdges, sharedBoundaryEdges);
}

Edges sqs::getBoundaryEdges(const Mesh &mIn)
{
  Edges out;
  for (const auto &e : mIn.edges())
  {
    if (mIn.is_border(e))
      out.push_back(e);
  }
  sqs::uniquefy(out);
  return out;
}

double sqs::getFaceArea(const Mesh &mIn, const Face &fIn)
{
  std::vector<Point> ps;
  for (const auto &he : CGAL::halfedges_around_face(mIn.halfedge(fIn), mIn))
    ps.push_back(mIn.point(mIn.source(he)));
  assert(ps.size() == 3);
  return Kernel::Compute_area_3()(ps[0], ps[1], ps[2]);
}

double sqs::getMeshFaceArea(const Mesh &mIn)
{
  double out = 0.0;
  for (const auto &f : mIn.faces())
    out += getFaceArea(mIn, f);
  
  return out;
}

double sqs::getEdgeLength(const Mesh &mIn, const Edge &eIn)
{
  std::vector<Point> ps;
  ps.push_back(mIn.point(mIn.vertex(eIn, 0)));
  ps.push_back(mIn.point(mIn.vertex(eIn, 1)));
  
  return std::sqrt(Kernel::Compute_squared_distance_3()(ps[0], ps[1]));
}

double sqs::getMeshEdgeLength(const Mesh &mIn)
{
  double out = 0.0;
  for (const auto &e : mIn.edges())
    out += getEdgeLength(mIn, e);
  
  return out;
}

Faces sqs::obtuse(const Mesh &mIn, double a) //angle in degrees
{
  //assumes orientation is taken care of.
  auto getAngle = [&](const HalfEdge &he1, const HalfEdge &he2) -> double
  {
    osg::Vec3d v1 = sqs::toOsg(mIn.point(mIn.target(he1))) - sqs::toOsg(mIn.point(mIn.source(he1)));
    osg::Vec3d v2 = sqs::toOsg(mIn.point(mIn.target(he2))) - sqs::toOsg(mIn.point(mIn.source(he2)));
    
    return std::acos(v1 * v2 / v1.length() / v2.length());
  };
  
  double ar = osg::PI * a / 180.0;
  
  Faces out;
  for (const auto &f : mIn.faces())
  {
    HalfEdges hes;
    for (const auto &he : CGAL::halfedges_around_face(mIn.halfedge(f), mIn))
      hes.push_back(he);
    if ((getAngle(mIn.opposite(hes[0]), hes[1])) > ar)
      out.push_back(f);
    else if ((getAngle(mIn.opposite(hes[1]), hes[2])) > ar)
      out.push_back(f);
    else if ((getAngle(mIn.opposite(hes[2]), hes[0])) > ar)
      out.push_back(f);
  }
  
  return out;
}

std::vector<HalfEdges> sqs::getBoundaries(const Mesh &mIn)
{
  HalfEdges abhes; //all border half edges
  for (const auto &he : mIn.halfedges())
  {
    if (mIn.is_border(he))
      abhes.push_back(he);
  }
  sqs::uniquefy(abhes);
  
  std::vector<HalfEdges> bheso; //border half edges out
  while(!abhes.empty())
  {
    HalfEdges cb; //current border.
    cb.push_back(abhes.back());
    abhes.pop_back();
    for (auto it = abhes.begin(); it != abhes.end();)
    {
      if (mIn.target(cb.back()) == mIn.source(*it))
      {
        cb.push_back(*it);
        abhes.erase(it);
        it = abhes.begin();
      }
      else
       ++it;
      
      if (mIn.source(cb.front()) == mIn.target(cb.back()))
      {
        break;
      }
    }
    bheso.push_back(cb);
  }
  
  std::vector<HalfEdges>::iterator lrit = bheso.end(); //largest radius iterator
  double largestRadius = -1.0;
  for (auto it = bheso.begin(); it != bheso.end(); ++it)
  {
    double cr = sqs::getBSphereRadius(mIn, *it);
    if (cr > largestRadius)
    {
      largestRadius = cr;
      lrit = it;
    }
  }
  if (lrit == bheso.end() || largestRadius <= 0.0)
    throw std::runtime_error("no largest boundary");
  
  HalfEdges temp = *lrit;
  bheso.erase(lrit);
  bheso.push_back(temp);
  
  return bheso;
}

void sqs::getBSphere(const Mesh &mIn, BSphere& bInOut)
{
  for (const auto &v : mIn.vertices())
    bInOut.insert(mIn.point(v));
}

double sqs::getBSphereRadius(const Mesh &mIn, const HalfEdges &hesIn)
{
  BSphere bs; //bounding sphere.
  for (const auto &he : hesIn)
    bs.insert(mIn.point(mIn.source(he)));
  return bs.radius();
}
