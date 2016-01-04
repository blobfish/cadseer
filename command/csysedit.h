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

#ifndef CMD_CSYSEDIT_H
#define CMD_CSYSEDIT_H

#include <osg/ref_ptr>

#include <command/base.h>

namespace osgManipulator{class Translate1DDragger;}

namespace lbr{class CSysDragger;}
namespace slc{class Message;}

namespace cmd
{
  class CSysEdit : public Base
  {
  public:
    enum class Type
    {
      None = 0,
      Origin,
      Vector
    };
    CSysEdit();
    virtual ~CSysEdit() override;
    virtual std::string getCommandName() override{return "CSysEdit";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
    
    Type type;
    osg::ref_ptr<lbr::CSysDragger> csysDragger;
    osg::ref_ptr<osgManipulator::Translate1DDragger> translateDragger;
    
  private:
    void setupDispatcher();
    void selectionAdditionDispatched(const msg::Message &);
    void selectionSubtractionDispatched(const msg::Message &);
    
    void analyzeSelections();
    void updateToVector(const osg::Vec3d &toVector);
    
    slc::Messages messages;
  };
}

#endif // CMD_CSYSEDIT_H
