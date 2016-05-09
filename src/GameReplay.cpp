// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "GameReplay.h"
#include "GameSavegame.h"
#include "gameTypes/MapInfo.h"
#include "Log.h"
#include "libendian/src/ConvertEndianess.h"
#include <boost/filesystem.hpp>

/// Kleine Signatur am Anfang "RTTRRP", die ein gültiges S25 RTTR Replay kennzeichnet
const char Replay::REPLAY_SIGNATURE[6] = {'R', 'T', 'T', 'R', 'R', 'P'};
/// Version des Replay-Formates
const unsigned short Replay::REPLAY_VERSION = 30;

Replay::Replay() : nwf_length(0), random_init(0),
                   lastGF_(0), last_gf_file_pos(0), gf_file_pos(0)
{
}

Replay::~Replay()
{
    StopRecording();
}

void Replay::StopRecording()
{
    file.Close();
    ClearPlayers();
}

bool Replay::WriteHeader(const std::string& filename, const MapInfo& mapInfo)
{
    // Deny overwrite, also avoids double-opening by different processes
    if(bfs::exists(filename))
        return false;
    // Datei öffnen
    if(!file.Open(filename, OFM_WRITE))
        return false;

    Replay::fileName_ = filename;

    // Versionszeug schreiben
    WriteVersion(file, 6, REPLAY_SIGNATURE, REPLAY_VERSION);
    unser_time_t tmpTime = libendian::ConvertEndianess<false>::fromNative(save_time);
    file.WriteRawData(&tmpTime, sizeof(tmpTime));
    /// NWF-Länge
    file.WriteUnsignedShort(nwf_length);
    /// Zufallsgeneratorinitialisierung
    file.WriteUnsignedInt(random_init);

    // Position merken für End-GF
    last_gf_file_pos = file.Tell();

    /// End-GF (erstmal nur 0, wird dann im Spiel immer geupdatet)
    file.WriteUnsignedInt(lastGF_);
    // Anzahl Spieler
    file.WriteUnsignedChar(GetPlayerCount());
    // Spielerdaten
    WritePlayerData(file);
    // GGS
    WriteGGS(file);

    // Map-Type
    file.WriteUnsignedShort(static_cast<unsigned short>(mapInfo.type));

    switch(mapInfo.type)
    {
        default:
            break;
        case MAPTYPE_OLDMAP:
        {
            RTTR_Assert(!mapInfo.savegame);
            // Map-Daten
            file.WriteUnsignedInt(mapInfo.mapData.length);
            file.WriteUnsignedInt(mapInfo.mapData.data.size());
            file.WriteRawData(&mapInfo.mapData.data[0], mapInfo.mapData.data.size());
            file.WriteUnsignedInt(mapInfo.luaData.length);
            if(mapInfo.luaData.length)
            {
                file.WriteUnsignedInt(mapInfo.luaData.data.size());
                file.WriteRawData(&mapInfo.luaData.data[0], mapInfo.luaData.data.size());
            }
        } break;
        case MAPTYPE_SAVEGAME:
        {
            // Savegame speichern
            if(!mapInfo.savegame->Save(file))
                return false;
        } break;
    }

    // Mapname
    file.WriteShortString(mapFileName);
    file.WriteShortString(mapName);

    // Alles sofort reinschreiben
    file.Flush();

    gf_file_pos  = 0;

    return true;
}

