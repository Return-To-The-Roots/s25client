// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Replay.h"
#include "Savegame.h"
#include "enum_cast.hpp"
#include "helpers/format.hpp"
#include "network/PlayerGameCommands.h"
#include "gameTypes/MapInfo.h"
#include <s25util/tmpFile.h>
#include <boost/filesystem.hpp>
#include <memory>
#include <mygettext/mygettext.h>

std::string Replay::getSignature() const
{
    return "RTTRRP2";
}

// clang-format off
/// (Sub)-Version of the current replay file
/// Usage:
////    - Always save for the most current version
////    - Loading code may cope with file format changes
/// If the format changes (e.g. new enum values, types, ... increase this version and handle it in the loading code.
/// If the change cannot be handled:
///     - Remove all code handling this version.
///     - Reset this version to 0
///     - Increase the version in getVersion
///
/// Changelog:
/// 1: Unused first CommandType (End) removed, GameCommand version added
static const uint8_t currentReplayDataVersion = 1;
// clang-format on

/// Format version of replay files
uint8_t Replay::getLatestMinorVersion() const
{
    // 8.1: Portraits support
    // 8.2: Set correct initial distributions if replay starts without savegame for leather addon (see GameClient.cpp
    //      startReplay function for detailed description)
    // 8.3  Remove invalid fish for replays started from start (i.e. map instead of savegame)
    // 8.4  Hunter/skinner/AI search for animals in a circle instead of a square (radius-based)
    return 4;
}

uint8_t Replay::getLatestMajorVersion() const
{
    // Search for "TODO(Replay)" when increasing this (breaking Replay compatibility)
    // and handle/remove the relevant code
    return 8;
}

//////////////////////////////////////////////////////////////////////////

Replay::Replay() = default;
Replay::~Replay() = default;

void Replay::close()
{
    file_.close();
    uncompressedDataFile_.reset();
    isRecording_ = false;
    filepath_.clear();
    clearPlayers();
}

bool Replay::stopRecording()
{
    if(!isRecording_)
        return true;
    const auto replayDataSize = file_.tell();
    isRecording_ = false;
    file_.close();

    // Remove empty replay recordings. They are mostly produced when a game is aborted
    // during startup, for example after an early script error.
    if(lastGF_ == 0)
    {
        boost::system::error_code ec;
        boost::filesystem::remove(filepath_, ec);
        return !ec;
    }

    BinaryFile file;
    if(!file.open(filepath_, OpenFileMode::Read))
        return false;
    try
    {
        TmpFile tmpReplayFile(".rpl");
        file.seek(0, SEEK_SET);
        tmpReplayFile.close();
        BinaryFile compressedReplay;
        compressedReplay.open(tmpReplayFile.filePath, OpenFileMode::Write);

        // Copy header data uncompressed
        std::vector<char> data(lastGfFilePos_);
        file.readRawData(data.data(), data.size());
        compressedReplay.writeRawData(data.data(), data.size());
        const auto lastGF = file.readUnsignedInt();
        RTTR_Assert(lastGF == lastGF_);
        compressedReplay.writeUnsignedInt(lastGF);
        file.readUnsignedChar(); // Ignore compressed flag (always zero for the temporary file)

        // Read and compress remaining data
        const auto uncompressedSize = replayDataSize - file.tell(); // Always positive as there is always some game data
        data.resize(uncompressedSize);
        file.readRawData(data.data(), data.size());
        data = CompressedData::compress(data);
        // If the compressed data turns out to be larger than the uncompressed one, bail out
        // This can happen for very short replays
        if(data.size() >= uncompressedSize)
            return true;
        compressedReplay.writeUnsignedChar(1); // Compressed flag
        compressedReplay.writeUnsignedInt(uncompressedSize);
        compressedReplay.writeUnsignedInt(data.size());
        compressedReplay.writeRawData(data.data(), data.size());

        // All done. Replace uncompressed replay
        compressedReplay.close();
        file.close();
        boost::filesystem::rename(tmpReplayFile.filePath, filepath_);
        return true;
    } catch(const std::exception& e)
    {
        lastErrorMsg = e.what();
        return false;
    }
}

