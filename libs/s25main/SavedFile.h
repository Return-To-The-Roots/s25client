// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GlobalGameSettings.h"
#include "s25util/MyTime.h"
#include <array>
#include <string>
#include <vector>

class BinaryFile;
struct BasePlayerInfo;

/// Basisklasse für Replays und Savegames
class SavedFile
{
public:
    SavedFile();
    virtual ~SavedFile();

    /// Return the file signature. Must be at most 32 bytes
    virtual std::string GetSignature() const = 0;
    /// Return the file format version
    virtual uint16_t GetVersion() const = 0;

    /// Schreibt Signatur und Version der Datei
    void WriteFileHeader(BinaryFile& file) const;
    /// Reads and validates the file header. On error false is returned and lastErrorMsg is set
    bool ReadFileHeader(BinaryFile& file);

    /// Write common information (program version, map name, time and player names)
    virtual void WriteExtHeader(BinaryFile& file, const std::string& mapName);
    virtual bool ReadExtHeader(BinaryFile& file);

    void WriteAllHeaderData(BinaryFile& file, const std::string& mapName);
    bool ReadAllHeaderData(BinaryFile& file);

    /// Schreibt Spielerdaten
    void WritePlayerData(BinaryFile& file);
    /// Liest Spielerdaten aus
    void ReadPlayerData(BinaryFile& file);

    /// schreibt die GlobalGameSettings in die Datei.
    void WriteGGS(BinaryFile& file) const;
    /// liest die GlobalGameSettings aus der Datei.
    void ReadGGS(BinaryFile& file);

    const BasePlayerInfo& GetPlayer(unsigned idx) const;
    unsigned GetNumPlayers();
    void AddPlayer(const BasePlayerInfo& player);
    void ClearPlayers();

    std::string GetLastErrorMsg() const { return lastErrorMsg; }

    std::string GetRevision() const;
    std::string GetMapName() const { return mapName_; }
    s25util::time64_t GetSaveTime() const { return saveTime_; }
    const std::vector<std::string>& GetPlayerNames() const { return playerNames_; }

    GlobalGameSettings ggs;

protected:
    /// Last error message during loading
    std::string lastErrorMsg;

private:
    std::vector<BasePlayerInfo> players;
    /// Revision as saved in the file
    std::array<char, 8> revision;
    /// Zeitpunkt der Aufnahme
    s25util::time64_t saveTime_;
    /// Mapname
    std::string mapName_;
    std::vector<std::string> playerNames_;
};
