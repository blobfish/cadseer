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

#ifndef STATES_H
#define STATES_H

#include <bitset>
#include <vector>
#include <string>
#include <assert.h>

namespace ftr
{
  typedef std::bitset<32> State; //!< = unsigned long
  
  namespace StateOffset
  {
    static const unsigned long ModelDirty =       0;
    static const unsigned long VisualDirty =      1;
    static const unsigned long Failure =          2;
    static const unsigned long Inactive =         3;
    static const unsigned long NonLeaf =          4; //keeping consistently negative.
  };
}

#endif // STATES_H
