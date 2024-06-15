// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Replay.h"
#include "Savegame.h"
#include "enum_cast.hpp"
#include "network/PlayerGameCommands.h"
#include "gameTypes/MapInfo.h"
#include <s25util/tmpFile.h>
#include <boost/filesystem.hpp>
#include <memory>
#include <mygettext/mygettext.h>

std::string Replay::GetSignature() const
{
    return "RTTRRP2";
}

/// Format version of replay files
uint16_t Replay::GetVersion() const
{
    // Search for "TODO(Replay)" when increasing this (breaking Replay compatibility)
    // and handle/remove the relevant code
    return 8;
}

//////////////////////////////////////////////////////////////////////////

Replay::Replay() = default;
Replay::~Replay() = default;

void Replay::Close()
{
    file_.Close();
    uncompressedDataFile_.reset();
    isRecording_ = false;
    filepath_.clear();
    ClearPlayers();
}

bool Replay::StopRecording()
{
    if(!isRecording_)
        return true;
    const auto replayDataSize = file_.Tell();
    isRecording_ = false;
    file_.Close();

    BinaryFile file;
    if(!file.Open(filepath_, OpenFileMode::Read))
        return false;
    try
    {
        TmpFile tmpReplayFile(".rpl");
        file.Seek(0, SEEK_SET);
        tmpReplayFile.close();
        BinaryFile compressedReplay;
        compressedReplay.Open(tmpReplayFile.filePath, OpenFileMode::Write);

        // Copy header data uncompressed
        std::vector<char> data(lastGfFilePos_);
        file.ReadRawData(data.data(), data.size());
        compressedReplay.WriteRawData(data.data(), data.size());
        const auto lastGF = file.ReadUnsignedInt();
        RTTR_Assert(lastGF == lastGF_);
        compressedReplay.WriteUnsignedInt(lastGF);
        file.ReadUnsignedChar(); // Ignore compressed flag (always zero for the temporary file)

        // Read and compress remaining data
        const auto uncompressedSize = replayDataSize - file.Tell(); // Always positive as there is always some game data
        data.resize(uncompressedSize);
        file.ReadRawData(data.data(), data.size());
        data = CompressedData::compress(data);
        // If the compressed data turns out to be larger than the uncompressed one, bail out
        // This can happen for very short replays
        if(data.size() >= uncompressedSize)
            return true;
        compressedReplay.WriteUnsignedChar(1); // Compressed flag
        compressedReplay.WriteUnsignedInt(uncompressedSize);
        compressedReplay.WriteUnsignedInt(data.size());
        compressedReplay.WriteRawData(data.data(), data.size());

        // All done. Replace uncompressed replay
        compressedReplay.Close();
        file.Close();
        boost::filesystem::rename(tmpReplayFile.filePath, filepath_);
        return true;
    } catch(const std::exception& e)
    {
        lastErrorMsg = e.what();
        return false;
    }
}

bool Replay::StartRecording(const boost::filesystem::path& filepath, const MapInfo& mapInfo, const unsigned randomSeed)
{
    // Deny overwrite, also avoids double-opening by different processes
    if(boost::filesystem::exists(filepath))
        return false;
    if(!file_.Open(filepath, OpenFileMode::Write))
        return false;
    filepath_ = filepath;

    isRecording_ = true;
    /// End-GF (will be updated during the game)
    lastGF_ = 0;
    mapType_ = mapInfo.type;
    randomSeed_ = randomSeed;

    WriteAllHeaderData(file_, mapInfo.title);
    file_.WriteUnsignedShort(rttr::enum_cast(mapType_));
    // For (savegame) format validation
    if(mapType_ == MapType::Savegame)
        mapInfo.savegame->WriteFileHeader(file_);

    // store position to update it later
    lastGfFilePos_ = file_.Tell();
    file_.WriteUnsignedInt(lastGF_);
    file_.WriteUnsignedChar(0); // Compressed flag

    WritePlayerData(file_);
    WriteGGS(file_);

    // Game data
    file_.WriteUnsignedInt(randomSeed_);
    file_.WriteLongString(mapInfo.filepath.string());

    switch(mapType_)
    {
        default: return false;
        case MapType::OldMap:
            RTTR_Assert(!mapInfo.savegame);
            file_.WriteUnsignedInt(mapInfo.mapData.uncompressedLength);
            file_.WriteUnsignedInt(mapInfo.mapData.data.size());
            file_.WriteRawData(&mapInfo.mapData.data[0], mapInfo.mapData.data.size());
            file_.WriteUnsignedInt(mapInfo.luaData.uncompressedLength);
            file_.WriteUnsignedInt(mapInfo.luaData.data.size());
            if(!mapInfo.luaData.data.empty())
                file_.WriteRawData(&mapInfo.luaData.data[0], mapInfo.luaData.data.size());
            break;
        case MapType::Savegame: mapInfo.savegame->Save(file_, GetMapName()); break;
    }
    // Flush now to not loose any information
    file_.Flush();

    return true;
}

const boost::filesystem::path& Replay::GetPath() const
{
    RTTR_Assert(file_.IsOpen());
    return filepath_;
}

