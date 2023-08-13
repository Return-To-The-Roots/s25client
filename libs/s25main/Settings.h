// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "driver/VideoMode.h"
#include "s25util/ProxySettings.h"
#include "s25util/Singleton.h"
#include <boost/optional.hpp>
#include <array>
#include <cstdint>
#include <gameData/const_gui_ids.h>
#include <map>
#include <string>

#undef interface

namespace validate {
boost::optional<uint16_t> checkPort(const std::string& port);
bool checkPort(int port);
} // namespace validate

struct PersistentWindowSettings
{
    DrawPoint lastPos = DrawPoint::Invalid();
    DrawPoint restorePos = DrawPoint::Invalid();
    bool isOpen = false;
    bool isPinned = false;
    bool isMinimized = false;
};

/// Configuration class
class Settings : public Singleton<Settings, SingletonPolicies::WithLongevity>
{
public:
    static constexpr unsigned Longevity = 18;

    Settings();

    void Load();
    void Save();

protected:
    void LoadDefaults();
    void LoadIngameDefaults();

    void LoadIngame();
    void SaveIngame();

public:
    struct
    {
        uint8_t submit_debug_data;
        bool use_upnp, smartCursor, debugMode, showGFInfo;
    } global;

    struct
    {
        VideoMode fullscreenSize, windowedSize;
        signed short framerate; // <0 for unlimited, 0 for HW Vsync
        bool fullscreen;
        bool vbo;
        bool shared_textures;
        unsigned guiScale; ///< UI scaling in percent; 0 indicates automatic selection
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
        bool musicEnabled;
        uint8_t musicVolume;
        bool effectsEnabled;
        uint8_t effectsVolume;
        std::string playlist; /// musicplayer playlist name
    } sound;

    struct
    {
        std::string name;
        std::string password;
        bool save_password;
    } lobby;

    struct
    {
        std::string last_ip; /// last entered ip or hostname
        uint16_t localPort;
        bool ipv6; /// listen/connect on ipv6 as default or not
    } server;

    ProxySettings proxy;

    struct
    {
        unsigned autosave_interval;
        bool invertMouse;
        bool enableWindowPinning;
    } interface;

    struct
    {
        bool scale_statistics;
        bool showNames;
        bool showProductivity;
        bool showBQ;
        bool minimapExtended;
    } ingame;

    struct
    {
        std::map<GUI_ID, PersistentWindowSettings> persistentSettings;
    } windows;

    struct
    {
        std::map<unsigned, unsigned> configuration;
    } addons;

    static const std::array<short, 13> SCREEN_REFRESH_RATES;

private:
    static const int VERSION;
    static const std::array<std::string, 10> SECTION_NAMES;
};

#define SETTINGS Settings::inst()
