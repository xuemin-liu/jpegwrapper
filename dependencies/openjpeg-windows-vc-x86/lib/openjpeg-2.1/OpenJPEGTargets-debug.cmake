#----------------------------------------------------------------
# Generated CMake target import file for configuration "DEBUG".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "openjp2" for configuration "DEBUG"
set_property(TARGET openjp2 APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(openjp2 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/openjp2_d.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS openjp2 )
list(APPEND _IMPORT_CHECK_FILES_FOR_openjp2 "${_IMPORT_PREFIX}/lib/openjp2_d.lib" )

# Import target "opj_decompress" for configuration "DEBUG"
set_property(TARGET opj_decompress APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(opj_decompress PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/opj_decompress.exe"
  )

list(APPEND _IMPORT_CHECK_TARGETS opj_decompress )
list(APPEND _IMPORT_CHECK_FILES_FOR_opj_decompress "${_IMPORT_PREFIX}/bin/opj_decompress.exe" )

# Import target "opj_compress" for configuration "DEBUG"
set_property(TARGET opj_compress APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(opj_compress PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/opj_compress.exe"
  )

list(APPEND _IMPORT_CHECK_TARGETS opj_compress )
list(APPEND _IMPORT_CHECK_FILES_FOR_opj_compress "${_IMPORT_PREFIX}/bin/opj_compress.exe" )

# Import target "opj_dump" for configuration "DEBUG"
set_property(TARGET opj_dump APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(opj_dump PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/opj_dump.exe"
  )

list(APPEND _IMPORT_CHECK_TARGETS opj_dump )
list(APPEND _IMPORT_CHECK_FILES_FOR_opj_dump "${_IMPORT_PREFIX}/bin/opj_dump.exe" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
