// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Savegame.h"
#include "gameTypes/CompressedData.h"
#include "s25util/BinaryFile.h"
#include <boost/filesystem/operations.hpp>
#include <boost/nowide/fstream.hpp>

std::string Savegame::getSignature() const
{
    return "RTTRSV";
}

uint8_t Savegame::getLatestMinorVersion() const
{
    // 4.1: Portraits support
    return 1;
}

uint8_t Savegame::getLatestMajorVersion() const
{
    // Note: If you increase the version, reset currentGameDataVersion in SerializedGameData.cpp (see note there)
    // Note2: Also remove the workaround for the team in BasePlayerInfo & CompressedFlag here
    // Search for "TODO(Savegame)" when increasing this (breaking Savegame compatibility)
    return 4; // SaveGameVersion -- Updater signature, do NOT remove
}

//////////////////////////////////////////////////////////////////////////

Savegame::Savegame() : start_gf(0) {}

Savegame::~Savegame() = default;

bool Savegame::save(const boost::filesystem::path& filepath, const std::string& mapName)
{
    BinaryFile file;

    return file.Open(filepath, OpenFileMode::Write) && save(file, mapName);
}

bool Savegame::save(BinaryFile& file, const std::string& mapName)
{
    writeAllHeaderData(file, mapName);
    writePlayerData(file);
    writeGGS(file);
    writeGameData(file);

    return true;
}

bool Savegame::load(const boost::filesystem::path& filePath, const SaveGameDataToLoad what)
{
    BinaryFile file;

    return file.Open(filePath, OpenFileMode::Read) && load(file, what);
}

bool Savegame::load(BinaryFile& file, const SaveGameDataToLoad what)
{
    try
    {
        clearPlayers();
        sgd.Clear();
        if(!readAllHeaderData(file))
            return false;

        if(what == SaveGameDataToLoad::Header)
            return true;

        readPlayerData(file);
        readGGS(file);

        if(what == SaveGameDataToLoad::HeaderAndSettings)
            return true;

        readGameData(file);
    } catch(const std::runtime_error& e)
    {
        lastErrorMsg = e.what();
        return false;
    }

    return true;
}

void Savegame::writeExtHeader(BinaryFile& file, const std::string& mapName)
{
    SavedFile::writeExtHeader(file, mapName);
    file.WriteUnsignedInt(start_gf);
}

bool Savegame::readExtHeader(BinaryFile& file)
{
    if(!SavedFile::readExtHeader(file))
        return false;
    start_gf = file.ReadUnsignedInt();
    return true;
}

void Savegame::writeGameData(BinaryFile& file)
{
    file.WriteUnsignedInt(1); // Compressed flag for compatibility
    std::vector<char> data(sgd.GetData(), sgd.GetData() + sgd.GetLength());
    const unsigned uncompressedLength = data.size();
    data = CompressedData::compress(data);
    file.WriteUnsignedInt(uncompressedLength);
    file.WriteUnsignedInt(data.size());
    file.WriteRawData(data.data(), data.size());
}

bool Savegame::readGameData(BinaryFile& file)
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
