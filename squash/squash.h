/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef SQS_SQUASH_H
#define SQS_SQUASH_H

#include <osg/Group>

#include <tools/occtools.h>

namespace sqs
{
  struct Parameters
  {
    Parameters(const TopoDS_Shell &sIn, const occt::FaceVector &fsIn): s(sIn), fs(fsIn){}
    
    //input
    const TopoDS_Shell &s; //!< shell to flatten
    const occt::FaceVector &fs; //!< faces to lock. must be part of the shell.
    int granularity = 3; //!< mesh density 1-5; 3 is approx 5000 triangles. 1 is coarse, 5 is fine.
    int tCount = 0; //!< number of triangles for mesh. if not 0, overrides granularity.
    
    //output
    TopoDS_Face ff; //!< flat face
    occt::EdgeVector es; //!< flat edges in case face is invalid.
    osg::ref_ptr<osg::Group> mesh3d;
    osg::ref_ptr<osg::Group> mesh2d;
    std::string message;
  };
  
  
  /*! flatten the shell with the faces locked*/
  void squash(Parameters &);
}

#endif // SQS_SQUASH_H
