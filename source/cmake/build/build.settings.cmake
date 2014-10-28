# The settings below are combined together to create the final set of settings 
# for CMake + Ninja builds. These settings attempt to match those found in 
# tools/build/basic.settings, which are used for building with scons. 
#
# Currently, this file only includes settings for CMake + Ninja release, debug,
# and graphics builds. This file is included in the CMakeLists.txt files found 
# in the build_release, build_debug, and build_graphics directories. The 
# CMakeLists.txt files only need to specify which mode (i.e., release, debug), 
# extras (i.e., graphics), and compiler (i.e., gcc, clang) should be used. The
# appropriate settings for each mode, extras, and compiler should be 
# implemented here.
#
# Ultimately, it may be more efficient to automate the generation of this 
# build.settings.cmake file so that it does not have to be updated manually 
# every time basic.settings is modified. 
#
# @file source/cmake/build/build.settings.cmake
# @author Caleb W. Geniesse (calebgeniesse@stanford.edu) 


###########################################################################
# GCC #####################################################################
###########################################################################

if( ${COMPILER} STREQUAL "gcc" )
	set( cc
			#-std=c99
			-isystem external/boost_1_55_0/
			-isystem external/include/
			-isystem external/dbio/
	)
	set( cxx
			-std=c++98
			-isystem external/boost_1_55_0/
			-isystem external/include/
			-isystem external/dbio/
	)
	set( compile
			-pipe
			-ffor-scope
	)
	set( warn
			-Wall
			-Wextra
			-pedantic
			-Werror
			-Wno-long-long
			-Wno-strict-aliasing
	)
endif()

# "gcc, 4.4"
if( ${COMPILER} STREQUAL "gcc" AND ${CMAKE_CXX_COMPILER_VERSION} MATCHES ".*4.4(.[0-9])*" )
	list( APPEND warn
			-Wno-uninitialized
	)
endif()

# modes ###################################################################

# "gcc, debug"
if( ${COMPILER} STREQUAL "gcc" AND ${MODE} STREQUAL "debug" )
	list( APPEND compile
			-O0
	)
	list( APPEND mode
			-g
			-ggdb
			-ffloat-store
			#-fstack-check
	)
	list( APPEND defines
			-D_DEBUG
	)
endif()

# "gcc, release"
if( ${COMPILER} STREQUAL "gcc" AND ${MODE} STREQUAL "release" )
	list( APPEND compile
			-O3
			-ffast-math
			-funroll-loops
			-finline-functions
			-finline-limit=20000
			-s
	)
	list( APPEND warn
			-Wno-unused-variable
			-Wno-unused-parameter
	)
	list( APPEND defines
			-DNDEBUG
	)
endif()

# "gcc, 4.4, release"
if( ${COMPILER} STREQUAL "gcc" AND ${MODE} STREQUAL "release" AND ${CMAKE_CXX_COMPILER_VERSION} MATCHES ".*4.4(.[0-9])*" )
	list( REMOVE_ITEM compile
			-finline-limit=20000
	)
	list( APPEND compile
			-finline-limit=487
	)
endif()


###########################################################################
# Clang ###################################################################
###########################################################################

if( ${COMPILER} STREQUAL "clang" )
	set( cc
			#-std=c99
			-isystem external/boost_1_55_0/
			-isystem external/include/
			-isystem external/dbio/
	)
	set( cxx
			-std=c++98
			-isystem external/boost_1_55_0/
			-isystem external/include/
			-isystem external/dbio/
	)
	set( compile
			-pipe
			-Qunused-arguments
			-DUNUSUAL_ALLOCATOR_DECLARATION
			-ftemplate-depth-256
			-stdlib=libstdc++
	)
	set( shlink
			-stdlib=libstdc++
	)
	set( link
			-stdlib=libstdc++
	)
	set( warn
			-W
			-Wall
			-Wextra
			-pedantic
			#-Weverything
			-Werror
			-Wno-long-long
			-Wno-strict-aliasing
			#-Wno-documentation
			#-Wno-padded
			#-Wno-weak-vtables
	)
endif()

# modes ###################################################################

# "clang, debug"
if( ${COMPILER} STREQUAL "clang" AND ${MODE} STREQUAL "debug" )
	list( APPEND compile
			-O0
	)
	list( APPEND mode
			-g
	)
endif()

# "clang, release"
if( ${COMPILER} STREQUAL "clang" AND ${MODE} STREQUAL "release" )
	list( APPEND compile
			-O3
	)
	list( APPEND warn
			-Wno-unused-variable
			-Wno-unused-parameter
	)
	list( APPEND defines
			-DNDEBUG
	)
endif()


###########################################################################
# Extras ###################################################################
###########################################################################

if( EXTRAS )

	# "macos, graphics"
	if( APPLE AND ${EXTRAS} STREQUAL "graphics" )
		find_package( GLUT REQUIRED )
		find_package( OpenGL REQUIRED )
		include_directories( /usr/X11R6/include )
		link_directories( /usr/X11R6/lib )
		list( APPEND defines
				-DGL_GRAPHICS
				-DMAC
		)
		list( APPEND link
				-framework GLUT
				-framework OpenGL
				-dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib
		)
		list( APPEND shlink
				-framework GLUT
				-framework OpenGL
				-dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib
		)
		set( compile
				-pipe
				-ffast-math
				-funroll-loops
				-finline-functions
				-ftemplate-depth=250
				-stdlib=libstdc++
				-mmacosx-version-min=10.6
		)
		set( warn
				-Werror=sign-compare
				-Werror=reorder
				-Werror=address
				-Werror=char-subscripts
				-Werror=comment
				-Werror=nonnull
				-Werror=type-limits
				-Werror=parentheses
				-Werror=ignored-qualifiers
				-Werror=enum-compare
		)
	endif()
	
	# "linux, graphics"
	if( UNIX AND ${EXTRAS} STREQUAL "graphics" )
		list( APPEND defines
				-DGL_GRAPHICS
		)
		list( APPEND link
				-lpthread
				-lGL
				-lGLU
				-lglut
		)
		list( APPEND shlink
				-lpthread
				-lGL
				-lGLU
				-lglut
		)
		set( warn
				-Werror=sign-compare
				-Werror=reorder
				-Werror=address
				-Werror=char-subscripts
				-Werror=comment
				-Werror=nonnull
				-Werror=type-limits
				-Werror=parentheses
				-Werror=ignored-qualifiers
				-Werror=enum-compare
		)
	endif()

endif()


###########################################################################
###########################################################################
###########################################################################

add_definitions( ${defines} )

foreach( flag ${shlink} )
	set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${flag}" )
endforeach()

foreach( flag ${cc} ${cxx} ${compile} ${warn} ${mode} )
	set( COMPILE_FLAGS "${COMPILE_FLAGS} ${flag}" )
endforeach()

foreach( flag ${COMPILE_FLAGS} )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" )
endforeach()