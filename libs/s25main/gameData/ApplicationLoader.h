// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>

class RttrConfig;
class Loader;
class Log;
class Playlist;

/// Load data (textures, playlist) at application startup
class ApplicationLoader
{
public:
    ApplicationLoader(const RttrConfig&, Loader&, Log&, std::string playlistPath);
    ~ApplicationLoader();

    bool load();
    Playlist* getPlaylist() const { return playlist_.get(); }

private:
    const RttrConfig& rttrConfig_;
    Loader& loader_;
    Log& logger_;
    std::string playlistPath_;
    std::unique_ptr<Playlist> playlist_;
};
