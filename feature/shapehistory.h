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

#ifndef FTR_SHAPEHISTORY_H
#define FTR_SHAPEHISTORY_H

#include <memory>

namespace prj{namespace srl{class ShapeHistory;}}

namespace ftr
{
  class ShapeHistoryStow;
  
  
  
  /*! @brief History of shape 'evolution'
   * 
   * Shape history will be used in 2 related ways.
   * 1) will be destroyed and regenerated and used during every
   * update to resolve identifing shapes.
   * 2) will be used for storing picks. These picks will be a
   * 'sub set' of the 'update generated graph' related to a user pick. 
   * These picks will remain in the feature until the user 'deselects'
   * the appropriate shape.
   * 
   * Most of the time we will want track from child to parent.
   * so this graph will point from child to parent. In other terms, child
   * is the source of an edge and the parent is the target of an edge.
   * 
   * see design file shapeHistoryGraph
   */
  class ShapeHistory
  {
  public:
    ShapeHistory();
    ShapeHistory(std::shared_ptr<ShapeHistoryStow>);
    ~ShapeHistory();
    
    //! reset all data structures.
    void clear();
    
    //! write out in graphViz format.
    void writeGraphViz(const std::string &fileName) const;
    
    //@{
    //! construction functions.
    void addShape(const boost::uuids::uuid &featureIdIn, const boost::uuids::uuid &shapeIdIn);
    
    /*!
     * @sourceShapeIdIn should be child shape id.
     * @targetShapeIdIn should be parent shape id.
     */
    void addConnection(const boost::uuids::uuid &sourceShapeIdIn, const boost::uuids::uuid &targetShapeIdIn);
    //@}
    
    //@{
    //! query functions.
    bool hasShape(const boost::uuids::uuid &shapeIdIn) const;
    
    /*! search for a decendant that is a result of @featureIdIn for the shape id @shapeIdIn.
     * @return id of shape or nil id if not found.
     */
    boost::uuids::uuid evolve(const boost::uuids::uuid &featureIdIn, const boost::uuids::uuid &shapeIdIn) const;
    
    /*! search for a ascendant that is a result of @featureIdIn for the shape id @shapeIdIn.
     * @return id of shape or nil id if not found.
     */
    boost::uuids::uuid devolve(const boost::uuids::uuid &featureIdIn, const boost::uuids::uuid &shapeIdIn) const;
    
    /*! @brief get all shape ids related to shapeId passed in that belong to the feature
    * 
    * @param this is the shapehistory graph for the whole project.
    * @param pick is the shapehistory of a pick created with createDevolveHistory.
    * @param featureId is the id of the feature we are interested in.
    * 
    * @note does a BFS of pick stopping at the first id that is in this graph.
    * then does another BFS of update (forward and back) to find all relatives in the feature id.
    */
    std::vector<boost::uuids::uuid> resolveHistories
    (
      const ShapeHistory &pick,
      const boost::uuids::uuid &featureId
    ) const;
    
    //@}
    
    //! create a 'subset', descendants graph related to shape id @shapeIdIn
    ShapeHistory createEvolveHistory(const boost::uuids::uuid &shapeIdIn) const;
    
    //! create a 'subset', anscendant graph related to shape id @shapeIdIn. use for picks.
    ShapeHistory createDevolveHistory(const boost::uuids::uuid &shapeIdIn) const;
    
    std::vector<boost::uuids::uuid> getAllIds() const;
    
    prj::srl::ShapeHistory serialOut() const;
    void serialIn(const prj::srl::ShapeHistory&);
    
    
  private:
    std::shared_ptr<ShapeHistoryStow> shapeHistoryStow;
  };
  
}




#endif // FTR_SHAPEHISTORY_H
