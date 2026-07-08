cmake_minimum_required(VERSION 3.23)

project(HighResTimestampInput VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

set(GODOTCPP_API_VERSION "4.6" CACHE STRING "Target Godot GDExtension API version")

FetchContent_Declare(
  godot-cpp
  GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
  GIT_TAG master
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(godot-cpp)

set(LIB_NAME high_res_timestamp_input)

add_library(${LIB_NAME} SHARED)

target_sources(${LIB_NAME}
  PRIVATE
    src/extension_entry.cpp
    src/register_types.cpp
    src/raw_input_event.cpp
    src/timestamp_input.cpp
)

target_include_directories(${LIB_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(${LIB_NAME} PRIVATE cxx_std_17)

target_link_libraries(${LIB_NAME}
  PRIVATE
    godot::cpp
    comctl32
    user32
)

set_target_properties(${LIB_NAME} PROPERTIES
  OUTPUT_NAME "high_res_timestamp_input.windows.$<IF:$<CONFIG:Debug>,template_debug,template_release>.x86_64"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)
