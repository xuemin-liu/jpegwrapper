#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "gdface::jpegwrapper-static" for configuration "Debug"
set_property(TARGET gdface::jpegwrapper-static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(gdface::jpegwrapper-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "openjp2;turbojpeg-static"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/jpegwrapper-static.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS gdface::jpegwrapper-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_gdface::jpegwrapper-static "${_IMPORT_PREFIX}/lib/jpegwrapper-static.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)