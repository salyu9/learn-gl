#include <iostream>
#include <cstdlib>
#include <chrono>
#include <optional>
#include <queue>
#include <format>
#include <functional>

#include "glad/gl.h"
#include "GLFW/glfw3.h"

#include "glm/matrix.hpp"
#include "glm/gtx/transform.hpp"

#include "glwrap.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "utils.hpp"
#include "skybox.hpp"
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
std::unique_ptr<example> create_ldr();
std::unique_ptr<example> create_hdr();
std::unique_ptr<example> create_bloom();
std::unique_ptr<example> create_deferred();
inline std::vector<std::tuple<std::string_view, example_creator>> get_examples()
{
    return {
        {"model load", create_backpack},
        {"explode (geometry shader)", create_nanosuit_explode},
        {"asteroid field", create_asteroids},
        {"asteroid field (instancing)", create_asteroids_instanced},
        {"normal mapping", create_normal_map},
        {"parallax mapping", create_parallax_map},
        {"ldr", create_ldr},
        {"hdr / tone mapping", create_hdr},
        {"bloom", create_bloom},
        {"deferred shading", create_deferred}
    };
}

// ----- window status ---------
const int init_width = 1600;
const int init_height = 1200;
int screen_width = init_width;
int screen_height = init_height;
bool is_hdr;
std::unique_ptr<example> example_ptr;
std::optional<frame_buffer> fbuffer{};
std::optional<frame_buffer> postbuffer{};
bool wireframe_mode = false;
GLsizei multisamples = 0;
GLint max_samples;

void reset_frame_buffer()
{
    example_ptr->reset_frame_buffer(screen_width, screen_height);
    fbuffer.emplace(screen_width, screen_height, multisamples, is_hdr);
    if (multisamples > 0) {
        postbuffer.emplace(screen_width, screen_height, 0, is_hdr);
    }
    else {
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
        else if (key == GLFW_KEY_SPACE)
        {
            example_ptr->on_switch();
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
    if (!wondering) {
        return;
    }

    int wnd_width, wnd_height;
    glfwGetWindowSize(window, &wnd_width, &wnd_height);

    auto ori_x = x;
    auto ori_y = y;

    while (x < 0) {
        x += wnd_width;
    }
    while (x > wnd_width) {
        x -= wnd_width;
    }
    while (y < 0) {
        y += wnd_height;
    }
    while (y > wnd_height) {
        y -= wnd_height;
    }
    if (x != ori_x || y != ori_y) {
        cursor.x = x;
        cursor.y = y;
        glfwSetCursorPos(window, x, y);
    }
    else {
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

int main(int argc, char* argv[])
try
{
    auto examples = get_examples();

    size_t in;
    if (argc >= 2) {
        in = std::stoul(argv[1]);
    }
    else {
        std::cout << "Examples:" << std::endl;
        auto i = 0;
        for (auto &[name, _] : examples)
        {
            std::cout << "    " << (++i) << ": " << name << std::endl;
        }
        std::cout << "Select [1~" << examples.size() << "]:" << std::flush;
        if (!(std::cin >> in) || in == 0 || in > examples.size())
        {
            std::cout << "Invalid input" << std::endl;
            return EXIT_FAILURE;
        }
    }
    auto [example_name, example_creator] = examples[in - 1U];

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

    if (gladLoadGL(glfwGetProcAddress) == 0)
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return EXIT_FAILURE;
    }
    glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, init_width, init_height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwGetCursorPos(window, &input_status::cursor.x, &input_status::cursor.y);

    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(message_callback, nullptr);

    // timer::update();
    utils::fps_counter<60> fps(timer::time());
    int counted_frames = 0;

    try
    {
        example_ptr = example_creator();
        is_hdr = example_ptr->is_hdr();
        framebuffer_size_callback(window, init_width, init_height);

        // frame buffer
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
        auto quad_program = shader_program(
            shader::compile_file("shaders/base/fbuffer_vs.glsl"sv, shader_type::vertex),
            shader::compile_file(is_hdr ? "shaders/base/fbuffer_fs_reinhard.glsl"sv : "shaders/base/fbuffer_fs.glsl"sv, shader_type::fragment));
        quad_program.uniform("screenTexture").set_int(0);

        auto cam = example_ptr->get_camera();
        if (cam.has_value()) {
            main_camera = cam.value();
        }

        while (!glfwWindowShouldClose(window))
        {
            timer::update();
            fps.count(timer::time());
            ++counted_frames;
            if (counted_frames == fps.capacity())
            {
                counted_frames = 0;
                glfwSetWindowTitle(window, std::format("LearnOpenGL - {} ({:.1f} fps, camera at {})", example_name, fps.fps(), main_camera.position()).c_str());
            }

            process_input(window);

            auto proj_mat = main_camera.projection(static_cast<float>(screen_width) / screen_height);

            if (!example_ptr->custom_render()) {
                auto &fb = fbuffer.value();
                fb.bind();
                if (wireframe_mode) {
                    fb.clear({0.2f, 0.3f, 0.3f, 1.0f});
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }
                else {
                    fb.clear();
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
            }

            example_ptr->draw(proj_mat, main_camera);

            if (!example_ptr->custom_render()) {
                frame_buffer::unbind_all();
                glDisable(GL_DEPTH_TEST);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                auto &fb = fbuffer.value();
                if (multisamples > 0) {
                    auto & pb = postbuffer.value();
                    fb.blit_to(pb, GL_COLOR_BUFFER_BIT);
                    pb.color_texture().bind_unit(0);
                }
                else {
                    fb.color_texture().bind_unit(0);
                }
                quad_varray.bind();

                auto postprocessor_ptr = example_ptr->postprocessor_ptr();
                if (postprocessor_ptr) {
                    postprocessor_ptr->use();
                }
                else {
                    quad_program.use();
                }
                quad_varray.draw(draw_mode::triangles);
                glEnable(GL_DEPTH_TEST);
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    catch (std::exception const &e)
    {
        std::cout << e.what() << std::endl;
    }
    glfwTerminate();
    return EXIT_SUCCESS;
}
catch (std::exception const& e)
{
    std::cout << e.what() << std::endl;
}