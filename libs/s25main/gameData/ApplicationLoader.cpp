// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
    loader_.ClearOverrideFolders();
    loader_.AddOverrideFolder(s25::folders::lstsGlobal);
    loader_.AddOverrideFolder(s25::folders::lstsUser);
    if(!loader_.LoadFilesAtStart())
        return false;
    if(!playlistPath_.empty())
    {
        playlist_ = std::make_unique<Playlist>();
        if(!playlist_->Load(logger_, playlistPath_) && !playlist_->Load(logger_, rttrConfig_.ExpandPath(s25::files::defaultPlaylist)))
            return false;
    }
    return true;
}