bool Replay::LoadHeader(const std::string& filename, MapInfo* mapInfo)
{
    this->fileName_ = filename;
    // Datei öffnen
    if(!file.Open(filename, OFM_READ))
        return false;

    // Version überprüfen
    // Signatur und Version einlesen
    if(!ValidateFile(file, sizeof(REPLAY_SIGNATURE), REPLAY_SIGNATURE, REPLAY_VERSION))
        return false;

    // Zeitstempel
    file.ReadRawData(&save_time, sizeof(save_time));
    save_time = libendian::ConvertEndianess<false>::toNative(save_time);
    // NWF-Länge
    nwf_length = file.ReadUnsignedShort();
    // Zufallsgeneratorinitialisierung
    random_init = file.ReadUnsignedInt();
    /// End-GF
    lastGF_ = file.ReadUnsignedInt();

    ReadPlayerData(file);
    ReadGGS(file);

    // Map-Type
    MapType mapType = static_cast<MapType>(file.ReadUnsignedShort());

    if(mapInfo)
    {
        switch(mapType)
        {
            default:
                break;
            case MAPTYPE_OLDMAP:
            {
                // Map-Daten
                mapInfo->mapData.length = file.ReadUnsignedInt();
                mapInfo->mapData.data.resize(file.ReadUnsignedInt());
                file.ReadRawData(&mapInfo->mapData.data[0], mapInfo->mapData.data.size());
                mapInfo->luaData.length = file.ReadUnsignedInt();
                if(mapInfo->luaData.length)
                {
                    mapInfo->luaData.data.resize(file.ReadUnsignedInt());
                    file.ReadRawData(&mapInfo->luaData.data[0], mapInfo->luaData.data.size());
                }
            } break;
            case MAPTYPE_SAVEGAME:
            {
                // Load savegame
                mapInfo->savegame.reset(new Savegame);
                if(!mapInfo->savegame->Load(file, true, true))
                    return false;
            } break;
        }

        mapFileName = file.ReadShortString();
        mapName = file.ReadShortString();
        mapInfo->title = mapName;
        mapInfo->type = mapType;
    } else if(mapType == MAPTYPE_SAVEGAME)
    {
        // Validate savegame
        Savegame save;
        if(!save.Load(file, false, false))
            return false;
    }

    return true;
}

void Replay::AddChatCommand(const unsigned gf, const unsigned char player, const unsigned char dest, const std::string& str)
{
    if(!file.IsValid())
        return;

    // GF-Anzahl
    if(gf_file_pos)
    {
        unsigned current_pos = file.Tell();
        file.Seek(gf_file_pos, SEEK_SET);
        file.WriteUnsignedInt(gf);
        file.Seek(current_pos, SEEK_SET);
    }
    else
        file.WriteUnsignedInt(gf);

    // Type (0)
    file.WriteUnsignedChar(RC_CHAT);
    // Spieler
    file.WriteUnsignedChar(player);
    // Ziel
    file.WriteUnsignedChar(dest);
    // Chat-Text
    file.WriteLongString(str);

    // Platzhalter für nächste GF-Zahl
    gf_file_pos = file.Tell();
    file.WriteUnsignedInt(0xffffffff);

    // Sofort rein damit
    file.Flush();
}

void Replay::AddGameCommand(const unsigned gf, const unsigned short length, const unsigned char* const data)
{
    if(!file.IsValid())
        return;

    //// Marker schreiben
    //file.WriteRawData("GCCM", 4);

    // GF-Anzahl
    if(gf_file_pos)
    {
        unsigned current_pos = file.Tell();
        file.Seek(gf_file_pos, SEEK_SET);
        file.WriteUnsignedInt(gf);
        file.Seek(current_pos, SEEK_SET);
    }
    else
        file.WriteUnsignedInt(gf);

    // Type (RC_GAME)
    file.WriteUnsignedChar(RC_GAME);

    // Länge der Daten
    file.WriteUnsignedShort(length);
    // Daten
    file.WriteRawData(data, length);

    // Platzhalter für nächste GF-Zahl
    gf_file_pos = file.Tell();
    file.WriteUnsignedInt(0xeeeeeeee);

    // Sofort rein damit
    file.Flush();
}

bool Replay::ReadGF(unsigned* gf)
{
    //// kein Marker bedeutet das Ende der Welt
    //if(memcmp(marker, "GCCM", 4) != 0)
    //  return false;

    // GF-Anzahl
    *gf = file.ReadUnsignedInt();

    // Replaydatei zu Ende?
    bool eof = file.EndOfFile();
    if(eof)
        *gf = 0xFFFFFFFF;

    return !eof;
}

Replay::ReplayCommand Replay::ReadRCType()
{
    // Type auslesen
    return ReplayCommand(file.ReadUnsignedChar());
}

void Replay::ReadChatCommand(unsigned char* player, unsigned char*   dest, std::string& str)
{
    *player = file.ReadUnsignedChar();
    *dest = file.ReadUnsignedChar();
    str = file.ReadLongString();
}

std::vector<unsigned char> Replay::ReadGameCommand()
{
    std::vector<unsigned char> result(file.ReadUnsignedShort());
    file.ReadRawData(&result.front(), result.size());
    return result;
}

void Replay::UpdateLastGF(const unsigned last_gf)
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
