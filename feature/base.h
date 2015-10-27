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

#include <boost/signals2.hpp>

#include <QIcon>
#include <QString>

#include <TopoDS_Compound.hxx> //used in for wrapper.
#include <Standard_Failure.hxx> //used in derived classes.
#include <Precision.hxx> //used in derived classes.

#include <osg/Switch>

#include "maps.h"
#include "types.h"
#include "inputtypes.h"
#include "../modelviz/connector.h"
#include "states.h"

namespace ftr
{
  class Base;
  typedef std::map<InputTypes, const Base*> UpdateMap;
  
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
  void setName(const QString &nameIn){name = nameIn;}
  QString getName() const {return name;}
  ftr::State getState() const {return state;}
  virtual void update(const UpdateMap&) = 0;
  virtual void updateVisual(); //called after update.
  virtual Type getType() const = 0;
  virtual const std::string& getTypeString() const = 0;
  virtual const QIcon& getIcon() const = 0;
  virtual Descriptor getDescriptor() const = 0;
  const EvolutionContainer& getEvolutioncontainer() const {return evolutionContainer;}
  const ResultContainer& getResultContainer() const {return resultContainer;}
  boost::uuids::uuid getId() const {return id;}
  const TopoDS_Shape& getShape() const {return shape;}
  static TopoDS_Compound compoundWrap(const TopoDS_Shape &shapeIn);
  osg::Switch* getMainSwitch() const {return mainSwitch.get();}
  const mdv::Connector& getConnector() const {return connector;}
  
  static std::size_t nextConstructionIndex;
  
  typedef boost::signals2::signal<void (boost::uuids::uuid, std::size_t)> StateChangedSignal;
  boost::signals2::connection connectState(const StateChangedSignal::slot_type &subscriber) const
  {
    return stateChangedSignal.connect(subscriber);
  }
  
protected:
  void setModelClean(); //!< clean can only set through virtual update.
  void setVisualClean(); //!< clean can only set through virtual visual update.
  void setFailure(); //!< set only through virtual update.
  void setSuccess(); //!< set only through virtual update.
  
  QString name;
  
  //mutable allows us to connect to the signal through a const object. neat!
  mutable StateChangedSignal stateChangedSignal;
  
  boost::uuids::uuid id;
  ftr::State state;
  std::size_t constructionIndex; //!< for consistently ordered iteration. @see DAGView
  
  TopoDS_Shape shape;
  
  EvolutionContainer evolutionContainer;
  ResultContainer resultContainer;
  FeatureContainer featureContainer;
  
  osg::ref_ptr<osg::Switch> mainSwitch;
  mdv::Connector connector;
};
}

#endif // FEATUREBASE_H
