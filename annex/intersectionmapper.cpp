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

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graphviz.hpp>

#include <BOPAlgo_Builder.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BOPDS_DS.hxx>
#include <TopoDS.hxx>

#include <feature/base.h>
#include <feature/shapehistory.h>
#include <feature/updatepayload.h>
#include <annex/seershape.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <annex/intersectionmapper.h>


using boost::uuids::uuid;

namespace ann
{
  namespace Node
  {
    enum class Type
    {
      None = 0,
      Old,
      New
    };
  }
  
  /*! associating an id to a position in parametric space of a face.
   * should the center update with the feature update? probably.
   * now thinking 'not'. we don't want ids moving around in parametric space.
   * this makes me question the alive concept...no I think i need it or
   * changes in size will cause id to change.
   */
  struct FaceNode
  {
    FaceNode(){}; //needed for graph. don't use.
    FaceNode(const TopoDS_Face&);
    double weight(const FaceNode&);
    void copyGeometry(const FaceNode&);
    void graphViz(std::ostream&) const;
    TopoDS_Face face; //!< face for new types, null for old types.
    gp_Vec2d center; //!< center of outerwire in parametric space of surface.
    boost::uuids::uuid faceId; //!< face id associated to center. only for 'old'
    boost::uuids::uuid wireId; //!< outer wire id associated to face. only for 'old'
    bool alive = false; //!< whether the id is currently used. between updates. only for 'old'
    bool used = false; //!< whether the id has been used during this update. for both 'new' and 'old'
    Node::Type type = Node::Type::None; //!< lets us know if this vertex is old or new.
  };
  
  /*! consistent identification of face splits.
   * should the history be updated when the feature updates? probably not.
   */
  struct FaceSplit
  {
    ftr::ShapeHistory faceHistory; //history upon construction.
    bool isMatch(const boost::uuids::uuid&) const; //!< does split origin match shapeHistory.
    
    /* run at the begginning and ending of feature for all face split objects. don't run before
     * each 'match'. the idea is that multiple faces might match and we want to carry
     * state of center ids from one to the next
     */
    void start();
    void finish();
    
    void match(const occt::FaceVector&); //!< cids is updated to reflect match.
    std::vector<FaceNode> getResults() const; //!< get all alive and used
    std::vector<FaceNode> nodes; //!< only set this in serialIn
  };
  
  struct EdgeNode
  {
    EdgeNode(){}; //needed for graph. don't use.
    EdgeNode(const TopoDS_Edge&);
    double weight(const EdgeNode&);
    void copyGeometry(const EdgeNode&);
    void graphViz(std::ostream&) const;
    TopoDS_Edge edge; //!< edge for new types, null for old types.
    double center; //!< center parametric range.
    boost::uuids::uuid edgeId; //!< edge id associated to center. only for 'old'
    bool alive = false; //!< whether the id is currently used. between updates. only for 'old'
    bool used = false; //!< whether the id has been used during this update. for both 'new' and 'old'
    Node::Type type = Node::Type::None; //!< lets us know if this vertex is old or new.
  };
  
  struct IntersectionNode
  {
    IntersectionNode(){}; //needed for graph. don't use.
    IntersectionNode(const TopoDS_Edge&, const gp_Vec2d&);
    double weight(const IntersectionNode&);
    void copyGeometry(const IntersectionNode&);
    void graphViz(std::ostream&) const;
    TopoDS_Edge edge; //!< edge for new types, null for old types.
    gp_Vec2d center; //!< center of bounding box of edge in face's parametric space.
    boost::uuids::uuid edgeId; //!< edge id associated to center. only for 'old'
    bool alive = false; //!< whether the id is currently used. between updates. only for 'old'
    bool used = false; //!< whether the id has been used during this update. for both 'new' and 'old'
    Node::Type type = Node::Type::None; //!< lets us know if this vertex is old or new.
  };
  
  struct EdgeSplit
  {
    ftr::ShapeHistory edgeHistory;
    
    bool isMatch(const boost::uuids::uuid&) const; //!< order doesn't matter
    
    // see facesplit on why we need these.
    void start();
    void finish();
    
    void match(const occt::EdgeVector&);
    std::vector<EdgeNode> getResults();
    std::vector<EdgeNode> nodes; //!< only set this in serialIn
  };
  
  /*! @brief Maps the intersection of 2 faces to 1 or more edges
   * 
   * this structure would work for an edge split, but doesn't
   * work for an edge intersection. See shapeTracking.svg, there is a small picture
   * of a subtraction between 2 cylinders. We have parallel lines that don't share
   * the same parametric space. That is my flaw, this was set up to where all edges to match
   * would share the same space. That will work for an edge split where they all come from the
   * same parent shape, but not for an intersection edge. We will have to put intersection edges
   * into a face or faces parametric space and then do the solve. Significant change. The question is
   * do we just use the first face as basis space for distance analysis? Choose the simplest
   * geometry type upon creation? always use the simplest and possible change between faces?
   */
  struct EdgeIntersection
  {
    ftr::ShapeHistory faceHistory1;
    ftr::ShapeHistory faceHistory2;
    
    bool isMatch(const boost::uuids::uuid&, const boost::uuids::uuid&) const; //!< order doesn't matter
    
    // see facesplit on why we need these.
    void start();
    void finish();
    
    void match(const std::vector<std::pair<TopoDS_Edge, gp_Vec2d>>&);
    std::vector<IntersectionNode> getResults();
    std::vector<IntersectionNode> nodes; //!< only set this in serialIn
  };
  
