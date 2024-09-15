cmake_minimum_required (VERSION 2.8)
project (Launcher)

option (APIINTERCEPTOR_DUMP_API_CALLS "No need for API dump support" OFF)

# Obtain and cache git commit hash for use in source code.
#
# This has been grabbed from https://jonathanhamberg.com/post/cmake-embedding-git-hash/
execute_process(COMMAND git log -1 --format=%h
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                OUTPUT_VARIABLE GIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE)

add_definitions (-DGIT_HASH=${GIT_HASH})

file(GLOB CommonIncludes   "${Launcher_SOURCE_DIR}/Common/include/*.h")
file(GLOB CommonSources    "${Launcher_SOURCE_DIR}/Common/src/*.cpp")
file(GLOB LauncherIncludes "${Launcher_SOURCE_DIR}/Launcher/include/*.h")
file(GLOB LauncherSources  "${Launcher_SOURCE_DIR}/Launcher/src/*.cpp")
file(GLOB VRPlayerIncludes "${Launcher_SOURCE_DIR}/VRPlayer/include/*.h")
file(GLOB VRPlayerSources  "${Launcher_SOURCE_DIR}/VRPlayer/src/*.cpp")

add_subdirectory(deps/APIInterceptor)
add_subdirectory(deps/glfw)
add_subdirectory(deps/imgui-cmake)
add_subdirectory(deps/LibOVR)

add_library   (Common   STATIC ${CommonIncludes}
                               ${CommonSources})
add_executable(Launcher        ${LauncherIncludes}
                               ${LauncherSources})
add_library   (VRPlayer SHARED ${VRPlayerIncludes}
                               ${VRPlayerSources})

include_directories  ("${APIInterceptor_SOURCE_DIR}")
include_directories  ("${APIInterceptor_SOURCE_DIR}/include")
include_directories  ("${APIInterceptor_SOURCE_DIR}/include/Khronos")
include_directories  ("${APIInterceptor_BINARY_DIR}")
include_directories  ("${Launcher_SOURCE_DIR}/Common/")
include_directories  ("${Launcher_SOURCE_DIR}/Common/include")
include_directories  ("${Launcher_SOURCE_DIR}/deps/")
include_directories  ("${Launcher_SOURCE_DIR}/deps/imgui")
include_directories  ("${Launcher_SOURCE_DIR}/Launcher/include")
include_directories  ("${Launcher_SOURCE_DIR}/VRPlayer/include/")
include_directories  ("${LibOVR_SOURCE_DIR}/Include")

target_link_libraries(Launcher Common Detours LibOVR)
target_link_libraries(VRPlayer APIInterceptor Common glfw imgui LibOVR)

add_dependencies     (Launcher VRPlayer Common)

source_group ("Common include files"     FILES ${CommonIncludes})
source_group ("Common source files"      FILES ${CommonSources})
source_group ("Launcher include files"   FILES ${LauncherIncludes})
source_group ("Launcher source files"    FILES ${LauncherSources})
source_group ("VR Player include files"  FILES ${VRPlayerIncludes})
source_group ("VR Player source files"   FILES ${VRPlayerSources})