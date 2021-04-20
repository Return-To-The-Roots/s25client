// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ApplicationLoader.h"
#include "Loader.h"
#include "Playlist.h"
#include "RttrConfig.h"
#include "files.h"

ApplicationLoader::ApplicationLoader(const RttrConfig& rttrConfig, Loader& loader, Log& log, std::string playlistPath)
    : rttrConfig_(rttrConfig), loader_(loader), logger_(log), playlistPath_(std::move(playlistPath))
{}

ApplicationLoader::~ApplicationLoader() = default;

bool ApplicationLoader::load()
{
    loader_.initResourceFolders();
    if(!loader_.LoadFilesAtStart())
        return false;
    if(!playlistPath_.empty())
    {
        playlist_ = std::make_unique<Playlist>();
        if(!playlist_->Load(logger_, playlistPath_)
           && !playlist_->Load(logger_, rttrConfig_.ExpandPath(s25::files::defaultPlaylist)))
            return false;
    }
    return true;
}
