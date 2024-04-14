cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(keep-building)

get_filename_component(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third-party" ABSOLUTE)
find_path(RL_INCLUDE_DIR NAMES raylib.h PATHS ${THIRD_PARTY_DIR}/raylib/include NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
find_path(FLECS_INCLUDE_DIR NAMES flecs.h PATHS ${THIRD_PARTY_DIR}/flecs NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

add_library(FLECS_LIBRARY ${THIRD_PARTY_DIR}/flecs/flecs.c)

if (EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -s STACK_SIZE=1mb -Dflecs_EXPORTS -sALLOW_MEMORY_GROWTH")
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so that you can directly open it.
    
    find_library(RL_LIBRARY NAMES raylib PATHS ${THIRD_PARTY_DIR}/raylib/lib NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
else()
    find_library(RL_LIBRARY NAMES raylib PATHS /opt/homebrew/opt/raylib/lib)
    message("RL_INCLUDE_DIR: ${RL_LIBRARY}")

    file(GLOB AUDIO_FILES ${CMAKE_SOURCE_DIR}/resources/Audio)

    file(COPY ${AUDIO_FILES} DESTINATION ${CMAKE_BINARY_DIR}/resources)
endif()

include_directories(${RL_INCLUDE_DIR} ${FLECS_INCLUDE_DIR} src/include)

add_executable(keep-building src/main.cpp)

if(EMSCRIPTEN)
    set_target_properties(keep-building PROPERTIES LINK_FLAGS "--preload-file ${CMAKE_SOURCE_DIR}/resources/Audio@resources/Audio")
endif()

target_link_libraries(keep-building ${RL_LIBRARY} FLECS_LIBRARY)