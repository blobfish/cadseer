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

#include <osg/ref_ptr>

#include <tools/idtools.h>
#include <selection/container.h>
#include <feature/pick.h>
#include <feature/updatepayload.h>
#include <feature/inputtype.h>
#include <feature/base.h>

class QDir;
namespace osg{class MatrixTransform;}
namespace mdv{class DatumPlane;}
namespace lbr{class IPGroup;}
namespace prj{namespace srl{class SolverChoice; class FeatureDatumPlane;}}

namespace ftr
{
  namespace prm{class Parameter;}
  class ShapeHistory;
  
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
    InputType inputType;
  };
  typedef std::vector<DatumPlaneConnection> DatumPlaneConnections;
  
  //! Base class for different types of datum plane generation
  class DatumPlaneGenre
  {
  public:
    DatumPlaneGenre(){};
    virtual ~DatumPlaneGenre(){};
    virtual DatumPlaneType getType() = 0;
    virtual osg::Matrixd solve(const UpdatePayload&) = 0; //throw std::runtime;
    virtual lbr::IPGroup* getIPGroup(){return nullptr;}
    virtual void connect(Base *){}
    virtual DatumPlaneConnections setUpFromSelection(const slc::Containers &, const ShapeHistory&) = 0;
    virtual void serialOut(prj::srl::SolverChoice &solverChoice) = 0;
    double radius = 1.0;
  };
  
  class DatumPlanePlanarOffset : public DatumPlaneGenre
  {
  public:
    DatumPlanePlanarOffset();
    virtual ~DatumPlanePlanarOffset() override;
    virtual DatumPlaneType getType() override {return DatumPlaneType::PlanarOffset;}
    virtual osg::Matrixd solve(const UpdatePayload&) override;
    virtual lbr::IPGroup* getIPGroup() override;
    virtual void connect(Base *) override;
    virtual DatumPlaneConnections setUpFromSelection(const slc::Containers &, const ShapeHistory&) override;
    virtual void serialOut(prj::srl::SolverChoice &solverChoice) override;
    
    Pick facePick;
    std::shared_ptr<prm::Parameter> offset;
    osg::ref_ptr<lbr::IPGroup> offsetIP;
    
    static bool canDoTypes(const slc::Containers &);
  };
  
  class DatumPlanePlanarCenter : public DatumPlaneGenre
  {
  public:
    DatumPlanePlanarCenter();
    virtual ~DatumPlanePlanarCenter() override;
    virtual DatumPlaneType getType() override {return DatumPlaneType::PlanarCenter;}
    virtual osg::Matrixd solve(const UpdatePayload&) override;
    virtual DatumPlaneConnections setUpFromSelection(const slc::Containers &, const ShapeHistory&) override;
    virtual void serialOut(prj::srl::SolverChoice &solverChoice) override;
    
    Pick facePick1;
    Pick facePick2;
    
    static bool canDoTypes(const slc::Containers &);
  };
  
  class DatumPlanePlanarParallelThroughEdge : public DatumPlaneGenre
  {
  public:
    DatumPlanePlanarParallelThroughEdge();
    virtual ~DatumPlanePlanarParallelThroughEdge() override;
    virtual DatumPlaneType getType() override {return DatumPlaneType::PlanarParallelThroughEdge;}
    virtual osg::Matrixd solve(const UpdatePayload&) override;
    virtual DatumPlaneConnections setUpFromSelection(const slc::Containers &, const ShapeHistory&) override;
    virtual void serialOut(prj::srl::SolverChoice &solverChoice) override;
    
    Pick facePick;
    Pick edgePick;
    
    static bool canDoTypes(const slc::Containers &);
  };
  
  class DatumPlane : public Base
  {
  public:
    DatumPlane();
    ~DatumPlane();
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual void updateVisual() override;
    virtual Type getType() const override {return Type::DatumPlane;}
    virtual const std::string& getTypeString() const override {return toString(Type::DatumPlane);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureDatumPlane &);
    virtual QTextStream& getInfo(QTextStream &) const override;
    
    void setSolver(std::shared_ptr<DatumPlaneGenre> solverIn);
    osg::Matrixd getSystem() const {return transform->getMatrix();}
    double getRadius() const;
    
    static std::vector<std::shared_ptr<DatumPlaneGenre> > solversFromSelection(const slc::Containers &);
    
  private:
    typedef Base Inherited;
    static QIcon icon;
    osg::ref_ptr<mdv::DatumPlane> display;
    osg::ref_ptr<osg::MatrixTransform> transform;
    std::shared_ptr<DatumPlaneGenre> solver;
    
    std::unique_ptr<prm::Parameter> radius; //!< double. distance to edges.
    std::unique_ptr<prm::Parameter> autoSize; //!< bool. auto calculate radius.
    
//     double radius = 1.0; //!< boundary size of plane.
//     bool autoSize = true; //!< whether the plane sizes itself.
    
    void updateGeometry();
  };
}

#endif // FTR_DATUMPLANE_H
