cmake_minimum_required(VERSION 3.24.0)
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
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(assimp)

# ------ json --------------

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# ------ imgui -------------

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.4
    GIT_PROGRESS TRUE
)
if (NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    set(IMGUI_INCLUDE_DIR "${imgui_SOURCE_DIR}/")
    file(GLOB IMGUI_SOURCES "${imgui_SOURCE_DIR}/*.cpp")
    file(GLOB IMGUI_HEADERS "${imgui_SOURCE_DIR}/*.h")
    add_library(imgui STATIC 
        "${IMGUI_SOURCES}"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
    )

    target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIR} ${GLAD_INCLUDE_DIR})
    target_link_libraries(imgui ${OPENGL_LIBRARIES} glfw)
endif ()

# ------ learn-gl ----------

file(GLOB_RECURSE include CONFIGURE_DEPENDS "include/*")
file(GLOB_RECURSE src CONFIGURE_DEPENDS "src/*")

# source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/base/dir" PREFIX "Header Files" FILES ${FILE_LIST})

add_executable(learn-gl "${src}" "external/glad/src/gl.c" "${include}")
target_include_directories(learn-gl PRIVATE include external/stb external/glad/include)
target_link_libraries(learn-gl PRIVATE glm glfw assimp nlohmann_json::nlohmann_json imgui)

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

