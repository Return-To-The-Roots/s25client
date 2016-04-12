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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include <build_version.h>
#include "BinaryFile.h"
#include "GameSavedFile.h"
#include "GamePlayerInfo.h"
#include "helpers/Deleter.h"
#include "libutil/src/Serializer.h"
#include "libutil/src/Log.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <cstring>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

SavedFile::SavedFile() : save_time(0)
{
}

SavedFile::~SavedFile()
{}

void SavedFile::WriteVersion(BinaryFile& file, unsigned int signature_length, const char* signature, unsigned short version)
{
    // Signatur schreiben
    file.WriteRawData(signature, signature_length);

    // Version vom Programm reinschreiben (mal 0 mit reinschreiben, damits ne runde 8 ergibt!)
    file.WriteRawData(GetWindowRevision(), 8);

    // Version des Save-Formats
    file.WriteUnsignedShort(version);
}

bool SavedFile::ValidateFile(BinaryFile& file, unsigned int signature_length, const char* signature, unsigned short version)
{
    char read_signature[32];

    file.ReadRawData(read_signature, signature_length);

    // Signatur überprüfen
    if(memcmp(read_signature, signature, signature_length) != 0)
    {
        // unterscheiden sich! --> raus
        LOG.lprintf("Error: File is not in a valid format! File path: %s\n", file.getFilePath().c_str());
        return false;
    }

    // Programmversion überspringen
    file.Seek(8, SEEK_CUR);

    // Version überprüfen
    unsigned short read_version = file.ReadUnsignedShort();
    if(read_version != version)
    {
        // anderes Dateiformat --> raus
        LOG.lprintf("Warning: File has an old version and cannot be used (version: %u; expected: %u, file path: %s)!\n", read_version, version, file.getFilePath().c_str());
        return false;
    }

    return true;
}

void SavedFile::WritePlayerData(BinaryFile& file)
{
    // Spielerdaten
    for(std::vector<Player>::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        file.WriteUnsignedInt(it->ps);

        if(it->ps != PS_LOCKED)
        {
            file.WriteShortString(it->name);
            file.WriteUnsignedChar(it->nation);
            file.WriteUnsignedInt(it->color);
            file.WriteUnsignedChar(it->team);
        }
    }
}

void SavedFile::ReadPlayerData(BinaryFile& file)
{
    for(std::vector<Player>::iterator it = players.begin(); it != players.end(); ++it)
    {
        it->ps = file.ReadUnsignedInt();

        if(it->ps != PS_LOCKED)
        {
            it->name = file.ReadShortString();
            it->nation = Nation(file.ReadUnsignedChar());
            it->color = file.ReadUnsignedInt();
            it->team = file.ReadUnsignedChar();
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
    ggs.Serialize(ser);

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
    boost::interprocess::unique_ptr<unsigned char, Deleter<unsigned char[]> > buffer(new unsigned char[length]);

    file.ReadRawData(buffer.get(), length);
    Serializer ser(buffer.get(), length);

    ggs.Deserialize(ser);
}
