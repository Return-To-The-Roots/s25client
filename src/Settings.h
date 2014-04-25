// $Id: Settings.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#pragma once

///////////////////////////////////////////////////////////////////////////////
// Header
#include "Singleton.h"

#undef interface

///////////////////////////////////////////////////////////////////////////////
// Klasse für die Konfiguration
class Settings : public Singleton<Settings>
{
    public:
        Settings(void);

        bool Load(void); // Lädt Einstellungen
        void Save(void); // Speichert Einstellungen

    protected:
        bool LoadDefaults();

    public:
        struct
        {
            unsigned int submit_debug_data;
            unsigned int use_upnp;
        } global;

        struct
        {
            unsigned short fullscreen_width;
            unsigned short fullscreen_height;
            unsigned short windowed_width;
            unsigned short windowed_height;
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
            std::string playlist; ///< musicplayer playlist name
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
            std::string last_ip; ///< last entered ip or hostname
            bool ipv6;           ///< listen/connect on ipv6 as default or not
        } server;

        struct
        {
            std::string proxy; ///< Serveradresse / Hostname
            unsigned int port; ///< Port
            unsigned char typ; ///< Socks 4 oder 5
        } proxy;

        struct
        {
            unsigned int autosave_interval;
            bool revert_mouse;
        } interface;

        struct
        {
            bool scale_statistics;
        } ingame;

        struct
        {
            std::map<unsigned int, unsigned int> configuration;
        } addons;

        static const unsigned char SCREEN_REFRESH_RATES_COUNT;
        static const unsigned short SCREEN_REFRESH_RATES[];

    private:
        static const unsigned int SETTINGS_VERSION;
        static const unsigned int SETTINGS_SECTIONS;
        static const std::string SETTINGS_SECTION_NAMES[];
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define SETTINGS Settings::inst()

#endif // SETTINGS_H_INCLUDED
