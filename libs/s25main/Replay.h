// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SavedFile.h"
#include "gameTypes/ChatDestination.h"
#include "gameTypes/MapType.h"
#include "s25util/BinaryFile.h"
#include <memory>
#include <string>

class MapInfo;
struct PlayerGameCommands;
class TmpFile;

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
    /// Stop recording. Will compress the data and return true if that succeeded. The file will be closed in any case
    bool StopRecording();

    /// Replaydatei gültig?
    bool IsValid() const { return file_.IsValid(); }
    bool IsRecording() const { return isRecording_ && file_.IsValid(); }
    bool IsReplaying() const { return !isRecording_ && file_.IsValid(); }
    const boost::filesystem::path& GetPath() const;

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

    unsigned GetLastGF() const { return lastGF_; }

    /// Zufallsgeneratorinitialisierung
    unsigned random_init;

protected:
    BinaryFile file_;
    std::unique_ptr<TmpFile> uncompressedDataFile_; /// Used when reading a compressed replay
    boost::filesystem::path filepath_;              /// Path to current file

    bool isRecording_;
    /// End-GF
    unsigned lastGF_;
    /// Position des End-GF in der Datei
    unsigned lastGfFilePos_;
    MapType mapType_;
};
