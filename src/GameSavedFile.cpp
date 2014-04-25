// $Id: GameSavedFile.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "BinaryFile.h"
#include "GameSavedFile.h"


#include "GamePlayerInfo.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
SavedFile::SavedFile() : save_time(0), player_count(0), players(0)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
SavedFile::~SavedFile()
{
    delete[] players;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void SavedFile::WriteVersion(BinaryFile& file, unsigned int signature_length, const char* signature, unsigned short version)
{
    // Signatur schreiben
    file.WriteRawData(signature, signature_length);

    // Version vom Programm reinschreiben (mal 0 mit reinschreiben, damits ne runde 8 ergibt!)
    file.WriteRawData(GetWindowRevision(), 8);

    // Version des Save-Formats
    file.WriteUnsignedShort(version);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
bool SavedFile::ValidateFile(BinaryFile& file, unsigned int signature_length, const char* signature, unsigned short version)
{
    char read_signature[32];

    file.ReadRawData(read_signature, signature_length);

    // Signatur überprüfen
    if(memcmp(read_signature, signature, signature_length))
    {
        // unterscheiden sich! --> raus
        LOG.lprintf("SavedFile::Load: ERROR: Not a valid file!\n");
        return false;
    }

    // Programmversion überspringen
    file.Seek(8, SEEK_CUR);

    // Version überprüfen
    unsigned short read_version = file.ReadUnsignedShort();
    if(read_version != version)
    {
        // anderes Dateiformat --> raus
        LOG.lprintf("SavedFile::Load: ERROR: Old file version (version: %u; expected: %u)!\n", read_version, version);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void SavedFile::WritePlayerData(BinaryFile& file)
{
    // Spielerdaten
    for(unsigned char i = 0; i < player_count; ++i)
    {
        file.WriteUnsignedInt(players[i].ps);

        if(players[i].ps != PS_LOCKED)
        {
            file.WriteShortString(players[i].name);
            file.WriteUnsignedChar(players[i].nation);
            file.WriteUnsignedChar(players[i].color);
            file.WriteUnsignedChar(players[i].team);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void SavedFile::ReadPlayerData(BinaryFile& file)
{
    // ggf. wieder löschen
    delete[] players;

    players = new SavedFile::Player[player_count];
    for(unsigned char i = 0; i < player_count; ++i)
    {
        players[i].ps = file.ReadUnsignedInt();

        if(players[i].ps != PS_LOCKED)
        {
            file.ReadShortString(players[i].name);
            players[i].nation = Nation(file.ReadUnsignedChar());
            players[i].color = file.ReadUnsignedChar();
            players[i].team = file.ReadUnsignedChar();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  schreibt die GlobalGameSettings in die Datei.
 *
 *  @author OLiver
 */
void SavedFile::WriteGGS(BinaryFile& file)
{
    Serializer ser;
    ggs.Serialize(&ser);

    file.WriteUnsignedInt(ser.GetLength());
    file.WriteRawData(ser.GetData(), ser.GetLength());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liest die GlobalGameSettings aus der Datei.
 *
 *  @author OLiver
 */
void SavedFile::ReadGGS(BinaryFile& file)
{
    unsigned length = file.ReadUnsignedInt();
    unsigned char* buffer = new unsigned char[length];

    file.ReadRawData(buffer, length);
    Serializer ser(buffer, length);

    ggs.Deserialize(&ser);

    delete[] buffer;
}

