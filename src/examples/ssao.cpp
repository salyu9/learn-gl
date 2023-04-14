#include "glwrap.hpp"
#include "common_obj.hpp"
#include "examples.hpp"
#include "skybox.hpp"
#include "imgui.h"

using namespace glwrap;

class ssao final : public example
{
public:
    void draw(glm::mat4 const &projection, camera &cam) override
    {
        
    }
};

std::unique_ptr<example> create_ssao()
{
    return std::make_unique<ssao>();
}
