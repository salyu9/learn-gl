#include <iostream>
#include <cstdlib>
#include <chrono>
#include <optional>
#include <queue>

#include <format>

#include "glad/gl.h"
#include "GLFW/glfw3.h"

#include "glm/matrix.hpp"
#include "glm/gtx/transform.hpp"

#include "glwrap.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "utils.hpp"

using namespace glwrap;

// ----- window status ---------
const int init_width = 1600;
const int init_height = 1200;
int screen_width = init_width;
int screen_height = init_height;
std::optional<frame_buffer> fbuffer{};
bool wireframe_mode = false;
GLsizei multisamples = 0;
GLint max_samples;

void reset_frame_buffer()
{
    fbuffer.reset();
    fbuffer.emplace(screen_width, screen_height, multisamples);
}

// ------------------------------

namespace timer
{
    namespace details
    {
        inline auto startup_tp = std::chrono::high_resolution_clock::now();
        inline auto get_time()
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(now - startup_tp);
            return duration.count();
        };
        inline float current_time = get_time();
        inline float delta_time = 0.0f;
    }
    inline auto time()
    {
        return details::current_time;
    }
    inline auto delta_time()
    {
        return details::delta_time;
    }
    inline void update()
    {
        auto new_time = details::get_time();
        details::delta_time = new_time - details::current_time;
        details::current_time = new_time;
    }
}

namespace input_status
{
    inline struct
    {
        double x = 0;
        double y = 0;
        double delta_x = 0;
        double delta_y = 0;
    } cursor;
}

camera main_camera(glm::vec3(-5.0f, 15.0f, 10.0f), glm::vec3(0, 1, 0), -60, -10);

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
            glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);
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
        glfwSetWindowShouldClose(window, true);

    for (auto [key, movement] : {
             std::pair(GLFW_KEY_W, camera_movement::forward),
             std::pair(GLFW_KEY_S, camera_movement::backward),
             std::pair(GLFW_KEY_A, camera_movement::left),
             std::pair(GLFW_KEY_D, camera_movement::right),
             std::pair(GLFW_KEY_C, camera_movement::down),
             std::pair(GLFW_KEY_Z, camera_movement::up),
         })
    {
        if (glfwGetKey(window, key) == GLFW_PRESS)
        {
            main_camera.process_keyboard(movement, timer::delta_time());
        }
    }
}

void mouse_callback(GLFWwindow *window, double x, double y)
{
    using namespace input_status;
    cursor.delta_x = x - cursor.x;
    cursor.delta_y = y - cursor.y;
    cursor.x = x;
    cursor.y = y;
    main_camera.process_mouse_movement(static_cast<float>(cursor.delta_x), -static_cast<float>(cursor.delta_y));
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
}

int main()
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

    if (gladLoadGL(glfwGetProcAddress) == 0)
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return EXIT_FAILURE;
    }
    glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, init_width, init_height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    framebuffer_size_callback(window, init_width, init_height);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwGetCursorPos(window, &input_status::cursor.x, &input_status::cursor.y);

    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(message_callback, nullptr);

    // timer::update();
    utils::fps_counter<60> fps(timer::time());
    int counted_frames = 0;

    try
    {
        // frame buffer
        struct quad_vertex_t
        {
            using vertex_desc_t = std::tuple<glm::vec2, glm::vec2>;
            glm::vec2 pos;
            glm::vec2 tex;
            quad_vertex_t(float x, float y, float u, float v) : pos(x, y), tex(u, v) {}
        };
        auto quad_vbuffer = vertex_buffer<quad_vertex_t>{
            {-1.0f, 1.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f, 0.0f, 0.0f},
            {1.0f, -1.0f, 1.0f, 0.0f},
            {-1.0f, 1.0f, 0.0f, 1.0f},
            {1.0f, -1.0f, 1.0f, 0.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
        };
        auto quad_varray = auto_vertex_array(quad_vbuffer);
        auto quad_program = shader_program(
            shader::compile_file("shaders/fbuffer_vs.glsl", shader_type::vertex),
            shader::compile_file("shaders/fbuffer_fs.glsl", shader_type::fragment));
        quad_program.uniform("screenTexture").set_int(0);

        // skybox
        auto skybox_vertices = vertex_buffer<glm::vec3>{
            // positions
            glm::vec3(-1.0f, 1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, 1.0f, -1.0f),
            glm::vec3(-1.0f, 1.0f, -1.0f),

            glm::vec3(-1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(-1.0f, 1.0f, -1.0f),
            glm::vec3(-1.0f, 1.0f, -1.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),

            glm::vec3(1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, -1.0f),
            glm::vec3(1.0f, -1.0f, -1.0f),

            glm::vec3(-1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),

            glm::vec3(-1.0f, 1.0f, -1.0f),
            glm::vec3(1.0f, 1.0f, -1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f),
            glm::vec3(-1.0f, 1.0f, -1.0f),

            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, -1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
        };
        auto skybox_varray = auto_vertex_array(skybox_vertices);
        auto skybox_cubemap = cubemap(
            "resources/cubemaps/skybox/right.jpg",
            "resources/cubemaps/skybox/left.jpg",
            "resources/cubemaps/skybox/top.jpg",
            "resources/cubemaps/skybox/bottom.jpg",
            "resources/cubemaps/skybox/front.jpg",
            "resources/cubemaps/skybox/back.jpg");
        auto skybox_program = shader_program(
            shader::compile_file("shaders/skybox_vs.glsl", shader_type::vertex),
            shader::compile_file("shaders/skybox_fs.glsl", shader_type::fragment));
        auto skybox_proj_uniform = skybox_program.uniform("projection");
        auto skybox_view_uniform = skybox_program.uniform("view");
        skybox_program.uniform("skybox").set_int(0);

        // object

        auto model_trans = glm::mat4(1.0f);
        auto m = utils::benchmark([]
                                  { return model::load_file("resources/models/crysis_nano_suit_2/scene.gltf"); },
                                  "load model: ");

        while (!glfwWindowShouldClose(window))
        {
            timer::update();
            fps.count(timer::time());
            ++counted_frames;
            if (counted_frames == fps.capacity())
            {
                counted_frames = 0;
                glfwSetWindowTitle(window, std::format("LearnOpenGL ({:.1f} fps)", fps.fps()).c_str());
            }

            process_input(window);

            auto proj_mat = main_camera.projection(static_cast<float>(screen_width) / screen_height);
            auto view_mat = main_camera.view();

            auto &fb = fbuffer.value();
            fb.bind();
            if (wireframe_mode)
            {
                fb.clear({0.2f, 0.3f, 0.3f, 1.0f});
            }
            else
            {
                fb.clear_depth();
            }

            // sky box

            // glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);
            skybox_program.use();
            skybox_proj_uniform.set_mat4(proj_mat);
            skybox_view_uniform.set_mat4(glm::mat4(glm::mat3(view_mat)));
            skybox_cubemap.bind_unit(0);
            skybox_varray.bind();
            skybox_varray.draw(draw_mode::triangles);
            // glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);

            m.draw(proj_mat, view_mat);

            frame_buffer::unbind_all();

            // frame buffer to screen - by render (post)
            /*
            glDisable(GL_DEPTH_TEST);
            fb.color_texture().bind_unit(0);
            quad_program.use();
            quad_varray.bind();
            quad_varray.draw(draw_mode::triangles);
            */

            // frame buffer to screen - by blit
            glBlitNamedFramebuffer(fb.handle(), 0, 0, 0, fb.width(), fb.height(), 0, 0, fb.width(), fb.height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

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
