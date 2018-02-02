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


#ifndef FTR_BASE_H
#define FTR_BASE_H

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
#include <feature/inputtype.h>
#include <feature/states.h>
#include <annex/base.h>

class QDir;
class QTextStream;

namespace prj{namespace srl{class FeatureBase;}}
namespace msg{class Message; class Observer;}
namespace ftr
{
class ShapeHistory;
class UpdatePayload;

namespace prm{class Parameter; typedef std::vector<Parameter*> Parameters;}

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
  bool isSkipped() const {return state.test(StateOffset::Skipped);}
  bool isNotSkipped() const {return !(state.test(StateOffset::Skipped));}
  void setSkipped();
  void setNotSkipped();
  bool isVisible3D() const;
  bool isHidden3D() const;
  bool isVisibleOverlay() const;
  bool isHiddenOverlay() const;
  bool isSuccess() const {return !(state.test(ftr::StateOffset::Failure));}
  bool isFailure() const {return state.test(ftr::StateOffset::Failure);}
  void setName(const QString &nameIn);
  QString getName() const {return name;}
  ftr::State getState() const {return state;}
  void setColor(const osg::Vec4 &);
  const osg::Vec4& getColor() const {return color;}
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
  static TopoDS_Compound compoundWrap(const TopoDS_Shape &shapeIn);
  osg::Switch* getMainSwitch() const {return mainSwitch.get();}
  osg::Switch* getOverlaySwitch() const {return overlaySwitch.get();}
  osg::MatrixTransform* getMainTransform() const {return mainTransform.get();}
  bool hasParameter(const QString &nameIn) const; //!< parameter names are not unique.
  prm::Parameter* getParameter(const QString &nameIn) const; //!< parameter names are not unique.
  bool hasParameter(const boost::uuids::uuid &idIn) const;
  prm::Parameter* getParameter(const boost::uuids::uuid &idin) const;
  const prm::Parameters& getParameters() const{return parameters;}
  
  bool hasAnnex(ann::Type t) const
  {
    return static_cast<bool>(annexes.count(t));
  }
  template <typename T> const T& getAnnex(ann::Type t) const
  {
    T* out = dynamic_cast<T*>(annexes.at(t));
    assert(out);
    return *out;
  }
  template <typename T> T& getAnnex(ann::Type t)
  {
    T* out = dynamic_cast<T*>(annexes.at(t));
    assert(out);
    return *out;
  }
  
  virtual void serialWrite(const QDir &); //!< override in leaf classes only.
  std::string getFileName() const; //!< used by git.
  QString buildFilePathName(const QDir&) const; //!<generate complete path to file
  
  static std::size_t nextConstructionIndex;
  
protected:
  void setModelClean(); //!< clean can only set through virtual update.
  void setVisualClean(); //!< clean can only set through virtual visual update.
  void setFailure(); //!< set only through virtual update.
  void setSuccess(); //!< set only through virtual update.
  void sendStateMessage(std::size_t); //!< just convenience.
  
  prj::srl::FeatureBase serialOut(); //!<convert this into serializable object. no const, we update the result container with offset
  void serialIn(const prj::srl::FeatureBase& sBaseIn);
  
  QString name;
  prm::Parameters parameters;
  ann::Annexes annexes;
  
  std::unique_ptr<msg::Observer> observer;
  
  boost::uuids::uuid id;
  ftr::State state;
  std::size_t constructionIndex; //!< for consistently ordered iteration. @see DAGView
  
  osg::ref_ptr<osg::Switch> mainSwitch;
  osg::ref_ptr<osg::MatrixTransform> mainTransform;
  osg::ref_ptr<osg::Switch> overlaySwitch;
  osg::ref_ptr<osg::LOD> lod;
  
  osg::Vec4 color;
  std::string lastUpdateLog;
};
}

#endif // FTR_BASE_H
