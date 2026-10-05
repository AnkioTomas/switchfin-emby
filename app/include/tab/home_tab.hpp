/*
    Copyright 2023 dragonflylee
*/

#pragma once

#include <view/auto_tab_frame.hpp>
#include <view/presenter.hpp>

class RecylingVideo;

namespace jellyfin {
struct Collection;
}

class HomeTab : public AttachedView, public Presenter {
public:
    HomeTab();
    ~HomeTab() override;

    void onCreate() override;

    void doRequest() override;

    static brls::View* create();

private:
    BRLS_BIND(brls::Box, boxHome, "home/box");

    std::vector<std::function<void(bool)>> sections;

    void doSections(const std::vector<std::string>& types);
    void addSection(const std::string& type, const std::vector<jellyfin::Collection>& views);
    RecylingVideo* addRow(const std::string& title, float width, float height);
    void addPaged(const std::string& title, float width, float height, std::function<std::string(size_t, size_t)> query);
};
