#include "view/music_view.hpp"
#include "view/mpv_core.hpp"
#include "view/svg_image.hpp"
#include "view/video_progress_slider.hpp"
#include "utils/config.hpp"
#include "utils/keybind.hpp"
#include "utils/misc.hpp"
#include "utils/image.hpp"
#include "api/jellyfin.hpp"

using namespace brls::literals;

/// "[00:12.34][01:02.00]text" -> one line per time tag, other tags ([ar:...]) are ignored
static std::vector<MusicView::LyricLine> parseLrc(const std::string& lrc) {
    std::vector<MusicView::LyricLine> lines;
    std::istringstream in(lrc);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::vector<double> times;
        size_t pos = 0;
        while (pos < line.size() && line[pos] == '[') {
            size_t end = line.find(']', pos);
            if (end == std::string::npos) break;
            int min = 0;
            double sec = 0;
            if (sscanf(line.c_str() + pos, "[%d:%lf]", &min, &sec) == 2) times.push_back(min * 60 + sec);
            pos = end + 1;
        }
        for (double t : times) lines.push_back({t, line.substr(pos)});
    }
    std::stable_sort(lines.begin(), lines.end(), [](auto& a, auto& b) { return a.time < b.time; });
    return lines;
}

static const std::string& findLyric(const jellyfin::Detail& detail) {
    static const std::string empty;
    for (auto& src : detail.MediaSources)
        for (auto& s : src.MediaStreams)
            if (s.Type == jellyfin::streamTypeSubtitle && !s.Extradata.empty()) return s.Extradata;
    return empty;
}

MusicView::MusicView() {
    this->inflateFromXMLRes("xml/view/music_view.xml");
    brls::Logger::debug("MusicView: create");

    auto& mpv = MPVCore::instance();

    /// 播放控制
    this->btnToggle->registerClickAction([&mpv](...) {
        if (mpv.isStopped())
            mpv.command("playlist-play-index", "current");
        else
            mpv.togglePlay();
        return true;
    });
    this->btnToggle->addGestureRecognizer(new brls::TapGestureRecognizer(this->btnToggle));

    this->btnPrev->registerClickAction([&mpv](...) {
        mpv.command("playlist-prev");
        return true;
    });
    this->btnPrev->addGestureRecognizer(new brls::TapGestureRecognizer(this->btnPrev));

    this->btnNext->registerClickAction([&mpv](...) {
        mpv.command("playlist-next");
        return true;
    });
    this->btnNext->addGestureRecognizer(new brls::TapGestureRecognizer(this->btnNext));

    this->btnSuffle->registerClickAction([this](...) { return this->toggleShuffle(); });
    this->btnSuffle->addGestureRecognizer(new brls::TapGestureRecognizer(this->btnSuffle));

    this->btnRepeat->registerClickAction([this](...) { return this->toggleLoop(); });
    this->btnRepeat->addGestureRecognizer(new brls::TapGestureRecognizer(this->btnRepeat));

    osdSlider->getProgressSetEvent().subscribe([](float progress) {
        brls::Logger::verbose("Set progress: {}", progress);
        MPVCore::instance().seek(progress * 100, "absolute-percent");
    });
}

MusicView::~MusicView() { brls::Logger::debug("MusicView: delete"); }

