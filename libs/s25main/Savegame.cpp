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

#include "Savegame.h"
#include "s25util/BinaryFile.h"

std::string Savegame::GetSignature() const
{
    return "RTTRSV";
}

uint16_t Savegame::GetVersion() const
{
    // Note: If you increase the version, reset currentGameDataVersion in SerializedGameData.cpp (see note there)
    return 4; // SaveGameVersion -- Updater signature, do NOT remove
}

//////////////////////////////////////////////////////////////////////////

Savegame::Savegame() : start_gf(0) {}

Savegame::~Savegame() = default;

bool Savegame::Save(const std::string& filename, const std::string& mapName)
{
    BinaryFile file;

    return file.Open(filename, OFM_WRITE) && Save(file, mapName);
}

bool Savegame::Save(BinaryFile& file, const std::string& mapName)
{
    WriteAllHeaderData(file, mapName);
    WritePlayerData(file);
    WriteGGS(file);
    WriteGameData(file);

    return true;
}

bool Savegame::Load(const std::string& filePath, bool loadSettings, bool loadGameData)
{
    BinaryFile file;

    return file.Open(filePath, OFM_READ) && Load(file, loadSettings, loadGameData);
}

bool Savegame::Load(BinaryFile& file, bool loadSettings, bool loadGameData)
{
    try
    {
        ClearPlayers();
        sgd.Clear();
        if(!ReadAllHeaderData(file))
            return false;

        if(!loadSettings)
            return true;

        ReadPlayerData(file);
        ReadGGS(file);

        if(loadGameData)
            ReadGameData(file);
    } catch(std::runtime_error& e)
    {
        lastErrorMsg = e.what();
        return false;
    }

    return true;
}

void Savegame::WriteExtHeader(BinaryFile& file, const std::string& mapName)
{
    SavedFile::WriteExtHeader(file, mapName);
    file.WriteUnsignedInt(start_gf);
}

bool Savegame::ReadExtHeader(BinaryFile& file)
{
    if(!SavedFile::ReadExtHeader(file))
        return false;
    start_gf = file.ReadUnsignedInt();
    return true;
}

void Savegame::WriteGameData(BinaryFile& file)
{
    sgd.WriteToFile(file);
}

bool Savegame::ReadGameData(BinaryFile& file)
{
    sgd.ReadFromFile(file);
    return true;
}