  /*! @brief maps an id to output geometry that has more than 1 parent
   * and they all coincide.
   *
   *this used for both faces and edges.
   */ 
  struct SameDomain
  {
    boost::uuids::uuid id = gu::createRandomId();
    bool used = false;
    std::vector<ftr::ShapeHistory> histories;
    bool isMatch(const std::vector<boost::uuids::uuid> &idsIn) const
    {
      /* I am concerned with how the shape history matching
       * works. Each history is completely traced to origin,
       * never concerned with depth. I am concerned with a
       * 'diamond' pattern and one search going too deep
       * and finding a match and cancelling out a potential
       * match. This isn't the only time I have come
       * across this concern.
       * 
       */
      
      std::vector<ftr::ShapeHistory> hsc = histories; //histories copy
      std::vector<uuid> idsc = idsIn; //ids copy
      for (auto idit = idsc.begin(); idit != idsc.end();) //id iterator
      {
        bool aMatch = false;
        for (auto hit = hsc.begin(); hit != hsc.end();)
        {
          if (hit->hasShape(*idit))
          {
            aMatch = true;
            hsc.erase(hit);
            break;
          }
          else
            ++hit;
        }
        if (aMatch)
          idit = idsc.erase(idit);
        else
          ++idit;
      }
      
      if (hsc.empty() && idsc.empty())
        return true;
      return false;
    }
  };
  
  struct EdgeProperty
  {
    bool traversed = false;
    bool eliminated = false;
  };

  typedef boost::adjacency_list
  <
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    FaceNode,
    boost::property<boost::edge_weight_t, double, EdgeProperty >
  > FaceGraph;

  typedef boost::adjacency_list
  <
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    EdgeNode,
    boost::property<boost::edge_weight_t, double, EdgeProperty >
  > EdgeGraph;

  typedef boost::adjacency_list
  <
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    IntersectionNode,
    boost::property<boost::edge_weight_t, double, EdgeProperty >
  > IntersectionGraph;


  template <class GraphEW>
  class Edge_writer {
  public:
    Edge_writer(const GraphEW &graphEWIn) : graphEW(graphEWIn) {}
    template <class EdgeW>
    void operator()(std::ostream& out, const EdgeW& edgeW) const
    {
      out << "[label=\""
        << std::to_string(boost::get(boost::edge_weight, graphEW)[edgeW]) << "\\n"
        << "\"]";
      if (graphEW[edgeW].traversed)
        out << "[penwidth=3.0][color=\"blueviolet\"]";
    }
  private:
    const GraphEW &graphEW;
  };

  template <class GraphVW>
  class Vertex_writer {
  public:
    Vertex_writer(const GraphVW &graphVWIn) : graphVW(graphVWIn) {}
    template <class VertexW>
    void operator()(std::ostream& out, const VertexW& vertexW) const
    {
      graphVW[vertexW].graphViz(out);
    }
  private:
    const GraphVW &graphVW;
  };
  
  template <typename T>
  struct Filter1
  {
    Filter1() : graph(nullptr) { }
    Filter1(const T &graphIn) : graph(&graphIn) { }
    template <typename V>
    bool operator()(const V& vertexIn) const
    {
      if(!graph)
        return false;
      
      const T &g = *graph; //just for syntax.
      if (g[vertexIn].type == Node::Type::Old)
        return g[vertexIn].alive && (!g[vertexIn].used);
      else if (g[vertexIn].type == Node::Type::New)
        return !g[vertexIn].used;
      else
        assert(0); //Node::Type is invalid
      
      return false;
    }
    const T *graph;
  };

  template <typename T>
  struct Filter2
  {
    Filter2() : graph(nullptr) { }
    Filter2(const T &graphIn) : graph(&graphIn) { }
    template <typename V>
    bool operator()(const V& vertexIn) const
    {
      if(!graph)
        return false;
      
      const T &g = *graph; //just for syntax.
      if (g[vertexIn].type == Node::Type::Old)
        return (!g[vertexIn].alive) && (!g[vertexIn].used);
      else if (g[vertexIn].type == Node::Type::New)
        return !g[vertexIn].used;
      else
        assert(0); //Node::Type is invalid
      
      return false;
    }
    const T *graph;
  };
  
  struct IntersectionMapper::Data
  {
    std::vector<EdgeIntersection> edgeIntersections;
    std::vector<FaceSplit> faceSplits;
    std::vector<EdgeSplit> edgeSplits;
    std::vector<SameDomain> sameDomains;
  };
}


using namespace ann;


template <class G>
static void writeGraph(const G &g, const std::string &path)
{
  std::ofstream file(path.c_str());
  boost::write_graphviz(file, g, Vertex_writer<G>(g),
                        Edge_writer<G>(g));
}

//connects each 'new' node to each 'old' node.
template <typename T>
static void connect(T &graph)
{
  typedef typename boost::graph_traits<T>::vertex_descriptor V;
  typedef typename boost::graph_traits<T>::edge_descriptor E;
  
  std::vector<V> ovs; //old vertices
  std::vector<V> nvs; //new vertices
  for (auto its = boost::vertices(graph); its.first != its.second; its.first++)
  {
    if (graph[*its.first].type == Node::Type::Old)
      ovs.push_back(*its.first);
    if (graph[*its.first].type == Node::Type::New)
      nvs.push_back(*its.first);
    assert(graph[*its.first].type != Node::Type::None);
  }
  
  for (const auto &nv : nvs)
  {
    for (const auto &ov : ovs)
    {
      E e;
      bool dummy;
      double weight = graph[nv].weight(graph[ov]);
      std::tie(e, dummy) = boost::add_edge(nv, ov, weight, graph);
      assert(dummy);
    }
  }
}

