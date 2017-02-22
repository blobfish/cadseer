cmake_minimum_required(VERSION 2.8)
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
project(cadseer)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wshadow" )

add_definitions(-DQT_NO_KEYWORDS) #macros clashing
find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)
set(QT_USE_QTOPENGL TRUE)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

find_package(Boost REQUIRED COMPONENTS system graph timer)

find_package(OpenCasCade REQUIRED)
SET(OCE_LIBRARIES ${OCC_LIBRARIES})

# OSG_PROJECT_PATH lets user define cmake variables based on a locally built and
# non-installed copy of the openscenegraph source code. assumes standard osg path
# and a 'build' sub directory
# example:
# cmake -DOSG_PROJECT_PATH="/home/tanderson/Programming/OpenSceneGraph/OpenSceneGraphGit/source" ../source
# don't forget about the library runtime path:
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/tanderson/Programming/OpenSceneGraph/OpenSceneGraphGit/build/lib
SET(OSG_PROJECT_PATH "" CACHE PATH "Path to OpenSceneGraph project path")
MARK_AS_ADVANCED(OSG_PROJECT_PATH)
IF(OSG_PROJECT_PATH)
    MESSAGE(STATUS "OSG_PROJECT_PATH is set to: ${OSG_PROJECT_PATH}")
    
    set(SOURCE_INCLUDE "${OSG_PROJECT_PATH}/include")
    message(STATUS "${SOURCE_INCLUDE}")
    if(NOT EXISTS ${SOURCE_INCLUDE})
        message(FATAL_ERROR "include path not valid. ${SOURCE_INCLUDE}")
    endif(NOT EXISTS ${SOURCE_INCLUDE})
    set(OPENSCENEGRAPH_INCLUDE_DIRS ${OPENSCENEGRAPH_INCLUDE_DIRS} ${SOURCE_INCLUDE})
    
    set(BUILD_INCLUDE "${OSG_PROJECT_PATH}/build/include")
    if(NOT EXISTS ${BUILD_INCLUDE})
        message(FATAL_ERROR "build include path not valid. ${BUILD_INCLUDE}")
    endif(NOT EXISTS ${BUILD_INCLUDE})
    set(OPENSCENEGRAPH_INCLUDE_DIRS ${OPENSCENEGRAPH_INCLUDE_DIRS} ${BUILD_INCLUDE})
    
    include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})
    
    set(BUILD_LIB "${OSG_PROJECT_PATH}/build/lib")
    if(NOT EXISTS ${BUILD_LIB})
        message(FATAL_ERROR "build library path not valid. ${BUILD_LIB}")
    endif(NOT EXISTS ${BUILD_LIB})
    
    link_directories(${BUILD_LIB})
    
ELSE(OSG_PROJECT_PATH)
    MESSAGE(STATUS "OSG_PROJECT_PATH is empty. Using find_package for OpenSceneGraph")
    FIND_PACKAGE(OpenSceneGraph REQUIRED)
ENDIF(OSG_PROJECT_PATH)
SET(OPENSCENEGRAPH_LIBRARIES "OpenThreads;osg;osgDB;osgUtil;osgViewer;osgQt;osgGA;osgText;osgManipulator;osgFX")

find_package(Spnav)
IF(SPNAV_FOUND)
    add_definitions(-DSPNAV_FOUND)
    include_directories(${SPNAV_INCLUDE_DIR})
ENDIF(SPNAV_FOUND)

find_package(Eigen3 REQUIRED)

find_package(XercesC REQUIRED)
find_package(XSDCXX REQUIRED)

add_subdirectory(project/libgit2pp)

SET(LIBRARY_INCLUDE_PATHS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR} #needed to find moc files.
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${XSDCXX_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/project/libgit2pp/libgit2/include/
    )
INCLUDE_DIRECTORIES(SYSTEM ${LIBRARY_INCLUDE_PATHS}) #system suppresses warnings for those libraries.

SET(MODELVIZ_SOURCES
    modelviz/base.cpp
    modelviz/shapegeometry.cpp
    modelviz/datumplane.cpp
    modelviz/hiddenlineeffect.cpp
    modelviz/hiddenlinetechnique.cpp
    )

SET(TESTING_SOURCES
    testing/plotter.cpp
    )

SET(GESTURE_SOURCES
    gesture/gesturehandler.cpp
    gesture/gesturenode.cpp
    gesture/animations.cpp
    )
    
SET(MESSAGE_SOURCES
    message/message.cpp
    message/dispatch.cpp
    message/observer.cpp
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
    feature/chamfer.cpp
    feature/draft.cpp
    feature/cylinderbuilder.cpp
    feature/booleanbase.cpp
    feature/union.cpp
    feature/subtract.cpp
    feature/intersect.cpp
    feature/booleanidmapper.cpp
    feature/booleanoperation.cpp
    feature/intersectionmapping.cpp
    feature/shapecheck.cpp
    feature/seershape.cpp
    feature/seershapeinfo.cpp
    feature/datumplane.cpp
    feature/message.cpp
    feature/hollow.cpp
    feature/pick.cpp
    )
    
SET(EXPRESSION_SOURCES
    expressions/abstractnode.cpp
    expressions/graph.cpp
    expressions/manager.cpp
    expressions/widget.cpp
    expressions/stringtranslator.cpp
    expressions/stringtranslatorstow.cpp
    expressions/tablemodel.cpp
    expressions/tableview.cpp
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
    selection/definitions.cpp
    )
    
