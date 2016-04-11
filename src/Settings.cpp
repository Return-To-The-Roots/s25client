// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "Settings.h"

#include "files.h"
#include "fileFuncs.h"
#include "Loader.h"
#include "languages.h"
#include "build_version.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "libsiedler2/src/ArchivItem_Ini.h"
#include "libsiedler2/src/ArchivItem_Text.h"
#include "libutil/src/error.h"
#include <sstream>
#ifndef _WIN32
#   include <cstring>
#endif

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

const unsigned int Settings::SETTINGS_VERSION = 12;
const unsigned int Settings::SETTINGS_SECTIONS = 11;
const std::string Settings::SETTINGS_SECTION_NAMES[] =
{
    "global", "video", "language", "driver", "sound", "lobby", "server", "proxy", "interface", "ingame", "addons"
};

const unsigned char Settings::SCREEN_REFRESH_RATES_COUNT = 14;
const unsigned short Settings::SCREEN_REFRESH_RATES[] =
{
    0, 1, 25, 30, 50, 60, 75, 80, 100, 120, 150, 180, 200, 240
};

Settings::Settings() //-V730
{
}

bool Settings::LoadDefaults()
{
    // force deletion of old values
    LOADER.GetInfoN(CONFIG_NAME)->clear();

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
    if (VIDEODRIVER.IsLoaded())
    {
        video.fullscreen_width = VIDEODRIVER.GetScreenWidth();
        video.fullscreen_height = VIDEODRIVER.GetScreenHeight();
        video.windowed_width = VIDEODRIVER.IsFullscreen() ? 800 : video.fullscreen_width;
        video.windowed_height = VIDEODRIVER.IsFullscreen() ? 600 : video.fullscreen_height;
        video.fullscreen = VIDEODRIVER.IsFullscreen();
    }else
    {
        video.fullscreen_width = 800;
        video.fullscreen_height = 600;
        video.windowed_width = video.fullscreen_width;
        video.windowed_height = video.fullscreen_height;
        video.fullscreen = false;
    }
    video.vsync             = 0;
    video.vbo               = false;
    video.shared_textures   = true;
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
    sound.musik          = false;
    sound.musik_volume   = 30;
    sound.effekte        = true;
    sound.effekte_volume = 75;
    sound.playlist       = "S2_Standard";
    // }

    // lobby
    // {
    char tmp_name[256];
#ifdef _WIN32
    DWORD size = 256;
    GetUserNameA(tmp_name, &size);
#else
    strncpy(tmp_name, getenv("USER"), 256);
#endif // !_WIN32

    lobby.name = tmp_name;
    lobby.password.clear();
    lobby.email.clear();
    lobby.save_password = false;
    // }

    // server
    // {
    server.last_ip.clear();
    server.ipv6 = false;
    // }

    // proxy
    // {
    proxy.proxy.clear();
    proxy.port = 0;
    proxy.typ = 0;
    // }

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
    if(!LOADER.LoadSettings() && LOADER.GetInfoN(CONFIG_NAME)->size() != SETTINGS_SECTIONS)
    {
        warning("No or corrupt \"%s\" found, using default values.\n", GetFilePath(FILE_PATHS[0]).c_str());
        return LoadDefaults();
    }

    const libsiedler2::ArchivItem_Ini* iniGlobal = LOADER.GetSettingsIniN("global");
    const libsiedler2::ArchivItem_Ini* iniVideo = LOADER.GetSettingsIniN("video");
    const libsiedler2::ArchivItem_Ini* iniLanguage = LOADER.GetSettingsIniN("language");
    const libsiedler2::ArchivItem_Ini* iniDriver = LOADER.GetSettingsIniN("driver");
    const libsiedler2::ArchivItem_Ini* iniSound = LOADER.GetSettingsIniN("sound");
    const libsiedler2::ArchivItem_Ini* iniLobby = LOADER.GetSettingsIniN("lobby");
    const libsiedler2::ArchivItem_Ini* iniServer = LOADER.GetSettingsIniN("server");
    const libsiedler2::ArchivItem_Ini* iniProxy = LOADER.GetSettingsIniN("proxy");
    const libsiedler2::ArchivItem_Ini* iniInterface = LOADER.GetSettingsIniN("interface");
    const libsiedler2::ArchivItem_Ini* iniIngame = LOADER.GetSettingsIniN("ingame");
    const libsiedler2::ArchivItem_Ini* iniAddons = LOADER.GetSettingsIniN("addons");

    // ist eine der Kategorien nicht vorhanden?
    if(!iniGlobal || !iniVideo || !iniLanguage || !iniDriver || !iniSound || !iniLobby || !iniServer || !iniProxy || !iniInterface || !iniIngame || !iniAddons ||
            // stimmt die Settingsversion?
            ((unsigned int)iniGlobal->getValueI("version") != SETTINGS_VERSION)
      )
    {
        // nein, dann Standardeinstellungen laden
        warning("\"%s\" found, but its corrupted or has wrong version. Loading default values.\n", GetFilePath(FILE_PATHS[0]).c_str());
        return LoadDefaults();
    }

    // global
    // {
    // stimmt die Spielrevision überein?
    if(iniGlobal->getValue("gameversion") != GetWindowRevision())
        warning("Your application version has changed - please recheck your settings!\n");

    global.submit_debug_data = iniGlobal->getValueI("submit_debug_data");
    global.use_upnp = iniGlobal->getValueI("use_upnp");
    global.smartCursor = (iniGlobal->getValue("smartCursor") == "" || iniGlobal->getValueI("smartCursor") != 0);
    global.debugMode = (iniGlobal->getValueI("debugMode") != 0);

    // };

    // video
    // {
    video.windowed_width =       iniVideo->getValueI("windowed_width");
    video.windowed_height =      iniVideo->getValueI("windowed_height");
    video.fullscreen_width =       iniVideo->getValueI("fullscreen_width");
    video.fullscreen_height =      iniVideo->getValueI("fullscreen_height");
    video.fullscreen = (iniVideo->getValueI("fullscreen") != 0);
    video.vsync =       iniVideo->getValueI("vsync");
    video.vbo =        (iniVideo->getValueI("vbo") != 0);
    video.shared_textures = (iniVideo->getValueI("shared_textures") != 0);
    // };

    if(video.fullscreen_width == 0 || video.fullscreen_height == 0
            || video.windowed_width == 0 || video.windowed_height == 0)
    {
        warning("Corrupted \"%s\" found, reverting to default values.\n", GetFilePath(FILE_PATHS[0]).c_str());
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
    sound.musik =         (iniSound->getValueI("musik") != 0);
    sound.musik_volume =   iniSound->getValueI("musik_volume");
    sound.effekte =       (iniSound->getValueI("effekte") != 0);
    sound.effekte_volume = iniSound->getValueI("effekte_volume");
    sound.playlist =       iniSound->getValue("playlist");
    // }

    // lobby
    // {
    lobby.name =           iniLobby->getValue("name");
    lobby.email =          iniLobby->getValue("email");
    lobby.password =       iniLobby->getValue("password");
    lobby.save_password = (iniLobby->getValueI("save_password") != 0);
    // }

    if(lobby.name.length() == 0)
    {
        char tmp_name[256];
#ifdef _WIN32
        DWORD size = 256;
        GetUserNameA(tmp_name, &size);
#else
        strncpy(tmp_name, getenv("USER"), 256);
#endif // !_WIN32

        lobby.name = tmp_name;
    }

    // server
    // {
    server.last_ip = iniServer->getValue("last_ip");
    server.ipv6 = (iniServer->getValueI("ipv6") != 0);
    // }

    // proxy
    // {
    proxy.proxy = iniProxy->getValue("proxy");
    proxy.port = iniProxy->getValueI("port");
    proxy.typ = iniProxy->getValueI("typ");
    // }

    // leere proxyadresse deaktiviert proxy komplett
    if(proxy.proxy.empty())
        proxy.typ = 0;

    // deaktivierter proxy entfernt proxyadresse
    if(proxy.typ == 0)
        proxy.proxy.clear();

    // aktivierter Socks v4 deaktiviert ipv6
    else if(proxy.typ == 4 && server.ipv6)
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
    for(unsigned int addon = 0; addon < iniAddons->size(); ++addon)
    {
        const libsiedler2::ArchivItem_Text* item = dynamic_cast<const libsiedler2::ArchivItem_Text*>(iniAddons->get(addon));

        if(item)
            addons.configuration.insert(std::make_pair(atoi(item->getName().c_str()), atoi(item->getText().c_str())));
    }
    // }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Routine zum Speichern der Konfiguration
void Settings::Save()
{
    libsiedler2::ArchivInfo& configInfo = *LOADER.GetInfoN(CONFIG_NAME);
    if(configInfo.size() != SETTINGS_SECTIONS)
    {
        libsiedler2::ArchivItem_Ini item;
        configInfo.alloc(SETTINGS_SECTIONS);
        for(unsigned int i = 0; i < SETTINGS_SECTIONS; ++i)
        {
            item.setName(SETTINGS_SECTION_NAMES[i]);
            configInfo.setC(i, item);
        }
    }

    libsiedler2::ArchivItem_Ini* iniGlobal = LOADER.GetSettingsIniN("global");
    libsiedler2::ArchivItem_Ini* iniVideo = LOADER.GetSettingsIniN("video");
    libsiedler2::ArchivItem_Ini* iniLanguage = LOADER.GetSettingsIniN("language");
    libsiedler2::ArchivItem_Ini* iniDriver = LOADER.GetSettingsIniN("driver");
    libsiedler2::ArchivItem_Ini* iniSound = LOADER.GetSettingsIniN("sound");
    libsiedler2::ArchivItem_Ini* iniLobby = LOADER.GetSettingsIniN("lobby");
    libsiedler2::ArchivItem_Ini* iniServer = LOADER.GetSettingsIniN("server");
    libsiedler2::ArchivItem_Ini* iniProxy = LOADER.GetSettingsIniN("proxy");
    libsiedler2::ArchivItem_Ini* iniInterface = LOADER.GetSettingsIniN("interface");
    libsiedler2::ArchivItem_Ini* iniIngame = LOADER.GetSettingsIniN("ingame");
    libsiedler2::ArchivItem_Ini* iniAddons = LOADER.GetSettingsIniN("addons");

    // ist eine der Kategorien nicht vorhanden?
    RTTR_Assert(iniGlobal && iniVideo && iniLanguage && iniDriver && iniSound && iniLobby && iniServer && iniProxy && iniInterface && iniIngame && iniAddons);

    // global
    // {
    iniGlobal->setValue("version", SETTINGS_VERSION);
    iniGlobal->setValue("gameversion", GetWindowRevision());
    iniGlobal->setValue("submit_debug_data", global.submit_debug_data);
    iniGlobal->setValue("use_upnp", global.use_upnp);
    iniGlobal->setValue("smartCursor", global.smartCursor ? 1 : 0);
    iniGlobal->setValue("debugMode", global.debugMode ? 1 : 0);
    // };

    // video
    // {
    iniVideo->setValue("fullscreen_width", video.fullscreen_width);
    iniVideo->setValue("fullscreen_height", video.fullscreen_height);
    iniVideo->setValue("windowed_width", video.windowed_width);
    iniVideo->setValue("windowed_height", video.windowed_height);
    iniVideo->setValue("fullscreen", (video.fullscreen ? 1 : 0) );
    iniVideo->setValue("vsync", video.vsync);
    iniVideo->setValue("vbo", (video.vbo ? 1 : 0) );
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
    iniSound->setValue("musik", (sound.musik ? 1 : 0) );
    iniSound->setValue("musik_volume", sound.musik_volume);
    iniSound->setValue("effekte", (sound.effekte ? 1 : 0) );
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

    // leere proxyadresse deaktiviert proxy komplett
    if(proxy.proxy.empty())
        proxy.typ = 0;

    // deaktivierter proxy entfernt proxyadresse
    if(proxy.typ == 0)
        proxy.proxy.clear();

    // aktivierter Socks v4 deaktiviert ipv6
    else if(proxy.typ == 4 && server.ipv6)
        server.ipv6 = false;

    // proxy
    // {
    iniProxy->setValue("proxy", proxy.proxy);
    iniProxy->setValue("port", proxy.port);
    iniProxy->setValue("typ", proxy.typ);
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
    for(std::map<unsigned int, unsigned int>::const_iterator it = addons.configuration.begin(); it != addons.configuration.end(); ++it)
    {
        std::stringstream name, value;
        name << it->first;
        value << it->second;
        iniAddons->addValue(name.str(), value.str());
    }
    // }

    LOADER.SaveSettings();
}