bool Replay::LoadHeader(const boost::filesystem::path& filepath)
{
    Close();
    if(!file_.Open(filepath, OpenFileMode::Read))
    {
        lastErrorMsg = _("File could not be opened.");
        return false;
    }
    filepath_ = filepath;

    try
    {
        // Check file header
        if(!ReadAllHeaderData(file_))
            return false;

        mapType_ = static_cast<MapType>(file_.ReadUnsignedShort());
        if(mapType_ == MapType::Savegame)
        {
            // Validate savegame
            Savegame save;
            if(!save.ReadFileHeader(file_))
            {
                lastErrorMsg = std::string(_("Savegame error: ")) + save.GetLastErrorMsg();
                return false;
            }
        }

        lastGF_ = file_.ReadUnsignedInt();
    } catch(std::runtime_error& e)
    {
        lastErrorMsg = e.what();
        return false;
    }

    return true;
}

bool Replay::LoadGameData(MapInfo& mapInfo)
{
    try
    {
        const bool isCompressed = file_.ReadUnsignedChar() != 0;
        if(isCompressed)
        {
            const auto uncompressedSize = file_.ReadUnsignedInt();
            const auto compressedSize = file_.ReadUnsignedInt();
            CompressedData compressedData(uncompressedSize);
            compressedData.data.resize(compressedSize);
            file_.ReadRawData(compressedData.data.data(), compressedSize);
            uncompressedDataFile_ = std::make_unique<TmpFile>(".rpl");
            uncompressedDataFile_->close();
            compressedData.DecompressToFile(uncompressedDataFile_->filePath);
            file_.Close();
            file_.Open(uncompressedDataFile_->filePath, OpenFileMode::Read);
        }

        ReadPlayerData(file_);
        ReadGGS(file_);
        randomSeed_ = file_.ReadUnsignedInt();

        mapInfo.Clear();
        mapInfo.type = mapType_;
        mapInfo.title = GetMapName();
        mapInfo.filepath = file_.ReadLongString();
        switch(mapType_)
        {
            default: return false;
            case MapType::OldMap:
                mapInfo.mapData.uncompressedLength = file_.ReadUnsignedInt();
                mapInfo.mapData.data.resize(file_.ReadUnsignedInt());
                file_.ReadRawData(&mapInfo.mapData.data[0], mapInfo.mapData.data.size());
                mapInfo.luaData.uncompressedLength = file_.ReadUnsignedInt();
                mapInfo.luaData.data.resize(file_.ReadUnsignedInt());
                if(!mapInfo.luaData.data.empty())
                    file_.ReadRawData(&mapInfo.luaData.data[0], mapInfo.luaData.data.size());
                break;
            case MapType::Savegame:
                mapInfo.savegame = std::make_unique<Savegame>();
                if(!mapInfo.savegame->Load(file_, SaveGameDataToLoad::All))
                {
                    lastErrorMsg = std::string(_("Savegame error: ")) + mapInfo.savegame->GetLastErrorMsg();
                    return false;
                }
                break;
        }
    } catch(std::runtime_error& e)
    {
        lastErrorMsg = e.what();
        return false;
    }
    return true;
}

void Replay::AddChatCommand(unsigned gf, uint8_t player, ChatDestination dest, const std::string& str)
{
    RTTR_Assert(IsRecording());
    if(!file_.IsOpen())
        return;

    file_.WriteUnsignedInt(gf);

    file_.WriteUnsignedChar(rttr::enum_cast(CommandType::Chat));
    file_.WriteUnsignedChar(player);
    file_.WriteUnsignedChar(rttr::enum_cast(dest));
    file_.WriteLongString(str);

    // Prevent loss in case of crash
    file_.Flush();
}

void Replay::AddGameCommand(unsigned gf, uint8_t player, const PlayerGameCommands& cmds)
{
    RTTR_Assert(IsRecording());
    if(!file_.IsOpen())
        return;

    file_.WriteUnsignedInt(gf);

    file_.WriteUnsignedChar(rttr::enum_cast(CommandType::Game));
    Serializer ser;
    ser.PushUnsignedChar(player);
    cmds.Serialize(ser);
    ser.WriteToFile(file_);

    // Prevent loss in case of crash
    file_.Flush();
}

std::optional<unsigned> Replay::ReadGF()
{
    RTTR_Assert(IsReplaying());
    try
    {
        return file_.ReadUnsignedInt();
    } catch(std::runtime_error&)
    {
        if(file_.IsEndOfFile())
            return std::nullopt;
        throw;
    }
}

boost_variant2<Replay::ChatCommand, Replay::GameCommand> Replay::ReadCommand()
{
    RTTR_Assert(IsReplaying());
    const auto type = static_cast<CommandType>(file_.ReadUnsignedChar());
    switch(type)
    {
        case CommandType::Chat: return ChatCommand(file_);
        case CommandType::Game: return GameCommand(file_);
        default: throw std::invalid_argument("Invalid command type: " + std::to_string(rttr::enum_cast(type)));
    }
}

void Replay::UpdateLastGF(unsigned last_gf)
{
    RTTR_Assert(IsRecording());
    if(!file_.IsOpen())
        return;

    file_.Seek(lastGfFilePos_, SEEK_SET);
    file_.WriteUnsignedInt(last_gf);
    file_.Seek(0, SEEK_END);
    lastGF_ = last_gf;
}

Replay::ChatCommand::ChatCommand(BinaryFile& file)
    : player(file.ReadUnsignedChar()), dest(static_cast<ChatDestination>(file.ReadUnsignedChar())),
      msg(file.ReadLongString())
{}

Replay::GameCommand::GameCommand(BinaryFile& file)
{
    Serializer ser;
    ser.ReadFromFile(file);
    player = ser.PopUnsignedChar();
    cmds.Deserialize(ser);
}
