cmake_minimum_required(VERSION 3.21.0)
project(
    learn-gl
    VERSION 0.1.0
    LANGUAGES CXX)

### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(FetchContent)

# -------- glfw --------------

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
FetchContent_MakeAvailable(glfw)

# -------- glm ---------------

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# -------- assimp --------------

set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp
    GIT_TAG v5.2.5
)
FetchContent_MakeAvailable(assimp)

# ------ learn-gl ----------

file(GLOB_RECURSE include CONFIGURE_DEPENDS "include/*")
file(GLOB_RECURSE src CONFIGURE_DEPENDS "src/*")

# source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/base/dir" PREFIX "Header Files" FILES ${FILE_LIST})

add_executable(learn-gl "${src}" "external/glad/src/gl.c" "${include}")
target_include_directories(learn-gl PRIVATE include external/stb external/glad/include)
target_link_libraries(learn-gl glm glfw assimp)

target_compile_features(learn-gl PUBLIC cxx_std_20)
set_target_properties(learn-gl PROPERTIES CXX_EXTENSIONS OFF)
if (MSVC)
    target_compile_options(learn-gl PRIVATE
        "/Zc:__cplusplus"
        "/Zc:throwingNew"
        "/Zc:externConstexpr" )
endif()

install(TARGETS learn-gl assimp glfw
    RUNTIME DESTINATION bin)
install(DIRECTORY "shaders" "resources"
    DESTINATION bin)

