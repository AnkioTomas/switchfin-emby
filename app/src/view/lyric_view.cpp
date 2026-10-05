#include "view/lyric_view.hpp"
#include "view/music_view.hpp"
#include "view/mpv_core.hpp"

using namespace brls::literals;

// the paddings keep every line centerable without clamping the scroll offset
const std::string lyricViewXML = R"xml(
    <brls:Box
        axis="column">

        <brls:ScrollingFrame
            id="lyric/scroll"
            grow="1">

            <brls:Box
                id="lyric/box"
                axis="column"
                alignItems="center"
                paddingTop="300"
                paddingBottom="300" />
        </brls:ScrollingFrame>
    </brls:Box>
)xml";

LyricView::LyricView() {
    this->inflateFromXMLString(lyricViewXML);

    this->eventSubscribeID = MPVCore::instance().getCustomEvent()->subscribe([this](const std::string& event, void*) {
        if (event == LYRIC_LOAD) this->rebuild();
    });
    this->rebuild();
}

LyricView::~LyricView() {
    MPVCore::instance().getCustomEvent()->unsubscribe(this->eventSubscribeID);
    // the page may be closed while lyrics are shown
    MusicView::instance().getView("music/play/lyric")->setVisibility(brls::Visibility::VISIBLE);
}

void LyricView::rebuild() {
    auto& lines = MusicView::instance().lyrics;
    auto grey = brls::Application::getTheme().getColor("font/grey");
    this->box->clearViews();
    this->shown = -1;

    if (lines.empty()) {
        auto* label = new brls::Label();
        label->setText("main/player/no_lyric"_i18n);
        label->setTextColor(grey);
        this->box->addView(label);
    }
    for (auto& line : lines) {
        auto* label = new brls::Label();
        label->setText(line.text);
        label->setFontSize(24);
        label->setMarginBottom(16);
        label->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        label->setTextColor(grey);
        this->box->addView(label);
    }
    this->scroll->setContentOffsetY(0, false);
}

void LyricView::draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style,
    brls::FrameContext* ctx) {
    int index = MusicView::instance().lyricIndex();
    if (index != this->shown) {
        auto theme = brls::Application::getTheme();
        auto& labels = this->box->getChildren();
        if (this->shown >= 0) static_cast<brls::Label*>(labels[this->shown])->setTextColor(theme.getColor("font/grey"));
        if (index >= 0) {
            auto* label = static_cast<brls::Label*>(labels[index]);
            label->setTextColor(theme.getColor("brls/accent"));
            float center = label->getLocalY() + label->getHeight() / 2;
            this->scroll->setContentOffsetY(center - this->scroll->getHeight() / 2, true);
        }
        this->shown = index;
    }
    Box::draw(vg, x, y, width, height, style, ctx);
}
