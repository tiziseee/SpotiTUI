#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace ftxui {

class Scroller : public ComponentBase {
public:
    explicit Scroller(Component child, int* selected_ref)
        : child_(child), selected_ref_(selected_ref) {
        Add(std::move(child));
    }

private:
    Element Render() override {
        auto focused = Focused() ? focus : select;
        Element background = child_->Render();
        background->ComputeRequirement();
        size_ = background->requirement().min_y;

        return dbox({
            std::move(background),
            vbox({
                text("") | size(HEIGHT, EQUAL, *selected_ref_),
                text("") | focused,
            }),
        }) | vscroll_indicator | yframe | yflex | reflect(box_);
    }

    bool OnEvent(Event event) override {
        if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y))
            TakeFocus();

        int old_selected = *selected_ref_;
        if (event == Event::ArrowUp || event == Event::Character('k') ||
            (event.is_mouse() && event.mouse().button == Mouse::WheelUp)) {
            (*selected_ref_)--;
        }
        if ((event == Event::ArrowDown || event == Event::Character('j') ||
             (event.is_mouse() && event.mouse().button == Mouse::WheelDown))) {
            (*selected_ref_)++;
        }
        if (event == Event::PageDown)
            *selected_ref_ += box_.y_max - box_.y_min;
        if (event == Event::PageUp)
            *selected_ref_ -= box_.y_max - box_.y_min;
        if (event == Event::Home)
            *selected_ref_ = 0;
        if (event == Event::End)
            *selected_ref_ = size_;

        *selected_ref_ = std::max(0, std::min(size_ - 1, *selected_ref_));

        return old_selected != *selected_ref_;
    }

    Component child_;
    int* selected_ref_;
    int size_ = 0;
    Box box_;
};

Component Scroller(Component child, int* selected_ref);

}  // namespace ftxui
