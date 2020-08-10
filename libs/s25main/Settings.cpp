// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "Settings.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/strUtils.h"
#include "languages.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/StringConversion.h"
#include "s25util/System.h"
#include "s25util/error.h"
#include <boost/filesystem/operations.hpp>

const int Settings::VERSION = 13;
const std::array<std::string, 11> Settings::SECTION_NAMES = {
  {"global", "video", "language", "driver", "sound", "lobby", "server", "proxy", "interface", "ingame", "addons"}};

const std::array<short, 13> Settings::SCREEN_REFRESH_RATES = {{-1, 25, 30, 50, 60, 75, 80, 100, 120, 150, 180, 200, 240}};

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
    // 0 = ask user at start,1 = enabled, 2 = disabled
    global.submit_debug_data = 0;
    global.use_upnp = 2;
    global.smartCursor = true;
    global.debugMode = false;
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
    video.vsync = 0;
    video.vbo = true;
    video.shared_textures = true;
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
    sound.musik = false;
    sound.musik_volume = 30;
    sound.effekte = true;
    sound.effekte_volume = 75;
    sound.playlist = s25::files::defaultPlaylist;
    // }

    // lobby
    // {

    lobby.name = System::getUserName();
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
    interface.revert_mouse = false;
    // }

    // ingame
    // {
    ingame.scale_statistics = false;
    // }

    // addons
    // {
    addons.configuration.clear();
    // }
}

