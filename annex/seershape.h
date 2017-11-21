/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef ANN_SEERSHAPE_H
#define ANN_SEERSHAPE_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>

#include <TopoDS_Shape.hxx>

#include <globalutilities.h>
#include <tools/occtools.h>
#include <tools/idtools.h>
#include <annex/base.h>

namespace osg{class Vec3d;}

class BRepBuilderAPI_MakeShape;

namespace prj{namespace srl{class SeerShape;}}

static const std::vector<std::string> shapeStrings
({
     "Compound",
     "Compound_Solid",
     "Solid",
     "Shell",
     "Face",
     "Wire",
     "Edge",
     "Vertex",
     "Shape"
 });

namespace ftr{class ShapeHistory;}
namespace ann
{
  //mapping a set of ids to one id. this is for deriving an id from multiple parent shapes.
  typedef std::set<boost::uuids::uuid> IdSet;
  typedef std::map<IdSet, boost::uuids::uuid> DerivedContainer;
  
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS> Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
  typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
  typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;
  typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
  typedef boost::graph_traits<Graph>::adjacency_iterator VertexAdjacencyIterator;
  
  namespace BMI = boost::multi_index;
  namespace BID = boost::uuids;
  
  struct ShapeIdRecord
  {
    BID::uuid id;
    Vertex graphVertex;
    TopoDS_Shape shape;
    
    ShapeIdRecord() :
      id(gu::createNilId()),
      graphVertex(boost::graph_traits<Graph>::null_vertex()),
      shape(TopoDS_Shape())
      {}
    
    //@{
    //! for tags
    struct ById{};
    struct ByVertex{};
    struct ByShape{};
    //@}
  };
  
  struct ShapeIdKeyHash
  {
    std::size_t operator()(const TopoDS_Shape& shape)const
    {
      int hashOut;
      hashOut = shape.HashCode(std::numeric_limits<int>::max());
      return static_cast<std::size_t>(hashOut);
    }
  };
  
  struct ShapeIdShapeEquality
  {
    bool operator()(const TopoDS_Shape &shape1, const TopoDS_Shape &shape2) const
    {
      return shape1.IsSame(shape2);
    }
  };
  
  typedef boost::multi_index_container
  <
    ShapeIdRecord,
    BMI::indexed_by
    <
      BMI::ordered_non_unique
      <
        BMI::tag<ShapeIdRecord::ById>,
        BMI::member<ShapeIdRecord, BID::uuid, &ShapeIdRecord::id>
      >,
      BMI::ordered_non_unique
      <
        BMI::tag<ShapeIdRecord::ByVertex>,
        BMI::member<ShapeIdRecord, Vertex, &ShapeIdRecord::graphVertex>
      >,
      BMI::hashed_non_unique
      <
        BMI::tag<ShapeIdRecord::ByShape>,
        BMI::member<ShapeIdRecord, TopoDS_Shape, &ShapeIdRecord::shape>,
        ShapeIdKeyHash,
        ShapeIdShapeEquality
      >
    >
  > ShapeIdContainer;
  
  std::ostream& operator<<(std::ostream&, const ShapeIdRecord&);
  std::ostream& operator<<(std::ostream&, const ShapeIdContainer&);
  
  
  
  struct EvolveRecord
  {
    boost::uuids::uuid inId;
    boost::uuids::uuid outId;
    EvolveRecord() : inId(gu::createNilId()), outId(gu::createNilId()) {}
    EvolveRecord(const boost::uuids::uuid &inIdIn, const boost::uuids::uuid &outIdIn):
      inId(inIdIn), outId(outIdIn){}
    
    //@{
    //! used as tags.
    struct ByInId{};
    struct ByOutId{};
    struct ByInOutIds{};
    //@}
  };
  
  typedef boost::multi_index_container
  <
    EvolveRecord,
    BMI::indexed_by
    <
      BMI::ordered_non_unique
      <
        BMI::tag<EvolveRecord::ByInId>,
        BMI::member<EvolveRecord, boost::uuids::uuid, &EvolveRecord::inId>
      >,
      BMI::ordered_non_unique
      <
        BMI::tag<EvolveRecord::ByOutId>,
        BMI::member<EvolveRecord, boost::uuids::uuid, &EvolveRecord::outId>
      >,
      BMI::ordered_unique
      < 
        BMI::tag<EvolveRecord::ByInOutIds>,
        BMI::composite_key
        <
          EvolveRecord,
          BMI::member<EvolveRecord, boost::uuids::uuid, &EvolveRecord::inId>,
          BMI::member<EvolveRecord, boost::uuids::uuid, &EvolveRecord::outId>
        >
      >
    >
  > EvolveContainer;
  
