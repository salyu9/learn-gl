#include <iostream>
#include <cstdlib>
#include <optional>
#include <format>
#include <functional>

#include "glad/gl.h"
#include "GLFW/glfw3.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "glwrap.hpp"
#include "camera.hpp"
#include "utils.hpp"
#include "examples.hpp"

using namespace glwrap;
using namespace std::literals;

using example_creator = std::function<std::unique_ptr<example>()>;
std::unique_ptr<example> create_backpack();
std::unique_ptr<example> create_nanosuit_explode();
std::unique_ptr<example> create_asteroids();
std::unique_ptr<example> create_asteroids_instanced();
std::unique_ptr<example> create_normal_map();
std::unique_ptr<example> create_parallax_map();
std::unique_ptr<example> create_hdr();
std::unique_ptr<example> create_bloom();
std::unique_ptr<example> create_deferred();
std::unique_ptr<example> create_direct_light_pbr();
std::unique_ptr<example> create_ibl_pbr();
std::unique_ptr<example> create_shadow_map();
std::unique_ptr<example> create_cascaded_shadow_map();
std::unique_ptr<example> create_ssao();
static inline std::vector<std::tuple<std::string, example_creator>> all_examples{
    {"Model Loading", create_backpack},
    {"Geometry Shader", create_nanosuit_explode},
    {"Asteroid Field", create_asteroids},
    {"Asteroid Field (Instancing)", create_asteroids_instanced},
    {"Normal Mapping", create_normal_map},
    {"Parallax Mapping", create_parallax_map},
    {"HDR / Tone Mapping", create_hdr},
    {"Bloom", create_bloom},
    {"Deferred Shading", create_deferred},
    {"Direct Light PBR", create_direct_light_pbr},
    {"IBL PBR", create_ibl_pbr},
    {"Shadow Mapping", create_shadow_map},
    {"Cascaded Shadow Mapping", create_cascaded_shadow_map},
    {"SSAO", create_ssao},
};

// ----- window status ---------
const int init_width = 1600;
const int init_height = 1200;
int screen_width = init_width;
int screen_height = init_height;
bool is_hdr;
std::unique_ptr<example> example_ptr;
std::optional<std::string> example_name;
int example_state = 0;
std::optional<frame_buffer> fbuffer{};
std::optional<frame_buffer> postbuffer{};
bool wireframe_mode = false;
GLsizei multisamples = 0;
GLint max_samples;

void reset_frame_buffer()
{
    if (!example_ptr)
    {
        return;
    }
    example_ptr->reset_frame_buffer(screen_width, screen_height);
    fbuffer.emplace(screen_width, screen_height, multisamples, is_hdr);
    if (multisamples > 0)
    {
        postbuffer.emplace(screen_width, screen_height, 0, is_hdr);
    }
    else
    {
        postbuffer.reset();
    }
}

// ------------------------------

namespace input_status
{
    inline struct
    {
        double x = 0;
        double y = 0;
        double delta_x = 0;
        double delta_y = 0;
    } cursor;

    inline bool wondering;
}

camera main_camera;

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    screen_width = width;
    screen_height = height;
    glViewport(0, 0, width, height);
    reset_frame_buffer();
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
            glfwSetWindowShouldClose(window, true);
        }
        else if (key == GLFW_KEY_TAB)
        {
            wireframe_mode = !wireframe_mode;
            std::cout << std::format("wireframe: {}", (wireframe_mode ? "on" : "off")) << std::endl;
        }
        else if (key == GLFW_KEY_EQUAL)
        {
            if (multisamples == 0)
                multisamples = 2;
            else
                multisamples = std::min(multisamples * 2, max_samples);

            std::cout << std::format("multisample: {}", multisamples) << std::endl;
            reset_frame_buffer();
        }
        else if (key == GLFW_KEY_MINUS)
        {
            if (multisamples > 2)
                multisamples /= 2;
            else
                multisamples = 0;
            std::cout << std::format("multisample: {}", multisamples) << std::endl;
            reset_frame_buffer();
        }
    }
}