template <typename T>
static void solve(T graph)
{
  typedef typename boost::graph_traits<T>::vertex_descriptor V;
  typedef typename boost::graph_traits<T>::edge_descriptor E;
  
  
  /* maybe we should calculate a factor for each edge removal and 
   * remove the lowest valued edge. the factor could be: the edge itself
   * would be positive and the siblings that need to be removed could
   * be negative. Then add together to get factor.
   */
  auto findMin = [&]() -> std::pair<E, bool>
  {
    E out;
    double weight = std::numeric_limits<double>::max();
    for (auto its = boost::edges(graph); its.first != its.second; its.first++)
    {
      if (graph[*its.first].traversed)
        continue;
      if (graph[*its.first].eliminated)
        continue;
      double tw = boost::get(boost::edge_weight, graph)[*its.first];
      if (tw < weight)
      {
        weight = tw;
        out = *its.first;
      }
    }
    
    return std::make_pair(out, (weight != std::numeric_limits<double>::max()));
  };
  
  for (auto its = boost::edges(graph); its.first != its.second; its.first++)
  {
    graph[*its.first].traversed = false;
    graph[*its.first].eliminated = false;
  }
  E edge;
  bool results;
  std::tie(edge, results) = findMin();
  while (results)
  {
    graph[edge].traversed = true;
    //set all other edges for this and the connected to eliminated.
    V source = boost::source(edge, graph);
    V target = boost::target(edge, graph);
    for (auto its = boost::out_edges(source, graph); its.first != its.second; its.first++)
    {
      if (*its.first != edge)
        graph[*its.first].eliminated = true;
    }
    for (auto its = boost::out_edges(target, graph); its.first != its.second; its.first++)
    {
      if (*its.first != edge)
        graph[*its.first].eliminated = true;
    }
    
    std::tie(edge, results) = findMin();
  }
  
  for (auto its = boost::edges(graph); its.first != its.second; its.first++)
  {
    if (!graph[*its.first].traversed)
      continue;
    
    V o = boost::target(*its.first, graph);
    V n = boost::source(*its.first, graph);

    if (graph[o].type != Node::Type::Old || graph[n].type != Node::Type::New)
      std::swap(o, n);
    assert(graph[o].type == Node::Type::Old);
    assert(graph[n].type == Node::Type::New);
    
    graph[o].copyGeometry(graph[n]);
    graph[o].used = true;
    graph[o].alive = true; //second filters on dead ones, so make it alive
    graph[n].used = true;
  }
}

FaceNode::FaceNode(const TopoDS_Face& fIn)
{
  assert(!fIn.IsNull());
  
  double umin, umax, vmin, vmax;
  BRepTools::UVBounds(fIn, umin, umax, vmin, vmax);
  
  gp_Vec2d minc(umin, vmin);
  gp_Vec2d maxc(umax, vmax);
  center = minc + ((maxc - minc) * 0.5);
  
  type = Node::Type::New;
  face = fIn;
  faceId = gu::createRandomId();
  wireId = gu::createRandomId();
}

double FaceNode::weight(const FaceNode &other)
{
  return (center - other.center).Magnitude();
}

void FaceNode::copyGeometry(const FaceNode &other)
{
  face = other.face;
}

void FaceNode::graphViz(std::ostream &s) const
{
  const static std::vector<std::string> typeStrings = 
  {
    "None",
    "Old",
    "New"
  };
    
  s << 
    "[label=\"" <<
    "faceId: " << gu::idToString(faceId) << "\\n" <<
    "wireId: " << gu::idToString(wireId) << "\\n" <<
    "shape hash: " << ((face.IsNull()) ? ("null") : (std::to_string(gu::getShapeHash(face)))) << "\\n" <<
    "type: " << typeStrings.at(static_cast<std::size_t>(type)) << "\\n" <<
    ((alive) ? "alive" : "dead") << "\\n" <<
    ((used) ? "used" : "NOT used") << "\\n" <<
    "\"]";
}

bool FaceSplit::isMatch(const uuid &idIn) const
{
  return faceHistory.hasShape(idIn);
}

void FaceSplit::start()
{
  for (auto &cid : nodes)
    cid.used = false;
}

void FaceSplit::finish()
{
  for (auto &cid : nodes)
  {
    //there shouldn't be any new ones
    assert (cid.type == Node::Type::Old);
    if (cid.used)
      cid.alive = true;
    else
      cid.alive = false;
    
    //let go of shape.
    cid.face = TopoDS_Face();
  }
}

