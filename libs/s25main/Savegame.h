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

    std::string getSignature() const override;
    uint8_t getLatestMinorVersion() const override;
    uint8_t getLatestMajorVersion() const override;

    /// Schreibst Savegame oder Teile davon
    bool save(const boost::filesystem::path& filepath, const std::string& mapName);
    bool save(BinaryFile& file, const std::string& mapName);

    /// Lädt Savegame oder Teile davon
    bool load(const boost::filesystem::path& filePath, SaveGameDataToLoad what);
    bool load(BinaryFile& file, SaveGameDataToLoad what);

    void writeExtHeader(BinaryFile& file, const std::string& mapName) override;
    bool readExtHeader(BinaryFile& file) override;

    /// Start-GF
    unsigned start_gf;
    /// Serialisierte Spieldaten
    SerializedGameData sgd;

protected:
    void writeGameData(BinaryFile& file);
    bool readGameData(BinaryFile& file);
};
