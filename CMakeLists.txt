cmake_minimum_required(VERSION 2.8)
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
project(cadseer)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wshadow" )

add_definitions(-DQT_NO_KEYWORDS) #macros clashing
find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)
set(QT_USE_QTOPENGL TRUE)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

find_package(Boost REQUIRED COMPONENTS system graph)

find_package(OCE REQUIRED COMPONENTS TKernel TKBRep TKMesh TKFeat TKFillet)
#override the libraries settings.
#message(${OCE_LIBRARIES})
SET(OCE_LIBRARIES "TKernel;TKBRep;TKMesh;TKFeat;TKFillet")

FIND_PACKAGE(OpenSceneGraph REQUIRED)
SET(OPENSCENEGRAPH_LIBRARIES "OpenThreads;osg;osgDB;osgUtil;osgViewer;osgQt;osgGA;osgText;osgManipulator")

find_package(Spnav)
IF(SPNAV_FOUND)
    add_definitions(-DSPNAV_FOUND)
    include_directories(${SPNAV_INCLUDE_DIR})
ENDIF(SPNAV_FOUND)

find_package(Eigen3 REQUIRED)

find_package(XercesC REQUIRED)
find_package(XSDCXX REQUIRED)

SET(LIBRARY_INCLUDE_PATHS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR} #needed to find moc files.
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
    ${OCE_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${XSDCXX_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    )
INCLUDE_DIRECTORIES(SYSTEM ${LIBRARY_INCLUDE_PATHS}) #system suppresses warnings for those libraries.

SET(LOCAL_INCLUDE_PATHS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR} #needed to find moc files.
    )
INCLUDE_DIRECTORIES(${LIBRARY_INCLUDE_PATHS})

SET(MODELVIZ_SOURCES
    modelviz/connector.cpp
    modelviz/shapegeometry.cpp
    )

SET(TESTING_SOURCES
    testing/plotter.cpp
    )

SET(GESTURE_SOURCES
    gesture/gesturehandler.cpp
    gesture/gesturenode.cpp
    )
    
SET(MESSAGE_SOURCES
    message/message.cpp
    message/dispatch.cpp
    )
    
SET(FEATURE_SOURCES
    feature/parameter.cpp
    feature/base.cpp
    feature/csysbase.cpp
    feature/box.cpp
    feature/boxbuilder.cpp
    feature/sphere.cpp
    feature/inert.cpp
    feature/cone.cpp
    feature/conebuilder.cpp
    feature/cylinder.cpp
    feature/blend.cpp
    feature/maps.cpp
    feature/cylinderbuilder.cpp
    feature/union.cpp
    feature/subtract.cpp
    feature/intersect.cpp
    )
    
SET(SELECTION_SOURCES
    selection/eventhandler.cpp
    selection/intersector.cpp
    selection/manager.cpp
    selection/message.cpp
    selection/container.cpp
    selection/overlayhandler.cpp
    selection/interpreter.cpp
    selection/visitors.cpp
    )

SET(DIALOG_SOURCES
    dialogs/boxdialog.cpp
    dialogs/parameterdialog.cpp
    )
    
SET(PREFERENCE_SOURCES
    preferences/preferencesXML.cpp
    preferences/manager.cpp
    preferences/dialog.cpp
    )
    
SET(PROJECT_SOURCES
    project/project.cpp
    project/message.cpp
    )
    
SET(APPLICATION_SOURCES
    application/mainwindow.cpp
    application/application.cpp
    application/factory.cpp
    application/message.cpp
    )
    
SET(COMMAND_SOURCES
    command/manager.cpp
    command/base.cpp
    command/csysedit.cpp
    )
    
SET(LIBRARY_SOURCES
    library/circlebuilder.cpp
    library/circleprojector.cpp
    library/conebuilder.cpp
    library/csysdragger.cpp
    library/cylinderbuilder.cpp
    library/geometrylibrary.cpp
    library/iconbuilder.cpp
    library/rotatecirculardragger.cpp
    library/spherebuilder.cpp
    library/torusbuilder.cpp
    library/lineardragger.cpp
    library/lineardimension.cpp
    library/ipgroup.cpp
    )

SET(CADSEER_SOURCES
    main.cpp
    viewer/viewerwidget.cpp
    dagview/dagview.cpp
    dagview/dagmodel.cpp
    dagview/dagmodelgraph.cpp
    dagview/dagrectitem.cpp
    viewer/spaceballmanipulator.cpp
    viewer/spaceballqevent.cpp
    viewer/gleventwidget.cpp
    viewer/spaceballosgevent.cpp
    globalutilities.cpp
    coordinatesystem.cpp
    viewer/textcamera.cpp
    viewer/overlaycamera.cpp
    ${MODELVIZ_SOURCES}
    ${TESTING_SOURCES}
    ${GESTURE_SOURCES}
    ${MESSAGE_SOURCES}
    ${FEATURE_SOURCES}
    ${SELECTION_SOURCES}
    ${DIALOG_SOURCES}
    ${PREFERENCE_SOURCES}
    ${PROJECT_SOURCES}
    ${APPLICATION_SOURCES}
    ${LIBRARY_SOURCES}
    ${COMMAND_SOURCES}
    )

SET(CADSEER_MOC_HDRS
    viewer/viewerwidget.h
    application/mainwindow.h
    dagview/dagview.h
    dagview/dagmodel.h
    application/application.h
    selection/manager.h
    dialogs/boxdialog.h
    preferences/dialog.h
    dialogs/parameterdialog.h
    )

SET(CADSEER_LIBRARIES
    ${QT_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENSCENEGRAPH_LIBRARIES}
    ${OCE_LIBRARIES}
    ${XERCESC_LIBRARIES}
    ${SPNAV_LIBRARIES}
    ${ModelViz}
    )

SET(CADSEER_UIS
    application/mainwindow.ui
    preferences/dialog.ui
    )

SET(CADSEER_RESOURCES
    resources.qrc
    )

QT4_ADD_RESOURCES(CADSEER_RESOURCES_RCC ${CADSEER_RESOURCES})
QT4_WRAP_UI(CADSEER_UIS_WRAPPED ${CADSEER_UIS} )
QT4_WRAP_CPP(CADSEER_MOC_SRCS ${CADSEER_MOC_HDRS} ${CADSEER_UIS_WRAPPED})
add_executable(cadseer ${CADSEER_SOURCES} ${CADSEER_MOC_SRCS} ${CADSEER_RESOURCES_RCC})
TARGET_LINK_LIBRARIES(cadseer ${CADSEER_LIBRARIES})
