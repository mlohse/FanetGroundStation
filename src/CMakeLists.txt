# vim:set ts=4 sw=4 noet :

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network DBus SerialPort Positioning)
qt_standard_project_setup()

find_library(BCM2835 bcm2835) # optional, for gpio on RaspberryPi
if ("${BCM2835}" STREQUAL "BCM2835-NOTFOUND")
	message("Library bcm2835 not found.")
else()
	message("Found library bcm2835: ${BCM2835}")
	set(RPI_GPIO 1)
	add_compile_definitions(RPI_GPIO)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

execute_process(COMMAND date +%s
				TIMEOUT 10
				OUTPUT_VARIABLE BUILD_DATE
				OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file(config.h.in config.h)

add_subdirectory(qtsingleapplication)

include_directories(log qtsingleapplication/src gpio)

set(SOURCES
	main.cpp
	application.cpp
	fanetmessagedispatcher.cpp
	log/logger.cpp
	gpio/gpio.cpp
	config/fagsconfig.cpp
	config/radioconfig.cpp
	config/fanetconfig.cpp
	config/stationconfig.cpp
	weatherstation/abstractweatherstation.cpp
	weatherstation/holfuywidget.cpp
	weatherstation/holfuyapi.cpp
	weatherstation/windbirdapi.cpp
	fanet/fanetradio.cpp
	fanet/fanetaddress.cpp
	fanet/fanetprotocolparser.cpp
	fanet/versioncommand.cpp
	fanet/regioncommand.cpp
	fanet/enablecommand.cpp
	fanet/transmitcommand.cpp
	fanet/versionreply.cpp
	fanet/genericreply.cpp
	fanet/transmitreply.cpp
	fanet/fanetpayload.cpp
	fanet/receiveevent.cpp
)

set(HEADERS
	application.h
	fanetmessagedispatcher.h
	log/logger.h
	gpio/gpio.h
	config/fagsconfig.h
	config/radioconfig.h
	config/fanetconfig.h
	config/stationconfig.h
	weatherstation/abstractweatherstation.h
	weatherstation/holfuywidget.h
	weatherstation/holfuyapi.h
	weatherstation/windbirdapi.h
	fanet/fanetradio.h
	fanet/fanetaddress.h
	fanet/fanetprotocolparser.h
	fanet/abstractfanetmessage.h
	fanet/versioncommand.h
	fanet/regioncommand.h
	fanet/enablecommand.h
	fanet/transmitcommand.h
	fanet/versionreply.h
	fanet/genericreply.h
	fanet/transmitreply.h
	fanet/fanetpayload.h
	fanet/receiveevent.h
)

set(EXTRAFILES
	config.h.in
)
add_custom_target(extra-project-files ${EXTRAFILES}) # make extra files show up in QtCreator

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

if ("${BCM2835}" STREQUAL "BCM2835-NOTFOUND")
	target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Network Qt6::DBus Qt6::SerialPort Qt6::Positioning qtsingleapplication)
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Network Qt6::DBus Qt6::SerialPort Qt6::Positioning qtsingleapplication ${BCM2835})
endif()
