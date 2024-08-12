cmake_minimum_required(VERSION 3.0)

project(imgui)

include_directories(../glfw/include)
include_directories(../imgui)
include_directories(../imgui/examples)
include_directories(../imgui/examples/example_null)

file (GLOB imgui_sourceFiles ../imgui/backends/imgui_impl_glfw.cpp
                             ../imgui/backends/imgui_impl_opengl3.cpp
                             ../imgui/imgui.cpp
                             ../imgui/imgui_draw.cpp
                             ../imgui/imgui_tables.cpp
                             ../imgui/imgui_widgets.cpp)

add_library(imgui ${imgui_sourceFiles})


