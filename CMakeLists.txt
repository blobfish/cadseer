cmake_minimum_required(VERSION 2.8)
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
project(cadseer)
SET(CMAKE_CXX_STANDARD 14)
SET(CXX_STANDARD_REQUIRED YES)
SET(CXX_EXTENSIONS NO)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -fext-numeric-literals -frounding-math" )

add_definitions(-DQT_NO_KEYWORDS) #macros clashing
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5X11Extras REQUIRED)
SET(QT_LIBRARIES Qt5::Core Qt5::Widgets Qt5::OpenGL Qt5::X11Extras)
add_definitions(${Qt5Widgets_DEFINITIONS})

find_package(Boost REQUIRED COMPONENTS system graph timer filesystem)

# OCCT_PROJECT_PATH lets user define cmake variables based on a locally built and
# non-installed copy of the OCCT source code. assumes standard OCCT path
# and a 'build' sub directory
# example:
# cmake -DOCCT_PROJECT_PATH="/home/tanderson/Programming/occt" ../source
# don't forget about the library runtime path:
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/tanderson/Programming/occt/build/lin64/gcc/libd
SET(OCCT_PROJECT_PATH "" CACHE PATH "Path to OpenCasCade project path")
MARK_AS_ADVANCED(OCCT_PROJECT_PATH)
IF(OCCT_PROJECT_PATH)
    MESSAGE(STATUS "OCCT_PROJECT_PATH is set to: ${OCCT_PROJECT_PATH}")
    
    set(OCCT_BUILD_INCLUDE "${OCCT_PROJECT_PATH}/build/include/opencascade")
    if(NOT EXISTS ${OCCT_BUILD_INCLUDE})
        message(FATAL_ERROR "build include path not valid. ${OCCT_BUILD_INCLUDE}")
    endif(NOT EXISTS ${OCCT_BUILD_INCLUDE})
    set(OCCT_INCLUDE_DIRS ${OCCT_INCLUDE_DIRS} ${OCCT_BUILD_INCLUDE})
    
    include_directories(${OCCT_INCLUDE_DIRS})
    
    set(OCCT_BUILD_LIB "${OCCT_PROJECT_PATH}/build/lin64/gcc/lib")
    if(NOT EXISTS ${OCCT_BUILD_LIB})
        message(FATAL_ERROR "build library path not valid. ${OCCT_BUILD_LIB}")
    endif(NOT EXISTS ${OCCT_BUILD_LIB})
    
    link_directories(${OCCT_BUILD_LIB})
    
    SET(OCE_LIBRARIES
        TKFillet
        TKMesh
        TKernel
        TKG2d
        TKG3d
        TKMath
        TKIGES
        TKSTL
        TKShHealing
        TKXSBase
        TKBin
        TKBool
        TKBO
        TKBRep
        TKTopAlgo
        TKGeomAlgo
        TKGeomBase
        TKOffset
        TKPrim
        TKSTEP
        TKSTEPBase
        TKSTEPAttr
        TKFeat
    )
ELSE(OCCT_PROJECT_PATH)
    MESSAGE(STATUS "OCCT_PROJECT_PATH is empty. Using find_package for OCCT")
    find_package(OpenCasCade REQUIRED)
    SET(OCE_LIBRARIES ${OCC_LIBRARIES})
ENDIF(OCCT_PROJECT_PATH)

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
    
    set(OSG_SOURCE_INCLUDE "${OSG_PROJECT_PATH}/include")
    message(STATUS "${OSG_SOURCE_INCLUDE}")
    if(NOT EXISTS ${OSG_SOURCE_INCLUDE})
        message(FATAL_ERROR "include path not valid. ${OSG_SOURCE_INCLUDE}")
    endif(NOT EXISTS ${OSG_SOURCE_INCLUDE})
    set(OPENSCENEGRAPH_INCLUDE_DIRS ${OPENSCENEGRAPH_INCLUDE_DIRS} ${OSG_SOURCE_INCLUDE})
    
    set(OSG_BUILD_INCLUDE "${OSG_PROJECT_PATH}/build/include")
    if(NOT EXISTS ${OSG_BUILD_INCLUDE})
        message(FATAL_ERROR "build include path not valid. ${OSG_BUILD_INCLUDE}")
    endif(NOT EXISTS ${OSG_BUILD_INCLUDE})
    set(OPENSCENEGRAPH_INCLUDE_DIRS ${OPENSCENEGRAPH_INCLUDE_DIRS} ${OSG_BUILD_INCLUDE})
    
    include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})
    
    set(OSG_BUILD_LIB "${OSG_PROJECT_PATH}/build/lib")
    if(NOT EXISTS ${OSG_BUILD_LIB})
        message(FATAL_ERROR "build library path not valid. ${OSG_BUILD_LIB}")
    endif(NOT EXISTS ${OSG_BUILD_LIB})
    
    link_directories(${OSG_BUILD_LIB})