bool Replay::startRecording(const boost::filesystem::path& filepath, const MapInfo& mapInfo, const unsigned randomSeed)
{
    // Deny overwrite, also avoids double-opening by different processes
    if(boost::filesystem::exists(filepath))
        return false;
    if(!file_.open(filepath, OpenFileMode::Write))
        return false;
    filepath_ = filepath;

    isRecording_ = true;
    /// End-GF (will be updated during the game)
    lastGF_ = 0;
    mapType_ = mapInfo.type;
    randomSeed_ = randomSeed;

    writeAllHeaderData(file_, mapInfo.title);
    file_.writeUnsignedChar(rttr::enumCast(mapType_));
    // TODO(Replay): Move before mapType
    file_.writeUnsignedChar(subVersion_ = currentReplayDataVersion);
    RTTR_Assert(gc::Deserializer::getCurrentVersion() <= std::numeric_limits<decltype(gcVersion_)>::max());
    file_.writeUnsignedChar(gcVersion_ = gc::Deserializer::getCurrentVersion());

    // For (savegame) format validation
    if(mapType_ == MapType::Savegame)
        mapInfo.savegame->writeFileHeader(file_);

    // store position to update it later
    lastGfFilePos_ = file_.tell();
    file_.writeUnsignedInt(lastGF_);
    file_.writeUnsignedChar(0); // Compressed flag

    writePlayerData(file_);
    writeGGS(file_);

    // Game data
    file_.writeUnsignedInt(randomSeed_);
    file_.writeLongString(mapInfo.filepath.string());

    switch(mapType_)
    {
        default: return false;
        case MapType::OldMap:
            RTTR_Assert(!mapInfo.savegame);
            file_.writeUnsignedInt(mapInfo.mapData.uncompressedLength);
            file_.writeUnsignedInt(mapInfo.mapData.data.size());
            file_.writeRawData(mapInfo.mapData.data.data(), mapInfo.mapData.data.size());
            file_.writeUnsignedInt(mapInfo.luaData.uncompressedLength);
            file_.writeUnsignedInt(mapInfo.luaData.data.size());
            if(!mapInfo.luaData.data.empty())
                file_.writeRawData(mapInfo.luaData.data.data(), mapInfo.luaData.data.size());
            break;
        case MapType::Savegame: mapInfo.savegame->save(file_, getMapName()); break;
    }
    // Flush now to not loose any information
    file_.flush();

    return true;
}

const boost::filesystem::path& Replay::getPath() const
{
    RTTR_Assert(file_.isOpen());
    return filepath_;
}

bool Replay::loadHeader(const boost::filesystem::path& filepath)
{
    close();
    if(!file_.open(filepath, OpenFileMode::Read))
    {
        lastErrorMsg = _("File could not be opened.");
        return false;
    }
    filepath_ = filepath;

    try
    {
        // Check file header
        if(!readAllHeaderData(file_))
            return false;

        mapType_ = static_cast<MapType>(file_.readUnsignedChar());
        // TODO(Replay): Move before mapType to have it as early as possible.
        // Previously mapType was an unsigned short, i.e. in little endian the 2nd byte was always unused/zero
        subVersion_ = file_.readUnsignedChar();
        if(subVersion_ > currentReplayDataVersion)
        {
            lastErrorMsg =
              helpers::format(_("Cannot play replay created with a more recent version (Current: %1%, Replay: %2%)"),
                              currentReplayDataVersion, subVersion_);
            return false;
        }

        if(subVersion_ >= 1)
            gcVersion_ = file_.readUnsignedChar();
        else
            gcVersion_ = 0;
        if(gcVersion_ > gc::Deserializer::getCurrentVersion())
        {
            lastErrorMsg =
              helpers::format(_("Cannot play replay created with a more recent GC version (Current: %1%, Replay: %2%)"),
                              gc::Deserializer::getCurrentVersion(), gcVersion_);
            return false;
        }

        if(mapType_ == MapType::Savegame)
        {
            // Validate savegame
            Savegame save;
            if(!save.readFileHeader(file_))
            {
                lastErrorMsg = std::string(_("Savegame error: ")) + save.getLastErrorMsg();
                return false;
            }
        }

        lastGF_ = file_.readUnsignedInt();
    } catch(const std::runtime_error& e)
    {
        lastErrorMsg = e.what();
        return false;
    }

    return true;
}

