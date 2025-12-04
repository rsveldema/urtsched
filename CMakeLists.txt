cmake_minimum_required(VERSION 3.18)

project(librealtime_scheduler CXX)

include(CTest)

enable_testing()

add_subdirectory(simple-logger)

# Include directories
set(PUBLIC_INCLUDE_DIRECTORIES include/)
set(PRIVATE_INCLUDE_DIRECTORIES src/)

# Source files
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.hpp")
file(GLOB_RECURSE INCLUDE_FILES "include/*.hpp")

add_library(urtsched)

target_compile_features(urtsched PUBLIC cxx_std_23)
target_link_libraries(urtsched slogger)
target_include_directories(urtsched PUBLIC ${PUBLIC_INCLUDE_DIRECTORIES})
target_include_directories(urtsched PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES})

target_sources(urtsched
    PUBLIC FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES ${INCLUDE_FILES}
    PRIVATE ${SOURCE_FILES}
)

install(TARGETS urtsched FILE_SET HEADERS)

# Set target properties for public headers
set_target_properties(urtsched PROPERTIES PUBLIC_HEADER "${INCLUDE_FILES}")

find_package(GTest)

if(GTest_FOUND)
add_subdirectory(tests)
endif()