#include "spotify_tui/ui/components/progress_bar.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/screen/box.hpp>
#include <sstream>
#include <algorithm>

namespace spotify_tui {
namespace ui {

using namespace ftxui;

ftxui::Component MakeVolumeSlider(int* volume, std::function<void(int)> on_change) {
    class Impl : public ftxui::ComponentBase {
    public:
        Impl(int* volume, std::function<void(int)> on_change)
            : volume_(volume), on_change_(std::move(on_change)) {}

        ftxui::Element OnRender() override {
            int vol = volume_ ? *volume_ : 0;
            if (vol < 0) vol = 0;
            if (vol > 100) vol = 100;

            int bar_width = box_.x_max - box_.x_min;
            if (bar_width <= 0) bar_width = 10;

            int filled = static_cast<int>((vol / 100.0) * bar_width + 0.5);
            int empty = bar_width - filled;

            ftxui::Elements bar_elements;
            bar_elements.push_back(ftxui::text("["));

            if (filled > 0) {
                std::string filled_part;
                for (int i = 0; i < filled; ++i) filled_part += "█";
                bar_elements.push_back(
                    ftxui::text(filled_part) | ftxui::color(ftxui::Color::Green));
            }
            if (empty > 0) {
                std::string empty_part;
                for (int i = 0; i < empty; ++i) empty_part += "░";
                bar_elements.push_back(
                    ftxui::text(empty_part) | ftxui::color(ftxui::Color::GrayDark));
            }

            std::ostringstream oss;
            oss << vol << "%";

            return ftxui::vbox({
                ftxui::hbox(bar_elements) | ftxui::reflect(box_),
                ftxui::text("[" + oss.str() + "]") | ftxui::color(ftxui::Color::White)
            });
        }

        bool OnEvent(ftxui::Event event) override {
            if (event.is_mouse()) {
                const auto& mouse = event.mouse();
                if (mouse.button == ftxui::Mouse::Left &&
                    mouse.motion == ftxui::Mouse::Pressed) {
                    if (box_.Contain(mouse.x, mouse.y)) {
                        int bar_width = box_.x_max - box_.x_min;
                        if (bar_width <= 0) return false;
                        int rel_x = mouse.x - box_.x_min;
                        if (rel_x < 0) rel_x = 0;
                        if (rel_x > bar_width) rel_x = bar_width;
                        int new_volume = static_cast<int>(
                            (static_cast<double>(rel_x) / bar_width) * 100 + 0.5);
                        new_volume = std::clamp(new_volume, 0, 100);
                        if (on_change_) on_change_(new_volume);
                        return true;
                    }
                }
                return false;
            }

            if (event == ftxui::Event::Character('+')) {
                int vol = volume_ ? *volume_ : 0;
                vol = std::min(vol + 5, 100);
                if (on_change_) on_change_(vol);
                return true;
            }
            if (event == ftxui::Event::Character('-')) {
                int vol = volume_ ? *volume_ : 0;
                vol = std::max(vol - 5, 0);
                if (on_change_) on_change_(vol);
                return true;
            }

            return false;
        }

    private:
        int* volume_;
        std::function<void(int)> on_change_;
        ftxui::Box box_;
    };

    return ftxui::Make<Impl>(volume, std::move(on_change));
}

}  // namespace ui
}  // namespace spotify_tui
