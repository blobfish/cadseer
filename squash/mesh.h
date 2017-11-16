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

#ifndef SQS_MESH_H
#define SQS_MESH_H

#include <vector>

#include <CGAL/Kernel/global_functions.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Min_sphere_of_spheres_d.h>
#include <CGAL/Min_sphere_of_points_d_traits_3.h>
#include <CGAL/algorithm.h>

typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Point_3 Point;
typedef CGAL::Vector_3<Kernel> Vector;
typedef CGAL::Direction_3<Kernel> Direction;
typedef CGAL::Triangle_3<Kernel> Triangle;
typedef CGAL::Line_3<Kernel> Line;
typedef CGAL::Segment_3<Kernel> Segment;

typedef CGAL::Surface_mesh<Point> Mesh;
typedef Mesh::Vertex_index Vertex;
typedef Mesh::Edge_index Edge;
typedef Mesh::Halfedge_index HalfEdge;
typedef Mesh::Face_index Face;
typedef std::vector<Vertex> Vertices;
typedef std::vector<HalfEdge> HalfEdges;
typedef std::vector<Edge> Edges;
typedef std::vector<Face> Faces;

typedef CGAL::Min_sphere_of_points_d_traits_3<Kernel, double, CGAL::Tag_true, CGAL::Default_algorithm> SphereTraits;
typedef CGAL::Min_sphere_of_spheres_d<SphereTraits> BSphere;

#endif // SQS_MESH_H
