# FindOPENJPEG
# --------
#
# Find the OpenJPEG with CONFIG mode,and set target property INTERFACE_INCLUDE_DIRECTORIES if absent

find_package(OpenJPEG CONFIG)

if(TARGET openjp2)
	# 判断是否有INTERFACE_INCLUDE_DIRECTORIES属性如果没有则添加该属性
	get_target_property(_include openjp2 INTERFACE_INCLUDE_DIRECTORIES)	
	if(_include)
		# do nothing		
	else()
		set_target_properties(openjp2 PROPERTIES  INTERFACE_INCLUDE_DIRECTORIES "${OPENJPEG_INCLUDE_DIRS}")
		message(STATUS "add INTERFACE_INCLUDE_DIRECTORIES property for openjp2")
	endif()
	unset(_include)
endif()


