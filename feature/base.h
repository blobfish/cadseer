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


#ifndef FEATUREBASE_H
#define FEATUREBASE_H

#include <map>
#include <memory>

#include <boost/uuid/uuid.hpp>

#include <QIcon>
#include <QString>

#include <TopoDS_Compound.hxx> //used in for wrapper.
#include <Standard_Failure.hxx> //used in derived classes.
#include <Precision.hxx> //used in derived classes.

#include <osg/Switch>
#include <osg/MatrixTransform>
#include <osg/LOD>

#include <feature/types.h>
#include <feature/inputtypes.h>
#include <feature/states.h>
#include <feature/parameter.h>

class QDir;
class QTextStream;

namespace prj{namespace srl{class FeatureBase;}}
namespace msg{class Message; class Observer;}

namespace ftr
{
  class SeerShape;
  class ShapeHistory;
  class Base;
  
  /*! used during editing to pass in parent features. 
   * not const because we want to hide and and show etc.
   * parents, but no 'real' change should be done.
   */
  typedef std::multimap<InputTypes, Base*> EditMap;
  
  /*! @brief Update information needed by features.
   * 
   * had a choice to make. Feature update will need shapeHistory
   * for resolving references. Features need to fill in shape history
   * even when the update isn't needed. so shape history passed into update
   * is const and we have a virtual method to fill in shape history.
   * 
   * 
   */
  class UpdatePayload
  {
  public:
    typedef std::multimap<InputTypes, const Base*> UpdateMap;
    
    UpdatePayload(const UpdateMap &updateMapIn, const ShapeHistory &shapeHistoryIn) :
    updateMap(updateMapIn),
    shapeHistory(shapeHistoryIn)
    {}
    
    const UpdateMap &updateMap;
    const ShapeHistory &shapeHistory;
  };
  
class Base
{
public:
  Base();
  virtual ~Base();
  bool isModelDirty() const {return state.test(ftr::StateOffset::ModelDirty);}
  bool isModelClean() const {return !(state.test(ftr::StateOffset::ModelDirty));}
  void setModelDirty();
  bool isVisualDirty() const {return state.test(ftr::StateOffset::VisualDirty);}
  bool isVisualClean() const {return !(state.test(ftr::StateOffset::VisualDirty));}
  void setVisualDirty();
  void show3D();
  void hide3D();
  void toggle3D();
  bool isVisible3D() const {return !(state.test(ftr::StateOffset::Hidden3D));}
  bool isHidden3D() const {return state.test(ftr::StateOffset::Hidden3D);}
  void showOverlay();
  void hideOverlay();
  void toggleOverlay();
  bool isVisibleOverlay() const {return !(state.test(ftr::StateOffset::HiddenOverlay));}
  bool isHiddenOverlay() const {return state.test(ftr::StateOffset::HiddenOverlay);}
  bool isSuccess() const {return !(state.test(ftr::StateOffset::Failure));}
  bool isFailure() const {return state.test(ftr::StateOffset::Failure);}
  void setActive();
  void setInActive();
  bool isActive() const {return !(state.test(ftr::StateOffset::Inactive));}
  bool isInactive() const {return state.test(ftr::StateOffset::Inactive);}
  void setLeaf();
  void setNonLeaf();
  bool isLeaf() const {return !(state.test(ftr::StateOffset::NonLeaf));}
  bool isNonLeaf() const {return (state.test(ftr::StateOffset::NonLeaf));}
  void setName(const QString &nameIn);
  QString getName() const {return name;}
  ftr::State getState() const {return state;}
  void setColor(const osg::Vec4 &);
  osg::Vec4& getColor(){return color;}
  virtual void updateModel(const UpdatePayload&) = 0;
  virtual void updateVisual(); //called after update.
  virtual Type getType() const = 0;
  virtual const std::string& getTypeString() const = 0;
  virtual const QIcon& getIcon() const = 0;
  virtual Descriptor getDescriptor() const = 0;
  virtual void applyColor(); //!< called by set color.
  virtual void fillInHistory(ShapeHistory &);
  virtual void replaceId(const boost::uuids::uuid&, const boost::uuids::uuid&, const ShapeHistory&);
  virtual QTextStream& getInfo(QTextStream &) const;
  QTextStream&  getShapeInfo(QTextStream &, const boost::uuids::uuid&) const;
  boost::uuids::uuid getId() const {return id;}
  const TopoDS_Shape& getShape() const;
  void setShape(const TopoDS_Shape &in); //!< only used for serialize in!
  static TopoDS_Compound compoundWrap(const TopoDS_Shape &shapeIn);
  osg::Switch* getMainSwitch() const {return mainSwitch.get();}
  osg::Switch* getOverlaySwitch() const {return overlaySwitch.get();}
  osg::MatrixTransform* getMainTransform() const {return mainTransform.get();}
  bool hasSeerShape() const {return static_cast<bool>(seerShape);}
  const SeerShape& getSeerShape() const {assert(seerShape); return *seerShape;}
  bool hasParameter(const QString &nameIn) const; //!< parameter names are not unique.
  Parameter* getParameter(const QString &nameIn) const; //!< parameter names are not unique.
  bool hasParameter(const boost::uuids::uuid &idIn) const;
  Parameter* getParameter(const boost::uuids::uuid &idin) const;
  const ParameterVector& getParameterVector() const{return parameterVector;}
  
  virtual void serialWrite(const QDir &); //!< override in leaf classes only.
  std::string getFileName() const; //!< used by git.
  QString buildFilePathName(const QDir&) const; //!<generate complete path to file
  
  static std::size_t nextConstructionIndex;
  
protected:
  void setModelClean(); //!< clean can only set through virtual update.
  void setVisualClean(); //!< clean can only set through virtual visual update.
  void setFailure(); //!< set only through virtual update.
  void setSuccess(); //!< set only through virtual update.
  
  prj::srl::FeatureBase serialOut(); //!<convert this into serializable object. no const, we update the result container with offset
  void serialIn(const prj::srl::FeatureBase& sBaseIn);
  
  QString name;
  ParameterVector parameterVector;
  
  std::unique_ptr<msg::Observer> observer;
  
  boost::uuids::uuid id;
  ftr::State state;
  std::size_t constructionIndex; //!< for consistently ordered iteration. @see DAGView
  
  std::shared_ptr<SeerShape> seerShape;
  
  osg::ref_ptr<osg::Switch> mainSwitch;
  osg::ref_ptr<osg::MatrixTransform> mainTransform;
  osg::ref_ptr<osg::Switch> overlaySwitch;
  osg::ref_ptr<osg::LOD> lod;
  
  osg::Vec4 color;
};
}

#endif // FEATUREBASE_H
