// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Settings.h"
#include "DrawPoint.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/strUtils.h"
#include "languages.h"
#include "gameData/PortraitConsts.h"
#include "gameData/const_gui_ids.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/StringConversion.h"
#include "s25util/System.h"
#include "s25util/error.h"
#include <boost/filesystem/operations.hpp>

const int Settings::VERSION = 13;
const std::array<std::string, 10> Settings::SECTION_NAMES = {
  {"global", "video", "language", "driver", "sound", "lobby", "server", "proxy", "interface", "addons"}};

const std::array<short, 13> Settings::SCREEN_REFRESH_RATES = {
  {-1, 25, 30, 50, 60, 75, 80, 100, 120, 150, 180, 200, 240}};

const std::map<GUI_ID, std::string> persistentWindows = {{CGI_CHAT, "wnd_chat"},
                                                         {CGI_POSTOFFICE, "wnd_postoffice"},
                                                         {CGI_DISTRIBUTION, "wnd_distribution"},
                                                         {CGI_BUILDORDER, "wnd_buildorder"},
                                                         {CGI_TRANSPORT, "wnd_transport"},
                                                         {CGI_MILITARY, "wnd_military"},
                                                         {CGI_TOOLS, "wnd_tools"},
                                                         {CGI_INVENTORY, "wnd_inventory"},
                                                         {CGI_MINIMAP, "wnd_minimap"},
                                                         {CGI_BUILDINGS, "wnd_buildings"},
                                                         {CGI_BUILDINGSPRODUCTIVITY, "wnd_buildingsproductivity"},
                                                         {CGI_MUSICPLAYER, "wnd_musicplayer"},
                                                         {CGI_STATISTICS, "wnd_statistics"},
                                                         {CGI_ECONOMICPROGRESS, "wnd_economicprogress"},
                                                         {CGI_DIPLOMACY, "wnd_diplomacy"},
                                                         {CGI_SHIP, "wnd_ship"},
                                                         {CGI_MERCHANDISE_STATISTICS, "wnd_merchandise_statistics"}};

namespace validate {
boost::optional<uint16_t> checkPort(const std::string& port)
{
    int32_t iPort;
    if((helpers::tryFromString(port, iPort) || s25util::tryFromStringClassic(port, iPort)) && checkPort(iPort))
        return static_cast<uint16_t>(iPort);
    else
        return boost::none;
}
bool checkPort(int port)
{
    // Disallow port 0 as it may cause problems
    return port > 0 && port <= 65535;
}
} // namespace validate

Settings::Settings() //-V730
{
    LoadDefaults();
}

void Settings::LoadDefaults()
{
    // global
    // {
    // 0 = ask user at start, 1 = enabled, 2 = always ask
    global.submit_debug_data = 0;
    global.use_upnp = false;
    global.smartCursor = true;
    global.debugMode = false;
    global.showGFInfo = false;
    // }

    // video
    // {
    if(VIDEODRIVER.IsLoaded())
    {
        video.fullscreenSize = VIDEODRIVER.GetWindowSize();
        video.windowedSize = VIDEODRIVER.IsFullscreen() ? VideoMode(800, 600) : video.fullscreenSize;
        video.fullscreen = VIDEODRIVER.IsFullscreen();
    } else
    {
        video.windowedSize = video.fullscreenSize = VideoMode(800, 600);
        video.fullscreen = false;
    }
    video.framerate = 0; // Special value for HW vsync
    video.vbo = true;
    video.shared_textures = true;
    video.guiScale = 0; // special value indicating automatic selection
    // }

    // language
    // {
    language.language.clear();
    // }

    LANGUAGES.setLanguage(language.language);

    // driver
    // {
    driver.audio = AUDIODRIVER.GetName();
    driver.video = VIDEODRIVER.GetName();
    // }

    // sound
    // {
    sound.musicEnabled = false;
    sound.musicVolume = 30;
    sound.effectsEnabled = true;
    sound.effectsVolume = 75;
    sound.birdsEnabled = true;
    sound.playlist = s25::files::defaultPlaylist;
    // }

    // lobby
    // {

    lobby.name = System::getUserName();
    lobby.portraitIndex = 0;
    lobby.password.clear();
    lobby.save_password = false;
    // }

    // server
    // {
    server.last_ip.clear();
    server.localPort = 3665;
    server.ipv6 = false;
    // }

    proxy = ProxySettings();
    proxy.port = 1080;

    // interface
    // {
    interface.autosave_interval = 0;
    interface.invertMouse = false;
    interface.enableWindowPinning = false;
    interface.windowSnapDistance = 8;
    // }

    // addons
    // {
    addons.configuration.clear();
    // }

    LoadIngameDefaults();
}