bool Replay::loadGameData(MapInfo& mapInfo)
{
    try
    {
        const bool isCompressed = file_.readUnsignedChar() != 0;
        if(isCompressed)
        {
            const auto uncompressedSize = file_.readUnsignedInt();
            const auto compressedSize = file_.readUnsignedInt();
            CompressedData compressedData(uncompressedSize);
            compressedData.data.resize(compressedSize);
            file_.readRawData(compressedData.data.data(), compressedSize);
            uncompressedDataFile_ = std::make_unique<TmpFile>(".rpl");
            uncompressedDataFile_->close();
            compressedData.decompressToFile(uncompressedDataFile_->filePath);
            file_.close();
            file_.open(uncompressedDataFile_->filePath, OpenFileMode::Read);
        }

        readPlayerData(file_);
        readGGS(file_);
        randomSeed_ = file_.readUnsignedInt();

        mapInfo.clear();
        mapInfo.type = mapType_;
        mapInfo.title = getMapName();
        mapInfo.filepath = file_.readLongString();
        switch(mapType_)
        {
            default: return false;
            case MapType::OldMap:
                mapInfo.mapData.uncompressedLength = file_.readUnsignedInt();
                mapInfo.mapData.data.resize(file_.readUnsignedInt());
                file_.readRawData(mapInfo.mapData.data.data(), mapInfo.mapData.data.size());
                mapInfo.luaData.uncompressedLength = file_.readUnsignedInt();
                mapInfo.luaData.data.resize(file_.readUnsignedInt());
                if(!mapInfo.luaData.data.empty())
                    file_.readRawData(mapInfo.luaData.data.data(), mapInfo.luaData.data.size());
                break;
            case MapType::Savegame:
                mapInfo.savegame = std::make_unique<Savegame>();
                if(!mapInfo.savegame->load(file_, SaveGameDataToLoad::All))
                {
                    lastErrorMsg = std::string(_("Savegame error: ")) + mapInfo.savegame->getLastErrorMsg();
                    return false;
                }
                break;
        }
    } catch(const std::runtime_error& e)
    {
        lastErrorMsg = e.what();
        return false;
    }
    return true;
}

void Replay::addChatCommand(unsigned gf, uint8_t player, ChatDestination dest, const std::string& str)
{
    RTTR_Assert(isRecording());
    if(!file_.isOpen())
        return;

    file_.writeUnsignedInt(gf);

    file_.writeUnsignedChar(rttr::enumCast(CommandType::Chat));
    file_.writeUnsignedChar(player);
    file_.writeUnsignedChar(rttr::enumCast(dest));
    file_.writeLongString(str);

    // Prevent loss in case of crash
    file_.flush();
}

void Replay::addGameCommand(unsigned gf, uint8_t player, const PlayerGameCommands& cmds)
{
    RTTR_Assert(isRecording());
    if(!file_.isOpen())
        return;

    file_.writeUnsignedInt(gf);

    file_.writeUnsignedChar(rttr::enumCast(CommandType::Game));
    Serializer ser;
    ser.pushUnsignedChar(player);
    cmds.serialize(ser);
    ser.writeToFile(file_);

    // Prevent loss in case of crash
    file_.flush();
}

std::optional<unsigned> Replay::readGF()
{
    RTTR_Assert(isReplaying());
    try
    {
        return file_.readUnsignedInt();
    } catch(const std::runtime_error&)
    {
        if(file_.isEndOfFile())
            return std::nullopt;
        throw;
    }
}

boost_variant2<Replay::ChatCommand, Replay::GameCommand> Replay::readCommand()
{
    RTTR_Assert(isReplaying());
    const auto type = static_cast<CommandType>(file_.readUnsignedChar() - (subVersion_ == 0 ? 1 : 0));
    switch(type)
    {
        case CommandType::Chat: return ChatCommand(file_);
        case CommandType::Game: return GameCommand(file_, gcVersion_);
        default: throw std::invalid_argument("Invalid command type: " + std::to_string(rttr::enumCast(type)));
    }
}

void Replay::updateLastGF(unsigned last_gf)
{
    RTTR_Assert(isRecording());
    if(!file_.isOpen())
        return;

    file_.seek(lastGfFilePos_, SEEK_SET);
    file_.writeUnsignedInt(last_gf);
    file_.seek(0, SEEK_END);
    lastGF_ = last_gf;
}

Replay::ChatCommand::ChatCommand(BinaryFile& file)
    : player(file.readUnsignedChar()), dest(static_cast<ChatDestination>(file.readUnsignedChar())),
      msg(file.readLongString())
{}

Replay::GameCommand::GameCommand(BinaryFile& file, const unsigned version)
{
    gc::Deserializer ser{version};
    ser.readFromFile(file);
    player = ser.popUnsignedChar();
    cmds.deserialize(ser);
}
