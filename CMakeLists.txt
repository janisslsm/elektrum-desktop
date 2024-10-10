cmake_minimum_required(VERSION 3.27)
project(elektrum)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_subdirectory("external/glfw")

# VCPKG
include(FetchContent)
# Fix warnings about DOWNLOAD_EXTRACT_TIMESTAMP
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
message(STATUS "Fetching vcpkg (2024.09.30)...")
FetchContent_Declare(vcpkg URL "https://github.com/microsoft/vcpkg/archive/refs/tags/2024.09.30.tar.gz")
FetchContent_GetProperties(vcpkg)
if(NOT vcpkg_POPULATED)
    FetchContent_Populate(vcpkg)
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL Darwin AND CMAKE_OSX_ARCHITECTURES STREQUAL "")
        set(CMAKE_OSX_ARCHITECTURES ${CMAKE_HOST_SYSTEM_PROCESSOR} CACHE STRING "" FORCE)
    endif()
    include("${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake")
endif()

# Packages
find_package(nlohmann_json REQUIRED)
find_package(cpr REQUIRED)
find_package(date REQUIRED)

# Target: imgui
set(imgui_SOURCES
	"external/imgui/backends/imgui_impl_glfw.cpp"
	"external/imgui/backends/imgui_impl_glfw.h"
	"external/imgui/backends/imgui_impl_opengl3.cpp"
	"external/imgui/backends/imgui_impl_opengl3.h"
	"external/imgui/backends/imgui_impl_opengl3_loader.h"
	"external/imgui/imconfig.h"
	"external/imgui/imgui.cpp"
	"external/imgui/imgui.h"
	"external/imgui/imgui_demo.cpp"
	"external/imgui/imgui_draw.cpp"
	"external/imgui/imgui_internal.h"
	"external/imgui/imgui_tables.cpp"
	"external/imgui/imgui_widgets.cpp"
	"external/imgui/imstb_rectpack.h"
	"external/imgui/imstb_textedit.h"
	"external/imgui/imstb_truetype.h"
	"external/implot/implot.cpp"
	"external/implot/implot.h"
	"external/implot/implot_internal.h"
	"external/implot/implot_items.cpp"
)

if(WIN32) # windows
	list(APPEND imgui_SOURCES
		"external/imgui/backends/imgui_impl_win32.cpp"
		"external/imgui/backends/imgui_impl_win32.h"
	)
endif()

add_library(imgui STATIC)

target_sources(imgui PRIVATE ${imgui_SOURCES})

target_compile_features(imgui PUBLIC
	cxx_std_20
)

target_include_directories(imgui PUBLIC
	"external/imgui"
	"external/imgui/backends"
	"external/implot"
)

target_link_libraries(imgui PUBLIC
	glfw
)

set(elektrum-desktop_SOURCES
	"src/core.cpp"
)

add_executable(elektrum-desktop)

target_sources(elektrum-desktop PRIVATE ${elektrum-desktop_SOURCES})

target_compile_features(elektrum-desktop PRIVATE
	cxx_std_20
	c_std_99
)

target_link_libraries(elektrum-desktop PRIVATE
	imgui
	cpr::cpr
	nlohmann_json::nlohmann_json
	date::date
)

if(MSVC) # msvc
	target_link_options(elektrum-desktop PRIVATE
		"/SUBSYSTEM:WINDOWS"
	)
endif()