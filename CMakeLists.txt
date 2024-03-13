cmake_minimum_required(VERSION 3.25)

# ---- Project ----

include(cmake/DynamicVersion.cmake)
dynamic_version(PROJECT_PREFIX filestorm_)

project(
  filestorm
  VERSION ${PROJECT_VERSION}
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.11.1")
CPMAddPackage(
  NAME nlohmann_json
  GITHUB_REPOSITORY nlohmann/json
  VERSION 3.9.1
  OPTIONS "JSON_BuildTests OFF"
)
CPMAddPackage(
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 2.2.1
  OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL YES"
)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----
# add_library(filestorm ${headers} ${sources}) Note: for header-only libraries change all PUBLIC
# flags to INTERFACE and create an interface target: add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

# Link dependencies
target_link_libraries(
  ${PROJECT_NAME} PRIVATE fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json cxxopts
)

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

# the location where the project's version header will be placed should match the project's regular
# header paths
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "spdlog;fmt" CPACK YES
)

file(GLOB standalone_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/standalone/source/*.cpp)

add_executable(${PROJECT_NAME}Executable ${standalone_sources})
set_target_properties(${PROJECT_NAME}Executable PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "filestorm")

target_link_libraries(
  ${PROJECT_NAME}Executable ${PROJECT_NAME} fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json
)

# Installation rules for the executable
install(TARGETS ${PROJECT_NAME}Executable RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                                  COMPONENT "${PROJECT_NAME}_Runtime"
)

set(CPACK_RPM_PACKAGE_REQUIRES "spdlog, fmt")
set(CPACK_RPM_BUILDREQUIRES "cmake, gcc-c++, spdlog-devel, fmt-devel, git")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_RPM_SOURCE_PKG_PACKAGING_INSTALL_PREFIX "/usr")

include(CPack)
