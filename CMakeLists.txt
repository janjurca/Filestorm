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

# ---- Optional engines ----

option(ENABLE_SYNC "Build and link with the sync IO engine" ON)

if(UNIX AND NOT APPLE)
  option(ENABLE_LIBAIO "Build and link the libaio IO engine" ON)
  option(ENABLE_IOURING "Build and link the io_uring IO engine" OFF)
else()
  option(ENABLE_LIBAIO "Build and link the libaio IO engine" OFF)
  option(ENABLE_IOURING "Build and link the io_uring IO engine" OFF)
endif()

# ---- Dependencies via CPM or system ----

option(USE_CPM "Use CPM for dependencies" OFF)

if(USE_CPM)
  include(cmake/CPM.cmake)
  CPMAddPackage("gh:fmtlib/fmt#10.2.1")
  CPMAddPackage("gh:nlohmann/json@3.10.5")
  CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    VERSION 1.14.1
  )
  CPMAddPackage(
    GITLAB_REPOSITORY libeigen/eigen
    GIT_TAG 3.4.0
    OPTIONS "EIGEN_BUILD_DOC OFF" "EIGEN_BUILD_PKGCONFIG OFF" "EIGEN_BUILD_DOC OFF"
  )

else()
  find_package(fmt REQUIRED)
  find_package(spdlog REQUIRED)
  find_package(nlohmann_json REQUIRED)
  find_package(Eigen3 REQUIRED)
endif()

# ---- Collect sources ----

file(GLOB_RECURSE common_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE all_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
list(FILTER all_sources EXCLUDE REGEX ".*/ioengines/engines/.*")
set(common_sources ${all_sources})

set(engine_sources "")

if(ENABLE_SYNC)
  list(APPEND engine_sources "${CMAKE_CURRENT_SOURCE_DIR}/source/ioengines/engines/sync.cpp")
endif()
if(ENABLE_LIBAIO)
  find_library(LIBAIO_LIB aio REQUIRED)
  link_libraries(${LIBAIO_LIB})
  list(APPEND engine_sources "${CMAKE_CURRENT_SOURCE_DIR}/source/ioengines/engines/libaio.cpp")
endif()
if(ENABLE_IOURING)
  find_library(IOURING_LIB uring REQUIRED)
  link_libraries(${IOURING_LIB})
  list(APPEND engine_sources "${CMAKE_CURRENT_SOURCE_DIR}/source/ioengines/engines/iouring.cpp")
endif()

# ---- Build filestorm object library ----

add_library(${PROJECT_NAME} OBJECT ${common_headers} ${common_sources} ${engine_sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

# Link dependencies
if(USE_CPM)
  target_link_libraries(
    ${PROJECT_NAME} PRIVATE fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json Eigen3::Eigen
  )
else()
  target_link_libraries(
    ${PROJECT_NAME} PRIVATE fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json Eigen3::Eigen
  )
endif()

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Package metadata ----

# Collect the names of the system‐package deps
set(PACKAGE_DEPS "spdlog;fmt;eigen3")
if(ENABLE_LIBAIO)
  list(APPEND PACKAGE_DEPS "libaio")
endif()
if(ENABLE_IOURING)
  list(APPEND PACKAGE_DEPS "liburing")
endif()

include(cmake/PackageProject.cmake)
packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  VERSION_HEADER "${PROJECT_NAME}/version.h"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "${PACKAGE_DEPS}" CPACK YES
)

# ---- Standalone executable ----

file(GLOB standalone_sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/standalone/source/*.cpp"
)

add_executable(${PROJECT_NAME}Executable ${standalone_sources})
set_target_properties(${PROJECT_NAME}Executable PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "filestorm")

if(USE_CPM)
  target_link_libraries(
    ${PROJECT_NAME}Executable ${PROJECT_NAME} fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json
    Eigen3::Eigen
  )
else()
  target_link_libraries(
    ${PROJECT_NAME}Executable ${PROJECT_NAME} fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json
    Eigen3::Eigen
  )
endif()

if(USE_CPM)
  install(
    TARGETS fmt spdlog nlohmann_json
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

# Installation rules for the executable
install(TARGETS ${PROJECT_NAME}Executable RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                                  COMPONENT "${PROJECT_NAME}_Runtime"
)

if(USE_CPM)
  set(CPACK_RPM_BUILDREQUIRES "cmake, gcc-c++, git")
else()
  set(CPACK_RPM_PACKAGE_REQUIRES "spdlog, fmt, json-devel, eigen3")
  set(CPACK_RPM_BUILDREQUIRES
      "cmake, gcc-c++, spdlog-devel, fmt-devel, git, json-devel, eigen3-devel"
  )
endif()

if(ENABLE_LIBAIO)
  set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, libaio")
  set(CPACK_RPM_BUILDREQUIRES "${CPACK_RPM_BUILDREQUIRES}, libaio-devel")
endif()

if(ENABLE_IOURING)
  set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, liburing")
  set(CPACK_RPM_BUILDREQUIRES "${CPACK_RPM_BUILDREQUIRES}, liburing-devel")
endif()

set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_RPM_SOURCE_PKG_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
include(CPack)
