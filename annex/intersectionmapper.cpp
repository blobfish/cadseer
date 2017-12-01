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
    bool isMatch(const boost::uuids::uuid&); //!< does split origin match shapeHistory.
    
    /* run at the begginning and ending of feature for all face split objects. don't run before
     * each 'match'. the idea is that multiple faces might match and we want to carry
     * state of center ids from one to the next
     */
    void start();
    void finish();
    
    void match(const occt::FaceVector&); //!< cids is updated to reflect match.
    std::vector<FaceNode> getResults(); //!< get all alive and used
    std::vector<FaceNode> nodes; //!< only set this in serialIn
  };
  
  struct EdgeNode
  {
    EdgeNode(){}; //needed for graph. don't use.
    EdgeNode(const TopoDS_Edge&);
    double weight(const EdgeNode&);
    void copyGeometry(const EdgeNode&);
    void graphViz(std::ostream&) const;
    TopoDS_Edge edge; //!< face for new types, null for old types.
    double center; //!< center parametric range.
    boost::uuids::uuid edgeId; //!< edge id associated to center. only for 'old'
    bool alive = false; //!< whether the id is currently used. between updates. only for 'old'
    bool used = false; //!< whether the id has been used during this update. for both 'new' and 'old'
    Node::Type type = Node::Type::None; //!< lets us know if this vertex is old or new.
  };
  
  /* maps the intersection of 2 faces to 1 or more edges */
  struct EdgeSplit
  {
    ftr::ShapeHistory faceHistory1;
    ftr::ShapeHistory faceHistory2;
    
    bool isMatch(const boost::uuids::uuid&, const boost::uuids::uuid&); //!< order doesn't matter
    
    // see facesplit on why we need these.
    void start();
    void finish();
    
    void match(const occt::EdgeVector&);
    std::vector<EdgeNode> getResults();
    std::vector<EdgeNode> nodes; //!< only set this in serialIn
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
    std::vector<EdgeSplit> edgeSplits;
    std::vector<FaceSplit> faceSplits;
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

bool FaceSplit::isMatch(const uuid &idIn)
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

std::vector<FaceNode> FaceSplit::getResults()
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

bool EdgeSplit::isMatch(const uuid& id1, const uuid& id2)
{
  if (faceHistory1.hasShape(id1))
    return faceHistory2.hasShape(id2);
  if (faceHistory2.hasShape(id1))
    return faceHistory1.hasShape(id2);
  
  return false;
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
//   writeGraph<EdgeGraph>(graph, "/home/tanderson/temp/edgePostConnect.dot");
  
  Filter1<EdgeGraph> filter1(graph);
  typedef boost::filtered_graph<EdgeGraph, boost::keep_all, Filter1<EdgeGraph> > Filtered1GraphType;
  Filtered1GraphType filtered1Graph(graph, boost::keep_all(), filter1);
//   writeGraph<Filtered1GraphType>(filtered1Graph, "/home/tanderson/temp/postFilter1.dot");
  solve<Filtered1GraphType>(filtered1Graph);
//   writeGraph<EdgeGraph>(graph, "/home/tanderson/temp/edgePostSolve1.dot");
  
  Filter2<EdgeGraph> filter2(graph);
  typedef boost::filtered_graph<EdgeGraph, boost::keep_all, Filter2<EdgeGraph> > Filtered2GraphType;
  Filtered2GraphType filtered2Graph(graph, boost::keep_all(), filter2);
//   writeGraph<Filtered2GraphType>(filtered2Graph, "/home/tanderson/temp/postFilter2.dot");
  solve<Filtered2GraphType>(filtered2Graph);
//   writeGraph<EdgeGraph>(graph, "/home/tanderson/temp/edgePostSolve2.dot");
  
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




IntersectionMapper::IntersectionMapper() : Base(), data(new IntersectionMapper::Data) {}

IntersectionMapper::~IntersectionMapper() {}

prj::srl::IntersectionMapper IntersectionMapper::serialOut()
{
  prj::srl::EdgeSplits eSplitsOut;
  for (const auto es : data->edgeSplits)
  {
    prj::srl::EdgeNodes ensOut;
    for (const auto ens : es.nodes)
    {
      assert(ens.type == Node::Type::Old);
      ensOut.array().push_back
      (
        prj::srl::EdgeNode
        (
          gu::idToString(ens.edgeId),
          ens.center,
          ens.alive
        )
      );
    }
    eSplitsOut.array().push_back
    (
      prj::srl::EdgeSplit
      (
        es.faceHistory1.serialOut(),
        es.faceHistory2.serialOut(),
        ensOut
      )
    );
  }
  
  prj::srl::FaceSplits fSplitsOut;
  for (const auto fs : data->faceSplits)
  {
    prj::srl::FaceNodes fnsOut;
    for (const auto fns : fs.nodes)
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
  
  return prj::srl::IntersectionMapper(eSplitsOut, fSplitsOut);
}

void IntersectionMapper::serialIn(const prj::srl::IntersectionMapper &sIn)
{
  for (const auto esIn : sIn.edgeSplits().array())
  {
    EdgeSplit es;
    es.faceHistory1.serialIn(esIn.faceHistory1());
    es.faceHistory2.serialIn(esIn.faceHistory2());
    for (const auto enIn : esIn.nodes().array())
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
  
  for (const auto fsIn : sIn.faceSplits().array())
  {
    FaceSplit fs;
    fs.faceHistory.serialIn(fsIn.faceHistory());
    for (const auto fnIn : fsIn.nodes().array())
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
}

void IntersectionMapper::go(const ftr::UpdatePayload &payloadIn, BOPAlgo_Builder &builder, SeerShape &sShape)
{
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
  
  for (auto &fs : data->faceSplits)
    fs.start();
  const BOPCol_DataMapOfShapeListOfShape &splits = builder.Splits();
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
      std::cout << "WARNING: can't find keyId in IntersectionMapper::go" << std::endl;
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
  
  
  //start of edge splits/intersection.
  for (auto &es : data->edgeSplits)
    es.start();
  
  const BOPDS_DS &bopDS = *(builder.PDS());
  const BOPCol_DataMapOfShapeListOfShape &origins = builder.Origins();
  
  //occt splits don't include edges, so we have to work backwards from
  //origins to accumulate edge 'splits'.
  typedef std::pair<std::set<uuid>, occt::EdgeVector> TempEdgeSplit;
  typedef std::vector<TempEdgeSplit> TempEdgeSplits;
  TempEdgeSplits teSplits;
  auto getEdgeVector = [&](const uuid& faceId1, const uuid& faceId2) -> occt::EdgeVector&
  {
    std::set<uuid> testSet = {faceId1, faceId2};
    assert(testSet.size() == 2);
    for (auto &teSplit : teSplits)
    {
      if (teSplit.first == testSet)
        return teSplit.second;
    }
    teSplits.push_back(TempEdgeSplit(testSet, occt::EdgeVector()));
    return teSplits.back().second;
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
    
    for (const auto &cShape : parentShapes)
    {
      for (const auto &sil : origins(cShape)) //sil = shape in list
      {
        for (const auto &ss : seerShapes)
        {
          if (ss.get().hasShapeIdRecord(sil))
          {
            faceIds.push_back(ss.get().findShapeIdRecord(sil).id);
            break;
          }
        }
      }
    }
    gu::uniquefy(faceIds);
    if (faceIds.size() != 2) //should we have a warning here?
      continue;
    occt::EdgeVector &ev = getEdgeVector(faceIds.front(), faceIds.back());
    ev.push_back(TopoDS::Edge(shape));
  }
  
  //loop through temp splits and try to match up
  //with an existing edge split or create a new one.
  for (auto &teSplit : teSplits)
  {
    assert(teSplit.first.size() == 2); //2 faces.
    bool foundMatch = false;
    for (auto &es : data->edgeSplits)
    {
      if (es.isMatch(*(teSplit.first.begin()), *(++teSplit.first.begin())))
      {
        foundMatch = true;
        es.match(teSplit.second);
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
      EdgeSplit nes; //new edge split
      nes.faceHistory1 = payloadIn.shapeHistory.createDevolveHistory(*(teSplit.first.begin()));
      nes.faceHistory2 = payloadIn.shapeHistory.createDevolveHistory(*(++teSplit.first.begin()));
      nes.match(teSplit.second);
      for (const auto &en : nes.getResults())
      {
        sShape.updateShapeIdRecord(en.edge, en.edgeId);
        sShape.insertEvolve(gu::createNilId(), en.edgeId); //intersection edges come from nothing.
      }
      data->edgeSplits.push_back(nes);
    }
  }
  
  for (auto &es : data->edgeSplits)
    es.finish();
}
