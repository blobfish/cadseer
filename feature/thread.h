/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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


#ifndef FTR_THREAD_H
#define FTR_THREAD_H

#include <feature/base.h>

namespace osg{class Matrixd;}

namespace prj{namespace srl{class FeatureThread;}}
namespace ann{class CSysDragger; class SeerShape;}
namespace lbr{class PLabel;}

namespace ftr
{
  namespace prm{class Parameter;}
  
  /**
  * @brief For creating screw threads.
  * 
  * Fake set true builds a simulated thread and
  * ignores the internal parameter setting. Fake set to false
  * allows Real threads to be internal or external.
  * Note a 'male' body is always built. for internal threads,
  * resulting body will need to be a tool for a subtraction.
  */
  class Thread : public Base
  {
  public:
    Thread();
    virtual ~Thread() override;
    
    void setDiameter(double);
    void setPitch(double);
    void setLength(double);
    void setAngle(double);
    void setInternal(bool);
    void setFake(bool);
    void setLeftHanded(bool);
    void setCSys(const osg::Matrixd&);
    
    double getDiameter() const;
    double getPitch() const;
    double getLength() const;
    double getAngle() const;
    bool getInternal() const;
    bool getFake() const;
    bool getLeftHanded() const;
    osg::Matrixd getCSys() const;
    
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureThread&);
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Thread;}
    virtual const std::string& getTypeString() const override {return toString(Type::Thread);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
  protected:
    std::unique_ptr<prm::Parameter> diameter; //!< major diameter.
    std::unique_ptr<prm::Parameter> pitch;
    std::unique_ptr<prm::Parameter> length;
    std::unique_ptr<prm::Parameter> angle; //!< included angle of thread section in degrees.
    std::unique_ptr<prm::Parameter> internal; //!< boolean to signal internal or external threads.
    std::unique_ptr<prm::Parameter> fake; //!< true means no helical.
    std::unique_ptr<prm::Parameter> leftHanded; //!< true means no helical.
    std::unique_ptr<prm::Parameter> csys;
    
    osg::ref_ptr<lbr::PLabel> diameterLabel;
    osg::ref_ptr<lbr::PLabel> pitchLabel;
    osg::ref_ptr<lbr::PLabel> lengthLabel;
    osg::ref_ptr<lbr::PLabel> angleLabel;
    osg::ref_ptr<lbr::PLabel> internalLabel;
    osg::ref_ptr<lbr::PLabel> fakeLabel;
    osg::ref_ptr<lbr::PLabel> leftHandedLabel;
  
    std::unique_ptr<ann::CSysDragger> csysDragger;
    std::unique_ptr<ann::SeerShape> sShape;
    
    boost::uuids::uuid solidId;
    std::vector<boost::uuids::uuid> ids;
    
    void updateIds();
    void updateLabels();
    
  private:
    static QIcon icon;
    
  };
}

#endif // FTR_THREAD_H
