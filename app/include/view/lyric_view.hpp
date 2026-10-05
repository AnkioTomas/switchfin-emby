#pragma once

#include <borealis.hpp>
#include <utils/event.hpp>

class LyricView : public brls::Box {
public:
    LyricView();
    ~LyricView() override;

    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style,
        brls::FrameContext* ctx) override;

private:
    BRLS_BIND(brls::ScrollingFrame, scroll, "lyric/scroll");
    BRLS_BIND(brls::Box, box, "lyric/box");
    BRLS_BIND(brls::Box, stats, "lyric/stats");

    void rebuild();

    MPVCustomEvent::Subscription eventSubscribeID;
    brls::Box* prevParent = nullptr;
    int shown = -1;
};