ELSE(OSG_PROJECT_PATH)
    MESSAGE(STATUS "OSG_PROJECT_PATH is empty. Using find_package for OpenSceneGraph")
    FIND_PACKAGE(OpenSceneGraph REQUIRED)
ENDIF(OSG_PROJECT_PATH)
SET(OPENSCENEGRAPH_LIBRARIES "OpenThreads;osg;osgDB;osgUtil;osgViewer;osgGA;osgText;osgManipulator;osgFX")

# OSGQT_PROJECT_PATH lets user define cmake variables based on a locally built and
# non-installed copy of the osgqt source code. assumes standard osgqt path
# and a 'build' sub directory
# example:
# cmake -DOSGQT_PROJECT_PATH="/home/tanderson/Programming/OpenSceneGraph/osgQt" ../source
# don't forget about the library runtime path:
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/tanderson/Programming/OpenSceneGraph/osgQt/build/lib
SET(OSGQT_PROJECT_PATH "" CACHE PATH "Path to osgQt project path")
MARK_AS_ADVANCED(OSGQT_PROJECT_PATH)
IF(OSGQT_PROJECT_PATH)
    MESSAGE(STATUS "OSGQT_PROJECT_PATH is set to: ${OSGQT_PROJECT_PATH}")
    
    set(OSGQT_SOURCE_INCLUDE "${OSGQT_PROJECT_PATH}/include")
    message(STATUS "${OSGQT_SOURCE_INCLUDE}")
    if(NOT EXISTS ${OSGQT_SOURCE_INCLUDE})
        message(FATAL_ERROR "include path not valid. ${OSGQT_SOURCE_INCLUDE}")
    endif(NOT EXISTS ${OSGQT_SOURCE_INCLUDE})
    set(OSGQT_INCLUDE_DIRS ${OSGQT_INCLUDE_DIRS} ${OSGQT_SOURCE_INCLUDE})
    include_directories(${OSGQT_INCLUDE_DIRS})
    
    set(OSGQT_BUILD_LIB "${OSGQT_PROJECT_PATH}/build/lib")
    if(NOT EXISTS ${OSGQT_BUILD_LIB})
        message(FATAL_ERROR "build library path not valid. ${OSGQT_BUILD_LIB}")
    endif(NOT EXISTS ${OSGQT_BUILD_LIB})
    
    link_directories(${OSGQT_BUILD_LIB})
    
ELSE(OSGQT_PROJECT_PATH)
    MESSAGE(STATUS "OSGQT_PROJECT_PATH is empty. Using find_package for osgQt")
    FIND_PACKAGE(OsgQt REQUIRED)
ENDIF(OSGQT_PROJECT_PATH)
SET(OSGQT_LIBRARIES "osgQt5")

find_package(Spnav)
IF(SPNAV_FOUND)
    add_definitions(-DSPNAV_FOUND)
    include_directories(${SPNAV_INCLUDE_DIR})
ENDIF(SPNAV_FOUND)

find_package(Eigen3 REQUIRED)

find_package(XercesC REQUIRED)
find_package(XSDCXX REQUIRED)

#begin squash
find_package(CGAL COMPONENTS CGAL_Core)
if(CGAL_FOUND)
  message(STATUS "CGAL found. ${CGAL_INCLUDE_DIRS}")
  set(CGAL_DONT_OVERRIDE_CMAKE_FLAGS TRUE CACHE BOOL "Force CGAL to maintain CMAKE flags")
  include(${CGAL_USE_FILE})
else(CGAL_FOUND)
  message(STATUS "CGAL not found. squash will not be built.")
endif(CGAL_FOUND)


SET(IGL_PROJECT_PATH "" CACHE PATH "Path to IGL project")
MARK_AS_ADVANCED(IGL_PROJECT_PATH)
IF(IGL_PROJECT_PATH)
  MESSAGE(STATUS "IGL_PROJECT_PATH is set to: ${IGL_PROJECT_PATH}")

  set(IGL_INCLUDE "${IGL_PROJECT_PATH}/include")
  message(STATUS "${IGL_INCLUDE}")
  if(NOT EXISTS ${IGL_INCLUDE})
    message(STATUS "igl include path not valid. squash will not be built.")
  else(NOT EXISTS ${IGL_INCLUDE})
    message(STATUS "igl include found.")
    set(IGL_INCLUDE_FOUND true)
  endif(NOT EXISTS ${IGL_INCLUDE})
  
  #static lib has problems just go with slow header only for now.
  #assuming 'lib' directory
  set(IGL_LIB "${IGL_PROJECT_PATH}/lib/libigl.a") #not using right now.
