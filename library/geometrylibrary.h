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

#ifndef GEOMETRYLIBRARY_H
#define GEOMETRYLIBRARY_H

#include <memory>

#include <osg/Geometry>

#include "geometrylibrarytag.h"

namespace lbr
{
  class MapWrapper;
  
  struct Manager
  {
    static Manager& getManager();
    void link(const Tag &tagIn, osg::Geometry *geometryIn);
    bool isLinked(const Tag &tagIn);
    osg::Geometry* getGeometry(const Tag &tagIn);
  private:
    Manager();
    Manager(const Manager& other) = delete;
    Manager& operator=(const Manager&) = delete;
    void setup();
    
    std::unique_ptr<MapWrapper> mapWrapper;
  };
  
  namespace csys
  {
    static const Tag TranslationLineTag
    ({
      "CSysDragger",
      "Translation",
      "Line"
    });

    static const Tag TranslationCylinderTag
    ({
      "CSysDragger",
      "Translation",
      "Cylinder"
    });

    static const Tag TranslationConeTag
    ({
      "CSysDragger",
      "Translation",
      "Cone"
    });

    static const Tag SphereTag
    ({
      "CSysDragger",
      "Spere"
    });

    static const Tag RotationLineTag
    ({
      "CSysDragger",
      "Rotation",
      "Line"
    });

    static const Tag RotationTorusTag
    ({
      "CSysDragger",
      "Rotation",
      "Torus"
    });
    
    static const Tag IconLinkTag
    ({
      "CSysDragger",
      "Icon",
      "Link"
    });
    
    static const Tag IconUnlinkTag
    ({
      "CSysDragger",
      "Icon",
      "Unlink"
    });
    
    //constructions here so any object can use
    //this geometry without any csys dragger headers.
    osg::Geometry* buildTranslationLine();
    osg::Geometry* buildTranslationCylinder();
    osg::Geometry* buildTranslationCone();
    osg::Geometry* buildSphere();
    osg::Geometry* buildRotationLine();
    osg::Geometry* buildRotationTorus();
    osg::Geometry* buildIconLink();
    osg::Geometry* buildIconUnlink();
    std::string fileNameFromResource(const std::string &);
  }
}

#endif // GEOMETRYLIBRARY_H
