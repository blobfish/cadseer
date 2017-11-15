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

#ifndef CMD_FEATURETOSYSTEM_H
#define CMD_FEATURETOSYSTEM_H

#include <command/base.h>

namespace cmd
{
  /*! @brief command to reposition a feature to the current coordinate system
   * 
   * for now, this will be only used in a pick first scenario. so we will forgo
   * the keeping of selection state and just analyse the selection upon command
   * execution. It will then just finish.
   */
  class FeatureToSystem : public Base
  {
  public:
    FeatureToSystem();
    virtual ~FeatureToSystem() override;
    virtual std::string getCommandName() override{return "FeatureToSystem";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  };
}

#endif // CMD_FEATURETOSYSTEM_H
