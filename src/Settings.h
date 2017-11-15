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
#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#pragma once

#include "Point.h"
#include "libutil/ProxySettings.h"
#include "libutil/Singleton.h"
#include <map>
#include <string>

#undef interface

///////////////////////////////////////////////////////////////////////////////
// Klasse für die Konfiguration
class Settings : public Singleton<Settings, SingletonPolicies::WithLongevity>
{
public:
    BOOST_STATIC_CONSTEXPR unsigned Longevity = 18;

    Settings();

    bool Load(); // Lädt Einstellungen
    void Save(); // Speichert Einstellungen

protected:
    bool LoadDefaults();

public:
    struct
    {
        unsigned submit_debug_data;
        unsigned use_upnp;
        bool smartCursor;
        bool debugMode;
    } global;

    struct
    {
        Extent fullscreenSize, windowedSize;
        bool fullscreen;
        unsigned short vsync;
        bool vbo;
        bool shared_textures;
    } video;

    struct
    {
        std::string language;
    } language;

    struct
    {
        std::string audio;
        std::string video;
    } driver;

    struct
    {
        bool musik;
        unsigned char musik_volume;
        bool effekte;
        unsigned char effekte_volume;
        std::string playlist; /// musicplayer playlist name
    } sound;

    struct
    {
        std::string name;
        std::string password;
        std::string email;
        bool save_password;
    } lobby;

    struct
    {
        std::string last_ip; /// last entered ip or hostname
        bool ipv6;           /// listen/connect on ipv6 as default or not
    } server;

    ProxySettings proxy;

    struct
    {
        unsigned autosave_interval;
        bool revert_mouse;
    } interface;

    struct
    {
        bool scale_statistics;
    } ingame;

    struct
    {
        std::map<unsigned, unsigned> configuration;
    } addons;

    static const unsigned char SCREEN_REFRESH_RATES_COUNT;
    static const unsigned short SCREEN_REFRESH_RATES[];

private:
    static const unsigned SETTINGS_VERSION;
    static const unsigned SETTINGS_SECTIONS;
    static const std::string SETTINGS_SECTION_NAMES[];
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define SETTINGS Settings::inst()

#endif // SETTINGS_H_INCLUDED
