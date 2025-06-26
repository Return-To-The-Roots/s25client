// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Savegame.h"
#include "gameTypes/CompressedData.h"
#include "s25util/BinaryFile.h"
#include <boost/filesystem/operations.hpp>
#include <boost/nowide/fstream.hpp>
#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

std::string Savegame::GetSignature() const
{
    return "RTTRSV";
}

uint8_t Savegame::GetLatestMinorVersion() const
{
    // 4.1: Portraits support
    return 1;
}

uint8_t Savegame::GetLatestMajorVersion() const
{
    // Note: If you increase the version, reset currentGameDataVersion in SerializedGameData.cpp (see note there)
    // Note2: Also remove the workaround for the team in BasePlayerInfo & CompressedFlag here
    // Search for "TODO(Savegame)" when increasing this (breaking Savegame compatibility)
    return 4; // SaveGameVersion -- Updater signature, do NOT remove
}

//////////////////////////////////////////////////////////////////////////

Savegame::Savegame() : start_gf(0) {}

Savegame::~Savegame() = default;

bool Savegame::Save(const boost::filesystem::path& filepath, const std::string& mapName)
{
    BinaryFile file;

    return file.Open(filepath, OpenFileMode::Write) && Save(file, mapName);
}

bool Savegame::Save(BinaryFile& file, const std::string& mapName)
{
    WriteAllHeaderData(file, mapName);
    WritePlayerData(file);
    WriteGGS(file);
    WriteGameData(file);
#if __EMSCRIPTEN__
    EM_ASM(Module.requireSync?.());
#endif
    return true;
}

bool Savegame::Load(const boost::filesystem::path& filePath, const SaveGameDataToLoad what)
{
    BinaryFile file;

    return file.Open(filePath, OpenFileMode::Read) && Load(file, what);
}

bool Savegame::Load(BinaryFile& file, const SaveGameDataToLoad what)
{
    try
    {
        ClearPlayers();
        sgd.Clear();
        if(!ReadAllHeaderData(file))
            return false;

        if(what == SaveGameDataToLoad::Header)
            return true;

        ReadPlayerData(file);
        ReadGGS(file);

        if(what == SaveGameDataToLoad::HeaderAndSettings)
            return true;

        ReadGameData(file);
    } catch(const std::runtime_error& e)
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
    file.WriteUnsignedInt(1); // Compressed flag for compatibility
    std::vector<char> data(sgd.GetData(), sgd.GetData() + sgd.GetLength());
    const unsigned uncompressedLength = data.size();
    data = CompressedData::compress(data);
    file.WriteUnsignedInt(uncompressedLength);
    file.WriteUnsignedInt(data.size());
    file.WriteRawData(data.data(), data.size());
}

bool Savegame::ReadGameData(BinaryFile& file)
{
    std::vector<char> data;
    const auto compressedFlagOrSize = file.ReadUnsignedInt();
    if(compressedFlagOrSize == 1u)
    {
        const auto uncompressedLength = file.ReadUnsignedInt();
        const auto compressedLength = file.ReadUnsignedInt();
        data.resize(compressedLength);
        file.ReadRawData(data.data(), data.size());
        data = CompressedData::decompress(data, uncompressedLength);
#ifndef NDEBUG
        // In debug builds write uncompressed game data to temporary file
        const auto gameDataPath = boost::filesystem::temp_directory_path() / "rttrGameData.raw";
        boost::nowide::ofstream f(gameDataPath, std::ios::binary);
        f.write(data.data(), data.size());
#endif
    } else
    { // Old savegames have a size here which is always bigger than 1
        RTTR_Assert(compressedFlagOrSize > 1u);
        data.resize(compressedFlagOrSize);
        file.ReadRawData(data.data(), data.size());
    }

    sgd.Clear();
    sgd.PushRawData(data.data(), data.size());
    return true;
}