  std::ostream& operator<<(std::ostream& os, const EvolveRecord& record);
  std::ostream& operator<<(std::ostream& os, const EvolveContainer& container);
  
  
  
  //! associate a string identifier to an id.
  struct FeatureTagRecord
  {
    boost::uuids::uuid id;
    std::string tag;
    
    FeatureTagRecord() : id(gu::createNilId()), tag() {}
    
    //@{
    //! used as tags.
    struct ById{};
    struct ByTag{};
    //@}
  };
  
  typedef boost::multi_index_container
  <
    FeatureTagRecord,
    BMI::indexed_by
    <
      BMI::ordered_unique
      <
        BMI::tag<FeatureTagRecord::ById>,
        BMI::member<FeatureTagRecord, boost::uuids::uuid, &FeatureTagRecord::id>
      >,
      BMI::ordered_unique
      <
        BMI::tag<FeatureTagRecord::ByTag>,
        BMI::member<FeatureTagRecord, std::string, &FeatureTagRecord::tag>
      >
    >
  > FeatureTagContainer;
  
  std::ostream& operator<<(std::ostream& os, const FeatureTagRecord& record);
  std::ostream& operator<<(std::ostream& os, const FeatureTagContainer& container);
  
  

  /*! @brief Extended wrapping around occt shape
  *
  * @shapeIdContainer is a 3 way link between the boost uuid shape id,
  * Graph vertex topology and the OCCT TopoDS_Shape. Any references to OCCT shapes
  * should use the ids and get the TopoDS_Shape out of this container. This container
  * is filled in by the @setOCCTShape function and then can be updated through member
  * functions. There is no direct access outside of class. One needs to remember
  * that the ids in the shapeIdContainer may be nil until the update completes. Searching
  * by ids in a container during update may lead to incorrect results.
  * 
  * @evolveContainer is for post update interrogation of shape changes. Joins and splits
  * will result in duplicate entries in either inId or outId. This container, like the
  * shapeIdContainer, will be completely regenerated every update. Any persistent information
  * must stored in other members. It is cleared in @setOCCTShape 
  * 
  * @featureTagContainer is linking a string identifier to a uuid. Once constructed this
  * container should be constant through out the life time of feature.
  * 
  * @derivedContainer is consistantly naming newly generated entities between feature
  * updates. This container doesn't get reset between updates.
  */ 
  struct SeerShape : public Base
  {
  public:
    SeerShape();
    virtual ~SeerShape() override;
    virtual Type getType(){return Type::SeerShape;}
    
    void setOCCTShape(const TopoDS_Shape& shapeIn); //!< resets container and graphs!
    
    const TopoDS_Shape& getRootOCCTShape() const;
    const TopoDS_Shape& getOCCTShape(const BID::uuid &) const;
    const boost::uuids::uuid& getRootShapeId() const;
    void setRootShapeId(const boost::uuids::uuid&);
    bool isNull() const; //meaning no TopoDS_shape has been established.
    
    //@{
    //! shapeId container related functions
    bool hasShapeIdRecord(const BID::uuid&) const;
    bool hasShapeIdRecord(const Vertex&) const;
    bool hasShapeIdRecord(const TopoDS_Shape&) const;
    const ShapeIdRecord& findShapeIdRecord(const BID::uuid&) const;
    const ShapeIdRecord& findShapeIdRecord(const Vertex&) const;
    const ShapeIdRecord& findShapeIdRecord(const TopoDS_Shape&) const;
    void updateShapeIdRecord(const BID::uuid&, const TopoDS_Shape&); //update shape by id.
    void updateShapeIdRecord(const BID::uuid&, const Vertex&); //update vertex by id.
    void updateShapeIdRecord(const TopoDS_Shape&, const BID::uuid&); //update id by shape.
    void updateShapeIdRecord(const TopoDS_Shape&, const Vertex&); //update vertex by shape.
    void updateShapeIdRecord(const Vertex&, const BID::uuid&); //update id by vertex. 
    void updateShapeIdRecord(const Vertex&, const TopoDS_Shape&); //update shape by vertex.
    std::vector<BID::uuid> getAllShapeIds() const; //all the ids in the container.
    occt::ShapeVector getAllShapes() const; //all the shapes in the container.
    occt::ShapeVector getAllNilShapes() const; //all the shapes in the container.
    //@}
    