void process_input(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    auto multiplier = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 5.0f : 1.0f;

    for (auto [key, movement] : {
             std::pair(GLFW_KEY_W, camera_movement::forward),
             std::pair(GLFW_KEY_S, camera_movement::backward),
             std::pair(GLFW_KEY_A, camera_movement::left),
             std::pair(GLFW_KEY_D, camera_movement::right),
             std::pair(GLFW_KEY_Q, camera_movement::down),
             std::pair(GLFW_KEY_E, camera_movement::up),
         })
    {
        if (glfwGetKey(window, key) == GLFW_PRESS)
        {
            main_camera.process_keyboard(movement, timer::delta_time(), multiplier);
        }
    }
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
    using namespace input_status;
    if (!wondering)
    {
        return;
    }

    int wnd_width, wnd_height;
    glfwGetWindowSize(window, &wnd_width, &wnd_height);

    auto ori_x = x;
    auto ori_y = y;

    while (x < 0)
    {
        x += wnd_width;
    }
    while (x > wnd_width)
    {
        x -= wnd_width;
    }
    while (y < 0)
    {
        y += wnd_height;
    }
    while (y > wnd_height)
    {
        y -= wnd_height;
    }
    if (x != ori_x || y != ori_y)
    {
        cursor.x = x;
        cursor.y = y;
        glfwSetCursorPos(window, x, y);
    }
    else
    {
        cursor.delta_x = x - cursor.x;
        cursor.delta_y = y - cursor.y;
        cursor.x = x;
        cursor.y = y;
        main_camera.process_mouse_movement(static_cast<float>(cursor.delta_x), -static_cast<float>(cursor.delta_y));
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            input_status::cursor.x = x;
            input_status::cursor.y = y;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            input_status::wondering = true;
        }
        else if (action == GLFW_RELEASE)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            input_status::wondering = false;
        }
    }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    main_camera.process_mouse_scroll(static_cast<float>(yoffset));
}

void GLAPIENTRY message_callback(GLenum source,
                                 GLenum type,
                                 GLuint id,
                                 GLenum severity,
                                 GLsizei length,
                                 const GLchar *message,
                                 const void * /*user_param*/)
{
    /*fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);*/
    if (type == GL_DEBUG_TYPE_ERROR)
    {
        std::cerr << std::format("GL Error: type = 0x{:x}, severity = 0x{:x}, message = {}", type, severity, message) << std::endl;
    }
    else if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        std::cout << std::format("GL Info: type = 0x{:x}, severity = 0x{:x}, message = {}", type, severity, message) << std::endl;
    }
}

