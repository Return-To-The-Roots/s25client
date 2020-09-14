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
    uint16_t GetVersion() const override;

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
