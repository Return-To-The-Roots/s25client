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
#include "GameSavegame.h"
#include "BinaryFile.h"
#include "Log.h"
#include "libendian/src/ConvertEndianess.h"

/// Kleine Signatur am Anfang "RTTRSAVE", die ein gültiges S25 RTTR Savegame kennzeichnet
const char Savegame::SAVE_SIGNATURE[8] = {'R', 'T', 'T', 'R', 'S', 'A', 'V', 'E'};
/// Version des Savegame-Formates
const unsigned short Savegame::SAVE_VERSION = 36;

Savegame::Savegame() : SavedFile(), start_gf(0)
{
}

Savegame::~Savegame()
{
}

bool Savegame::Save(const std::string& filename)
{
    BinaryFile file;

    if(!file.Open(filename, OFM_WRITE))
        return false;

    bool ret = Save(file);

    file.Close();

    return ret;
}

bool Savegame::Save(BinaryFile& file)
{
    // Versionszeug schreiben
    WriteVersion(file, 8, SAVE_SIGNATURE, SAVE_VERSION);

    // Timestamp der Aufzeichnung
    unser_time_t tmpTime = libendian::ConvertEndianess<false>::fromNative(save_time);
    file.WriteRawData(&tmpTime, sizeof(tmpTime));

    // Mapname
    file.WriteShortString(mapName);

    WritePlayerData(file);
    WriteGGS(file);

    // Start-GF
    file.WriteUnsignedInt(start_gf);

    // Serialisiertes Spielzeug reinschreiben
    sgd.WriteToFile(file);

    return true;
}

bool Savegame::Load(const std::string& filePath, const bool load_players, const bool load_sgd)
{
    BinaryFile file;

    if(!file.Open(filePath, OFM_READ))
        return false;

    bool ret = Load(file, load_players, load_sgd);

    file.Close();

    return ret;
}

bool Savegame::Load(BinaryFile& file, const bool load_players, const bool load_sgd)
{
    // Signatur und Version einlesen
    if(!ValidateFile(file, sizeof(SAVE_SIGNATURE), SAVE_SIGNATURE, SAVE_VERSION))
        return false;

    // Zeitstempel
    file.ReadRawData(&save_time, sizeof(save_time));
    save_time = libendian::ConvertEndianess<false>::toNative(save_time);

    // Map-Name
    mapName = file.ReadShortString();

    ReadPlayerData(file);
    ReadGGS(file);

    // Start-GF
    start_gf = file.ReadUnsignedInt();

    if(load_sgd)
        sgd.ReadFromFile(file); // Serialisiertes Spielzeug lesen

    return true;
}