#   add_definitions(-DIGL_STATIC_LIBRARY)
  if(NOT EXISTS ${IGL_LIB})
    message(STATUS "igl lib missing. squash will not be built.")
  else(NOT EXISTS ${IGL_LIB})
    message(STATUS "igl lib found.")
    set(IGL_LIB_FOUND true)
  endif(NOT EXISTS ${IGL_LIB})
ELSE(IGL_PROJECT_PATH)
  MESSAGE(STATUS "IGL_PROJECT_PATH is empty. squash will not be built.")
ENDIF(IGL_PROJECT_PATH)


SET(NETGEN_PROJECT_PATH "" CACHE PATH "Path to Netgen project")
MARK_AS_ADVANCED(NETGEN_PROJECT_PATH)
IF(NETGEN_PROJECT_PATH)
  MESSAGE(STATUS "NETGEN_PROJECT_PATH is set to: ${NETGEN_PROJECT_PATH}")

  set(NETGEN_INCLUDE "${NETGEN_PROJECT_PATH}/nglib")
  message(STATUS "${NETGEN_INCLUDE}")
  if(NOT EXISTS ${NETGEN_INCLUDE})
    message(STATUS "netgen include path not valid. squash will not be built.")
  else(NOT EXISTS ${NETGEN_INCLUDE})
    message(STATUS "netgen include found.")
    set(NETGEN_INCLUDE_FOUND true)
  endif(NOT EXISTS ${NETGEN_INCLUDE})
  
  #assuming 'lib' directory
  set(NETGEN_LIB "${NETGEN_PROJECT_PATH}/build/nglib/libnglib.so")
  if(NOT EXISTS ${NETGEN_LIB})
    message(STATUS "netgen lib missing. squash will not be built.")
  else(NOT EXISTS ${NETGEN_LIB})
    message(STATUS "netgen lib found.")
    set(NETGEN_LIB_FOUND true)
  endif(NOT EXISTS ${NETGEN_LIB})
ELSE(NETGEN_PROJECT_PATH)
  MESSAGE(STATUS "NETGEN_PROJECT_PATH is empty. squash will not be built.")
ENDIF(NETGEN_PROJECT_PATH)

if(IGL_INCLUDE_FOUND AND IGL_LIB_FOUND AND NETGEN_INCLUDE_FOUND AND NETGEN_LIB_FOUND AND CGAL_FOUND)
  message(STATUS "squash is being built.")
  set(SQUASH_INCLUDE_DIRS ${CGAL_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR} ${IGL_INCLUDE} ${NETGEN_INCLUDE})
  set(SQUASH_LIBRARIES ${CGAL_LIBRARIES} ${IGL_LIB} ${NETGEN_LIB})
  add_definitions(-DSQUASH_PRESENT)
  
  set (SQUASH_SOURCES
    squash/squash.cpp
    squash/igl.cpp
    squash/netgen.cpp
    squash/support.cpp
    squash/tools.cpp
  )
else(IGL_INCLUDE_FOUND AND IGL_LIB_FOUND AND NETGEN_INCLUDE_FOUND AND NETGEN_LIB_FOUND AND CGAL_FOUND)
  message(STATUS "squash is not being built.")
endif(IGL_INCLUDE_FOUND AND IGL_LIB_FOUND AND NETGEN_INCLUDE_FOUND AND NETGEN_LIB_FOUND AND CGAL_FOUND)
#end squash

#begin spreadsheet
find_package(ZLIB REQUIRED)
find_package(LibZip REQUIRED)
#end spreadsheet

add_subdirectory(project/libgit2pp)

