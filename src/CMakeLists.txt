
set(TARGET_NAME speex_webrtc_test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h)
file(GLOB_RECURSE RESOURCES *.qrc)

if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
	add_executable(${TARGET_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})
else()
	add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

target_link_libraries(${TARGET_NAME}
	speexdsp
	WebRTC
	Qt5::Core
	Qt5::Widgets
	Qt5::Multimedia
	Threads::Threads
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

if (WIN32)
	add_custom_target(deploy)
	get_filename_component(QT_DIR "${Qt5_DIR}/../../.." ABSOLUTE)
	set(DEPLOYQT_PATH "${QT_DIR}/bin/windeployqt.exe")

	set(DEPLOY_DIR "${CMAKE_BINARY_DIR}/deploy")
	add_custom_command(TARGET deploy POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOY_DIR})
	add_custom_command(TARGET deploy POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${TARGET_NAME}>" "${DEPLOY_DIR}")

	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		set(BUILD_TYPE_KEY "--release")
	else()
		set(BUILD_TYPE_KEY "--debug")
	endif()

	set(DEPLOY_COMMAND COMMAND ${DEPLOYQT_PATH} ${DEPLOY_DIR} ${BUILD_TYPE_KEY} --verbose 0)
	add_custom_command(TARGET deploy POST_BUILD COMMAND ${DEPLOY_COMMAND} WORKING_DIRECTORY ${DEPLOY_DIR})
endif()