//set all to unused in caller scope.
void FaceSplit::match(const occt::FaceVector &fsIn)
{
  FaceGraph graph;
  for (const auto &cid : nodes)
    boost::add_vertex(cid, graph);
  for (const auto &f : fsIn)
    boost::add_vertex(FaceNode(f), graph);
    
  connect<FaceGraph>(graph);
//   writeGraph<FaceGraph>(graph, "/home/tanderson/temp/postConnect.dot");
  
  Filter1<FaceGraph> filter1(graph);
  typedef boost::filtered_graph<FaceGraph, boost::keep_all, Filter1<FaceGraph> > Filtered1GraphType;
  Filtered1GraphType filtered1Graph(graph, boost::keep_all(), filter1);
//   writeGraph<Filtered1GraphType>(filtered1Graph, "/home/tanderson/temp/postFilter1.dot");
  solve<Filtered1GraphType>(filtered1Graph);
//   writeGraph<FaceGraph>(graph, "/home/tanderson/temp/postSolve1.dot");
  
  Filter2<FaceGraph> filter2(graph);
  typedef boost::filtered_graph<FaceGraph, boost::keep_all, Filter2<FaceGraph> > Filtered2GraphType;
  Filtered2GraphType filtered2Graph(graph, boost::keep_all(), filter2);
//   writeGraph<Filtered2GraphType>(filtered2Graph, "/home/tanderson/temp/postFilter2.dot");
  solve<Filtered2GraphType>(filtered2Graph);
//   writeGraph<FaceGraph>(graph, "/home/tanderson/temp/postSolve2.dot");
  
  nodes.clear();
  for (auto its = boost::vertices(graph); its.first != its.second; its.first++)
  {
    if (graph[*its.first].type == Node::Type::Old)
      nodes.push_back(graph[*its.first]);
    
    if //there should never be new unused.
    (
      (graph[*its.first].type == Node::Type::New)
      && (!graph[*its.first].used)
    )
    {
      FaceNode fresh = graph[*its.first];
      fresh.alive = true;
      fresh.used = true;
      fresh.type = Node::Type::Old;
      nodes.push_back(fresh);
    }
  }
}

std::vector<FaceNode> FaceSplit::getResults() const
{
  std::vector<FaceNode> out;
  
  for (const auto &cid : nodes)
  {
    if (!cid.alive || !cid.used)
      continue;
    assert(cid.type == Node::Type::Old);
    assert(!cid.face.IsNull());
    assert(!cid.faceId.is_nil());
    assert(!cid.wireId.is_nil());
    out.push_back(cid);
  }
  
  return out;
}

EdgeNode::EdgeNode(const TopoDS_Edge& eIn)
{
  assert(!eIn.IsNull());
  
  double first, last;
  BRep_Tool::Curve(eIn, first, last);
  center = first + ((last - first) / 2.0);
  
  type = Node::Type::New;
  edge = eIn;
  edgeId = gu::createRandomId();
}

double EdgeNode::weight(const EdgeNode &other)
{
  return std::fabs(center - other.center);
}

void EdgeNode::copyGeometry(const EdgeNode &other)
{
  edge = other.edge;
}

void EdgeNode::graphViz(std::ostream &s) const
{
  const static std::vector<std::string> typeStrings = 
  {
    "None",
    "Old",
    "New"
  };
    
  s << 
    "[label=\"" <<
    "edgeId: " << gu::idToString(edgeId) << "\\n" <<
    "shape hash: " << ((edge.IsNull()) ? ("null") : (std::to_string(gu::getShapeHash(edge)))) << "\\n" <<
    "type: " << typeStrings.at(static_cast<std::size_t>(type)) << "\\n" <<
    ((alive) ? "alive" : "dead") << "\\n" <<
    ((used) ? "used" : "NOT used") << "\\n" <<
    "\"]";
}

IntersectionNode::IntersectionNode(const TopoDS_Edge &eIn, const gp_Vec2d &cIn)
{
  assert(!eIn.IsNull());
  
  center = cIn;
  type = Node::Type::New;
  edge = eIn;
  edgeId = gu::createRandomId();
}

double IntersectionNode::weight(const IntersectionNode &other)
{
  return (center - other.center).Magnitude();
}

void IntersectionNode::copyGeometry(const IntersectionNode &other)
{
  edge = other.edge;
}

void IntersectionNode::graphViz(std::ostream &s) const
{
  const static std::vector<std::string> typeStrings = 
  {
    "None",
    "Old",
    "New"
  };
    
  s << 
    "[label=\"" <<
    "edgeId: " << gu::idToString(edgeId) << "\\n" <<
    "shape hash: " << ((edge.IsNull()) ? ("null") : (std::to_string(gu::getShapeHash(edge)))) << "\\n" <<
    "type: " << typeStrings.at(static_cast<std::size_t>(type)) << "\\n" <<
    ((alive) ? "alive" : "dead") << "\\n" <<
    ((used) ? "used" : "NOT used") << "\\n" <<
    "\"]";
}

bool EdgeSplit::isMatch(const uuid& idIn) const
{
  return edgeHistory.hasShape(idIn);
}

void EdgeSplit::start()
{
  for (auto &node : nodes)
    node.used = false;
}

void EdgeSplit::finish()
{
  for (auto &node : nodes)
  {
    //there shouldn't be any new ones
    assert (node.type == Node::Type::Old);
    if (node.used)
      node.alive = true;
    else
      node.alive = false;
    
    //let go of shape.
    node.edge = TopoDS_Edge();
  }
}

