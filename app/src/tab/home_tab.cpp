/*
    Copyright 2023 dragonflylee
*/

#include "tab/home_tab.hpp"
#include "view/recyling_video.hpp"
#include "api/jellyfin.hpp"
#include "utils/keybind.hpp"

using namespace brls::literals;  // for _i18n

// Emby web defaults for homesection0..6
static const std::vector<std::string> defaultSections = {
    "smalllibrarytiles", "resume", "resumeaudio", "livetv", "none", "latestmedia", "none"};

HomeTab::HomeTab() {
    brls::Logger::debug("Tab HomeTab: create");
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/home.xml");
}

HomeTab::~HomeTab() { brls::Logger::debug("View HomeTab: delete"); }

brls::View* HomeTab::create() { return new HomeTab(); }

void HomeTab::doRequest() {
    for (auto& refresh : this->sections) refresh(false);
}

void HomeTab::onCreate() {
    auto actionRefresh = [this](brls::View* view) {
        for (auto& refresh : this->sections) refresh(true);
        return true;
    };

    this->registerAction("hints/refresh"_i18n, brls::BUTTON_BACK, actionRefresh);
    this->registerAction(KeyBind::getRefresh(), actionRefresh);

    ASYNC_RETAIN
    jellyfin::getJSON<jellyfin::DisplayPreferences>(
        [ASYNC_TOKEN](const jellyfin::DisplayPreferences& r) {
            ASYNC_RELEASE
            std::vector<std::string> types = defaultSections;
            for (size_t i = 0; i < types.size(); i++) {
                auto it = r.CustomPrefs.find(fmt::format("homesection{}", i));
                if (it != r.CustomPrefs.end() && it->is_string()) types[i] = it->get<std::string>();
            }
            this->doSections(types);
        },
        [ASYNC_TOKEN](const std::string& ex) {
            ASYNC_RELEASE
            this->doSections(defaultSections);
        },
        jellyfin::apiUserSetting, AppConfig::instance().getUserId());
}

void HomeTab::doSections(const std::vector<std::string>& types) {
    ASYNC_RETAIN
    jellyfin::getJSON<jellyfin::Result<jellyfin::Collection>>(
        [ASYNC_TOKEN, types](const jellyfin::Result<jellyfin::Collection>& r) {
            ASYNC_RELEASE
            for (auto& type : types) this->addSection(type, r.Items);
        },
        [ASYNC_TOKEN](const std::string& ex) {
            ASYNC_RELEASE
            auto dialog = new brls::Dialog(ex);
            dialog->addButton("hints/retry"_i18n, [this]() { brls::sync([this]() { this->onCreate(); }); });
            dialog->addButton("hints/cancel"_i18n, []() {});
            dialog->open();
        },
        jellyfin::apiUserViews, AppConfig::instance().getUserId());
}

RecylingVideo* HomeTab::addRow(const std::string& title, float width, float height) {
    RecylingVideo* row = new RecylingVideo();
    row->setTitle(title);
    row->setItemWidth(width);
    row->setFrameHeight(height);
    this->boxHome->addView(row);
    return row;
}

void HomeTab::addPaged(
    const std::string& title, float width, float height, std::function<std::string(size_t, size_t)> query) {
    RecylingVideo* row = this->addRow(title, width, height);
    row->setPageSize(12);
    row->onQuery(query);
    row->applyXMLAttribute("nextPage", "auto");
    row->doRequest();
    this->sections.push_back([row](bool refresh) {
        row->reset();
        row->doRequest(refresh);
    });
}

