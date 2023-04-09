// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Replay.h"
#include "Savegame.h"
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

uint16_t Replay::GetVersion() const
{
    /// Version des Replay-Formates
    /// Search for "TODO(Replay)" when increasing this (breaking Replay compatibility)
    return 8;
}

//////////////////////////////////////////////////////////////////////////

Replay::Replay() : random_init(0), isRecording_(false), lastGF_(0), lastGfFilePos_(0), mapType_(MapType::OldMap) {}

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
    const unsigned replayDataSize = file_.Tell();
    isRecording_ = false;
    file_.Close();

    BinaryFile file;
    if(!file.Open(filepath_, OpenFileMode::OFM_READ))
        return false;
    try
    {
        TmpFile tmpReplayFile(".rpl");
        file.Seek(0, SEEK_SET);
        tmpReplayFile.close();
        BinaryFile compressedReplay;
        compressedReplay.Open(tmpReplayFile.filePath, OpenFileMode::OFM_WRITE);

        // Copy header data uncompressed
        std::vector<char> data(lastGfFilePos_);
        file.ReadRawData(data.data(), data.size());
        compressedReplay.WriteRawData(data.data(), data.size());
        const auto lastGF = file.ReadUnsignedInt();
        RTTR_Assert(lastGF == lastGF_);
        compressedReplay.WriteUnsignedInt(lastGF);
        file.ReadUnsignedChar(); // Ignore compressed flag
        compressedReplay.WriteUnsignedChar(1);

        // Read and compress remaining data
        const auto uncompressedSize = replayDataSize - file.Tell();
        data.resize(uncompressedSize);
        file.ReadRawData(data.data(), data.size());
        data = CompressedData::compress(data);
        // If the compressed data turns out to be larger than the uncompressed one, bail out
        // This can happen for very short replays
        if(data.size() >= uncompressedSize)
            return true;
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

bool Replay::StartRecording(const boost::filesystem::path& filepath, const MapInfo& mapInfo)
{
    // Deny overwrite, also avoids double-opening by different processes
    if(boost::filesystem::exists(filepath))
        return false;
    // Datei öffnen
    if(!file_.Open(filepath, OFM_WRITE))
        return false;
    filepath_ = filepath;

    isRecording_ = true;
    /// End-GF (erstmal nur 0, wird dann im Spiel immer geupdatet)
    lastGF_ = 0;
    mapType_ = mapInfo.type;

    // Write header
    WriteAllHeaderData(file_, mapInfo.title);

    file_.WriteUnsignedShort(static_cast<unsigned short>(mapType_));
    // For validation purposes
    if(mapType_ == MapType::Savegame)
        mapInfo.savegame->WriteFileHeader(file_);

    // Position merken für End-GF
    lastGfFilePos_ = file_.Tell();
    file_.WriteUnsignedInt(lastGF_);
    file_.WriteUnsignedChar(0); // Compressed flag

    WritePlayerData(file_);
    WriteGGS(file_);

    // Game data
    file_.WriteUnsignedInt(random_init);
    file_.WriteLongString(mapInfo.filepath.string());

    switch(mapType_)
    {
        default: return false;
        case MapType::OldMap:
            RTTR_Assert(!mapInfo.savegame);
            // Map-Daten
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
    // Alles sofort reinschreiben
    file_.Flush();

    return true;
}

const boost::filesystem::path& Replay::GetPath() const
{
    RTTR_Assert(IsValid());
    return filepath_;
}

bool Replay::LoadHeader(const boost::filesystem::path& filepath)
{
    Close();
    if(!file_.Open(filepath, OFM_READ))
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
            file_.Open(uncompressedDataFile_->filePath, OpenFileMode::OFM_READ);
        }

        ReadPlayerData(file_);
        ReadGGS(file_);
        random_init = file_.ReadUnsignedInt();

        mapInfo.Clear();
        mapInfo.type = mapType_;
        mapInfo.title = GetMapName();
        mapInfo.filepath = file_.ReadLongString();
        switch(mapType_)
        {
            default: return false;
            case MapType::OldMap:
                // Map-Daten
                mapInfo.mapData.uncompressedLength = file_.ReadUnsignedInt();
                mapInfo.mapData.data.resize(file_.ReadUnsignedInt());
                file_.ReadRawData(&mapInfo.mapData.data[0], mapInfo.mapData.data.size());
                mapInfo.luaData.uncompressedLength = file_.ReadUnsignedInt();
                mapInfo.luaData.data.resize(file_.ReadUnsignedInt());
                if(!mapInfo.luaData.data.empty())
                    file_.ReadRawData(&mapInfo.luaData.data[0], mapInfo.luaData.data.size());
                break;
            case MapType::Savegame:
                // Load savegame
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
    if(!file_.IsValid())
        return;

    file_.WriteUnsignedInt(gf);

    file_.WriteUnsignedChar(static_cast<uint8_t>(ReplayCommand::Chat));
    file_.WriteUnsignedChar(player);
    file_.WriteUnsignedChar(static_cast<uint8_t>(dest));
    file_.WriteLongString(str);

    // Sofort rein damit
    file_.Flush();
}

void Replay::AddGameCommand(unsigned gf, uint8_t player, const PlayerGameCommands& cmds)
{
    RTTR_Assert(IsRecording());
    if(!file_.IsValid())
        return;

    file_.WriteUnsignedInt(gf);

    file_.WriteUnsignedChar(static_cast<uint8_t>(ReplayCommand::Game));
    Serializer ser;
    ser.PushUnsignedChar(player);
    cmds.Serialize(ser);
    ser.WriteToFile(file_);

    // Sofort rein damit
    file_.Flush();
}

bool Replay::ReadGF(unsigned* gf)
{
    RTTR_Assert(IsReplaying());
    try
    {
        *gf = file_.ReadUnsignedInt();
    } catch(std::runtime_error&)
    {
        *gf = 0xFFFFFFFF;
        if(file_.EndOfFile())
            return false;
        throw;
    }
    return true;
}

ReplayCommand Replay::ReadRCType()
{
    RTTR_Assert(IsReplaying());
    // Type auslesen
    return ReplayCommand(file_.ReadUnsignedChar());
}

void Replay::ReadChatCommand(uint8_t& player, uint8_t& dest, std::string& str)
{
    RTTR_Assert(IsReplaying());
    player = file_.ReadUnsignedChar();
    dest = file_.ReadUnsignedChar();
    str = file_.ReadLongString();
}

void Replay::ReadGameCommand(uint8_t& player, PlayerGameCommands& cmds)
{
    RTTR_Assert(IsReplaying());
    Serializer ser;
    ser.ReadFromFile(file_);
    player = ser.PopUnsignedChar();
    cmds.Deserialize(ser);
}

void Replay::UpdateLastGF(unsigned last_gf)
{
    RTTR_Assert(IsRecording());
    if(!file_.IsValid())
        return;

    // An die Stelle springen
    file_.Seek(lastGfFilePos_, SEEK_SET);
    // Dorthin schreiben
    file_.WriteUnsignedInt(last_gf);
    // Wieder ans Ende springen
    file_.Seek(0, SEEK_END);
    lastGF_ = last_gf;
}
