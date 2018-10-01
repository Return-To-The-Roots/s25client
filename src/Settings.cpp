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

#include "rttrDefines.h" // IWYU pragma: keep
#include "Settings.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "languages.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/libsiedler2.h"
#include "libutil/StringConversion.h"
#include "libutil/System.h"
#include "libutil/error.h"
#include <boost/filesystem/operations.hpp>

const unsigned Settings::SETTINGS_VERSION = 12;
const unsigned Settings::SETTINGS_SECTIONS = 11;
const std::string Settings::SETTINGS_SECTION_NAMES[] = {"global", "video", "language",  "driver", "sound", "lobby",
                                                        "server", "proxy", "interface", "ingame", "addons"};

const unsigned char Settings::NUM_SCREEN_REFRESH_RATESS = 14;
const unsigned short Settings::SCREEN_REFRESH_RATES[] = {0, 1, 25, 30, 50, 60, 75, 80, 100, 120, 150, 180, 200, 240};

Settings::Settings() //-V730
{}

bool Settings::LoadDefaults()
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
        video.fullscreenSize = VIDEODRIVER.GetScreenSize();
        video.windowedSize = VIDEODRIVER.IsFullscreen() ? Extent(800, 600) : video.fullscreenSize;
        video.fullscreen = VIDEODRIVER.IsFullscreen();
    } else
    {
        video.windowedSize = video.fullscreenSize = Extent(800, 600);
        video.fullscreen = false;
    }
    video.vsync = 0;
    video.vbo = false;
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
    sound.playlist = "S2_Standard";
    // }

    // lobby
    // {

    lobby.name = System::getUserName();
    lobby.password.clear();
    lobby.email.clear();
    lobby.save_password = false;
    // }

    // server
    // {
    server.last_ip.clear();
    server.ipv6 = false;
    // }

    proxy = ProxySettings();

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

    Save();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Routine zum Laden der Konfiguration
