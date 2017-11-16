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

#ifndef FTR_SEERSHAPEINFO_H
#define FTR_SEERSHAPEINFO_H

#include <memory>

namespace boost{namespace uuids{struct uuid;}}

class QTextStream;

class TopoDS_Shape;
namespace ann{class SeerShape;}
namespace ftr
{
    class SeerShapeInfo
    {
        public:
            explicit SeerShapeInfo(const ann::SeerShape &);
            ~SeerShapeInfo();
            QTextStream& getShapeInfo(QTextStream&, const boost::uuids::uuid&);
        private:
            const ann::SeerShape &seerShape;
            
            class FunctionMapper;
            std::unique_ptr<FunctionMapper> functionMapper;
            void compInfo(QTextStream&, const TopoDS_Shape&);
            void compSolidInfo(QTextStream&, const TopoDS_Shape&);
            void solidInfo(QTextStream&, const TopoDS_Shape&);
            void shellInfo(QTextStream&, const TopoDS_Shape&);
            void faceInfo(QTextStream&, const TopoDS_Shape&);
            void wireInfo(QTextStream&, const TopoDS_Shape&);
            void edgeInfo(QTextStream&, const TopoDS_Shape&);
            void vertexInfo(QTextStream&, const TopoDS_Shape&);
    };
}

#endif // FTR_SEERSHAPEINFO_H