int main(int argc, char *argv[])
try
{
    if (glfwInit() != GLFW_TRUE)
    {
        std::cout << "glfwInit() failed." << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for macOS

    GLFWwindow *window = glfwCreateWindow(init_width, init_height, "LearnOpenGL", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    float content_scale_x, content_scale_y;
    glfwGetWindowContentScale(window, &content_scale_x, &content_scale_y);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwGetCursorPos(window, &input_status::cursor.x, &input_status::cursor.y);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    auto &imgui_style = ImGui::GetStyle();
    imgui_style.AntiAliasedFill = true;
    imgui_style.AntiAliasedLines = true;
    imgui_style.AntiAliasedLinesUseTex = true;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    // io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF("./resources/fonts/SourceHanSans-Regular.ttc", 24.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

    auto imgui_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    if (gladLoadGL(glfwGetProcAddress) == 0)
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return EXIT_FAILURE;
    }
    glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, init_width, init_height);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(message_callback, nullptr);

    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // timer::update();
    utils::fps_counter<60> fps(timer::time());
    int counted_frames = 0;

    try
    { // frame buffer
        struct quad_vertex_t
        {
            using vertex_desc_t = std::tuple<glm::vec2, glm::vec2>;
            glm::vec2 pos;
            glm::vec2 tex;
            quad_vertex_t(float x, float y, float u, float v) : pos(x, y), tex(u, v) {}
        };
        auto quad_varray = auto_vertex_array(vertex_buffer<quad_vertex_t>{
            {-1.0f, 1.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f, 0.0f, 0.0f},
            {1.0f, -1.0f, 1.0f, 0.0f},
            {-1.0f, 1.0f, 0.0f, 1.0f},
            {1.0f, -1.0f, 1.0f, 0.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
        });
        auto quad_program = shader_program{make_vf_program(
            "shaders/base/fbuffer_vs.glsl"sv,
            is_hdr ? "shaders/base/fbuffer_fs_reinhard.glsl"sv : "shaders/base/fbuffer_fs.glsl"sv,
            "screenTexture", 0
        )};

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            process_input(window);
            if (!example_ptr)
            {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Select example...", nullptr, ImGuiWindowFlags_NoResize);

                ImGui::SetWindowFontScale(content_scale_x);
                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {0.5f, 0.5f});
                for (auto [name, creator] : all_examples)
                {
                    if (ImGui::Button(name.c_str(), {500, 45}))
                    {
                        example_ptr = creator();
                        example_name = name;
                        example_state = 0;
                        is_hdr = example_ptr->is_hdr();
                        framebuffer_size_callback(window, init_width, init_height);

                        auto cam = example_ptr->get_camera();
                        if (cam.has_value())
                        {
                            main_camera = cam.value();
                        }
                        break;
                    }
                }
                ImGui::PopStyleVar();

                ImGui::End();
                ImGui::Render();

                glClear(GL_COLOR_BUFFER_BIT);
            }
            else
            {
                auto destroy = false;

                timer::update();
                fps.count(timer::time());

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                static float f = 0.0f;
                static int counter = 0;

                ImGui::SetNextWindowSizeConstraints({400, 200}, {static_cast<float>(screen_width), static_cast<float>(screen_height)});
                ImGui::Begin(example_name.value().c_str());
                ImGui::SetWindowFontScale(content_scale_x);

                if (ImGui::Button("<<"))
                {
                    example_ptr.reset();
                    example_name.reset();
                }
                else
                {
                    ImGui::SameLine();
                    ImGui::Text(std::format("FPS: {:.1f}", fps.fps()).c_str());
                    auto &states = example_ptr->get_states();
                    if (!states.empty())
                    {
                        int index = 0;
                        int prev_state = example_state;
                        for (auto &state : states)
                        {
                            if (index > 0) {
                                ImGui::SameLine();
                            }
                            ImGui::RadioButton(state.c_str(), &example_state, index++);
                        }
                        if (prev_state != example_state)
                        {
                            example_ptr->switch_state(example_state);
                        }
                    }
                    example_ptr->draw_gui();
                }

                ImGui::End();
                ImGui::Render();

                if (example_ptr)
                {
                    main_camera.set_aspect(static_cast<float>(screen_width) / screen_height);
                    auto proj_mat = main_camera.projection();

                    if (!example_ptr->custom_render())
                    {
                        auto &fb = fbuffer.value();
                        fb.bind();
                        if (wireframe_mode)
                        {
                            fb.clear({0.2f, 0.3f, 0.3f, 1.0f});
                            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        }
                        else
                        {
                            fb.clear();
                            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        }
                    }

                    example_ptr->draw(proj_mat, main_camera);

                    if (!example_ptr->custom_render())
                    {
                        frame_buffer::unbind_all();
                        glDisable(GL_DEPTH_TEST);
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        auto &fb = fbuffer.value();
                        if (multisamples > 0)
                        {
                            auto &pb = postbuffer.value();
                            fb.blit_to(pb, GL_COLOR_BUFFER_BIT);
                            pb.color_texture().bind_unit(0);
                        }
                        else
                        {
                            fb.color_texture().bind_unit(0);
                        }

                        auto postprocessor_ptr = example_ptr->postprocessor_ptr();
                        if (postprocessor_ptr)
                        {
                            postprocessor_ptr->use();
                        }
                        else
                        {
                            quad_program.use();
                        }
                        quad_varray.draw(draw_mode::triangles);
                        glEnable(GL_DEPTH_TEST);
                    }
                }
            }

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    }
    catch (std::exception const &e)
    {
        std::cout << e.what() << std::endl;
    }
    glfwTerminate();
    return EXIT_SUCCESS;
}
catch (std::exception const &e)
{
    std::cout << e.what() << std::endl;
}