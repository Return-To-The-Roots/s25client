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

#include "defines.h" // IWYU pragma: keep
#include "Savegame.h"
#include "BasePlayerInfo.h"
#include "libendian/ConvertEndianess.h"
#include "libutil/BinaryFile.h"

std::string Savegame::GetSignature() const
{
    /// Kleine Signatur am Anfang "RTTRSAVE", die ein gültiges S25 RTTR Savegame kennzeichnet
    return "RTTRSAVE";
}

uint16_t Savegame::GetVersion() const
{
    return 36; // SaveGameVersion -- Updater signature, do NOT remove
}

//////////////////////////////////////////////////////////////////////////

Savegame::Savegame() : SavedFile(), start_gf(0) {}

Savegame::~Savegame() {}

bool Savegame::Save(const std::string& filename)
{
    BinaryFile file;

    return file.Open(filename, OFM_WRITE) && Save(file);
}

bool Savegame::Save(BinaryFile& file)
{
    // Versionszeug schreiben
    WriteFileHeader(file);

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

    return file.Open(filePath, OFM_READ) && Load(file, load_players, load_sgd);
}

bool Savegame::Load(BinaryFile& file, const bool load_players, const bool load_sgd)
{
    // Signatur und Version einlesen
    if(!ReadFileHeader(file))
        return false;

    // Zeitstempel
    file.ReadRawData(&save_time, sizeof(save_time));
    save_time = libendian::ConvertEndianess<false>::toNative(save_time);

    // Map-Name
    mapName = file.ReadShortString();

    if(!load_players)
    {
        players.clear();
        start_gf = 0xFFFFFFFF;
        return true;
    }
    
    ReadPlayerData(file);

    ReadGGS(file);

    // Start-GF
    start_gf = file.ReadUnsignedInt();

    if(load_sgd)
        sgd.ReadFromFile(file); // Serialisiertes Spielzeug lesen

    return true;
}
