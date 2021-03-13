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

#include "Replay.h"
#include "Savegame.h"
#include "network/PlayerGameCommands.h"
#include "gameTypes/MapInfo.h"
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
    return 6;
}

//////////////////////////////////////////////////////////////////////////

Replay::Replay() : random_init(0), isRecording(false), lastGF_(0), last_gf_file_pos(0), mapType_(MapType::OldMap) {}

Replay::~Replay()
{
    StopRecording();
}

void Replay::Close()
{
    file.Close();
    ClearPlayers();
}

void Replay::StopRecording()
{
    file.Close();
    isRecording = false;
}

bool Replay::StartRecording(const boost::filesystem::path& filepath, const MapInfo& mapInfo)
{
    // Deny overwrite, also avoids double-opening by different processes
    if(boost::filesystem::exists(filepath))
        return false;
    // Datei öffnen
    if(!file.Open(filepath, OFM_WRITE))
        return false;

    isRecording = true;
    /// End-GF (erstmal nur 0, wird dann im Spiel immer geupdatet)
    lastGF_ = 0;
    mapType_ = mapInfo.type;

    // Write header
    WriteAllHeaderData(file, mapInfo.title);

    file.WriteUnsignedShort(static_cast<unsigned short>(mapType_));
    // For validation purposes
    if(mapType_ == MapType::Savegame)
        mapInfo.savegame->WriteFileHeader(file);

    // Position merken für End-GF
    last_gf_file_pos = file.Tell();
    file.WriteUnsignedInt(lastGF_);

    WritePlayerData(file);
    WriteGGS(file);

    // Game data
    file.WriteUnsignedInt(random_init);
    file.WriteLongString(mapInfo.filepath.string());

    switch(mapType_)
    {
        default: return false;
        case MapType::OldMap:
            RTTR_Assert(!mapInfo.savegame);
            // Map-Daten
            file.WriteUnsignedInt(mapInfo.mapData.length);
            file.WriteUnsignedInt(mapInfo.mapData.data.size());
            file.WriteRawData(&mapInfo.mapData.data[0], mapInfo.mapData.data.size());
            file.WriteUnsignedInt(mapInfo.luaData.length);
            file.WriteUnsignedInt(mapInfo.luaData.data.size());
            if(!mapInfo.luaData.data.empty())
                file.WriteRawData(&mapInfo.luaData.data[0], mapInfo.luaData.data.size());
            break;
        case MapType::Savegame: mapInfo.savegame->Save(file, GetMapName()); break;
    }
    // Alles sofort reinschreiben
    file.Flush();

    return true;
}

bool Replay::LoadHeader(const boost::filesystem::path& filepath)
{
    // Datei öffnen
    if(!file.Open(filepath, OFM_READ))
    {
        lastErrorMsg = _("File could not be opened.");
        return false;
    }
    isRecording = false;

    try
    {
        // Check file header
        if(!ReadAllHeaderData(file))
            return false;

        mapType_ = static_cast<MapType>(file.ReadUnsignedShort());
        if(mapType_ == MapType::Savegame)
        {
            // Validate savegame
            Savegame save;
            if(!save.ReadFileHeader(file))
            {
                lastErrorMsg = std::string(_("Savegame error: ")) + save.GetLastErrorMsg();
                return false;
            }
        }

        lastGF_ = file.ReadUnsignedInt();
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
        ReadPlayerData(file);
        ReadGGS(file);
        random_init = file.ReadUnsignedInt();

        mapInfo.Clear();
        mapInfo.type = mapType_;
        mapInfo.title = GetMapName();
        mapInfo.filepath = file.ReadLongString();
        switch(mapType_)
        {
            default: return false;
            case MapType::OldMap:
                // Map-Daten
                mapInfo.mapData.length = file.ReadUnsignedInt();
                mapInfo.mapData.data.resize(file.ReadUnsignedInt());
                file.ReadRawData(&mapInfo.mapData.data[0], mapInfo.mapData.data.size());
                mapInfo.luaData.length = file.ReadUnsignedInt();
                mapInfo.luaData.data.resize(file.ReadUnsignedInt());
                if(!mapInfo.luaData.data.empty())
                    file.ReadRawData(&mapInfo.luaData.data[0], mapInfo.luaData.data.size());
                break;
            case MapType::Savegame:
                // Load savegame
                mapInfo.savegame = std::make_unique<Savegame>();
                if(!mapInfo.savegame->Load(file, SaveGameDataToLoad::All))
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
    if(!file.IsValid())
        return;

    file.WriteUnsignedInt(gf);

    file.WriteUnsignedChar(static_cast<uint8_t>(ReplayCommand::Chat));
    file.WriteUnsignedChar(player);
    file.WriteUnsignedChar(static_cast<uint8_t>(dest));
    file.WriteLongString(str);

    // Sofort rein damit
    file.Flush();
}

void Replay::AddGameCommand(unsigned gf, uint8_t player, const PlayerGameCommands& cmds)
{
    RTTR_Assert(IsRecording());
    if(!file.IsValid())
        return;

    file.WriteUnsignedInt(gf);

    file.WriteUnsignedChar(static_cast<uint8_t>(ReplayCommand::Game));
    Serializer ser;
    ser.PushUnsignedChar(player);
    cmds.Serialize(ser);
    ser.WriteToFile(file);

    // Sofort rein damit
    file.Flush();
}

bool Replay::ReadGF(unsigned* gf)
{
    RTTR_Assert(IsReplaying());
    try
    {
        *gf = file.ReadUnsignedInt();
    } catch(std::runtime_error&)
    {
        *gf = 0xFFFFFFFF;
        if(file.EndOfFile())
            return false;
        throw;
    }
    return true;
}

ReplayCommand Replay::ReadRCType()
{
    RTTR_Assert(IsReplaying());
    // Type auslesen
    return ReplayCommand(file.ReadUnsignedChar());
}

void Replay::ReadChatCommand(uint8_t& player, uint8_t& dest, std::string& str)
{
    RTTR_Assert(IsReplaying());
    player = file.ReadUnsignedChar();
    dest = file.ReadUnsignedChar();
    str = file.ReadLongString();
}

void Replay::ReadGameCommand(uint8_t& player, PlayerGameCommands& cmds)
{
    RTTR_Assert(IsReplaying());
    Serializer ser;
    ser.ReadFromFile(file);
    player = ser.PopUnsignedChar();
    cmds.Deserialize(ser);
}

void Replay::UpdateLastGF(unsigned last_gf)
{
    RTTR_Assert(IsRecording());
    if(!file.IsValid())
        return;

    // An die Stelle springen
    file.Seek(last_gf_file_pos, SEEK_SET);
    // Dorthin schreiben
    file.WriteUnsignedInt(last_gf);
    // Wieder ans Ende springen
    file.Seek(0, SEEK_END);
}