void HomeTab::addSection(const std::string& type, const std::vector<jellyfin::Collection>& views) {
    if (type == "resume") {
        this->addPaged("main/home/resume"_i18n, 325, 235, [](size_t start, size_t pageSize) {
            std::string query = HTTP::encode_form({
                {"enableImageTypes", "Primary,Backdrop,Thumb"},
                {"mediaTypes", "Video"},
                {"fields", "BasicSyncInfo,Chapters"},
                {"limit", std::to_string(pageSize)},
                {"startIndex", std::to_string(start)},
            });
            return fmt::format(fmt::runtime(jellyfin::apiUserResume), AppConfig::instance().getUserId(), query);
        });
    } else if (type == "nextup") {
        this->addPaged("main/home/nextup"_i18n, 325, 235, [](size_t start, size_t pageSize) {
            char cutoff[21] = {};
            const int maxNextup = AppConfig::instance().getItem(AppConfig::MAXDAY_NEXTUP, 365);
            const time_t tt = std::time(nullptr) - maxNextup * 24 * 3600;
            std::strftime(cutoff, sizeof(cutoff), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&tt));
            std::string query = HTTP::encode_form({
                {"userId", AppConfig::instance().getUserId()},
                {"fields", "BasicSyncInfo,Chapters"},
                {"enableImageTypes", "Primary,Backdrop,Thumb"},
                {"enableResumable", "false"},
                {"enableRewatching", "false"},
                {"nextUpDateCutoff", cutoff},
                {"LegacyNextUp", "true"},
                {"limit", std::to_string(pageSize)},
                {"startIndex", std::to_string(start)},
            });
            return fmt::format(fmt::runtime(jellyfin::apiShowNextUp), query);
        });
    } else if (type == "latestmoviereleases") {
        this->addPaged("main/home/movie_releases"_i18n, 175, 300, [](size_t start, size_t pageSize) {
            char since[21] = {};
            const time_t tt = std::time(nullptr) - 365 * 24 * 3600;
            std::strftime(since, sizeof(since), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&tt));
            std::string query = HTTP::encode_form({
                {"includeItemTypes", jellyfin::mediaTypeMovie},
                {"recursive", "true"},
                {"sortBy", "ProductionYear,PremiereDate,SortName"},
                {"sortOrder", "Descending"},
                {"minPremiereDate", since},
                {"enableImageTypes", "Primary"},
                {"fields", "BasicSyncInfo"},
                {"limit", std::to_string(pageSize)},
                {"startIndex", std::to_string(start)},
            });
            return fmt::format(fmt::runtime(jellyfin::apiUserLibrary), AppConfig::instance().getUserId(), query);
        });
    } else if (type == "collections" || type == "playlists") {
        std::string itemType = type == "collections" ? jellyfin::mediaTypeBoxSet : jellyfin::mediaTypePlaylist;
        std::string title = type == "collections" ? "main/home/collections"_i18n : "main/home/playlists"_i18n;
        this->addPaged(title, 175, 300, [itemType](size_t start, size_t pageSize) {
            std::string query = HTTP::encode_form({
                {"includeItemTypes", itemType},
                {"recursive", "true"},
                {"enableImageTypes", "Primary"},
                {"limit", std::to_string(pageSize)},
                {"startIndex", std::to_string(start)},
            });
            return fmt::format(fmt::runtime(jellyfin::apiUserLibrary), AppConfig::instance().getUserId(), query);
        });
    } else if (type == "latestmedia") {
        auto& excludes = AppConfig::instance().userConfig().LatestItemsExcludes;
        auto excluded = [&excludes](const std::string& id) {
            return !id.empty() && std::find(excludes.begin(), excludes.end(), id) != excludes.end();
        };
        const std::vector<std::string> skipTypes = {"playlists", "livetv", "boxsets", "channels"};

        for (auto& item : views) {
            if (excluded(item.Id) || excluded(item.Guid)) continue;
            if (std::find(skipTypes.begin(), skipTypes.end(), item.CollectionType) != skipTypes.end()) continue;

            float height = 300;
            if (item.CollectionType == "music") height = 225;
            if (item.CollectionType == "books") height = 280;

            RecylingVideo* row = this->addRow(item.Name, 175, height);
            std::string itemId = item.Id;
            row->onQuery([itemId](size_t start, size_t pageSize) {
                std::string query = HTTP::encode_form({
                    {"enableImageTypes", "Primary"},
                    {"parentId", itemId},
                    {"fields", "BasicSyncInfo,Chapters"},
                    {"limit", std::to_string(pageSize)},
                });
                return fmt::format(fmt::runtime(jellyfin::apiUserLatest), AppConfig::instance().getUserId(), query);
            });
            row->doLatest();
            this->sections.push_back([row](bool refresh) { row->doLatest(refresh); });
        }
    } else if (type == "livetv" || type == "onnow") {
        RecylingVideo* row = this->addRow("main/home/onnow"_i18n, 200, 150);
        row->setPageSize(24);
        row->onQuery([](size_t start, size_t pageSize) {
            std::string query = HTTP::encode_form({
                {"fields", "ChannelInfo"},
                {"enableImageTypes", "Primary"},
                {"isAiring", "true"},
                {"userId", AppConfig::instance().getUserId()},
                {"startIndex", std::to_string(start)},
                {"limit", std::to_string(pageSize)},
            });
            return fmt::format(fmt::runtime(jellyfin::apiProgramRecommend), query);
        });
        row->doLiveTV();
        this->sections.push_back([row](bool refresh) {
            row->reset();
            row->doLiveTV(refresh);
        });
    }
}
