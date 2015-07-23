// $Id: Settings.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h"
#include "Settings.h"

#include "files.h"
#include "fileFuncs.h"
#include "Loader.h"
#include "languages.h"
#include "build_version.h"

#include "../libutil/src/error.h"
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

///////////////////////////////////////////////////////////////////////////////
// Konstruktor
Settings::Settings(void)
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
    // }

    // video
    // {
    video.fullscreen_width  = 800;
    video.fullscreen_height = 600;
    video.windowed_width    = 800;
    video.windowed_height   = 600;
    video.fullscreen        = false;
    video.vsync             = 0;
    video.vbo               = false;
    video.shared_textures   = true;
    // }

    // language
    // {
    language.language = "";
    // }

    LANGUAGES.setLanguage(language.language);

    // driver
    // {
    driver.audio = "";
    driver.video = "";
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
    lobby.password = "";
    lobby.email = "";
    lobby.save_password = false;
    // }

    // server
    // {
    server.last_ip = "";
    server.ipv6 = false;
    // }

    // proxy
    // {
    proxy.proxy = "";
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
bool Settings::Load(void)
{
    if(!LOADER.LoadSettings() && LOADER.GetInfoN(CONFIG_NAME)->getCount() != SETTINGS_SECTIONS)
    {
        warning("No or corrupt \"%s\" found, using default values.\n", GetFilePath(FILE_PATHS[0]).c_str());
        return LoadDefaults();
    }

    const libsiedler2::ArchivItem_Ini* global = LOADER.GetSettingsIniN("global");
    const libsiedler2::ArchivItem_Ini* video = LOADER.GetSettingsIniN("video");
    const libsiedler2::ArchivItem_Ini* language = LOADER.GetSettingsIniN("language");
    const libsiedler2::ArchivItem_Ini* driver = LOADER.GetSettingsIniN("driver");
    const libsiedler2::ArchivItem_Ini* sound = LOADER.GetSettingsIniN("sound");
    const libsiedler2::ArchivItem_Ini* lobby = LOADER.GetSettingsIniN("lobby");
    const libsiedler2::ArchivItem_Ini* server = LOADER.GetSettingsIniN("server");
    const libsiedler2::ArchivItem_Ini* proxy = LOADER.GetSettingsIniN("proxy");
    const libsiedler2::ArchivItem_Ini* interface = LOADER.GetSettingsIniN("interface");
    const libsiedler2::ArchivItem_Ini* ingame = LOADER.GetSettingsIniN("ingame");
    const libsiedler2::ArchivItem_Ini* addons = LOADER.GetSettingsIniN("addons");

    // ist eine der Kategorien nicht vorhanden?
    if(!global || !video || !language || !driver || !sound || !lobby || !server || !proxy || !interface || !ingame || !addons ||
            // stimmt die Settingsversion?
            ((unsigned int)global->getValueI("version") != SETTINGS_VERSION)
      )
    {
        // nein, dann Standardeinstellungen laden
        warning("\"%s\" found, but its corrupted or has wrong version. Loading default values.\n", GetFilePath(FILE_PATHS[0]).c_str());
        return LoadDefaults();
    }

    // global
    // {
    // stimmt die Spielrevision überein?
    if(strcmp(global->getValue("gameversion"), GetWindowRevision()) != 0)
        warning("Your application version has changed - please recheck your settings!\n");

    this->global.submit_debug_data = global->getValueI("submit_debug_data");
    this->global.use_upnp = global->getValueI("use_upnp");

    // };

    // video
    // {
    this->video.windowed_width =       video->getValueI("windowed_width");
    this->video.windowed_height =      video->getValueI("windowed_height");
    this->video.fullscreen_width =       video->getValueI("fullscreen_width");
    this->video.fullscreen_height =      video->getValueI("fullscreen_height");
    this->video.fullscreen = (video->getValueI("fullscreen") ? true : false);
    this->video.vsync =       video->getValueI("vsync");
    this->video.vbo =        (video->getValueI("vbo") ? true : false);
    this->video.shared_textures = (video->getValueI("shared_textures") ? true : false);
    // };

    if(this->video.fullscreen_width == 0 || this->video.fullscreen_height == 0
            || this->video.windowed_width == 0 || this->video.windowed_height == 0)
    {
        warning("Corrupted \"%s\" found, reverting to default values.\n", GetFilePath(FILE_PATHS[0]).c_str());
        return LoadDefaults();
    }

    // language
    // {
    this->language.language = language->getValue("language");
    // }

    LANGUAGES.setLanguage(this->language.language);

    // driver
    // {
    this->driver.video = driver->getValue("video");
    this->driver.audio = driver->getValue("audio");
    // }

    // sound
    // {
    this->sound.musik =         (sound->getValueI("musik") ? true : false);
    this->sound.musik_volume =   sound->getValueI("musik_volume");
    this->sound.effekte =       (sound->getValueI("effekte") ? true : false);
    this->sound.effekte_volume = sound->getValueI("effekte_volume");
    this->sound.playlist =       sound->getValue("playlist");
    // }

    // lobby
    // {
    this->lobby.name =           lobby->getValue("name");
    this->lobby.email =          lobby->getValue("email");
    this->lobby.password =       lobby->getValue("password");
    this->lobby.save_password = (lobby->getValueI("save_password") ? true : false);
    // }

    if(this->lobby.name.length() == 0)
    {
        char tmp_name[256];
#ifdef _WIN32
        DWORD size = 256;
        GetUserNameA(tmp_name, &size);
#else
        strncpy(tmp_name, getenv("USER"), 256);
#endif // !_WIN32

        this->lobby.name = tmp_name;
    }

    // server
    // {
    this->server.last_ip = server->getValue("last_ip");
    this->server.ipv6 = (server->getValueI("ipv6") ? true : false);
    // }

    // proxy
    // {
    this->proxy.proxy = proxy->getValue("proxy");
    this->proxy.port = proxy->getValueI("port");
    this->proxy.typ = proxy->getValueI("typ");
    // }

    // leere proxyadresse deaktiviert proxy komplett
    if(this->proxy.proxy == "")
        this->proxy.typ = 0;

    // deaktivierter proxy entfernt proxyadresse
    if(this->proxy.typ == 0)
        this->proxy.proxy = "";

    // aktivierter Socks v4 deaktiviert ipv6
    else if(this->proxy.typ == 4 && this->server.ipv6)
        this->server.ipv6 = false;

    // interface
    // {
    this->interface.autosave_interval = interface->getValueI("autosave_interval");
    this->interface.revert_mouse = (interface->getValueI("revert_mouse") ? true : false);
    // }

    // ingame
    // {
    this->ingame.scale_statistics = (ingame->getValueI("scale_statistics") ? true : false);
    // }

    // addons
    // {
    for(unsigned int addon = 0; addon < addons->getCount(); ++addon)
    {
        const libsiedler2::ArchivItem_Text* item = dynamic_cast<const libsiedler2::ArchivItem_Text*>(addons->get(addon));

        if(item)
            this->addons.configuration.insert(std::make_pair(atoi(item->getName()), atoi(item->getText())));
    }
    // }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Routine zum Speichern der Konfiguration
void Settings::Save(void)
{
    if(LOADER.GetInfoN(CONFIG_NAME)->getCount() != SETTINGS_SECTIONS)
    {
        libsiedler2::ArchivItem_Ini item;
        LOADER.GetInfoN(CONFIG_NAME)->alloc(SETTINGS_SECTIONS);
        for(unsigned int i = 0; i < SETTINGS_SECTIONS; ++i)
        {
            item.setName(SETTINGS_SECTION_NAMES[i].c_str());
            LOADER.GetInfoN(CONFIG_NAME)->setC(i, &item);
        }
    }

    libsiedler2::ArchivItem_Ini* global = LOADER.GetSettingsIniN("global");
    libsiedler2::ArchivItem_Ini* video = LOADER.GetSettingsIniN("video");
    libsiedler2::ArchivItem_Ini* language = LOADER.GetSettingsIniN("language");
    libsiedler2::ArchivItem_Ini* driver = LOADER.GetSettingsIniN("driver");
    libsiedler2::ArchivItem_Ini* sound = LOADER.GetSettingsIniN("sound");
    libsiedler2::ArchivItem_Ini* lobby = LOADER.GetSettingsIniN("lobby");
    libsiedler2::ArchivItem_Ini* server = LOADER.GetSettingsIniN("server");
    libsiedler2::ArchivItem_Ini* proxy = LOADER.GetSettingsIniN("proxy");
    libsiedler2::ArchivItem_Ini* interface = LOADER.GetSettingsIniN("interface");
    libsiedler2::ArchivItem_Ini* ingame = LOADER.GetSettingsIniN("ingame");
    libsiedler2::ArchivItem_Ini* addons = LOADER.GetSettingsIniN("addons");

    // ist eine der Kategorien nicht vorhanden?
    assert(global && video && language && driver && sound && lobby && server && proxy && interface && ingame && addons);

    // global
    // {
    global->setValue("version", SETTINGS_VERSION);
    global->setValue("gameversion", GetWindowRevision());
    global->setValue("submit_debug_data", this->global.submit_debug_data);
    global->setValue("use_upnp", this->global.use_upnp);
    // };

    // video
    // {
    video->setValue("fullscreen_width", this->video.fullscreen_width);
    video->setValue("fullscreen_height", this->video.fullscreen_height);
    video->setValue("windowed_width", this->video.windowed_width);
    video->setValue("windowed_height", this->video.windowed_height);
    video->setValue("fullscreen", (this->video.fullscreen ? 1 : 0) );
    video->setValue("vsync", this->video.vsync);
    video->setValue("vbo", (this->video.vbo ? 1 : 0) );
    video->setValue("shared_textures", (this->video.shared_textures ? 1 : 0));
    // };

    // language
    // {
    language->setValue("language", this->language.language.c_str());
    // }

    // driver
    // {
    driver->setValue("video", this->driver.video.c_str());
    driver->setValue("audio", this->driver.audio.c_str());
    // }

    // sound
    // {
    sound->setValue("musik", (this->sound.musik ? 1 : 0) );
    sound->setValue("musik_volume", this->sound.musik_volume);
    sound->setValue("effekte", (this->sound.effekte ? 1 : 0) );
    sound->setValue("effekte_volume", this->sound.effekte_volume);
    sound->setValue("playlist", this->sound.playlist.c_str());
    // }

    // lobby
    // {
    lobby->setValue("name", this->lobby.name.c_str());
    lobby->setValue("email", this->lobby.email.c_str());
    lobby->setValue("password", this->lobby.password.c_str());
    lobby->setValue("save_password", (this->lobby.save_password ? 1 : 0));
    // }

    // server
    // {
    server->setValue("last_ip", this->server.last_ip.c_str());
    server->setValue("ipv6", (this->server.ipv6 ? 1 : 0));
    // }

    // leere proxyadresse deaktiviert proxy komplett
    if(this->proxy.proxy == "")
        this->proxy.typ = 0;

    // deaktivierter proxy entfernt proxyadresse
    if(this->proxy.typ == 0)
        this->proxy.proxy = "";

    // aktivierter Socks v4 deaktiviert ipv6
    else if(this->proxy.typ == 4 && this->server.ipv6)
        this->server.ipv6 = false;

    // proxy
    // {
    proxy->setValue("proxy", this->proxy.proxy.c_str());
    proxy->setValue("port", this->proxy.port);
    proxy->setValue("typ", this->proxy.typ);
    // }

    // interface
    // {
    interface->setValue("autosave_interval", this->interface.autosave_interval);
    interface->setValue("revert_mouse", (this->interface.revert_mouse ? true : false));
    // }

    // ingame
    // {
    ingame->setValue("scale_statistics", (this->ingame.scale_statistics ? 1 : 0));
    // }

    // addons
    // {
    addons->clear();
    for(std::map<unsigned int, unsigned int>::const_iterator it = this->addons.configuration.begin(); it != this->addons.configuration.end(); ++it)
    {
        std::stringstream name, value;
        name << it->first;
        value << it->second;
        addons->addValue(name.str().c_str(), value.str().c_str());
    }
    // }

    LOADER.SaveSettings();
}