void Settings::LoadIngameDefaults()
{
    // ingame
    // {
    ingame.scale_statistics = false;
    ingame.showBQ = false;
    ingame.showNames = false;
    ingame.showProductivity = false;
    ingame.minimapExtended = false;
    // }

    // windows
    // {
    for(const auto& window : persistentWindows)
        windows.persistentSettings[window.first] = PersistentWindowSettings();
    // }
}

///////////////////////////////////////////////////////////////////////////////
// Routine zum Laden der Konfiguration
void Settings::Load()
{
    libsiedler2::Archiv settings;
    const auto settingsPath = RTTRCONFIG.ExpandPath(s25::resources::config);
    try
    {
        if(libsiedler2::Load(settingsPath, settings) != 0 || settings.size() < SECTION_NAMES.size())
            throw std::runtime_error("File missing or invalid");

        const libsiedler2::ArchivItem_Ini* iniGlobal =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("global"));
        const libsiedler2::ArchivItem_Ini* iniVideo = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("video"));
        const libsiedler2::ArchivItem_Ini* iniLanguage =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("language"));
        const libsiedler2::ArchivItem_Ini* iniDriver =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("driver"));
        const libsiedler2::ArchivItem_Ini* iniSound = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("sound"));
        const libsiedler2::ArchivItem_Ini* iniLobby = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("lobby"));
        const libsiedler2::ArchivItem_Ini* iniServer =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("server"));
        const libsiedler2::ArchivItem_Ini* iniProxy = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("proxy"));
        const libsiedler2::ArchivItem_Ini* iniInterface =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("interface"));
        const libsiedler2::ArchivItem_Ini* iniAddons =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("addons"));

        // ist eine der Kategorien nicht vorhanden?
        if(!iniGlobal || !iniVideo || !iniLanguage || !iniDriver || !iniSound || !iniLobby || !iniServer || !iniProxy
           || !iniInterface || !iniAddons)
        {
            throw std::runtime_error("Missing section");
        }
        // stimmt die Settingsversion?
        if(iniGlobal->getValue("version", 0) != VERSION)
            throw std::runtime_error("Wrong version");

        // global
        // {
        if(iniGlobal->getValue("gameversion") != rttr::version::GetRevision())
            s25util::warning("Your application version has changed - please recheck your settings!");

        global.submit_debug_data = iniGlobal->getIntValue("submit_debug_data");
        global.use_upnp = iniGlobal->getBoolValue("use_upnp");
        global.smartCursor = iniGlobal->getValue("smartCursor", true);
        global.debugMode = iniGlobal->getValue("debugMode", false);
        global.showGFInfo = iniGlobal->getValue("showGFInfo", false);
        // };

        // video
        // {
        video.windowedSize.width = iniVideo->getIntValue("windowed_width");
        video.windowedSize.height = iniVideo->getIntValue("windowed_height");
        video.fullscreenSize.width = iniVideo->getIntValue("fullscreen_width");
        video.fullscreenSize.height = iniVideo->getIntValue("fullscreen_height");
        video.fullscreen = iniVideo->getBoolValue("fullscreen");
        video.framerate = iniVideo->getValue("framerate", 0);
        video.vbo = iniVideo->getBoolValue("vbo");
        video.shared_textures = iniVideo->getBoolValue("shared_textures");
        video.guiScale = iniVideo->getValue("gui_scale", 0);
        // };

        if(video.fullscreenSize.width == 0 || video.fullscreenSize.height == 0 || video.windowedSize.width == 0
           || video.windowedSize.height == 0)
            throw std::runtime_error("Invalid video settings");

        // language
        // {
        language.language = iniLanguage->getValue("language");
        // }

        LANGUAGES.setLanguage(language.language);

        // driver
        // {
        driver.video = iniDriver->getValue("video");
        driver.audio = iniDriver->getValue("audio");
        // }

        // sound
        // {
        sound.musicEnabled = iniSound->getBoolValue("musik");
        sound.musicVolume = iniSound->getIntValue("musik_volume");
        sound.effectsEnabled = iniSound->getBoolValue("effekte");
        sound.effectsVolume = iniSound->getIntValue("effekte_volume");
        sound.birdsEnabled = iniSound->getBoolValue("bird_sounds");
        sound.playlist = iniSound->getValue("playlist");
        // }

        // lobby
        // {
        lobby.name = iniLobby->getValue("name");
        lobby.portraitIndex = iniLobby->getIntValue("portrait_index");
        lobby.password = iniLobby->getValue("password");
        lobby.save_password = iniLobby->getBoolValue("save_password");
        // }

        if(lobby.name.empty())
            lobby.name = System::getUserName();

        if(lobby.portraitIndex >= Portraits.size())
        {
            lobby.portraitIndex = 0;
        }

        // server
        // {
        server.last_ip = iniServer->getValue("last_ip");
        boost::optional<uint16_t> port = validate::checkPort(iniServer->getValue("local_port"));
        server.localPort = port.value_or(3665);
        server.ipv6 = iniServer->getBoolValue("ipv6");
        // }

        // proxy
        // {
        proxy.hostname = iniProxy->getValue("proxy");
        port = validate::checkPort(iniProxy->getValue("port"));
        proxy.port = port.value_or(1080);
        proxy.type = ProxyType(iniProxy->getIntValue("typ"));
        // }

        // leere proxyadresse deaktiviert proxy komplett
        // deaktivierter proxy entfernt proxyadresse
        if(proxy.hostname.empty() || (proxy.type != ProxyType::Socks4 && proxy.type != ProxyType::Socks5))
        {
            proxy.type = ProxyType::None;
            proxy.hostname.clear();
        }
        // aktivierter Socks v4 deaktiviert ipv6
        else if(proxy.type == ProxyType::Socks4 && server.ipv6)
            server.ipv6 = false;

        // interface
        // {
        interface.autosave_interval = iniInterface->getIntValue("autosave_interval");
        interface.invertMouse = iniInterface->getValue("invert_mouse", false);
        interface.enableWindowPinning = iniInterface->getValue("enable_window_pinning", false);
        interface.windowSnapDistance = iniInterface->getValue("window_snap_distance", 8);
        // }

        // addons
        // {
        for(unsigned addon = 0; addon < iniAddons->size(); ++addon)
        {
            const auto* item = dynamic_cast<const libsiedler2::ArchivItem_Text*>(iniAddons->get(addon));

            if(item)
                addons.configuration.insert(std::make_pair(s25util::fromStringClassic<unsigned>(item->getName()),
                                                           s25util::fromStringClassic<unsigned>(item->getText())));
        }

        LoadIngame();
        // }
    } catch(const std::runtime_error& e)
    {
        s25util::warning(std::string("Could not use settings from \"") + settingsPath.string()
                         + "\", using default values. Reason: " + e.what());
        LoadDefaults();
        Save();
    }
}