void MusicView::registerMpvEvent() {
    auto& mpv = MPVCore::instance();
    // 生成播放 ID
    const auto ts = std::chrono::system_clock::now().time_since_epoch();
    this->playSession = std::chrono::duration_cast<std::chrono::milliseconds>(ts).count();
    // 注册播放事件回调
    this->eventSubscribeID = mpv.getEvent()->subscribe([this](MpvEventEnum event) {
        auto& mpv = MPVCore::instance();
        switch (event) {
        case MpvEventEnum::START_FILE:
            this->setLyrics({});
            if (playList.size() > 0) {
                std::string key = fmt::format("playlist/{}/id", mpv.getInt("playlist-playing-pos"));
                auto it = playList.find(mpv.getInt(key));
                if (it != playList.end()) {
                    this->playTitle->setText(it->second.Title);
                    this->itemId = it->second.Id;
                    mpv.getCustomEvent()->fire(TRACK_START, &it->second);
                    this->doLyric(it->second.Id);
                }
            }
            break;
        case MpvEventEnum::MPV_RESUME:
            this->btnToggleIcon->setImageFromSVGRes("icon/ico-pause.svg");
            break;
        case MpvEventEnum::MPV_PAUSE:
            this->btnToggleIcon->setImageFromSVGRes("icon/ico-play.svg");
            break;
        case MpvEventEnum::UPDATE_DURATION:
            this->leftStatusLabel->setText(misc::sec2Time(0));
            this->rightStatusLabel->setText(misc::sec2Time(mpv.duration));
            this->osdSlider->setProgress((float)mpv.playback_time / mpv.duration);
            break;
        case MpvEventEnum::UPDATE_PROGRESS:
            this->leftStatusLabel->setText(misc::sec2Time(mpv.video_progress));
            this->osdSlider->setProgress((float)mpv.playback_time / mpv.duration);
            break;
        case MpvEventEnum::END_OF_FILE:
            this->reset();
            break;
        case MpvEventEnum::MPV_STOP:
            this->reset();
            if (!this->getParent()) brls::sync([this]() { this->unregisterMpvEvent(); });
            break;
        default:;
        }
    });
    // 注冊命令回調
    replySubscribeID = mpv.getCommandReply()->subscribe([this](uint64_t userdata, int64_t entryId) {
        auto item = reinterpret_cast<jellyfin::Track*>(userdata);
        if (item) playList.insert(std::make_pair(entryId, item));
    });

    brls::Logger::info("MusicView: registerMpvEvent {}", this->playSession);
}

void MusicView::unregisterMpvEvent() {
    auto& mpv = MPVCore::instance();
    mpv.getEvent()->unsubscribe(eventSubscribeID);
    mpv.getCommandReply()->unsubscribe(replySubscribeID);

    brls::Logger::info("MusicView: unregisterMpvEvent {}", this->playSession);
    // 清空播放ID
    this->playSession = 0;
}

void MusicView::registerViewAction(brls::View* view) {
    auto& mpv = MPVCore::instance();

    view->registerAction(
        "main/player/toggle"_i18n, brls::BUTTON_Y,
        [&mpv](brls::View* view) {
            if (mpv.isStopped())
                mpv.command("playlist-play-index", "current");
            else
                mpv.togglePlay();
            return true;
        },
        true);

    view->registerAction("main/player/prev"_i18n, brls::BUTTON_LB, [&mpv](brls::View* view) {
        mpv.command("playlist-prev");
        return true;
    });
    view->registerAction(KeyBind::getLast(), [&mpv](brls::View* view) {
        mpv.command("playlist-prev");
        return true;
    });

    view->registerAction("main/player/next"_i18n, brls::BUTTON_RB, [&mpv](brls::View* view) {
        mpv.command("playlist-next");
        return true;
    });
    view->registerAction(KeyBind::getNext(), [&mpv](brls::View* view) {
        mpv.command("playlist-next");
        return true;
    });

    // lyrics and the track list share the same place on music pages.
    // Captures the page: a tapped hint passes the hint view, not the page.
    view->registerAction("main/player/lyric"_i18n, brls::BUTTON_X, [this, view](brls::View*) {
        auto* tracks = view->getView("album/tracks");
        auto* lyric = view->getView("album/lyric");
        bool show = lyric->getVisibility() == brls::Visibility::GONE;
        lyric->setVisibility(show ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
        tracks->setVisibility(show ? brls::Visibility::GONE : brls::Visibility::VISIBLE);
        this->playLyric->setVisibility(show ? brls::Visibility::INVISIBLE : brls::Visibility::VISIBLE);
        brls::Application::giveFocus(show ? this->btnToggle : tracks);
        return true;
    });
}

void MusicView::draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style,
    brls::FrameContext* ctx) {
    int index = this->lyricIndex();
    if (index != this->shownLyric) {
        this->shownLyric = index;
        this->playLyric->setText(index < 0 ? "" : this->lyrics[index].text);
    }
    Box::draw(vg, x, y, width, height, style, ctx);
}

int MusicView::lyricIndex() const {
    double t = MPVCore::instance().playback_time;
    auto it = std::upper_bound(
        this->lyrics.begin(), this->lyrics.end(), t, [](double t, const LyricLine& l) { return t < l.time; });
    return int(it - this->lyrics.begin()) - 1;
}

