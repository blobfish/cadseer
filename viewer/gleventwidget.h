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

#ifndef VWR_GLEVENTWIDGET_H
#define VWR_GLEVENTWIDGET_H

#include <osgQt/GraphicsWindowQt>

namespace vwr{class SpaceballOSGEvent;}

namespace vwr
{
class GLEventWidget : public osgQt::GLWidget
{
    typedef osgQt::GLWidget inherited;
public:
    GLEventWidget(const QGLFormat& format, QWidget* parent = NULL, const QGLWidget* shareWidget = NULL,
                  Qt::WindowFlags f = 0, bool forwardKeyEvents = false);
protected:
    virtual bool event(QEvent* event);

private:
    osg::ref_ptr<vwr::SpaceballOSGEvent> convertEvent(QEvent* qEvent);
};
}

#endif // VWR_GLEVENTWIDGET_H
