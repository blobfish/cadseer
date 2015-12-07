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

#include <application/application.h>
#include <application/mainwindow.h>
#include <viewer/viewerwidget.h>
#include <message/dispatch.h>

int main( int argc, char** argv )
{
//    osg::ArgumentParser arguments(&argc, argv);

//    osgViewer::ViewerBase::ThreadingModel threadingModel = osgViewer::ViewerBase::CullDrawThreadPerContext;
//    while (arguments.read("--SingleThreaded")) threadingModel = osgViewer::ViewerBase::SingleThreaded;
//    while (arguments.read("--CullDrawThreadPerContext")) threadingModel = osgViewer::ViewerBase::CullDrawThreadPerContext;
//    while (arguments.read("--DrawThreadPerContext")) threadingModel = osgViewer::ViewerBase::DrawThreadPerContext;
//    while (arguments.read("--CullThreadPerCameraDrawThreadPerContext")) threadingModel = osgViewer::ViewerBase::CullThreadPerCameraDrawThreadPerContext;

    app::Application app(argc, argv);
    QObject::connect(&app, SIGNAL(aboutToQuit()), &app, SLOT(quittingSlot()));

    app.initializeSpaceball();

    app.getMainWindow()->showMaximized();
    
    //build an empty project.
    msg::Message messageOut;
    
    messageOut.mask = msg::Request | msg::NewProject;
    msg::dispatch().messageInSlot(messageOut);
    
    messageOut.mask = msg::Request | msg::Construct | msg::Box;
    msg::dispatch().messageInSlot(messageOut);
    
    messageOut.mask = msg::Request | msg::Construct | msg::Cylinder;
    msg::dispatch().messageInSlot(messageOut);
    
    messageOut.mask = msg::Request | msg::Construct | msg::Cone;
    msg::dispatch().messageInSlot(messageOut);
    
    messageOut.mask = msg::Request | msg::Construct | msg::Sphere;
    msg::dispatch().messageInSlot(messageOut);
    
    messageOut.mask = msg::Request | msg::ViewFit;
    msg::dispatch().messageInSlot(messageOut);
    
    return app.exec();
}
