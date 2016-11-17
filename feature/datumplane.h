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

#ifndef FTR_DATUMPLANE_H
#define FTR_DATUMPLANE_H

#include <boost/uuid/nil_generator.hpp>

#include <osg/ref_ptr>

#include <selection/container.h>
#include <feature/base.h>

class QDir;
namespace osg{class MatrixTransform;}
namespace mdv{class DatumPlane;}
namespace lbr{class IPGroup;}
namespace prj{namespace srl{class SolverChoice;}}

namespace ftr
{
  enum class DatumPlaneType
  {
    None = 0,
    PlanarOffset,
    PlanarCenter, //!< 2 parallel planar faces
    PlanarParallelThroughEdge //!< planar parallel through edge.
  };
  
  inline const static QString getDatumPlaneTypeString(DatumPlaneType typeIn)
  {
    const static QStringList strings 
    ({
      QObject::tr("None"),
      QObject::tr("Planar Offset"),
      QObject::tr("Planar Center"),
      QObject::tr("Planar Parallel Through Edge")
    });
    
    int casted = static_cast<int>(typeIn);
    assert(casted < strings.size());
    return strings.at(casted);
  }
  
  //! just used to relay connection info to to dplane feature. 
  struct DatumPlaneConnection
  {
    boost::uuids::uuid parentId;
    InputTypes inputType;
  };
  typedef std::vector<DatumPlaneConnection> DatumPlaneConnections;
  
  //! Base class for different types of datum plane generation
  class DatumPlaneGenre
  {
  public:
    virtual DatumPlaneType getType() = 0;
    virtual osg::Matrixd solve(const UpdateMap&) = 0; //throw std::runtime;
    virtual lbr::IPGroup* getIPGroup(){return nullptr;}
    virtual void connect(Base *){}
    virtual DatumPlaneConnections setUpFromSelection(const slc::Containers &) = 0;
    virtual void serialOut(prj::srl::SolverChoice &solverChoice) = 0;
    double radius;
  };
  
  class DatumPlanePlanarOffset : public DatumPlaneGenre
  {
  public:
    DatumPlanePlanarOffset();
    ~DatumPlanePlanarOffset();
    virtual DatumPlaneType getType() override {return DatumPlaneType::PlanarOffset;}
    virtual osg::Matrixd solve(const UpdateMap&) override;
    virtual lbr::IPGroup* getIPGroup() override;
    virtual void connect(Base *) override;
    virtual DatumPlaneConnections setUpFromSelection(const slc::Containers &) override;
    virtual void serialOut(prj::srl::SolverChoice &solverChoice) override;
    
    boost::uuids::uuid faceId = boost::uuids::nil_uuid();
    std::shared_ptr<Parameter> offset;
    osg::ref_ptr<lbr::IPGroup> offsetIP;
    
    static bool canDoTypes(const slc::Containers &);
  };
  
  class DatumPlanePlanarCenter : public DatumPlaneGenre
  {
  public:
    DatumPlanePlanarCenter();
    ~DatumPlanePlanarCenter();
    virtual DatumPlaneType getType() override {return DatumPlaneType::PlanarCenter;}
    virtual osg::Matrixd solve(const UpdateMap&) override;
    virtual DatumPlaneConnections setUpFromSelection(const slc::Containers &) override;
    virtual void serialOut(prj::srl::SolverChoice &solverChoice) override;
    
    boost::uuids::uuid faceId1 = boost::uuids::nil_uuid();
    boost::uuids::uuid faceId2 = boost::uuids::nil_uuid();
    
    static bool canDoTypes(const slc::Containers &);
  };
  
  class DatumPlanePlanarParallelThroughEdge : public DatumPlaneGenre
  {
  public:
    DatumPlanePlanarParallelThroughEdge();
    ~DatumPlanePlanarParallelThroughEdge();
    virtual DatumPlaneType getType() override {return DatumPlaneType::PlanarParallelThroughEdge;}
    virtual osg::Matrixd solve(const UpdateMap&) override;
    virtual DatumPlaneConnections setUpFromSelection(const slc::Containers &) override;
    virtual void serialOut(prj::srl::SolverChoice &solverChoice) override;
    
    boost::uuids::uuid faceId = boost::uuids::nil_uuid();
    boost::uuids::uuid edgeId = boost::uuids::nil_uuid();
    
    static bool canDoTypes(const slc::Containers &);
  };
  
  class DatumPlane : public Base
  {
  public:
    DatumPlane();
    ~DatumPlane();
    
    virtual void updateModel(const UpdateMap&) override;
    virtual void updateVisual() override;
    virtual Type getType() const override {return Type::DatumPlane;}
    virtual const std::string& getTypeString() const override {return toString(Type::DatumPlane);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
//     void serialRead(const prj::srl::FeatureDraft &);
    
    void setSolver(std::shared_ptr<DatumPlaneGenre> solverIn);
    osg::Matrixd getSystem() const {return transform->getMatrix();}
    double getRadius() const {return radius;}
    
    static std::vector<std::shared_ptr<DatumPlaneGenre> > solversFromSelection(const slc::Containers &);
    
  private:
    static QIcon icon;
    osg::ref_ptr<mdv::DatumPlane> display;
    osg::ref_ptr<osg::MatrixTransform> transform;
    std::shared_ptr<DatumPlaneGenre> solver;
    
    double radius = 1.0; //!< boundary size of plane.
    bool autoSize = true; //!< whether the plane sizes itself.
    
    void updateGeometry();
  };
}

#endif // FTR_DATUMPLANE_H
