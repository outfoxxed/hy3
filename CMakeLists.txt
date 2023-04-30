cmake_minimum_required(VERSION 3.19)
project(Hy3 VERSION "0.1")
set(CMAKE_CXX_STANDARD 23)
add_compile_definitions(WLR_USE_UNSTABLE)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-value -Wno-missing-field-initializers -Wno-narrowing -Wno-pointer-arith)

# nix workaround
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED pixman-1 libdrm)

if(DEFINED ENV{HYPRLAND_INCLUDE})
	set(HYPRLAND_INCLUDE_DIRS $ENV{HYPRLAND_INCLUDE})
	message(STATUS "Using provided hyprland include dir " ${HYPRLAND_INCLUDE_DIRS} " instead of pkg-config")
else()
	pkg_check_modules(HYPRLAND REQUIRED hyprland)
endif()

add_library(hy3 SHARED
	src/main.cpp
	src/Hy3Layout.cpp
	src/SelectionHook.cpp
)

target_include_directories(hy3 PRIVATE ${HYPRLAND_INCLUDE_DIRS} ${DEPS_INCLUDE_DIRS})

install(TARGETS hy3 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
