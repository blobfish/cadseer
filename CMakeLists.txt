 
cmake_minimum_required(VERSION 2.8)
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
project(cadseer)

find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)
set(QT_USE_QTOPENGL TRUE)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

find_package(Boost REQUIRED COMPONENTS system)

find_package(OCE REQUIRED COMPONENTS TKernel TKBRep TKMesh)
#override the libraries settings.
#message(${OCE_LIBRARIES})
SET(OCE_LIBRARIES "TKernel;TKBRep;TKMesh")

FIND_PACKAGE(OpenSceneGraph REQUIRED)
SET(OPENSCENEGRAPH_LIBRARIES "OpenThreads;osg;osgDB;osgUtil;osgViewer;osgQt;osgGA")

find_package(Spnav)
IF(SPNAV_FOUND)
    add_definitions(-DSPNAV_FOUND)
    include_directories(${SPNAV_INCLUDE_DIR})
ENDIF(SPNAV_FOUND)

SET(ADDITIONAL_INCLUDE_PATHS
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
    ${OCE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR} #needed to find moc files.
    )

INCLUDE_DIRECTORIES(${ADDITIONAL_INCLUDE_PATHS})

SET(CADSEER_SOURCES
    ./main.cpp
    ./viewerwidget.cpp
    ./mainwindow.cpp
    ./spaceballmanipulator.cpp
    ./spaceballqevent.cpp
    ./gleventwidget.cpp
    ./spaceballosgevent.cpp
    ./application.cpp
    )

SET(CADSEER_HDRS
    ./spaceballmanipulator.h
    ./spaceballqevent.h
    ./gleventwidget.h
    ./spaceballosgevent.h
    )

SET(CADSEER_MOC_HDRS
    ./viewerwidget.h
    ./mainwindow.h
    ./application.h
    )

SET(CADSEER_LIBRARIES
    ${QT_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENSCENEGRAPH_LIBRARIES}
    ${OCE_LIBRARIES}
    ${SPNAV_LIBRARIES}
    )

SET(CADSEER_UIS
    ./mainwindow.ui
    )

SET(CADSEER_RESOURCES
    ./resources.qrc
    )

QT4_ADD_RESOURCES(CADSEER_RESOURCES_RCC ${CADSEER_RESOURCES})
QT4_WRAP_UI(CADSEER_UIS_WRAPPED ${CADSEER_UIS} )
QT4_WRAP_CPP(CADSEER_MOC_SRCS ${CADSEER_MOC_HDRS} ${CADSEER_UIS_WRAPPED})
add_executable(cadseer ${CADSEER_SOURCES} ${CADSEER_MOC_SRCS} ${CADSEER_RESOURCES_RCC})
TARGET_LINK_LIBRARIES(cadseer ${CADSEER_LIBRARIES})
