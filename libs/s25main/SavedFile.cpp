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

#include "SavedFile.h"
#include "BasePlayerInfo.h"
#include "RTTR_Version.h"
#include "libendian/ConvertEndianess.h"
#include "s25util/BinaryFile.h"
#include "s25util/Serializer.h"
#include <boost/format.hpp>
#include <algorithm>
#include <mygettext/mygettext.h>
#include <stdexcept>

SavedFile::SavedFile() : saveTime_(0)
{
    const std::string rev = RTTR_Version::GetRevision();
    std::copy(rev.begin(), rev.begin() + revision.size(), revision.begin());
}

SavedFile::~SavedFile() = default;

void SavedFile::WriteFileHeader(BinaryFile& file)
{
    // Signature
    const std::string signature = GetSignature();
    file.WriteRawData(signature.c_str(), signature.length());
    // Format version
    file.WriteUnsignedShort(GetVersion());
}

void SavedFile::WriteExtHeader(BinaryFile& file, const std::string& mapName)
{
    // Store data in struct
    saveTime_ = s25util::Time::CurrentTime();
    mapName_ = mapName;

    // Program version
    file.WriteRawData(&revision[0], revision.size());
    s25util::time64_t tmpTime = libendian::ConvertEndianess<false>::fromNative(saveTime_);
    file.WriteRawData(&tmpTime, sizeof(tmpTime));
    file.WriteShortString(mapName);
    file.WriteUnsignedInt(playerNames_.size());
    for(const std::string& name : playerNames_)
        file.WriteShortString(name);
}

bool SavedFile::ReadFileHeader(BinaryFile& file)
{
    lastErrorMsg.clear();

    const std::string signature = GetSignature();
    std::array<char, 32> read_signature;
    if(signature.size() > read_signature.size())
        throw std::range_error("Program signature is to long!");
    try
    {
        file.ReadRawData(&read_signature[0], signature.size());

        // Signatur überprüfen
        if(!std::equal(signature.begin(), signature.end(), read_signature.begin()))
        {
            lastErrorMsg = _("File is not in a valid format!");
            return false;
        }

        // Version überprüfen
        uint16_t read_version = file.ReadUnsignedShort();
        if(read_version != GetVersion())
        {
            boost::format fmt =
              boost::format((read_version < GetVersion()) ?
                              _("File has an old version and cannot be used (version: %1%, expected: %2%)!") :
                              _("File was created with more recent program and cannot be used (version: %1%, expected: %2%)!"));
            lastErrorMsg = (fmt % read_version % GetVersion()).str();
            return false;
        }
    } catch(std::runtime_error& e)
    {
        lastErrorMsg = e.what();
        return false;
    }

    return true;
}

bool SavedFile::ReadExtHeader(BinaryFile& file)
{
    file.ReadRawData(&revision[0], revision.size());
    file.ReadRawData(&saveTime_, sizeof(saveTime_));
    saveTime_ = libendian::ConvertEndianess<false>::toNative(saveTime_);
    mapName_ = file.ReadShortString();
    playerNames_.resize(file.ReadUnsignedInt());
    for(std::string& name : playerNames_)
        name = file.ReadShortString();
    return true;
}

void SavedFile::WriteAllHeaderData(BinaryFile& file, const std::string& mapName)
{
    // Versionszeug schreiben
    WriteFileHeader(file);
    WriteExtHeader(file, mapName);
}

bool SavedFile::ReadAllHeaderData(BinaryFile& file)
{
    if(!ReadFileHeader(file))
        return false;
    if(!ReadExtHeader(file))
        return false;
    return true;
}

void SavedFile::WritePlayerData(BinaryFile& file)
{
    Serializer ser;
    ser.PushUnsignedChar(players.size());
    for(const auto& player : players)
        player.Serialize(ser, true);

    ser.WriteToFile(file);
}

void SavedFile::ReadPlayerData(BinaryFile& file)
{
    ClearPlayers();
    Serializer ser;
    ser.ReadFromFile(file);
    const unsigned playerCt = ser.PopUnsignedChar();
    players.reserve(playerCt);
    for(unsigned i = 0; i < playerCt; i++)
        AddPlayer(BasePlayerInfo(ser, true));
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

unsigned SavedFile::GetNumPlayers()
{
    return players.size();
}

void SavedFile::AddPlayer(const BasePlayerInfo& player)
{
    players.push_back(player);
    if(player.isUsed())
        playerNames_.push_back(player.name);
}

void SavedFile::ClearPlayers()
{
    players.clear();
    playerNames_.clear();
}

std::string SavedFile::GetRevision() const
{
    return std::string(revision.begin(), revision.end());
}
