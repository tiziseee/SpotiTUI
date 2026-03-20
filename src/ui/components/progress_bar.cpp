#include "spotify_tui/ui/components/progress_bar.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/screen/box.hpp>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace spotify_tui {
namespace ui {

using namespace ftxui;

namespace {

std::string FormatTime(int ms) {
    int total_seconds = ms / 1000;
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    std::ostringstream oss;
    oss << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;
    return oss.str();
}

}  // namespace

ftxui::Component MakeProgressBar(int* progress_ms, int* duration_ms,
                                 std::function<void(int)> on_seek) {
    class Impl : public ftxui::ComponentBase {
    public:
        Impl(int* progress_ms, int* duration_ms, std::function<void(int)> on_seek)
            : progress_ms_(progress_ms),
              duration_ms_(duration_ms),
              on_seek_(std::move(on_seek)) {}

        ftxui::Element OnRender() override {
            int progress = progress_ms_ ? *progress_ms_ : 0;
            int duration = duration_ms_ ? *duration_ms_ : 0;
            if (duration <= 0) duration = 1;
            if (progress < 0) progress = 0;
            if (progress > duration) progress = duration;

            double ratio = static_cast<double>(progress) / static_cast<double>(duration);

            std::string filled_char = "█";
            std::string empty_char = "░";

            int bar_width = box_.x_max - box_.x_min;
            if (bar_width <= 0) bar_width = 20;

            int filled = static_cast<int>(std::round(ratio * bar_width));
            int empty = bar_width - filled;

            ftxui::Elements bar_elements;
            bar_elements.push_back(ftxui::text("["));
            if (filled > 0) {
                std::string filled_str(filled, ' ');
                std::string filled_part;
                for (int i = 0; i < filled; ++i) filled_part += filled_char;
                bar_elements.push_back(
                    ftxui::text(filled_part) | ftxui::color(ftxui::Color::Green));
            }
            if (empty > 0) {
                std::string empty_part;
                for (int i = 0; i < empty; ++i) empty_part += empty_char;
                bar_elements.push_back(
                    ftxui::text(empty_part) | ftxui::color(ftxui::Color::GrayDark));
            }

            std::string time_str =
                FormatTime(progress) + " / " + FormatTime(duration);

            return ftxui::vbox({
                ftxui::hbox(bar_elements) | ftxui::reflect(box_),
                ftxui::text("[" + time_str + "]") | ftxui::color(ftxui::Color::White)
            });
        }

        bool OnEvent(ftxui::Event event) override {
            if (!event.is_mouse()) return false;

            const auto& mouse = event.mouse();
            if (mouse.button == ftxui::Mouse::Left &&
                mouse.motion == ftxui::Mouse::Pressed) {
                if (box_.Contain(mouse.x, mouse.y)) {
                    int bar_width = box_.x_max - box_.x_min;
                    if (bar_width <= 0) return false;
                    int rel_x = mouse.x - box_.x_min;
                    if (rel_x < 0) rel_x = 0;
                    if (rel_x > bar_width) rel_x = bar_width;
                    double ratio =
                        static_cast<double>(rel_x) / static_cast<double>(bar_width);
                    int duration = duration_ms_ ? *duration_ms_ : 0;
                    int seek_ms = static_cast<int>(ratio * duration);
                    if (on_seek_) on_seek_(seek_ms);
                    return true;
                }
            }
            return false;
        }

    private:
        int* progress_ms_;
        int* duration_ms_;
        std::function<void(int)> on_seek_;
        ftxui::Box box_;
    };

    return ftxui::Make<Impl>(progress_ms, duration_ms, std::move(on_seek));
}

}  // namespace ui
}  // namespace spotify_tui
