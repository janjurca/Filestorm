# [`DynamicVersion.cmake`](DynamicVersion.cmake)

Calculate the project version from the git tags or `.git_archival.txt` if the source is not a git repository

## Example

```cmake
cmake_minimum_required(VERSION 3.25)

find_package(CMakeExtraUtils REQUIRED)

include(DynamicVersion)
dynamic_version(PROJECT_PREFIX My_Project_)

project(My_Project
		VERSION ${PROJECT_VERSION})

configure_file(version.cpp.in version.cpp)
add_library(version_lib ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

# Make sure version is re-calculated even if you pass `cmake --build . --target version_lib`
# `My_Project_Version` is automatically generated target with the name from `PROJECT_PREFIX`
add_dependencies(version_lib My_Project_Version)

# Rebuild `version.cpp` whenever the version changes
# `.version` is automatically generated
set_property(SOURCE version.cpp.in APPEND PROPERTY
		OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/.version)
```