void MusicView::setLyrics(std::vector<LyricLine> lines) {
    this->lyrics = std::move(lines);
    // force draw() to refresh the label even if the index stays the same
    this->shownLyric = -2;
    MPVCore::instance().getCustomEvent()->fire(LYRIC_LOAD, nullptr);
}

void MusicView::doLyric(const std::string& id) {
    jellyfin::getJSON<jellyfin::Detail>(
        [this, id](const jellyfin::Detail& r) {
            // track may have changed while the request was in flight
            if (id == this->itemId) this->setLyrics(parseLrc(findLyric(r)));
        },
        nullptr, jellyfin::apiUserItem, AppConfig::instance().getUserId(), id);
}

const std::string& MusicView::currentId() { return this->itemId; }

void MusicView::image(brls::Image* image) {
    for (auto& it : this->playList) {
        if (it.second.Id == this->itemId) {
            Image::load(image, jellyfin::apiPrimaryImage, it.second.ImageId,
                HTTP::encode_form({
                    {"tag", it.second.ImageTag},
                    {"maxWidth", "240"},
                }));
        }
    }
}

void MusicView::load(const std::vector<jellyfin::Track>& items, size_t index) {
    auto& conf = AppConfig::instance();
    auto& mpv = MPVCore::instance();
    std::stringstream ssextra;
    ssextra << fmt::format("network-timeout={}", HTTP::TIMEOUT / 100);
    if (HTTP::PROXY_STATUS) ssextra << ",http-proxy=\"" << HTTP::PROXY << "\"";

    if (!this->playSession) this->registerMpvEvent();

    mpv.stop();
    mpv.enableVO(false);
    mpv.command("playlist-clear");
    this->playList.clear();
    this->btnSuffle->setBorderThickness(0);

    std::string query = HTTP::encode_form({
        {"static", "true"},
        {"PlaySessionId", std::to_string(playSession)},
        {"api_key", conf.getToken()},
    });

    for (auto& item : items) {
        uint64_t userdata = reinterpret_cast<uint64_t>(&item);
        std::string url = fmt::format(fmt::runtime(jellyfin::apiAudio), item.Id, query);
        mpv.setUrl(conf.getUrl() + url, ssextra.str(), "append", userdata);
    }

    mpv.command("playlist-play-index", std::to_string(index).c_str());
}

void MusicView::load(const std::vector<remote::DirEntry>& items, size_t index, const std::string& extra) {
    auto& mpv = MPVCore::instance();

    if (!this->playSession) this->registerMpvEvent();

    mpv.stop();
    mpv.enableVO(false);
    mpv.command("playlist-clear");
    this->playList.clear();
    this->btnSuffle->setBorderThickness(0);

    for (auto& item : items) {
        mpv.setUrl(item.url(), extra, "append");
    }

    mpv.command("playlist-play-index", std::to_string(index).c_str());
}

void MusicView::reset() {
    this->btnToggleIcon->setImageFromSVGRes("icon/ico-play.svg");
    this->rightStatusLabel->setText("--:--");
    this->leftStatusLabel->setText("--:--");
    this->osdSlider->setProgress(0);
    this->itemId.clear();
    this->setLyrics({});
}

bool MusicView::toggleShuffle() {
    auto& mpv = MPVCore::instance();

    if (this->btnSuffle->getBorderThickness() > 0) {
        mpv.command("playlist-unshuffle");
        this->btnSuffle->setBorderThickness(0);
    } else {
        mpv.command("playlist-shuffle");
        this->btnSuffle->setBorderThickness(2.0f);
    }
    return true;
}

bool MusicView::toggleLoop() {
    auto& mpv = MPVCore::instance();
    switch (this->repeat) {
    case RepeatNone:
        mpv.command("set", "loop-file", "inf");
        this->repeat = RepeatOne;
        this->btnRepeatIcon->setImageFromSVGRes("icon/ico-repeat-song.svg");
        break;
    case RepeatOne:
        mpv.command("set", "loop-file", "no");
        mpv.command("set", "loop-playlist", "inf");
        this->repeat = RepeatAll;
        this->btnRepeatIcon->setImageFromSVGRes("icon/ico-repeat-list.svg");
        break;
    default:
        mpv.command("set", "loop-playlist", "no");
        this->repeat = RepeatNone;
        this->btnRepeatIcon->setImageFromSVGRes("icon/ico-playlist.svg");
    }
    return true;
}