// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SavedFile.h"
#include "SerializedGameData.h"
#include <boost/filesystem/path.hpp>

class BinaryFile;

enum class SaveGameDataToLoad
{
    Header,
    HeaderAndSettings,
    All
};

class Savegame : public SavedFile
{
public:
    Savegame();
    ~Savegame() override;

    std::string GetSignature() const override;
    uint8_t GetLatestMinorVersion() const override;
    uint8_t GetLatestMajorVersion() const override;

    /// Schreibst Savegame oder Teile davon
    bool Save(const boost::filesystem::path& filepath, const std::string& mapName);
    bool Save(BinaryFile& file, const std::string& mapName);

    /// Lädt Savegame oder Teile davon
    bool Load(const boost::filesystem::path& filePath, SaveGameDataToLoad what);
    bool Load(BinaryFile& file, SaveGameDataToLoad what);

    void WriteExtHeader(BinaryFile& file, const std::string& mapName) override;
    bool ReadExtHeader(BinaryFile& file) override;

    /// Start-GF
    unsigned start_gf;
    /// Serialisierte Spieldaten
    SerializedGameData sgd;

protected:
    void WriteGameData(BinaryFile& file);
    bool ReadGameData(BinaryFile& file);
};