void EdgeSplit::match(const occt::EdgeVector &esIn)
{
  EdgeGraph graph;
  for (const auto &node : nodes)
    boost::add_vertex(node, graph);
  for (const auto &e : esIn)
    boost::add_vertex(EdgeNode(e), graph);
    
  connect<EdgeGraph>(graph);
  
  Filter1<EdgeGraph> filter1(graph);
  typedef boost::filtered_graph<EdgeGraph, boost::keep_all, Filter1<EdgeGraph> > Filtered1GraphType;
  Filtered1GraphType filtered1Graph(graph, boost::keep_all(), filter1);
  solve<Filtered1GraphType>(filtered1Graph);
  
  Filter2<EdgeGraph> filter2(graph);
  typedef boost::filtered_graph<EdgeGraph, boost::keep_all, Filter2<EdgeGraph> > Filtered2GraphType;
  Filtered2GraphType filtered2Graph(graph, boost::keep_all(), filter2);
  solve<Filtered2GraphType>(filtered2Graph);
  
  nodes.clear();
  
  for (auto its = boost::vertices(graph); its.first != its.second; its.first++)
  {
    if (graph[*its.first].type == Node::Type::Old)
      nodes.push_back(graph[*its.first]);
    
    if //there should never be new unused.
    (
      (graph[*its.first].type == Node::Type::New)
      && (!graph[*its.first].used)
    )
    {
      EdgeNode fresh = graph[*its.first];
      fresh.alive = true;
      fresh.used = true;
      fresh.type = Node::Type::Old;
      nodes.push_back(fresh);
    }
  }
}

std::vector<EdgeNode> EdgeSplit::getResults()
{
  std::vector<EdgeNode> out;
  
  for (const auto &node : nodes)
  {
    if (!node.alive || !node.used)
      continue;
    assert(node.type == Node::Type::Old);
    assert(!node.edge.IsNull());
    assert(!node.edgeId.is_nil());
    out.push_back(node);
  }
  
  return out;
}

bool EdgeIntersection::isMatch(const uuid& id1, const uuid& id2) const
{
  if (faceHistory1.hasShape(id1))
    return faceHistory2.hasShape(id2);
  if (faceHistory2.hasShape(id1))
    return faceHistory1.hasShape(id2);
  
  return false;
}

void EdgeIntersection::start()
{
  for (auto &node : nodes)
    node.used = false;
}

void EdgeIntersection::finish()
{
  for (auto &node : nodes)
  {
    //there shouldn't be any new ones
    assert (node.type == Node::Type::Old);
    if (node.used)
      node.alive = true;
    else
      node.alive = false;
    
    //let go of shape.
    node.edge = TopoDS_Edge();
  }
}

void EdgeIntersection::match(const std::vector<std::pair<TopoDS_Edge, gp_Vec2d>> &pairs)
{
  IntersectionGraph graph;
  for (const auto &node : nodes)
    boost::add_vertex(node, graph);
  for (const auto &e : pairs)
    boost::add_vertex(IntersectionNode(e.first, e.second), graph);
    
  connect<IntersectionGraph>(graph);
//   writeGraph<IntersectionGraph>(graph, "/home/tanderson/temp/edgePostConnect.dot");
  
  Filter1<IntersectionGraph> filter1(graph);
  typedef boost::filtered_graph<IntersectionGraph, boost::keep_all, Filter1<IntersectionGraph> > Filtered1GraphType;
  Filtered1GraphType filtered1Graph(graph, boost::keep_all(), filter1);
//   writeGraph<Filtered1GraphType>(filtered1Graph, "/home/tanderson/temp/postFilter1.dot");
  solve<Filtered1GraphType>(filtered1Graph);
//   writeGraph<IntersectionGraph>(graph, "/home/tanderson/temp/edgePostSolve1.dot");
  
  Filter2<IntersectionGraph> filter2(graph);
  typedef boost::filtered_graph<IntersectionGraph, boost::keep_all, Filter2<IntersectionGraph> > Filtered2GraphType;
  Filtered2GraphType filtered2Graph(graph, boost::keep_all(), filter2);
//   writeGraph<Filtered2GraphType>(filtered2Graph, "/home/tanderson/temp/postFilter2.dot");
  solve<Filtered2GraphType>(filtered2Graph);
//   writeGraph<IntersectionGraph>(graph, "/home/tanderson/temp/edgePostSolve2.dot");
  
  nodes.clear();
  
  for (auto its = boost::vertices(graph); its.first != its.second; its.first++)
  {
    if (graph[*its.first].type == Node::Type::Old)
      nodes.push_back(graph[*its.first]);
    
    if //there should never be new unused.
    (
      (graph[*its.first].type == Node::Type::New)
      && (!graph[*its.first].used)
    )
    {
      IntersectionNode fresh = graph[*its.first];
      fresh.alive = true;
      fresh.used = true;
      fresh.type = Node::Type::Old;
      nodes.push_back(fresh);
    }
  }
}

std::vector<IntersectionNode> EdgeIntersection::getResults()
{
  std::vector<IntersectionNode> out;
  
  for (const auto &node : nodes)
  {
    if (!node.alive || !node.used)
      continue;
    assert(node.type == Node::Type::Old);
    assert(!node.edge.IsNull());
    assert(!node.edgeId.is_nil());
    out.push_back(node);
  }
  
  return out;
}

IntersectionMapper::IntersectionMapper() : Base(), data(new IntersectionMapper::Data) {}

IntersectionMapper::~IntersectionMapper() {}

