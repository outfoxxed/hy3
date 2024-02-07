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
pkg_check_modules(DEPS REQUIRED hyprland pixman-1 libdrm pango pangocairo)

add_library(hy3 SHARED
	src/main.cpp
	src/dispatchers.cpp
	src/conversions.cpp
	src/Hy3Layout.cpp
	src/Hy3Node.cpp
	src/TabGroup.cpp
	src/SelectionHook.cpp
)

option(HY3_NO_VERSION_CHECK "Disable hyprland version check" FALSE)

if (HY3_NO_VERSION_CHECK)
	target_compile_definitions(hy3 PRIVATE -DHY3_NO_VERSION_CHECK=TRUE)
endif()

target_include_directories(hy3 PRIVATE ${DEPS_INCLUDE_DIRS})

install(TARGETS hy3 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
