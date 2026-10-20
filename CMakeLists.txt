cmake_minimum_required(VERSION 3.22)

# Default the Visual Studio generator to the v143 toolset (VS 2022) unless the
# user selected one explicitly with -T.
if(CMAKE_GENERATOR MATCHES "Visual Studio" AND NOT CMAKE_GENERATOR_TOOLSET)
    set(CMAKE_GENERATOR_TOOLSET "v143" CACHE STRING "Toolset for Visual Studio generators")
endif()

project(AudioProcessor
    VERSION 0.1.0
    DESCRIPTION "Audio Processor - VST3 utility effect plugin"
    LANGUAGES C CXX)

if(MSVC AND MSVC_VERSION LESS 1930)
    message(FATAL_ERROR "MSVC v143 (Visual Studio 2022) is required; found MSVC ${MSVC_VERSION}.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# VST3 SDK acquisition (design D2)
#
# FetchContent pulls a pinned v3.7.x release tag; -DVST3_SDK_DIR=<path> points
# the build at an existing checkout instead (offline / local development).
# ---------------------------------------------------------------------------
set(VST3_SDK_DIR "" CACHE PATH "Existing VST3 SDK checkout; when set, FetchContent is skipped")
set(VST3_SDK_TAG "v3.7.9_build_61" CACHE STRING "Pinned VST3 SDK release tag")

# Keep the SDK subtree to the libraries this plugin consumes. The validator
# runs explicitly in CI (task 1.3), not as a post-build step, and builds must
# not touch the system VST3 folder: the bundle stays under <build>/VST3.
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)

if(VST3_SDK_DIR)
    set(VST3SDK_ROOT "${VST3_SDK_DIR}")
    add_subdirectory("${VST3SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk-build")
else()
    include(FetchContent)
    FetchContent_Declare(vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG "${VST3_SDK_TAG}"
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(vst3sdk)
    set(VST3SDK_ROOT "${vst3sdk_SOURCE_DIR}")
endif()

# ---------------------------------------------------------------------------
# Plugin target: emits AudioProcessor.vst3 under <build>/VST3/.
# smtg_add_vst3plugin (defined by the SDK's CMake above) creates the module
# target and the Windows bundle/moduleinfo scaffolding.
# ---------------------------------------------------------------------------
smtg_add_vst3plugin(AudioProcessor
    SOURCES_LIST
        source/factory.cpp
        source/processor.cpp
        source/controller.cpp
)

# smtg_add_vst3plugin creates the module target but does not link the SDK
# libraries (see the SDK's own example CMakeLists).
target_link_libraries(AudioProcessor PRIVATE sdk)

set_target_properties(AudioProcessor PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
