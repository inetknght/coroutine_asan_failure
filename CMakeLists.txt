CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

PROJECT(esi-cpp  CXX)

FIND_PACKAGE(OpenSSL REQUIRED)

# Find Boost for subprojects
SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_USE_MULTITHREADED ON)
SET(Boost_USE_STATIC_RUNTIME OFF)
FIND_PACKAGE(Boost 1.81.0 REQUIRED
	COMPONENTS
		program_options
		system
		iostreams
		context
		url
)

INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS})

ADD_LIBRARY(spdlog INTERFACE)
TARGET_INCLUDE_DIRECTORIES(spdlog
	INTERFACE 
	spdlog/include
)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED 20)

# Address Sanitizer - available in clang 3.2+ and gcc 4.8+
# Remember, you can turn on LEAK_SANTIZIER by setting environment variable ASAN_OPTIONS=detect_leaks=1 when running executable.
OPTION(ASAN_BUILD "When on, we will build for and link against Address Sanitizer" off)
IF(ASAN_BUILD)
        MESSAGE(STATUS "Address Sanitizer libraries selected")
        ADD_COMPILE_OPTIONS(-fsanitize=address -fno-omit-frame-pointer)
        ADD_LINK_OPTIONS(-fsanitize=address -fno-omit-frame-pointer)
ENDIF()

# -Werror=switch-enum
ADD_COMPILE_OPTIONS(
	-Werror=all -pedantic-errors
	-Werror=unknown-pragmas
	-Werror=return-type
	-Werror=strict-aliasing
	-Wimplicit-fallthrough
	-Wno-long-long
	-Wno-unused-function
	-Wno-vla
)

ADD_EXECUTABLE(
	example
	main.cpp
)

TARGET_LINK_LIBRARIES(
	example
	${Boost_LIBRARIES}
	${OPENSSL_LIBRARIES}
	spdlog
)
