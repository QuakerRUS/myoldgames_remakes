#pragma once
#include <functional>

#include <vs/engine/texture.hpp>

#include "modal.hpp"

namespace ks {

// Contents of the app's dialogs (Russian UI text).
Modal::Spec help_dialog(std::function<void()> on_close);
Modal::Spec about_dialog(const vs::Texture* icon, std::function<void()> on_close);
Modal::Spec quit_dialog(std::function<void()> on_quit, std::function<void()> on_stay);

}  // namespace ks
