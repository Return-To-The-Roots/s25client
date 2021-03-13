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

#pragma once

#include "SavedFile.h"
#include "gameTypes/ChatDestination.h"
#include "gameTypes/MapType.h"
#include "s25util/BinaryFile.h"
#include <string>

class MapInfo;
struct PlayerGameCommands;

/// Replay-Command-Art
enum class ReplayCommand
{
    End,
    Chat,
    Game
};

/// Holds a replay that is being recorded or was recorded and loaded
/// It has a header that holds minimal information:
///     File header (version etc.), record time, map name, player names, length (last GF), savegame header (if
///     applicable)
/// All game relevant data is stored afterwards
class Replay : public SavedFile
{
public:
    Replay();
    ~Replay() override;

    void Close();

    std::string GetSignature() const override;
    uint16_t GetVersion() const override;

    /// Beginnt die Save-Datei und schreibt den Header
    bool StartRecording(const boost::filesystem::path& filepath, const MapInfo& mapInfo);
    /// Räumt auf, schließt datei
    void StopRecording();

    /// Replaydatei gültig?
    bool IsValid() const { return file.IsValid(); }
    bool IsRecording() const { return isRecording && file.IsValid(); }
    bool IsReplaying() const { return !isRecording && file.IsValid(); }

    /// Loads the header and optionally the mapInfo (former "extended header")
    bool LoadHeader(const boost::filesystem::path& filepath);
    bool LoadGameData(MapInfo& mapInfo);

    /// Fügt ein Chat-Kommando hinzu (schreibt)
    void AddChatCommand(unsigned gf, uint8_t player, ChatDestination dest, const std::string& str);
    /// Fügt ein Spiel-Kommando hinzu (schreibt)
    void AddGameCommand(unsigned gf, uint8_t player, const PlayerGameCommands& cmds);

    /// Liest RC-Type aus, liefert false, wenn das Replay zu Ende ist
    bool ReadGF(unsigned* gf);
    /// RC-Type aus, liefert false
    ReplayCommand ReadRCType();
    /// Liest ein Chat-Command aus
    void ReadChatCommand(uint8_t& player, uint8_t& dest, std::string& str);
    void ReadGameCommand(uint8_t& player, PlayerGameCommands& cmds);

    /// Aktualisiert den End-GF, schreibt ihn in die Replaydatei (nur beim Spielen bzw. Schreiben verwenden!)
    void UpdateLastGF(unsigned last_gf);

    BinaryFile& GetFile() { return file; }
    unsigned GetLastGF() const { return lastGF_; }

    /// Zufallsgeneratorinitialisierung
    unsigned random_init;

protected:
    BinaryFile file;
    bool isRecording;
    /// End-GF
    unsigned lastGF_;
    /// Position des End-GF in der Datei
    unsigned last_gf_file_pos;
    MapType mapType_;
};
