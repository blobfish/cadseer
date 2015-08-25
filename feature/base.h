/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  tanderson <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef FEATUREBASE_H
#define FEATUREBASE_H

#include <map>

#include <Standard_Failure.hxx> //used in derived classes.
#include <Precision.hxx> //used in derived classes.

#include <osg/Switch>

#include "maps.h"
#include "types.h"
#include "inputtypes.h"
#include "../modelviz/connector.h"

class TopoDS_Compound;

namespace Feature
{
  class Base;
  typedef std::map<InputTypes, const Base*> UpdateMap;
  
class Base
{
public:
  Base();
  virtual ~Base();
  bool isDirty() const {return dirty;}
  bool isClean() const {return !dirty;}
  void setDirty(){dirty = true; visualDirty = true;}
  bool isVisualDirty() const {return visualDirty;}
  bool isVisualClean() const {return !visualDirty;}
  virtual void update(const UpdateMap&) = 0;
  virtual void updateVisual(); //called after update.
  virtual Type getType() const = 0;
  virtual const std::string& getTypeString() const = 0;
  const EvolutionContainer& getEvolutioncontainer() const {return evolutionContainer;}
  const ResultContainer& getResultContainer() const {return resultContainer;}
  boost::uuids::uuid getId() const {return id;}
  const TopoDS_Shape& getShape() const {return shape;}
  static TopoDS_Compound compoundWrap(const TopoDS_Shape &shapeIn);
  osg::Switch* getMainSwitch() const {return mainSwitch.get();}
  const ModelViz::Connector& getConnector() const {return connector;}
  
protected:
  void setClean(){dirty = false;} //!< clean can only set through virtual update.
  void setVisualClean(){visualDirty = false;} //!< clean can only set through virtual visul update.
  
  bool dirty = true;
  bool visualDirty = true;
  
  boost::uuids::uuid id;
  
  TopoDS_Shape shape;
  
  EvolutionContainer evolutionContainer;
  ResultContainer resultContainer;
  FeatureContainer featureContainer;
  
  osg::ref_ptr<osg::Switch> mainSwitch;
  ModelViz::Connector connector;
};
}

#endif // FEATUREBASE_H