SET(LIBRARY_INCLUDE_PATHS
    ${Qt5Widgets_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR} #needed to find moc files.
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
    ${OSGQT_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${XSDCXX_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/project/libgit2pp/libgit2/include/
    ${SQUASH_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${LIBZIP_INCLUDE_DIR_ZIP}
    ${CMAKE_SOURCE_DIR}/libreoffice/libzippp/src/
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
    feature/seershapeinfo.cpp
    feature/datumplane.cpp
    feature/message.cpp
    feature/hollow.cpp
    feature/pick.cpp
    feature/oblong
    feature/oblongbuilder.cpp
    feature/shapehistory.cpp
    feature/extract.cpp
    feature/squash.cpp
    feature/strip.cpp
    feature/nest.cpp
    feature/dieset.cpp
    feature/quote.cpp
    )
    
SET(ANNEX_SOURCES
    annex/base.cpp
    annex/csysdragger.cpp
    annex/seershape.cpp
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
    selection/manager.cpp
    selection/message.cpp
    selection/container.cpp
    selection/overlayhandler.cpp
    selection/interpreter.cpp
    selection/visitors.cpp
    selection/definitions.cpp
    selection/intersection.cpp
    )
    
SET(TOOLS_SOURCES
    tools/idtools.cpp
    tools/infotools.cpp
    tools/occtools.cpp
    )
    
SET(DIALOG_SOURCES
    dialogs/parameter.cpp
    dialogs/checkgeometry.cpp
    dialogs/widgetgeometry.cpp
    dialogs/blend.cpp
    dialogs/expressionedit.cpp
    dialogs/strip.cpp
    dialogs/selectionbutton.cpp
    dialogs/quote.cpp
    dialogs/preferences.cpp
    dialogs/project.cpp
    )
    
SET(PREFERENCE_SOURCES
    preferences/preferencesXML.cpp
    preferences/manager.cpp
    )
    
SET(PROJECT_SOURCES
    project/project.cpp
    project/stow.cpp
    project/message.cpp
    project/gitmanager.cpp
    project/featureload.cpp
    project/serial/xsdcxxoutput/project.cpp
    project/serial/xsdcxxoutput/view.cpp
    project/serial/xsdcxxoutput/featurebase.cpp
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
    project/serial/xsdcxxoutput/featureoblong.cpp
    project/serial/xsdcxxoutput/featureextract.cpp
    project/serial/xsdcxxoutput/featuresquash.cpp
    project/serial/xsdcxxoutput/featurenest.cpp
    project/serial/xsdcxxoutput/featuredieset.cpp
    project/serial/xsdcxxoutput/featurestrip.cpp
    project/serial/xsdcxxoutput/featurequote.cpp
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
    command/extract.cpp
    command/featurereposition.cpp
    command/squash.cpp
    command/strip.cpp
    command/nest.cpp
    command/dieset.cpp
    command/quote.cpp
    command/isolate.cpp
    command/measurelinear.cpp
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
    viewer/widget.cpp
    viewer/spaceballmanipulator.cpp
    viewer/spaceballqevent.cpp
    viewer/gleventwidget.cpp
    viewer/spaceballosgevent.cpp
    viewer/textcamera.cpp
    viewer/overlay.cpp
    viewer/message.cpp
)

SET(LIBREOFFICE_SOURCES
    libreoffice/odshack.cpp
    libreoffice/libzippp/src/libzippp.cpp
)

SET(CADSEER_SOURCES
    main.cpp
    dagview/view.cpp
    dagview/model.cpp
    dagview/stow.cpp
    dagview/rectitem.cpp
    globalutilities.cpp
    ${MODELVIZ_SOURCES}
    ${TESTING_SOURCES}
    ${GESTURE_SOURCES}
    ${MESSAGE_SOURCES}
    ${FEATURE_SOURCES}
    ${ANNEX_SOURCES}
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
    ${SQUASH_SOURCES}
    ${LIBREOFFICE_SOURCES}
    )

SET(CADSEER_MOC_HDRS
    viewer/widget.h
    application/mainwindow.h
    dagview/view.h
    dagview/model.h
    application/application.h
    application/splitterdecorated.h
    application/infowindow.h
    application/incrementwidget.h
    selection/manager.h
    dialogs/checkgeometry.h
    dialogs/widgetgeometry.h
    dialogs/blend.h
    dialogs/expressionedit.h
    dialogs/preferences.h
    dialogs/parameter.h
    expressions/widget.h
    expressions/tablemodel.h
    expressions/tableview.h
    dialogs/strip.h
    dialogs/selectionbutton.h
    dialogs/quote.h
    dialogs/project.h
    )

SET(CADSEER_LIBRARIES
    ${QT_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENSCENEGRAPH_LIBRARIES}
    ${OSGQT_LIBRARIES}
    ${OCE_LIBRARIES}
    ${XERCESC_LIBRARIES}
    ${SPNAV_LIBRARIES}
    ${SQUASH_LIBRARIES}
    git2pp
    ${ZLIB_LIBRARIES}
    ${LIBZIP_LIBRARY}
    )

SET(CADSEER_UIS
    application/mainwindow.ui
    dialogs/preferences.ui
    dialogs/project.ui
    )

SET(CADSEER_RESOURCES
    resources.qrc
    )

qt5_add_resources(CADSEER_RESOURCES_RCC ${CADSEER_RESOURCES})
qt5_wrap_ui(CADSEER_UIS_WRAPPED ${CADSEER_UIS} )
qt5_wrap_cpp(CADSEER_MOC_SRCS ${CADSEER_MOC_HDRS} ${CADSEER_UIS_WRAPPED})
add_executable(cadseer ${CADSEER_SOURCES} ${CADSEER_MOC_SRCS} ${CADSEER_RESOURCES_RCC})
TARGET_LINK_LIBRARIES(cadseer ${CADSEER_LIBRARIES})
