#pragma once

#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>

namespace spotify_tui {
namespace ui {

ftxui::Component MakeProgressBar(int* progress_ms, int* duration_ms,
                                  std::function<void(int)> on_seek);

ftxui::Component MakeVolumeSlider(int* volume, std::function<void(int)> on_change);

} // namespace ui
} // namespace spotify_tui