void Settings::LoadIngame()
{
    libsiedler2::Archiv settingsIngame;
    const auto settingsPathIngame = RTTRCONFIG.ExpandPath(s25::resources::ingameOptions);
    try
    {
        if(libsiedler2::Load(settingsPathIngame, settingsIngame) != 0)
            throw std::runtime_error("File missing");

        const libsiedler2::ArchivItem_Ini* iniIngame =
          static_cast<libsiedler2::ArchivItem_Ini*>(settingsIngame.find("ingame"));
        if(!iniIngame)
            throw std::runtime_error("Missing section");
        // ingame
        // {
        ingame.scale_statistics = iniIngame->getBoolValue("scale_statistics");
        ingame.showBQ = iniIngame->getBoolValue("show_building_quality");
        ingame.showNames = iniIngame->getBoolValue("show_names");
        ingame.showProductivity = iniIngame->getBoolValue("show_productivity");
        ingame.minimapExtended = iniIngame->getBoolValue("minimap_extended");
        // }
        // ingame windows
        for(const auto& window : persistentWindows)
        {
            const auto* iniWindow = static_cast<const libsiedler2::ArchivItem_Ini*>(settingsIngame.find(window.second));
            if(!iniWindow)
                continue;
            auto& settings = windows.persistentSettings[window.first];
            const auto lastPos = settings.lastPos =
              DrawPoint(iniWindow->getIntValue("pos_x"), iniWindow->getIntValue("pos_y"));
            settings.restorePos = DrawPoint(iniWindow->getValue("restore_pos_x", lastPos.x),
                                            iniWindow->getValue("restore_pos_y", lastPos.y));
            settings.isOpen = iniWindow->getIntValue("is_open");
            settings.isPinned = iniWindow->getValue("is_pinned", false);
            settings.isMinimized = iniWindow->getValue("is_minimized", false);
        }
    } catch(const std::runtime_error& e)
    {
        s25util::warning(std::string("Could not use ingame settings from \"") + settingsPathIngame.string()
                         + "\", using default values. Reason: " + e.what());
        LoadIngameDefaults();
        SaveIngame();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Routine zum Speichern der Konfiguration
void Settings::Save()
{
    libsiedler2::Archiv settings;
    settings.alloc(SECTION_NAMES.size());
    for(unsigned i = 0; i < SECTION_NAMES.size(); ++i)
        settings.set(i, std::make_unique<libsiedler2::ArchivItem_Ini>(SECTION_NAMES[i]));

    libsiedler2::ArchivItem_Ini* iniGlobal = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("global"));
    libsiedler2::ArchivItem_Ini* iniVideo = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("video"));
    libsiedler2::ArchivItem_Ini* iniLanguage = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("language"));
    libsiedler2::ArchivItem_Ini* iniDriver = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("driver"));
    libsiedler2::ArchivItem_Ini* iniSound = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("sound"));
    libsiedler2::ArchivItem_Ini* iniLobby = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("lobby"));
    libsiedler2::ArchivItem_Ini* iniServer = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("server"));
    libsiedler2::ArchivItem_Ini* iniProxy = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("proxy"));
    libsiedler2::ArchivItem_Ini* iniInterface = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("interface"));
    libsiedler2::ArchivItem_Ini* iniAddons = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("addons"));

    // ist eine der Kategorien nicht vorhanden?
    RTTR_Assert(iniGlobal && iniVideo && iniLanguage && iniDriver && iniSound && iniLobby && iniServer && iniProxy
                && iniInterface && iniAddons);

    // global
    // {
    iniGlobal->setValue("version", VERSION);
    iniGlobal->setValue("gameversion", rttr::version::GetRevision());
    iniGlobal->setValue("submit_debug_data", global.submit_debug_data);
    iniGlobal->setValue("use_upnp", global.use_upnp);
    iniGlobal->setValue("smartCursor", global.smartCursor);
    iniGlobal->setValue("debugMode", global.debugMode);
    iniGlobal->setValue("showGFInfo", global.showGFInfo);
    // };

    // video
    // {
    iniVideo->setValue("fullscreen_width", video.fullscreenSize.width);
    iniVideo->setValue("fullscreen_height", video.fullscreenSize.height);
    iniVideo->setValue("windowed_width", video.windowedSize.width);
    iniVideo->setValue("windowed_height", video.windowedSize.height);
    iniVideo->setValue("fullscreen", video.fullscreen);
    iniVideo->setValue("framerate", video.framerate);
    iniVideo->setValue("vbo", video.vbo);
    iniVideo->setValue("shared_textures", video.shared_textures);
    iniVideo->setValue("gui_scale", video.guiScale);
    // };

    // language
    // {
    iniLanguage->setValue("language", language.language);
    // }

    // driver
    // {
    iniDriver->setValue("video", driver.video);
    iniDriver->setValue("audio", driver.audio);
    // }

    // sound
    // {
    iniSound->setValue("musik", sound.musicEnabled);
    iniSound->setValue("musik_volume", sound.musicVolume);
    iniSound->setValue("effekte", sound.effectsEnabled);
    iniSound->setValue("effekte_volume", sound.effectsVolume);
    iniSound->setValue("bird_sounds", sound.birdsEnabled);
    iniSound->setValue("playlist", sound.playlist);
    // }

    // lobby
    // {
    iniLobby->setValue("name", lobby.name);
    iniLobby->setValue("portrait_index", lobby.portraitIndex);
    iniLobby->setValue("password", lobby.password);
    iniLobby->setValue("save_password", lobby.save_password);
    // }

    // server
    // {
    iniServer->setValue("last_ip", server.last_ip);
    iniServer->setValue("local_port", server.localPort);
    iniServer->setValue("ipv6", server.ipv6);
    // }

    // proxy
    // {
    iniProxy->setValue("proxy", proxy.hostname);
    iniProxy->setValue("port", proxy.port);
    iniProxy->setValue("typ", static_cast<int>(proxy.type));
    // }

    // interface
    // {
    iniInterface->setValue("autosave_interval", interface.autosave_interval);
    iniInterface->setValue("invert_mouse", interface.invertMouse);
    iniInterface->setValue("enable_window_pinning", interface.enableWindowPinning);
    iniInterface->setValue("window_snap_distance", interface.windowSnapDistance);
    // }

    // addons
    // {
    iniAddons->clear();
    for(const auto& it : addons.configuration)
        iniAddons->setValue(s25util::toStringClassic(it.first), s25util::toStringClassic(it.second));
    // }

    bfs::path settingsPath = RTTRCONFIG.ExpandPath(s25::resources::config);
    if(libsiedler2::Write(settingsPath, settings) == 0)
        bfs::permissions(settingsPath, bfs::owner_read | bfs::owner_write);

    SaveIngame();
}