prj::srl::IntersectionMapper IntersectionMapper::serialOut()
{
  prj::srl::EdgeIntersections eiOut;
  for (const auto &es : data->edgeIntersections)
  {
    prj::srl::IntersectionNodes ensOut;
    for (const auto &ens : es.nodes)
    {
      assert(ens.type == Node::Type::Old);
      ensOut.array().push_back
      (
        prj::srl::IntersectionNode
        (
          gu::idToString(ens.edgeId),
          ens.center.X(),
          ens.center.Y(),
          ens.alive
        )
      );
    }
    eiOut.array().push_back
    (
      prj::srl::EdgeIntersection
      (
        es.faceHistory1.serialOut(),
        es.faceHistory2.serialOut(),
        ensOut
      )
    );
  }
  
  prj::srl::FaceSplits fSplitsOut;
  for (const auto &fs : data->faceSplits)
  {
    prj::srl::FaceNodes fnsOut;
    for (const auto &fns : fs.nodes)
    {
      assert(fns.type == Node::Type::Old);
      fnsOut.array().push_back
      (
        prj::srl::FaceNode
        (
          gu::idToString(fns.faceId),
          gu::idToString(fns.wireId),
          fns.center.X(),
          fns.center.Y(),
          fns.alive
        )
      );
    }
    fSplitsOut.array().push_back
    (
      prj::srl::FaceSplit
      (
        fs.faceHistory.serialOut(),
        fnsOut
      )
    );
  }
  
  prj::srl::EdgeSplits eSplitsOut;
  for (const auto &ens : data->edgeSplits)
  {
    prj::srl::EdgeNodes ensOut;
    for (const auto &en : ens.nodes)
    {
      assert(en.type == Node::Type::Old);
      ensOut.array().push_back
      (
        prj::srl::EdgeNode
        (
          gu::idToString(en.edgeId),
          en.center,
          en.alive
        )
      );
    }
    eSplitsOut.array().push_back
    (
      prj::srl::EdgeSplit
      (
        ens.edgeHistory.serialOut(),
        ensOut
      )
    );
  }
  
  prj::srl::SameDomains sdso; //same domains out.
  for (const auto &sds : data->sameDomains)
  {
    prj::srl::ShapeHistories sdhs; //same domain histories
    for (const auto &h : sds.histories)
      sdhs.array().push_back(h.serialOut());
    sdso.array().push_back(prj::srl::SameDomain(gu::idToString(sds.id), sdhs));
  }
  
  
  return prj::srl::IntersectionMapper(eiOut, fSplitsOut, eSplitsOut, sdso);
}

void IntersectionMapper::serialIn(const prj::srl::IntersectionMapper &sIn)
{
  for (const auto &eisIn : sIn.edgeIntersections().array())
  {
    EdgeIntersection ei;
    ei.faceHistory1.serialIn(eisIn.faceHistory1());
    ei.faceHistory2.serialIn(eisIn.faceHistory2());
    for (const auto &enIn : eisIn.nodes().array())
    {
      IntersectionNode en;
      en.edgeId = gu::stringToId(enIn.edgeId());
      en.center = gp_Vec2d(enIn.centerX(), enIn.centerY());
      en.alive = enIn.alive();
      en.type = Node::Type::Old;
      ei.nodes.push_back(en);
    }
    data->edgeIntersections.push_back(ei);
  }
  
  for (const auto &fsIn : sIn.faceSplits().array())
  {
    FaceSplit fs;
    fs.faceHistory.serialIn(fsIn.faceHistory());
    for (const auto &fnIn : fsIn.nodes().array())
    {
      FaceNode fn;
      fn.faceId = gu::stringToId(fnIn.faceId());
      fn.wireId = gu::stringToId(fnIn.wireId());
      fn.center = gp_Vec2d(fnIn.centerX(), fnIn.centerY());
      fn.alive = fnIn.alive();
      fn.type = Node::Type::Old;
      fs.nodes.push_back(fn);
    }
    data->faceSplits.push_back(fs);
  }
  
  for (const auto &ess : sIn.edgeSplits().array())
  {
    EdgeSplit es;
    es.edgeHistory.serialIn(ess.edgeHistory());
    for (const auto &enIn : ess.nodes().array())
    {
      EdgeNode en;
      en.edgeId = gu::stringToId(enIn.edgeId());
      en.center = enIn.center();
      en.alive = enIn.alive();
      en.type = Node::Type::Old;
      es.nodes.push_back(en);
    }
    data->edgeSplits.push_back(es);
  }
  
  for (const auto &sdIn : sIn.sameDomains().array())
  {
    SameDomain sd;
    sd.id = gu::stringToId(sdIn.id());
    for (const auto &hIn : sdIn.histories().array())
    {
      ftr::ShapeHistory h;
      h.serialIn(hIn);
      sd.histories.push_back(h);
    }
    data->sameDomains.push_back(sd);
  }
}

