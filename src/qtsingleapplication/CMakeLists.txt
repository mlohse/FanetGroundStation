# vim:set ts=4 sw=4 noet :

find_package(Qt6 REQUIRED COMPONENTS Core Network)
#find_package(Qt5Core)
#find_package(Qt5Network)

project(qtsingleapplication)

SET(SOURCES
src/qtlocalpeer.cpp
src/qtlockedfile.cpp
#src/qtsingleapplication.cpp
src/qtsinglecoreapplication.cpp
)

IF(UNIX)
	SET(SOURCES ${SOURCES}
		    src/qtlockedfile_unix.cpp
	)
ELSE(UNIX)
	SET(SOURCES ${SOURCES}
	 	    src/qtlockedfile_win.cpp
	)
ENDIF(UNIX)

SET(HEADERS
src/qtlocalpeer.h
src/qtlockedfile.h
#src/qtsingleapplication.h
src/qtsinglecoreapplication.h
)

add_library(qtsingleapplication STATIC ${SOURCES} ${HEADERS})
IF(UNIX AND NOT APPLE)
	SET_TARGET_PROPERTIES(qtsingleapplication PROPERTIES COMPILE_FLAGS -fPIC)
ENDIF(UNIX AND NOT APPLE)
TARGET_LINK_LIBRARIES(qtsingleapplication PRIVATE Qt6::Core Qt6::Network)

IF (MSVC_IDE)
	ADD_CUSTOM_COMMAND(TARGET qtsingleapplication
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/qtsingleapplication.lib" ${CMAKE_BINARY_DIR}
	)
ENDIF (MSVC_IDE)
