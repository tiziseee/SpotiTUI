#pragma once

#include <ftxui/component/component_base.hpp>

namespace spotify_tui {
namespace ui {

ftxui::Component MakeVolumeSlider(int* volume, std::function<void(int)> on_change);

} // namespace ui
} // namespace spotify_tui
