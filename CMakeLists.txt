cmake_minimum_required(VERSION 3.21.0)
project(learn-gl VERSION 0.1.0)

set(BUILD_SHARED_LIBS OFF)

include_directories(
    ./external/glad/include
    ./external/stb
    ./external/glm
    ./external/glfw/include
    ./include
)

add_subdirectory(external/glm)

set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/assimp)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)


add_executable(learn-gl src/main.cpp src/bitmap.cpp external/glad/src/gl.c)

target_link_libraries(learn-gl glfw ${GLFW_LIBRARIES} assimp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

set_property(TARGET learn-gl PROPERTY C_STANDARD 17)
set_property(TARGET learn-gl PROPERTY CXX_STANDARD 20)

file(COPY shaders
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/shaders)