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
#include "SavedFile.h"
#include "BasePlayerInfo.h"
#include "RTTR_Version.h"
#include "libutil/BinaryFile.h"
#include "libutil/Serializer.h"
#include <boost/format.hpp>
#include <algorithm>
#include <stdexcept>

SavedFile::SavedFile() : save_time(0)
{
    const std::string rev = RTTR_Version::GetRevision();
    std::copy(rev.begin(), rev.begin() + revision.size(), revision.begin());
}

SavedFile::~SavedFile() {}

void SavedFile::WriteFileHeader(BinaryFile& file)
{
    // Signatur schreiben
    const std::string signature = GetSignature();
    file.WriteRawData(signature.c_str(), signature.length());

    // Version vom Programm reinschreiben
    file.WriteRawData(&revision[0], revision.size());

    // Version des Save-Formats
    file.WriteUnsignedShort(GetVersion());
}

bool SavedFile::ReadFileHeader(BinaryFile& file)
{
    lastErrorMsg = "";

    const std::string signature = GetSignature();
    boost::array<char, 32> read_signature;
    if(signature.size() > read_signature.size())
        throw std::range_error("Program signature is to long!");
    file.ReadRawData(&read_signature[0], signature.size());

    // Signatur überprüfen
    if(!std::equal(signature.begin(), signature.end(), read_signature.begin()))
    {
        lastErrorMsg = _("File is not in a valid format!");
        return false;
    }

    file.ReadRawData(&revision[0], revision.size());

    // Version überprüfen
    uint16_t read_version = file.ReadUnsignedShort();
    if(read_version != GetVersion())
    {
        boost::format fmt = boost::format(
          (read_version < GetVersion()) ? _("File has an old version and cannot be used (version: %1%, expected: %2%)!") :
                                          _("File was created with more recent program and cannot be used (version: %1%, expected: %2%)!"));
        lastErrorMsg = (fmt % read_version % GetVersion()).str();
        return false;
    }

    return true;
}

void SavedFile::WritePlayerData(BinaryFile& file)
{
    Serializer ser;
    ser.PushUnsignedChar(players.size());
    for(std::vector<BasePlayerInfo>::const_iterator it = players.begin(); it != players.end(); ++it)
        it->Serialize(ser, true);

    ser.WriteToFile(file);
}

void SavedFile::ReadPlayerData(BinaryFile& file)
{
    players.clear();
    Serializer ser;
    ser.ReadFromFile(file);
    const unsigned playerCt = ser.PopUnsignedChar();
    players.reserve(playerCt);
    for(unsigned i = 0; i < playerCt; i++)
        players.push_back(BasePlayerInfo(ser, true));
}

/**
 *  schreibt die GlobalGameSettings in die Datei.
 */
void SavedFile::WriteGGS(BinaryFile& file)
{
    Serializer ser;
    ggs.Serialize(ser);
    ser.WriteToFile(file);
}

/**
 *  liest die GlobalGameSettings aus der Datei.
 */
void SavedFile::ReadGGS(BinaryFile& file)
{
    Serializer ser;
    ser.ReadFromFile(file);
    ggs.Deserialize(ser);
}

const BasePlayerInfo& SavedFile::GetPlayer(unsigned idx) const
{
    return players[idx];
}

unsigned SavedFile::GetPlayerCount()
{
    return players.size();
}

void SavedFile::AddPlayer(const BasePlayerInfo& player)
{
    players.push_back(player);
}

void SavedFile::ClearPlayers()
{
    players.clear();
}

std::string SavedFile::GetLastErrorMsg() const
{
    return lastErrorMsg;
}

std::string SavedFile::GetRevision() const
{
    return std::string(revision.begin(), revision.end());
}
