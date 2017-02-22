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

#ifndef EXPR_EDGEPROPERTY_H
#define EXPR_EDGEPROPERTY_H

#include <vector>
#include <map>
#include <boost/unordered_map.hpp>

namespace expr{
  
  class AbstractNode;

/*! @brief Edge properties are attached to graph edges.
 * 
 * These properties indicate child node relevance to parent operation. @see Graph
 */
enum class EdgeProperty
{
  None,
  Lhs,
  Rhs,
  Parameter1,
  Parameter2,
  Then,
  Else
};

//! @brief Return a string associated to an edge property.
static std::string getEdgePropertyString(const EdgeProperty &property)
{
  static std::map<EdgeProperty, std::string> strings;
  static bool init =  false;
  if (!init) {
      init = true;
      strings[EdgeProperty::None] = "None";
      strings[EdgeProperty::Lhs] = "LHS";
      strings[EdgeProperty::Rhs] = "RHS";
      strings[EdgeProperty::Parameter1] = "Parameter1";
      strings[EdgeProperty::Parameter2] = "Parameter2";
      strings[EdgeProperty::Then] = "Then";
      strings[EdgeProperty::Else] = "Else";
  }
  std::map<EdgeProperty, std::string>::iterator it = strings.find(property);
  return it != strings.end() ? it->second : std::string();
}

/*! @brief container for edge properties and dependent values
 * 
 * this allows us to pass in values to the nodes calculate method without the nodes
 * needing to know anything about the graph.
 */
typedef boost::unordered_map<EdgeProperty, AbstractNode*> EdgePropertiesMap;

}

#endif //EXPR_EDGEPROPERTY_H

