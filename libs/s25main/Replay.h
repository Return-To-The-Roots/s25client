// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SavedFile.h"
#include "network/PlayerGameCommands.h"
#include "variant.h"
#include "gameTypes/ChatDestination.h"
#include "gameTypes/MapType.h"
#include "s25util/BinaryFile.h"
#include <memory>
#include <optional>
#include <string>

class MapInfo;
struct PlayerGameCommands;
class TmpFile;

/// Holds a replay that is being recorded or was recorded and loaded
/// It has a header that holds minimal information:
///     File header (version etc.), record time, map name, player names, length (last GF), savegame header (if
///     applicable)
/// All game relevant data is stored afterwards
class Replay : public SavedFile
{
public:
    enum class CommandType
    {
        Chat,
        Game,
    };
    struct ChatCommand
    {
        ChatCommand(BinaryFile& file);
        uint8_t player;
        ChatDestination dest;
        std::string msg;
    };
    struct GameCommand
    {
        GameCommand(BinaryFile& file, unsigned version);
        uint8_t player;
        PlayerGameCommands cmds;
    };

    Replay();
    ~Replay() override;

    void Close();

    std::string GetSignature() const override;
    uint8_t GetLatestMinorVersion() const override;
    uint8_t GetLatestMajorVersion() const override;

    /// Opens the replay for recording
    bool StartRecording(const boost::filesystem::path& filepath, const MapInfo& mapInfo, unsigned randomSeed);
    /// Stop recording. Will compress the data and return true if that succeeded. The file will be closed in any case
    bool StopRecording();

    /// Is the replay in the state for accepting commands to record
    bool IsRecording() const { return isRecording_ && file_.IsOpen(); }
    /// Is the replay open for reading commands
    bool IsReplaying() const { return !isRecording_ && file_.IsOpen(); }
    const boost::filesystem::path& GetPath() const;

    /// Load the header (containing map and player names)
    bool LoadHeader(const boost::filesystem::path& filepath);
    /// Load the remaining data into the mapInfo
    bool LoadGameData(MapInfo& mapInfo);

    /// Record a chat message
    void AddChatCommand(unsigned gf, uint8_t player, ChatDestination dest, const std::string& str);
    /// Record game commands (player actions)
    void AddGameCommand(unsigned gf, uint8_t player, const PlayerGameCommands& cmds);

    /// Read the next GameFrame to which the following replay command applies if there are any left
    std::optional<unsigned> ReadGF();
    boost_variant2<ChatCommand, GameCommand> ReadCommand();

    /// Update the (currently) last GameFrame in the file
    void UpdateLastGF(unsigned last_gf);

    unsigned getSeed() const { return randomSeed_; }
    unsigned GetLastGF() const { return lastGF_; }

protected:
    BinaryFile file_;
    std::unique_ptr<TmpFile> uncompressedDataFile_; /// Used when reading a compressed replay
    boost::filesystem::path filepath_;              /// Path to current file

    bool isRecording_ = false;
    /// Seed for the random number generator
    unsigned randomSeed_ = 0;
    /// End-GF
    unsigned lastGF_ = 0;
    /// Position of the last GF value in the file
    unsigned lastGfFilePos_ = 0;
    MapType mapType_ = MapType(0);

    /// Sub version for backwards compatibility (i.e. allow loading older files with same file version)
    uint8_t subVersion_ = 0;
    uint8_t gcVersion_ = 0;
};
