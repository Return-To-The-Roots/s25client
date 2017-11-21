// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "Replay.h"
#include "BasePlayerInfo.h"
#include "Savegame.h"
#include "network/PlayerGameCommands.h"
#include "gameTypes/MapInfo.h"
#include "libendian/ConvertEndianess.h"
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

std::string Replay::GetSignature() const
{
    return "RTTRRP2";
}

uint16_t Replay::GetVersion() const
{
    /// Version des Replay-Formates
    return 4;
}

//////////////////////////////////////////////////////////////////////////

Replay::Replay() : random_init(0), isRecording(false), lastGF_(0), last_gf_file_pos(0), mapType_(MAPTYPE_OLDMAP) {}

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

bool Replay::StartRecording(const std::string& filename, const MapInfo& mapInfo)
{
    // Deny overwrite, also avoids double-opening by different processes
    if(bfs::exists(filename))
        return false;
    // Datei öffnen
    if(!file.Open(filename, OFM_WRITE))
        return false;

    isRecording = true;
    /// End-GF (erstmal nur 0, wird dann im Spiel immer geupdatet)
    lastGF_ = 0;
    mapType_ = mapInfo.type;

    // Write header
    WriteAllHeaderData(file, mapInfo.title);

    file.WriteUnsignedShort(static_cast<unsigned short>(mapType_));
    // For validation purposes
    if(mapType_ == MAPTYPE_SAVEGAME)
        mapInfo.savegame->WriteFileHeader(file);

    // Position merken für End-GF
    last_gf_file_pos = file.Tell();
    file.WriteUnsignedInt(lastGF_);

    WritePlayerData(file);
    WriteGGS(file);

    // Game data
    file.WriteUnsignedInt(random_init);
    file.WriteLongString(mapInfo.filepath);

    switch(mapType_)
    {
        default: return false;
        case MAPTYPE_OLDMAP:
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
        case MAPTYPE_SAVEGAME: mapInfo.savegame->Save(file, GetMapName()); break;
    }
    // Alles sofort reinschreiben
    file.Flush();

    return true;
}

bool Replay::LoadHeader(const std::string& filename, bool loadSettings)
{
    // Datei öffnen
    if(!file.Open(filename, OFM_READ))
    {
        lastErrorMsg = _("File could not be opened.");
        return false;
    }
    isRecording = false;

    // Check file header
    if(!ReadAllHeaderData(file))
        return false;

    mapType_ = static_cast<MapType>(file.ReadUnsignedShort());
    if(mapType_ == MAPTYPE_SAVEGAME)
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

    if(loadSettings)
    {
        ReadPlayerData(file);
        ReadGGS(file);
    }

    return true;
}

bool Replay::LoadGameData(MapInfo& mapInfo)
{
    random_init = file.ReadUnsignedInt();

    mapInfo.Clear();
    mapInfo.type = mapType_;
    mapInfo.title = GetMapName();
    mapInfo.filepath = file.ReadLongString();
    switch(mapType_)
    {
        default: return false;
        case MAPTYPE_OLDMAP:
        {
            // Map-Daten
            mapInfo.mapData.length = file.ReadUnsignedInt();
            mapInfo.mapData.data.resize(file.ReadUnsignedInt());
            file.ReadRawData(&mapInfo.mapData.data[0], mapInfo.mapData.data.size());
            mapInfo.luaData.length = file.ReadUnsignedInt();
            mapInfo.luaData.data.resize(file.ReadUnsignedInt());
            if(!mapInfo.luaData.data.empty())
                file.ReadRawData(&mapInfo.luaData.data[0], mapInfo.luaData.data.size());
            break;
        }
        case MAPTYPE_SAVEGAME:
        {
            // Load savegame
            mapInfo.savegame.reset(new Savegame);
            if(!mapInfo.savegame->Load(file, true, true))
            {
                lastErrorMsg = std::string(_("Savegame error: ")) + mapInfo.savegame->GetLastErrorMsg();
                return false;
            }
            break;
        }
    }

    return true;
}

void Replay::AddChatCommand(unsigned gf, uint8_t player, uint8_t dest, const std::string& str)
{
    if(!file.IsValid())
        return;

    file.WriteUnsignedInt(gf);

    file.WriteUnsignedChar(RC_CHAT);
    file.WriteUnsignedChar(player);
    file.WriteUnsignedChar(dest);
    file.WriteLongString(str);

    // Sofort rein damit
    file.Flush();
}

void Replay::AddGameCommand(unsigned gf, uint8_t player, const PlayerGameCommands& cmds)
{
    if(!file.IsValid())
        return;

    file.WriteUnsignedInt(gf);

    file.WriteUnsignedChar(RC_GAME);
    Serializer ser;
    ser.PushUnsignedChar(player);
    cmds.Serialize(ser);
    ser.WriteToFile(file);

    // Sofort rein damit
    file.Flush();
}

bool Replay::ReadGF(unsigned* gf)
{
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

Replay::ReplayCommand Replay::ReadRCType()
{
    // Type auslesen
    return ReplayCommand(file.ReadUnsignedChar());
}

void Replay::ReadChatCommand(uint8_t& player, uint8_t& dest, std::string& str)
{
    player = file.ReadUnsignedChar();
    dest = file.ReadUnsignedChar();
    str = file.ReadLongString();
}

void Replay::ReadGameCommand(uint8_t& player, PlayerGameCommands& cmds)
{
    Serializer ser;
    ser.ReadFromFile(file);
    player = ser.PopUnsignedChar();
    cmds.Deserialize(ser);
}

void Replay::UpdateLastGF(unsigned last_gf)
{
    if(!file.IsValid())
        return;

    // An die Stelle springen
    file.Seek(last_gf_file_pos, SEEK_SET);
    // Dorthin schreiben
    file.WriteUnsignedInt(last_gf);
    // Wieder ans Ende springen
    file.Seek(0, SEEK_END);
}
