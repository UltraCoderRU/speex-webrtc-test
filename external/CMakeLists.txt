if (NOT MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4018 /wd4244 /wd4267 /wd4305")
endif()

add_subdirectory(SpeexDSP)
