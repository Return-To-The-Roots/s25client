// $Id: GameSavegame.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "GameSavegame.h"

/// Kleine Signatur am Anfang "RTTRSAVE", die ein gültiges S25 RTTR Savegame kennzeichnet
const char Savegame::SAVE_SIGNATURE[8] = {'R', 'T', 'T', 'R', 'S', 'A', 'V', 'E'};
/// Version des Savegame-Formates
const unsigned short Savegame::SAVE_VERSION = 25;

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
Savegame::Savegame() : SavedFile(), start_gf(0)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
Savegame::~Savegame()
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
bool Savegame::Save(const std::string& filename)
{
    BinaryFile file;

    if(!file.Open(filename.c_str(), OFM_WRITE))
        return false;

    bool ret = Save(file);

    file.Close();

    return ret;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
bool Savegame::Save(BinaryFile& file)
{
    // Versionszeug schreiben
    WriteVersion(file, 8, SAVE_SIGNATURE, SAVE_VERSION);

    // Timestamp der Aufzeichnung (TODO: Little/Big Endian unterscheidung)
    file.WriteRawData(&save_time, 8);

    // Mapname
    file.WriteShortString(map_name);

    // Anzahl Spieler
    file.WriteUnsignedChar(player_count);

    // Größe der Spielerdaten (später ausfüllen)
    unsigned players_size = 0;
    unsigned players_pos = file.Tell();
    file.WriteUnsignedInt(players_size);

    // Spielerdaten
    WritePlayerData(file);

    // Wieder zurückspringen und Größe des Spielerblocks eintragen
    unsigned new_pos = file.Tell();
    file.Seek(players_pos, SEEK_SET);
    file.WriteUnsignedInt(new_pos - players_pos - 4);
    file.Seek(new_pos, SEEK_SET);

    // GGS
    WriteGGS(file);

    // Start-GF
    file.WriteUnsignedInt(start_gf);

    // Serialisiertes Spielzeug reinschreiben
    sgd.WriteToFile(file);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
bool Savegame::Load(const std::string&  filename, const bool load_players, const bool load_sgd)
{
    BinaryFile file;

    if(!file.Open(filename.c_str(), OFM_READ))
        return false;

    bool ret = Load(file, load_players, load_sgd);

    file.Close();

    return ret;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
bool Savegame::Load(BinaryFile& file, const bool load_players, const bool load_sgd)
{
    // Signatur und Version einlesen
    if(!ValidateFile(file, 8, SAVE_SIGNATURE, SAVE_VERSION))
    {
        LOG.lprintf("Savegame::Load: ERROR: File is not a valid RTTR savegame!\n");
        return false;
    }

    // Zeitstempel
    file.ReadRawData(&save_time, 8);

    // Map-Name
    file.ReadShortString(map_name);

    // Anzahl Spieler
    player_count = file.ReadUnsignedChar();

    // Spielerzeug
    if(load_players)
    {
        // Größe des Spielerblocks überspringen
        file.Seek(4, SEEK_CUR);

        ReadPlayerData(file);
    }
    else
    {
        // Überspringen
        players = 0;
        unsigned player_size = file.ReadUnsignedInt();
        file.Seek(player_size, SEEK_CUR);
    }

    // GGS
    ReadGGS(file);

    // Start-GF
    start_gf = file.ReadUnsignedInt();

    if(load_sgd)
    {
        // Serialisiertes Spielzeug lesen
        sgd.ReadFromFile(file);
    }

    return true;
}