    //@{
    //! evolve container related functions.
    bool hasEvolveRecordIn(const BID::uuid&) const;
    bool hasEvolveRecordOut(const BID::uuid&) const;
    bool hasEvolveRecord(const BID::uuid&, const BID::uuid&) const;
    std::vector<BID::uuid> evolve(const BID::uuid&) const; //!< forward evolution. in to out.
    std::vector<BID::uuid> devolve(const BID::uuid&) const; //!< reverse evolution. out to in.
    void insertEvolve(const BID::uuid&, const BID::uuid&); //!< add entry into evolve container
    void insertEvolve(const EvolveRecord&); //!< add entry into evolve container
    void fillInHistory(ftr::ShapeHistory &, const BID::uuid&) const;
    void replaceId(const BID::uuid &, const BID::uuid &, const ftr::ShapeHistory &);
    std::vector<BID::uuid> resolvePick(const ftr::ShapeHistory&) const;
    //@}
    
    //@{
    //! feature tag container related functions
    BID::uuid featureTagId(const std::string &tagIn);
    void insertFeatureTag(const FeatureTagRecord&);
    //@}
    
    //@{
    //! graph related functions
    std::vector<BID::uuid> useGetParentsOfType(const BID::uuid &, const TopAbs_ShapeEnum &) const;
    occt::ShapeVector useGetParentsOfType(const TopoDS_Shape &, const TopAbs_ShapeEnum &) const;
    std::vector<BID::uuid> useGetChildrenOfType(const BID::uuid &, const TopAbs_ShapeEnum &) const;
    std::vector<BID::uuid> useGetChildrenOfType(const TopoDS_Shape &, const TopAbs_ShapeEnum &) const;
    BID::uuid useGetWire(const BID::uuid &, const BID::uuid &) const;
    BID::uuid useGetClosestWire(const BID::uuid &faceIn, const osg::Vec3d &pointIn) const;
    std::vector<BID::uuid> useGetFacelessWires(const BID::uuid &edgeIn) const;
    bool useIsEdgeOfFace(const BID::uuid &edgeIn, const BID::uuid &faceIn) const;
    std::vector<osg::Vec3d> useGetEndPoints(const BID::uuid &) const;
    std::vector<osg::Vec3d> useGetMidPoint(const BID::uuid &) const;
    std::vector<osg::Vec3d> useGetCenterPoint(const BID::uuid &) const;
    std::vector<osg::Vec3d> useGetQuadrantPoints(const BID::uuid &) const;
    std::vector<osg::Vec3d> useGetNearestPoint(const BID::uuid &, const osg::Vec3d&) const;
    BID::uuid useGetStartVertex(const BID::uuid &) const;
    BID::uuid useGetEndVertex(const BID::uuid &) const;
    //@}
    
    //@{
    //! common shape mapping.
    
    /*! copy ids from source to target if the shapes match. based upon TopoDS_Shape::IsSame.
    * this will overwrite the id in target whether it is nil or not.
    */
    void shapeMatch(const SeerShape &);
  
  /*! Copy id from source to target based on unique types.
    * will only copy the id into target if each container only contains 1 shape of
    * the specified type and if the shapes id in target container is_nil
    */
    void uniqueTypeMatch(const SeerShape &);
    
    /*! Copy outer wire ids from source to target based on face id matching.
    */
    void outerWireMatch(const SeerShape &);
    
    /*! Copy ids from source to target using the make shape class as a guide*/
    void modifiedMatch(BRepBuilderAPI_MakeShape&, const SeerShape&);
    
