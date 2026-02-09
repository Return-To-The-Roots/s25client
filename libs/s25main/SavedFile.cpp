// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SavedFile.h"
#include "BasePlayerInfo.h"
#include "RTTR_Version.h"
#include "enum_cast.hpp"
#include "helpers/format.hpp"
#include "gameData/MaxPlayers.h"
#include "libendian/ConvertEndianess.h"
#include "s25util/BinaryFile.h"
#include "s25util/Serializer.h"
#include <boost/format.hpp>
#include <algorithm>
#include <mygettext/mygettext.h>
#include <stdexcept>

SavedFile::SavedFile() : saveTime_(0)
{
    const std::string rev = rttr::version::GetRevision();
    std::copy(rev.begin(), rev.begin() + revision.size(), revision.begin());
}

SavedFile::~SavedFile() = default;

uint8_t SavedFile::GetMinorVersion() const
{
    return minorVersion_.value();
}

uint8_t SavedFile::GetMajorVersion() const
{
    return majorVersion_.value();
}

void SavedFile::WriteFileHeader(BinaryFile& file) const
{
    // Signature
    const std::string signature = GetSignature();
    file.WriteRawData(signature.c_str(), signature.length());
    // Format version
    file.WriteUnsignedChar(GetLatestMajorVersion());
    file.WriteUnsignedChar(GetLatestMinorVersion());
}

void SavedFile::WriteExtHeader(BinaryFile& file, const std::string& mapName)
{
    // Store data in struct
    saveTime_ = s25util::Time::CurrentTime();
    mapName_ = mapName;

    // Program version
    file.WriteRawData(revision.data(), revision.size());
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
        file.ReadRawData(read_signature.data(), signature.size());

        // Signatur überprüfen
        if(!std::equal(signature.begin(), signature.end(), read_signature.begin()))
        {
            lastErrorMsg = _("File is not in a valid format!");
            return false;
        }

        // Version überprüfen
        majorVersion_ = file.ReadUnsignedChar();
        minorVersion_ = file.ReadUnsignedChar();
        if(majorVersion_ != GetLatestMajorVersion() || minorVersion_ > GetLatestMinorVersion())
        {
            boost::format fmt =
              boost::format((majorVersion_ < GetLatestMajorVersion()) ?
                              _("File has an old version and cannot be used (version: %1%.%2%, expected: %3%.%4%)!") :
                              _("File was created with more recent program and cannot be used (version: %1%.%2%, "
                                "expected: %3%.%4%)!"));
            lastErrorMsg =
              (fmt % majorVersion_.value() % minorVersion_.value() % GetLatestMajorVersion() % GetLatestMinorVersion())
                .str();
            return false;
        }
    } catch(const std::runtime_error& e)
    {
        lastErrorMsg = e.what();
        return false;
    }

    return true;
}

bool SavedFile::ReadExtHeader(BinaryFile& file)
{
    file.ReadRawData(revision.data(), revision.size());
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
    if(playerCt > MAX_PLAYERS)
    {
        throw std::length_error(
          helpers::format(_("Number of players (%1%) exceeds maximum allowed of %2%!"), playerCt, MAX_PLAYERS));
    }
    players.reserve(playerCt);
    for(unsigned i = 0; i < playerCt; i++)
    {
        // TODO(Replay) TODO(Savegame) minor versions will change their meaning when major is bumped
        BasePlayerInfo player(ser, GetMinorVersion() >= 1 ? 1 : 0, true);
        // Temporary workaround: The random team was stored in the file but should not anymore, see PR #1331
        if(player.team > Team::Team4)
            player.team = Team(rttr::enum_cast(player.team) - 3); // Was random team 2-4
        else if(player.team == Team::Random)
            player.team = Team::Team1; // Was random team 1
        AddPlayer(player);
    }
}

/**
 *  schreibt die GlobalGameSettings in die Datei.
 */
void SavedFile::WriteGGS(BinaryFile& file) const
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
