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

#include "igl.h"


std::pair<Eigen::MatrixXd, Eigen::MatrixXi> sqs::toIgl(const Mesh &mIn)
{
  //warning: if mIn has garbage then output will be out of sync.
  
  //vertices
  Eigen::MatrixXd vsOut;
  vsOut.resize(CGAL::vertices(mIn).size(), 3);
  int cr = 0; //current row.
  for (const auto &v : CGAL::vertices(mIn))
  {
    vsOut(cr, 0) = mIn.point(v).x();
    vsOut(cr, 1) = mIn.point(v).y();
    vsOut(cr, 2) = mIn.point(v).z();
    cr++;
  }
  
  //faces
  Eigen::MatrixXi fsOut;
  fsOut.resize(CGAL::faces(mIn).size(), 3);
  cr = 0;
  for (const auto &f : CGAL::faces(mIn))
  {
    int cc = 0;
    for (const auto &v : CGAL::vertices_around_face(mIn.halfedge(f), mIn))
    {
      fsOut(cr, cc) = static_cast<int>(v);
      cc++;
    }
    cr++;
  }
  
  return std::make_pair(vsOut, fsOut);
}

Mesh sqs::toCgal(const Eigen::MatrixXd &vsIn, const Eigen::MatrixXi &fsIn)
{
  Mesh out;
  
  for (int r = 0; r < vsIn.rows(); ++r)
  {
    if (vsIn.cols() == 2)
      out.add_vertex(Point(vsIn(r,0), vsIn(r, 1), 0.0));
    else if (vsIn.cols() == 3)
      out.add_vertex(Point(vsIn(r,0), vsIn(r, 1), vsIn(r, 2)));
  }
  
  for (int r = 0; r < fsIn.rows(); ++r)
  {
    out.add_face
    (
      static_cast<Vertex>(fsIn(r,0)),
      static_cast<Vertex>(fsIn(r,1)),
      static_cast<Vertex>(fsIn(r,2))
    );
  }
  
  return out;
}
