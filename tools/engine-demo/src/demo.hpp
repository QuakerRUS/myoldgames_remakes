#pragma once
#include <memory>
#include <string>

#include <vs/engine/scene.hpp>

namespace demo {

std::unique_ptr<vs::Scene> make_demo_scene();
// --screenshot <file.png>: the demo scene saves one frame there (for automated visual checks).
std::string& screenshot_path();
int run_demo_editor(int argc, char** argv);

}  // namespace demo
