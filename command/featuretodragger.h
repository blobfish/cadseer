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

#ifndef CMD_FEATURETODRAGGER_H
#define CMD_FEATURETODRAGGER_H

#include <command/base.h>

namespace cmd
{
    /*! @brief command to reset a feature to its dragger.
   */
  class FeatureToDragger : public Base
  {
  public:
    FeatureToDragger();
    virtual ~FeatureToDragger() override;
    virtual std::string getCommandName() override{return "FeatureToDragger";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  };
}

#endif // CMD_FEATURETODRAGGER_H