void IntersectionMapper::go(const ftr::UpdatePayload &payloadIn, BOPAlgo_Builder &builder, SeerShape &sShape)
{
  /* any TopoDS_Edge or TopoDS_Face may change back and forth between being a split or not.
   * this means that for each edge and face we need to construct an EdgeSplit or FaceSplit
   * object so we can keep consistent naming between updates
   */
  
  std::vector<std::reference_wrapper<const ann::SeerShape>> seerShapes;
  for (auto its = payloadIn.updateMap.equal_range(ftr::InputType::target); its.first != its.second; ++its.first)
  {
    assert((*its.first).second->hasAnnex(ann::Type::SeerShape)); //no seershape for feature.
    seerShapes.push_back((*its.first).second->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    assert(!seerShapes.back().get().isNull());
  }
  for (auto its = payloadIn.updateMap.equal_range(ftr::InputType::tool); its.first != its.second; ++its.first)
  {
    assert((*its.first).second->hasAnnex(ann::Type::SeerShape)); //no seershape for feature.
    seerShapes.push_back((*its.first).second->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    assert(!seerShapes.back().get().isNull());
  }
  
  //cache builder info
  const BOPCol_DataMapOfShapeListOfShape &splits = builder.Splits();
  const BOPCol_DataMapOfShapeListOfShape &origins = builder.Origins();
  const BOPCol_DataMapOfShapeListOfShape &images = builder.Images();

  //start of face splits  
  for (auto &fs : data->faceSplits)
    fs.start();
  BOPCol_DataMapOfShapeListOfShape::Iterator splitIt(splits);
  for (; splitIt.More(); splitIt.Next())
  {
    //splits contain only face types.
    occt::FaceVector faces;
    for (const auto &shape : splitIt.Value())
    {
      assert(shape.ShapeType() == TopAbs_FACE);
      if (sShape.hasShapeIdRecord(shape))
        faces.push_back(TopoDS::Face(shape));
    }
    if (faces.empty())
      continue;
    
    const TopoDS_Shape &key = TopoDS::Face(splitIt.Key());
    uuid keyId = gu::createNilId();
    for (const auto &ss : seerShapes)
    {
      if (ss.get().hasShapeIdRecord(key))
      {
        keyId = ss.get().findShapeIdRecord(key).id;
        break;
      }
    }
    if (keyId.is_nil())
    {
      //same domain causes some funning split results and will trigger the following warning. FYI
      std::cout << "WARNING: can't find keyId for face split in IntersectionMapper::go" << std::endl;
      continue;
    }
    
    bool foundMatch = false;
    for (auto &fs : data->faceSplits)
    {
      if (!fs.isMatch(keyId))
        continue;
      foundMatch = true;
      fs.match(faces);
      for (const auto &cid : fs.getResults())
        sShape.updateShapeIdRecord(cid.face, cid.faceId);
      break;
    }
    if (!foundMatch)
    {
      FaceSplit nfs; //new face split
      nfs.faceHistory = payloadIn.shapeHistory.createDevolveHistory(keyId);
      nfs.match(faces);
      for (const auto &cid : nfs.getResults())
      {
        sShape.updateShapeIdRecord(cid.face, cid.faceId);
        sShape.insertEvolve(keyId, cid.faceId);
        const TopoDS_Shape &wire = BRepTools::OuterWire(cid.face);
        if(!wire.IsNull())
        {
          sShape.updateShapeIdRecord(wire, cid.wireId);
          sShape.insertEvolve(gu::createNilId(), cid.wireId);
        }
      }
      data->faceSplits.push_back(nfs);
    }
  }
  for (auto &fs : data->faceSplits)
    fs.finish();
  
  //start of edge splits.
  for (auto &es : data->edgeSplits)
    es.start();
  occt::ShapeVector shapes = sShape.getAllNilShapes();
  for (const auto &os : shapes) //output shape
  {
    if (os.ShapeType() != TopAbs_EDGE)
      continue; //only interested in edges here.
    // id might have been set by previous loop iteration
    if (!sShape.findShapeIdRecord(os).id.is_nil())
      continue;
    
    TopoDS_Shape sourceShape = os; //default non split 
    if (origins.IsBound(os))
    {
      const auto& oss = origins(os); //origin shapes.
      if (oss.Size() != 1)
        continue; //more than 1 means a same domain shape we will handle later.
      sourceShape = oss.First();
    }
    uuid sId = gu::createNilId(); //source id
    for (const auto &ss : seerShapes)
    {
      if (ss.get().hasShapeIdRecord(sourceShape))
      {
        sId = ss.get().findShapeIdRecord(sourceShape).id;
        break;
      }
    }
    if (sId.is_nil()) // probably an intersection edge
      continue;
    occt::EdgeVector edges;
    if (images.IsBound(sourceShape))
    {
      for (const auto &e : images(sourceShape))
      {
        if (sShape.hasShapeIdRecord(e))
          edges.push_back(TopoDS::Edge(e));
      }
    }
    else
    {
      edges.push_back(TopoDS::Edge(os));
    }
    bool foundMatch = false;
    for (auto &es : data->edgeSplits)
    {
      if (!es.isMatch(sId))
        continue;
      foundMatch = true;
      es.match(edges);
      for (const auto &en : es.getResults())
      {
        sShape.updateShapeIdRecord(en.edge, en.edgeId);
        sShape.insertEvolve(sId, en.edgeId);
      }
      break;
    }
    if (!foundMatch)
    {
      EdgeSplit nes; //new edge intersection
      nes.edgeHistory = payloadIn.shapeHistory.createDevolveHistory(sId);
      nes.match(edges);
      for (const auto &en : nes.getResults())
      {
        sShape.updateShapeIdRecord(en.edge, en.edgeId);
        sShape.insertEvolve(sId, en.edgeId);
      }
      data->edgeSplits.push_back(nes);
    }
  }
  
  for (auto &es : data->edgeSplits)
    es.finish();
  
  //start of edge intersections.
  for (auto &es : data->edgeIntersections)
    es.start();
  
  //bopalgo doesn't have edges created by face intersection in there
  //exposed data structures. So we get down and dirty with BOP_DS.
  const BOPDS_DS &bopDS = *(builder.PDS());
  
  struct TempEdgeIntersection
  {
    std::set<uuid> ids; //2 face ids.
    std::vector<std::pair<TopoDS_Edge, gp_Vec2d>> pairs;
  };
  typedef std::vector<TempEdgeIntersection> TempEdgeIntersections;
  TempEdgeIntersections teis; //temp edge intersections
  auto getTempEntry = [&](const uuid& faceId1, const uuid& faceId2) -> std::vector<std::pair<TopoDS_Edge, gp_Vec2d>>&
  {
    std::set<uuid> testSet = {faceId1, faceId2};
    assert(testSet.size() == 2);
    for (auto &tei : teis)
    {
      if (tei.ids == testSet)
        return tei.pairs;
    }
    TempEdgeIntersection freshEntry;
    freshEntry.ids = testSet;
    teis.push_back(freshEntry);
    return teis.back().pairs;
  };
  
  for (int index = 0; index < bopDS.NbShapes(); ++index)
  {
    //work with a valid edge.
    if (!bopDS.IsNewShape(index))
      continue;
    const TopoDS_Shape &shape = bopDS.Shape(index);
    if (shape.ShapeType() != TopAbs_EDGE)
      continue;
    if (!sShape.hasShapeIdRecord(shape))
      continue;
    if (!sShape.findShapeIdRecord(shape).id.is_nil())
      continue;
    
    std::vector<uuid> faceIds;
    occt::ShapeVector parentShapes = sShape.useGetParentsOfType(shape, TopAbs_FACE);
    gp_Vec2d tc; //tempCenter
    
    for (const auto &cShape : parentShapes)
    {
      for (const auto &sil : origins(cShape)) //sil = shape in list
      {
        for (const auto &ss : seerShapes)
        {
          if (ss.get().hasShapeIdRecord(sil))
          {
            faceIds.push_back(ss.get().findShapeIdRecord(sil).id);
            
            //get the current center
            double umin, umax, vmin, vmax;
            BRepTools::UVBounds(TopoDS::Face(cShape), TopoDS::Edge(shape), umin, umax, vmin, vmax);
            gp_Vec2d minc(umin, vmin);
            gp_Vec2d maxc(umax, vmax);
            gp_Vec2d center = minc + ((maxc - minc) * 0.5);
            
            //'average' in the new center to old.
            if (tc.Magnitude() != 0.0)
              tc = tc + ((center - tc) * 0.5);
            
            break;
          }
        }
      }
    }
    gu::uniquefy(faceIds);
    if (faceIds.size() != 2) //should we have a warning here?
      continue;
    auto &te = getTempEntry(faceIds.front(), faceIds.back());
    te.push_back(std::make_pair(TopoDS::Edge(shape), tc));
  }
  
  //loop through temp edge intersections and try to match up
  //with an existing edge intersections or create a new one.
  for (auto &tei : teis)
  {
    assert(tei.ids.size() == 2); //2 faces.
    bool foundMatch = false;
    for (auto &es : data->edgeIntersections)
    {
      if (es.isMatch(*(tei.ids.begin()), *(++tei.ids.begin())))
      {
        foundMatch = true;
        es.match(tei.pairs);
        for (const auto &en : es.getResults())
        {
          sShape.updateShapeIdRecord(en.edge, en.edgeId);
          sShape.insertEvolve(gu::createNilId(), en.edgeId); //intersection edges come from nothing.
        }
        break;
      }
    }
    if (!foundMatch)
    {
      EdgeIntersection nei; //new edge intersection
      nei.faceHistory1 = payloadIn.shapeHistory.createDevolveHistory(*(tei.ids.begin()));
      nei.faceHistory2 = payloadIn.shapeHistory.createDevolveHistory(*(++tei.ids.begin()));
      nei.match(tei.pairs);
      for (const auto &en : nei.getResults())
      {
        sShape.updateShapeIdRecord(en.edge, en.edgeId);
        sShape.insertEvolve(gu::createNilId(), en.edgeId); //intersection edges come from nothing.
      }
      data->edgeIntersections.push_back(nei);
    }
  }
  
  for (auto &es : data->edgeIntersections)
    es.finish();
  
  
  //start of same domain.
  for (auto &sd : data->sameDomains)
    sd.used = false;
  //the same domain data structure of the builder appears FUBAR. occt 7.2. using origins with multiples.
  for (const auto &os : sShape.getAllShapes()) //output shape
  {
    if (!origins.IsBound(os))
      continue;
    if (os.ShapeType() != TopAbs_EDGE && os.ShapeType() != TopAbs_FACE)
      continue;
    const auto &ogs = origins(os);
    if (ogs.Extent() < 2)
      continue;
    
    std::vector<uuid> oids; //origin ids.
    for (const auto &oShape : ogs)
    {
      for (const auto &ss : seerShapes)
      {
        if (ss.get().hasShapeIdRecord(oShape))
        {
          oids.push_back(ss.get().findShapeIdRecord(oShape).id);
          break;
        }
      }
    }
    
    if (oids.size() != static_cast<std::size_t>(ogs.Extent()))
    {
      std::cout << "WARNING: couldn't find all ids for shapes in: intersection mapper same domain" << std::endl;
      continue;
    }
    
    bool foundMatch = false;
    for (const auto &sd : data->sameDomains)
    {
      if (!sd.isMatch(oids))
        continue;
      foundMatch = true;
      sShape.updateShapeIdRecord(os, sd.id);
      for (const auto &oid : oids)
        sShape.insertEvolve(oid, sd.id);
      break;
    }
    if (!foundMatch)
    {
      SameDomain nsd; //new same domain
      for (const auto &oid : oids)
        nsd.histories.push_back(payloadIn.shapeHistory.createDevolveHistory(oid));
      data->sameDomains.push_back(nsd);
      sShape.updateShapeIdRecord(os, nsd.id);
      for (const auto &oid : oids)
        sShape.insertEvolve(oid, nsd.id);
    }
  }
}