    /*! when we have edges and vertices that can't be identified, we map parent shapes ids to
    * Shape the shapeIdContainer. Once a mapping is established, the derived container
    * will be checked. if the dervied container has mapping the shapeIdContainer will be
    * updated. Else new ids will be generated and the derived container will be updated.
    */
    void derivedMatch();
    
    /*! give some development feedback if container has nil entries.
    */
    void dumpNils(const std::string &);
    
    /*! give some development feedback if the container has duplicate entries for id
    */
    void dumpDuplicates(const std::string &);
    
    /*! nil ids will eventually cause a crash. So this help keep a running program, but
    * will also hide errors from development. Best to call dumpNils before this, so
    * there is a sign of failed mappings.
    */
    void ensureNoNils();
    
    /*! scans container generates new ids for duplicate. This is a fail safe to keep things
    * working while developing id mapping system. Shouldn't do anything and some day
    * should be excluded.
    */
    void ensureNoDuplicates();
    
    /*! a lot of occ routines when reporting generated, modified etc.., only deal with faces.
    * so here when an edge has no id but both faces do, we try to derived what the id of the old
    * edge is and assign it to the new edge. Have to be careful when 2 faces share more than 1 edge.
    * These 2 functions are not used anywhere.
    */
    void faceEdgeMatch(const SeerShape &);
    
    /*! same as above only using edges and vertices. */
    void edgeVertexMatch(const SeerShape &);
    //@}
    
    //@{
    //! debug related functions
    void dumpGraph(const std::string &) const;
    void dumpReverseGraph(const std::string &) const;
    void dumpShapeIdContainer(std::ostream &) const;
    void dumpEvolveContainer(std::ostream &) const;
    void dumpFeatureTagContainer(std::ostream &) const;
    //@}
    
    prj::srl::SeerShape serialOut(); //!<convert this into serializable object.
    void serialIn(const prj::srl::SeerShape &); //intialize this from serial object.
    
    /*! @brief Create a copy to work on.
     * 
     * calls BRepBuilderAPI_Copy to copy the Root OCCT shape
     * of this. Returned shape will have valid:
     * shapeIdContainer
     * evolveContainer
     * graph
     * all other data and related functions may not work.
     * 
     * this is useful, because sometimes occt algorithms
     * need a copy in order to function properly. This
     * creates an object that we can pass to those functions
     * and still map shapes back to the original through the
     * evolve container.
     */
    SeerShape createWorkCopy() const;
    
  private:
    BID::uuid rootShapeId;
    ShapeIdContainer shapeIdContainer;
    EvolveContainer evolveContainer;
    FeatureTagContainer featureTagContainer;
    DerivedContainer derivedContainer;
    Graph graph;
    Graph rGraph; //reversed graph.
    
    void updateGraphs();
  };
  
  class TypeCollectionVisitor : public boost::default_bfs_visitor
  {
  public:
    TypeCollectionVisitor(const TopAbs_ShapeEnum &shapeTypeIn, const SeerShape &seerShapeIn, std::vector<Vertex> &vertOut) :
      shapeType(shapeTypeIn), seerShape(seerShapeIn), graphVertices(vertOut){}
    template <typename VisitorVertex, typename VisitorGraph>
    void discover_vertex(VisitorVertex u, const VisitorGraph &g)
    {
      if (seerShape.findShapeIdRecord(u).shape.ShapeType() == this->shapeType)
	graphVertices.push_back(u);
    }

  private:
    const TopAbs_ShapeEnum &shapeType;
    const SeerShape &seerShape;
    std::vector<Vertex> &graphVertices;
  };
  
  template <class GraphTypeIn>
  class Node_writer {
  public:
    Node_writer(const GraphTypeIn &graphIn, const SeerShape &seerShapeIn):
      graph(graphIn), seerShape(seerShapeIn){}
    template <class NodeW>
    void operator()(std::ostream& out, const NodeW& v) const
    {
        out << 
            "[label=\"" <<
            shapeStrings.at(static_cast<int>(seerShape.findShapeIdRecord(v).shape.ShapeType())) << "\\n" <<
            gu::idToString(seerShape.findShapeIdRecord(v).id) <<
            "\"]";
    }
  private:
    const GraphTypeIn &graph;
    const SeerShape &seerShape;
  };
}

#endif // ANN_SEERSHAPE_H