bool Settings::Load()
{
    libsiedler2::Archiv settings;
    std::string settingsPath = RTTRCONFIG.ExpandPath(FILE_PATHS[0]);
    if(libsiedler2::Load(settingsPath, settings) != 0 || settings.size() != SETTINGS_SECTIONS)
    {
        s25util::warning(std::string("No or corrupt \"") + settingsPath + "\" found, using default values.");
        return LoadDefaults();
    }

    try
    {
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
           || !iniIngame || !iniAddons ||
           // stimmt die Settingsversion?
           ((unsigned)iniGlobal->getValueI("version") != SETTINGS_VERSION))
        {
            // nein, dann Standardeinstellungen laden
            s25util::warning(settingsPath + " found, but its corrupted or has wrong version. Loading default values.");
            return LoadDefaults();
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
        video.windowedSize.x = iniVideo->getValueI("windowed_width");
        video.windowedSize.y = iniVideo->getValueI("windowed_height");
        video.fullscreenSize.x = iniVideo->getValueI("fullscreen_width");
        video.fullscreenSize.y = iniVideo->getValueI("fullscreen_height");
        video.fullscreen = (iniVideo->getValueI("fullscreen") != 0);
        video.vsync = iniVideo->getValueI("vsync");
        video.vbo = (iniVideo->getValueI("vbo") != 0);
        video.shared_textures = (iniVideo->getValueI("shared_textures") != 0);
        // };

        if(video.fullscreenSize.x == 0 || video.fullscreenSize.y == 0 || video.windowedSize.x == 0 || video.windowedSize.y == 0)
        {
            s25util::warning(std::string("Corrupted \"") + settingsPath + "\" found, using default values.");
            return LoadDefaults();
        }

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
        lobby.email = iniLobby->getValue("email");
        lobby.password = iniLobby->getValue("password");
        lobby.save_password = (iniLobby->getValueI("save_password") != 0);
        // }

        if(lobby.name.empty())
            lobby.name = System::getUserName();

        // server
        // {
        server.last_ip = iniServer->getValue("last_ip");
        server.ipv6 = (iniServer->getValueI("ipv6") != 0);
        // }

        // proxy
        // {
        proxy.hostname = iniProxy->getValue("proxy");
        proxy.port = iniProxy->getValueI("port");
        proxy.type = ProxyType(iniProxy->getValueI("typ"));
        // }

        // leere proxyadresse deaktiviert proxy komplett
        // deaktivierter proxy entfernt proxyadresse
        if(proxy.hostname.empty() || (proxy.type != PROXY_SOCKS4 && proxy.type != PROXY_SOCKS5))
        {
            proxy.type = PROXY_NONE;
            proxy.hostname.clear();
        }
        // aktivierter Socks v4 deaktiviert ipv6
        else if(proxy.type == PROXY_SOCKS4 && server.ipv6)
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
            const libsiedler2::ArchivItem_Text* item = dynamic_cast<const libsiedler2::ArchivItem_Text*>(iniAddons->get(addon));

            if(item)
                addons.configuration.insert(std::make_pair(s25util::fromStringClassic<unsigned>(item->getName()),
                                                           s25util::fromStringClassic<unsigned>(item->getText())));
        }
        // }

    } catch(s25util::ConversionError& e)
    {
        s25util::warning(std::string("Corrupt \"") + settingsPath + "\" found, using default values. Error: " + e.what());
        return LoadDefaults();
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Routine zum Speichern der Konfiguration
void Settings::Save()
{
    libsiedler2::Archiv settings;
    settings.alloc(SETTINGS_SECTIONS);
    for(unsigned i = 0; i < SETTINGS_SECTIONS; ++i)
        settings.set(i, new libsiedler2::ArchivItem_Ini(SETTINGS_SECTION_NAMES[i]));

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
    iniGlobal->setValue("version", SETTINGS_VERSION);
    iniGlobal->setValue("gameversion", RTTR_Version::GetRevision());
    iniGlobal->setValue("submit_debug_data", global.submit_debug_data);
    iniGlobal->setValue("use_upnp", global.use_upnp);
    iniGlobal->setValue("smartCursor", global.smartCursor ? 1 : 0);
    iniGlobal->setValue("debugMode", global.debugMode ? 1 : 0);
    // };

    // video
    // {
    iniVideo->setValue("fullscreen_width", video.fullscreenSize.x);
    iniVideo->setValue("fullscreen_height", video.fullscreenSize.y);
    iniVideo->setValue("windowed_width", video.windowedSize.x);
    iniVideo->setValue("windowed_height", video.windowedSize.y);
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
    iniLobby->setValue("email", lobby.email);
    iniLobby->setValue("password", lobby.password);
    iniLobby->setValue("save_password", (lobby.save_password ? 1 : 0));
    // }

    // server
    // {
    iniServer->setValue("last_ip", server.last_ip);
    iniServer->setValue("ipv6", (server.ipv6 ? 1 : 0));
    // }

    // proxy
    // {
    iniProxy->setValue("proxy", proxy.hostname);
    iniProxy->setValue("port", proxy.port);
    iniProxy->setValue("typ", proxy.type);
    // }

    // interface
    // {
    iniInterface->setValue("autosave_interval", interface.autosave_interval);
    iniInterface->setValue("revert_mouse", (interface.revert_mouse));
    // }

    // ingame
    // {
    iniIngame->setValue("scale_statistics", (ingame.scale_statistics ? 1 : 0));
    // }

    // addons
    // {
    iniAddons->clear();
    for(std::map<unsigned, unsigned>::const_iterator it = addons.configuration.begin(); it != addons.configuration.end(); ++it)
        iniAddons->addValue(s25util::toStringClassic(it->first), s25util::toStringClassic(it->second));
    // }

    bfs::path settingsPath = RTTRCONFIG.ExpandPath(FILE_PATHS[0]);
    if(libsiedler2::Write(settingsPath.string(), settings) == 0)
        bfs::permissions(settingsPath, bfs::owner_read | bfs::owner_write);
}
