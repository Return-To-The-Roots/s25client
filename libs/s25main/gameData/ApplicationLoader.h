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
#ifndef APPLICATIONLOADER_H_INCLUDED
#define APPLICATIONLOADER_H_INCLUDED

#include <memory>
#include <string>

class Loader;
class Log;
class Playlist;

/// Load data (textures, playlist) at application startup
class ApplicationLoader
{
public:
    ApplicationLoader(Loader&, Log&, std::string playlistPath);
    ~ApplicationLoader();

    bool load();
    Playlist* getPlaylist() const { return playlist_.get(); }

private:
    Loader& loader_;
    Log& logger_;
    std::string playlistPath_;
    std::unique_ptr<Playlist> playlist_;
};

#endif // !APPLICATIONLOADER_H_INCLUDED
