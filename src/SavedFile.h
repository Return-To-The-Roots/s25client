// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef GAMESAVEDFILE_H_INCLUDED
#define GAMESAVEDFILE_H_INCLUDED

#pragma once

#include "gameData/NationConsts.h"
#include "GlobalGameSettings.h"
#include "../libutil/src/MyTime.h"
#include <string>

#include <boost/cstdint.hpp>

struct BasePlayerInfo;
class BinaryFile;

/// Basisklasse für Replays und Savegames
class SavedFile
{
public:
    SavedFile();
    virtual ~SavedFile();

protected:
    /// Return the file signature. Must be at most 32 bytes
    virtual std::string GetSignature() const = 0;
    /// Return the file format version
    virtual uint16_t GetVersion() const = 0;

    /// Schreibt Signatur und Version der Datei
    void WriteFileHeader(BinaryFile& file);
    /// Reads and validates the file header. On error false is returned and lastErrorMsg is set
    bool ReadFileHeader(BinaryFile& file);

    /// Schreibt Spielerdaten
    void WritePlayerData(BinaryFile& file);
    /// Liest Spielerdaten aus
    void ReadPlayerData(BinaryFile& file);

    /// schreibt die GlobalGameSettings in die Datei.
    void WriteGGS(BinaryFile& file);
    /// liest die GlobalGameSettings aus der Datei.
    void ReadGGS(BinaryFile& file);

public:

    /// Zeitpunkt der Aufnahme
    unser_time_t save_time;
    /// Mapname
    std::string mapName;
    /// GGS
    GlobalGameSettings ggs;

    const BasePlayerInfo& GetPlayer(unsigned idx) const;
    unsigned GetPlayerCount();
    void AddPlayer(const BasePlayerInfo& player);
    void ClearPlayers();
    std::string GetLastErrorMsg() const;
    std::string GetRevision() const;

protected:
    /// Last error message during loading
    std::string lastErrorMsg;

private:
    std::vector<BasePlayerInfo> players;
    /// Revision as saved in the file
    boost::array<char, 8> revision;
};

#endif // !GAMEFILES_H_INCLUDED
