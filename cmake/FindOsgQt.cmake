FIND_PATH(OSGQT_INCLUDE_DIR GraphicsWindowQt
  /usr/include/osgQt
  /usr/local/include/osgQt
)
FIND_LIBRARY(OSGQT_LIBRARY osgQt
  /usr/lib
  /usr/local/lib
)


include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSGQT REQUIRED_VARS OSGQT_INCLUDE_DIR VERSION_VAR)