void Settings::SaveIngame()
{
    libsiedler2::Archiv settingsIngame;
    settingsIngame.alloc(1 + persistentWindows.size());
    settingsIngame.set(0, std::make_unique<libsiedler2::ArchivItem_Ini>("ingame"));
    unsigned i = 1;
    for(const auto& window : persistentWindows)
    {
        settingsIngame.set(i, std::make_unique<libsiedler2::ArchivItem_Ini>(window.second));
        i++;
    }

    auto* iniIngame = static_cast<libsiedler2::ArchivItem_Ini*>(settingsIngame.find("ingame"));

    RTTR_Assert(iniIngame);

    // ingame
    // {
    iniIngame->setValue("scale_statistics", ingame.scale_statistics);
    iniIngame->setValue("show_building_quality", ingame.showBQ);
    iniIngame->setValue("show_names", ingame.showNames);
    iniIngame->setValue("show_productivity", ingame.showProductivity);
    iniIngame->setValue("minimap_extended", ingame.minimapExtended);
    // }

    // ingame windows
    for(const auto& window : persistentWindows)
    {
        auto* iniWindow = static_cast<libsiedler2::ArchivItem_Ini*>(settingsIngame.find(window.second));
        if(!iniWindow)
            continue;
        const auto& settings = windows.persistentSettings[window.first];
        iniWindow->setValue("pos_x", settings.lastPos.x);
        iniWindow->setValue("pos_y", settings.lastPos.y);
        if(settings.restorePos != settings.lastPos)
        {
            // only save if different; defaults to lastPos on load
            iniWindow->setValue("restore_pos_x", settings.restorePos.x);
            iniWindow->setValue("restore_pos_y", settings.restorePos.y);
        }
        iniWindow->setValue("is_open", settings.isOpen);
        iniWindow->setValue("is_pinned", settings.isPinned);
        iniWindow->setValue("is_minimized", settings.isMinimized);
    }

    bfs::path settingsPathIngame = RTTRCONFIG.ExpandPath(s25::resources::ingameOptions);
    if(libsiedler2::Write(settingsPathIngame, settingsIngame) == 0)
        bfs::permissions(settingsPathIngame, bfs::owner_read | bfs::owner_write);
}