///////////////////////////////////////////////////////////////////////////////
// Routine zum Laden der Konfiguration
void Settings::Load()
{
    libsiedler2::Archiv settings;
    std::string settingsPath = RTTRCONFIG.ExpandPath(s25::resources::config);
    try
    {
        if(libsiedler2::Load(settingsPath, settings) != 0 || settings.size() != SECTION_NAMES.size())
            throw std::runtime_error("File missing or invalid");

        const libsiedler2::ArchivItem_Ini* iniGlobal = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("global"));
        const libsiedler2::ArchivItem_Ini* iniVideo = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("video"));
        const libsiedler2::ArchivItem_Ini* iniLanguage = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("language"));
        const libsiedler2::ArchivItem_Ini* iniDriver = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("driver"));
        const libsiedler2::ArchivItem_Ini* iniSound = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("sound"));
        const libsiedler2::ArchivItem_Ini* iniLobby = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("lobby"));
        const libsiedler2::ArchivItem_Ini* iniServer = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("server"));
        const libsiedler2::ArchivItem_Ini* iniProxy = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("proxy"));
        const libsiedler2::ArchivItem_Ini* iniInterface = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("interface"));
        const libsiedler2::ArchivItem_Ini* iniIngame = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("ingame"));
        const libsiedler2::ArchivItem_Ini* iniAddons = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("addons"));

        // ist eine der Kategorien nicht vorhanden?
        if(!iniGlobal || !iniVideo || !iniLanguage || !iniDriver || !iniSound || !iniLobby || !iniServer || !iniProxy || !iniInterface
           || !iniIngame || !iniAddons)
        {
            throw std::runtime_error("Missing section");
        }
        // stimmt die Settingsversion?
        if(iniGlobal->getValueI("version") != VERSION)
        {
            throw std::runtime_error("Wrong version");
        }

        // global
        // {
        // stimmt die Spielrevision überein?
        if(iniGlobal->getValue("gameversion") != RTTR_Version::GetRevision())
            s25util::warning("Your application version has changed - please recheck your settings!\n");

        global.submit_debug_data = iniGlobal->getValueI("submit_debug_data");
        global.use_upnp = iniGlobal->getValueI("use_upnp");
        global.smartCursor = (iniGlobal->getValue("smartCursor").empty() || iniGlobal->getValueI("smartCursor") != 0);
        global.debugMode = (iniGlobal->getValueI("debugMode") != 0);

        // };

        // video
        // {
        video.windowedSize.width = iniVideo->getValueI("windowed_width");
        video.windowedSize.height = iniVideo->getValueI("windowed_height");
        video.fullscreenSize.width = iniVideo->getValueI("fullscreen_width");
        video.fullscreenSize.height = iniVideo->getValueI("fullscreen_height");
        video.fullscreen = (iniVideo->getValueI("fullscreen") != 0);
        video.vsync = iniVideo->getValueI("vsync");
        video.vbo = (iniVideo->getValueI("vbo") != 0);
        video.shared_textures = (iniVideo->getValueI("shared_textures") != 0);
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
        sound.musik = (iniSound->getValueI("musik") != 0);
        sound.musik_volume = iniSound->getValueI("musik_volume");
        sound.effekte = (iniSound->getValueI("effekte") != 0);
        sound.effekte_volume = iniSound->getValueI("effekte_volume");
        sound.playlist = iniSound->getValue("playlist");
        // }

        // lobby
        // {
        lobby.name = iniLobby->getValue("name");
        lobby.password = iniLobby->getValue("password");
        lobby.save_password = (iniLobby->getValueI("save_password") != 0);
        // }

        if(lobby.name.empty())
            lobby.name = System::getUserName();

        // server
        // {
        server.last_ip = iniServer->getValue("last_ip");
        boost::optional<uint16_t> port = validate::checkPort(iniServer->getValue("local_port"));
        server.localPort = port.value_or(3665);
        server.ipv6 = (iniServer->getValueI("ipv6") != 0);
        // }

        // proxy
        // {
        proxy.hostname = iniProxy->getValue("proxy");
        port = validate::checkPort(iniProxy->getValue("port"));
        proxy.port = port.value_or(1080);
        proxy.type = ProxyType(iniProxy->getValueI("typ"));
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
        interface.autosave_interval = iniInterface->getValueI("autosave_interval");
        interface.revert_mouse = (iniInterface->getValueI("revert_mouse") != 0);
        // }

        // ingame
        // {
        ingame.scale_statistics = (iniIngame->getValueI("scale_statistics") != 0);
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
        // }

    } catch(std::runtime_error& e)
    {
        s25util::warning(std::string("Could not use settings from \"") + settingsPath + "\", using default values. Reason: " + e.what());
        LoadDefaults();
        Save();
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
    libsiedler2::ArchivItem_Ini* iniIngame = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("ingame"));
    libsiedler2::ArchivItem_Ini* iniAddons = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("addons"));

    // ist eine der Kategorien nicht vorhanden?
    RTTR_Assert(iniGlobal && iniVideo && iniLanguage && iniDriver && iniSound && iniLobby && iniServer && iniProxy && iniInterface
                && iniIngame && iniAddons);

    // global
    // {
    iniGlobal->setValue("version", VERSION);
    iniGlobal->setValue("gameversion", RTTR_Version::GetRevision());
    iniGlobal->setValue("submit_debug_data", global.submit_debug_data);
    iniGlobal->setValue("use_upnp", global.use_upnp);
    iniGlobal->setValue("smartCursor", global.smartCursor ? 1 : 0);
    iniGlobal->setValue("debugMode", global.debugMode ? 1 : 0);
    // };

    // video
    // {
    iniVideo->setValue("fullscreen_width", video.fullscreenSize.width);
    iniVideo->setValue("fullscreen_height", video.fullscreenSize.height);
    iniVideo->setValue("windowed_width", video.windowedSize.width);
    iniVideo->setValue("windowed_height", video.windowedSize.height);
    iniVideo->setValue("fullscreen", (video.fullscreen ? 1 : 0));
    iniVideo->setValue("vsync", video.vsync);
    iniVideo->setValue("vbo", (video.vbo ? 1 : 0));
    iniVideo->setValue("shared_textures", (video.shared_textures ? 1 : 0));
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
    iniSound->setValue("musik", (sound.musik ? 1 : 0));
    iniSound->setValue("musik_volume", sound.musik_volume);
    iniSound->setValue("effekte", (sound.effekte ? 1 : 0));
    iniSound->setValue("effekte_volume", sound.effekte_volume);
    iniSound->setValue("playlist", sound.playlist);
    // }

    // lobby
    // {
    iniLobby->setValue("name", lobby.name);
    iniLobby->setValue("password", lobby.password);
    iniLobby->setValue("save_password", (lobby.save_password ? 1 : 0));
    // }

    // server
    // {
    iniServer->setValue("last_ip", server.last_ip);
    iniServer->setValue("local_port", server.localPort);
    iniServer->setValue("ipv6", (server.ipv6 ? 1 : 0));
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
    iniInterface->setValue("revert_mouse", (interface.revert_mouse ? 1 : 0));
    // }

    // ingame
    // {
    iniIngame->setValue("scale_statistics", (ingame.scale_statistics ? 1 : 0));
    // }

    // addons
    // {
    iniAddons->clear();
    for(const auto& it : addons.configuration)
        iniAddons->addValue(s25util::toStringClassic(it.first), s25util::toStringClassic(it.second));
    // }

    bfs::path settingsPath = RTTRCONFIG.ExpandPath(s25::resources::config);
    if(libsiedler2::Write(settingsPath.string(), settings) == 0)
        bfs::permissions(settingsPath, bfs::owner_read | bfs::owner_write);
}