SET(TOOLS_SOURCES
    tools/idtools.cpp
    tools/infotools.cpp
    )
    
SET(DIALOG_SOURCES
    dialogs/parameterdialog.cpp
    dialogs/checkgeometry.cpp
    dialogs/widgetgeometry.cpp
    dialogs/blend.cpp
    dialogs/expressionedit.cpp
    )
    
SET(PREFERENCE_SOURCES
    preferences/preferencesXML.cpp
    preferences/manager.cpp
    preferences/dialog.cpp
    )
    
SET(PROJECT_SOURCES
    project/project.cpp
    project/message.cpp
    project/gitmanager.cpp
    project/featureload.cpp
    project/projectdialog.cpp
    project/serial/xsdcxxoutput/project.cpp
    project/serial/xsdcxxoutput/featurebase.cpp
    project/serial/xsdcxxoutput/featurecsysbase.cpp
    project/serial/xsdcxxoutput/featurebox.cpp
    project/serial/xsdcxxoutput/featurecylinder.cpp
    project/serial/xsdcxxoutput/featuresphere.cpp
    project/serial/xsdcxxoutput/featurecone.cpp
    project/serial/xsdcxxoutput/featurebooleanbase.cpp
    project/serial/xsdcxxoutput/featureunion.cpp
    project/serial/xsdcxxoutput/featureintersect.cpp
    project/serial/xsdcxxoutput/featuresubtract.cpp
    project/serial/xsdcxxoutput/featureinert.cpp
    project/serial/xsdcxxoutput/featureblend.cpp
    project/serial/xsdcxxoutput/featurechamfer.cpp
    project/serial/xsdcxxoutput/featuredraft.cpp
    project/serial/xsdcxxoutput/featuredatumplane.cpp
    project/serial/xsdcxxoutput/featurehollow.cpp
    )
    
SET(APPLICATION_SOURCES
    application/mainwindow.cpp
    application/application.cpp
    application/factory.cpp
    application/message.cpp
    application/incrementwidget.cpp
    application/splitterdecorated.cpp
    application/infowindow.cpp
    )
    
SET(COMMAND_SOURCES
    command/manager.cpp
    command/base.cpp
    command/csysedit.cpp
    command/featuretosystem.cpp
    command/systemtofeature.cpp
    command/featuretodragger.cpp
    command/draggertofeature.cpp
    command/checkgeometry.cpp
    command/editcolor.cpp
    command/featurerename.cpp
    command/blend.cpp
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
    library/plabel.cpp
    )
    
SET(VIEWER_SOURCES
    viewer/viewerwidget.cpp
    viewer/spaceballmanipulator.cpp
    viewer/spaceballqevent.cpp
    viewer/gleventwidget.cpp
    viewer/spaceballosgevent.cpp
    viewer/textcamera.cpp
    viewer/overlaycamera.cpp
    viewer/message.cpp
)

SET(CADSEER_SOURCES
    main.cpp
    dagview/dagview.cpp
    dagview/dagmodel.cpp
    dagview/dagmodelgraph.cpp
    dagview/dagrectitem.cpp
    globalutilities.cpp
    ${MODELVIZ_SOURCES}
    ${TESTING_SOURCES}
    ${GESTURE_SOURCES}
    ${MESSAGE_SOURCES}
    ${FEATURE_SOURCES}
    ${EXPRESSION_SOURCES}
    ${SELECTION_SOURCES}
    ${TOOLS_SOURCES}
    ${DIALOG_SOURCES}
    ${PREFERENCE_SOURCES}
    ${PROJECT_SOURCES}
    ${APPLICATION_SOURCES}
    ${LIBRARY_SOURCES}
    ${COMMAND_SOURCES}
    ${VIEWER_SOURCES}
    )

SET(CADSEER_MOC_HDRS
    viewer/viewerwidget.h
    application/mainwindow.h
    dagview/dagview.h
    dagview/dagmodel.h
    application/application.h
    application/splitterdecorated.h
    application/infowindow.h
    application/incrementwidget.h
    selection/manager.h
    dialogs/checkgeometry.h
    dialogs/widgetgeometry.h
    dialogs/blend.h
    dialogs/expressionedit.h
    preferences/dialog.h
    dialogs/parameterdialog.h
    project/projectdialog.h
    expressions/widget.h
    expressions/tablemodel.h
    expressions/tableview.h
    )

SET(CADSEER_LIBRARIES
    ${QT_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENSCENEGRAPH_LIBRARIES}
    ${OCE_LIBRARIES}
    ${XERCESC_LIBRARIES}
    ${SPNAV_LIBRARIES}
    ${ModelViz}
    git2pp
    )

SET(CADSEER_UIS
    application/mainwindow.ui
    preferences/dialog.ui
    project/projectdialog.ui
    )

SET(CADSEER_RESOURCES
    resources.qrc
    )

QT4_ADD_RESOURCES(CADSEER_RESOURCES_RCC ${CADSEER_RESOURCES})
QT4_WRAP_UI(CADSEER_UIS_WRAPPED ${CADSEER_UIS} )
QT4_WRAP_CPP(CADSEER_MOC_SRCS ${CADSEER_MOC_HDRS} ${CADSEER_UIS_WRAPPED})
add_executable(cadseer ${CADSEER_SOURCES} ${CADSEER_MOC_SRCS} ${CADSEER_RESOURCES_RCC})
TARGET_LINK_LIBRARIES(cadseer ${CADSEER_LIBRARIES})
