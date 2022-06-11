#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "gdface::jpegwrapper-static" for configuration "Release"
set_property(TARGET gdface::jpegwrapper-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(gdface::jpegwrapper-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "openjp2;turbojpeg-static"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/jpegwrapper-static.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS gdface::jpegwrapper-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_gdface::jpegwrapper-static "${_IMPORT_PREFIX}/lib/jpegwrapper-static.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
