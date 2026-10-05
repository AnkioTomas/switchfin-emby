#pragma once

#include <borealis.hpp>
#include <utils/event.hpp>

class LyricView : public brls::Box {
public:
    LyricView();
    ~LyricView() override;

    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style,
        brls::FrameContext* ctx) override;

    static brls::View* create() { return new LyricView(); }

private:
    BRLS_BIND(brls::ScrollingFrame, scroll, "lyric/scroll");
    BRLS_BIND(brls::Box, box, "lyric/box");

    void rebuild();

    MPVCustomEvent::Subscription eventSubscribeID;
    int shown = -1;
};
