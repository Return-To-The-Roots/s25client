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
    const std::string rev = rttr::version::getRevision();
    std::copy(rev.begin(), rev.begin() + revision.size(), revision.begin());
}

SavedFile::~SavedFile() = default;

uint8_t SavedFile::getMinorVersion() const
{
    return minorVersion_.value();
}

uint8_t SavedFile::getMajorVersion() const
{
    return majorVersion_.value();
}

void SavedFile::writeFileHeader(BinaryFile& file) const
{
    // Signature
    const std::string signature = getSignature();
    file.WriteRawData(signature.c_str(), signature.length());
    // Format version
    file.WriteUnsignedChar(getLatestMajorVersion());
    file.WriteUnsignedChar(getLatestMinorVersion());
}

void SavedFile::writeExtHeader(BinaryFile& file, const std::string& mapName)
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

bool SavedFile::readFileHeader(BinaryFile& file)
{
    lastErrorMsg.clear();

    const std::string signature = getSignature();
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
        if(majorVersion_ != getLatestMajorVersion() || minorVersion_ > getLatestMinorVersion())
        {
            boost::format fmt =
              boost::format((majorVersion_ < getLatestMajorVersion()) ?
                              _("File has an old version and cannot be used (version: %1%.%2%, expected: %3%.%4%)!") :
                              _("File was created with more recent program and cannot be used (version: %1%.%2%, "
                                "expected: %3%.%4%)!"));
            lastErrorMsg =
              (fmt % majorVersion_.value() % minorVersion_.value() % getLatestMajorVersion() % getLatestMinorVersion())
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

bool SavedFile::readExtHeader(BinaryFile& file)
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

void SavedFile::writeAllHeaderData(BinaryFile& file, const std::string& mapName)
{
    // Versionszeug schreiben
    writeFileHeader(file);
    writeExtHeader(file, mapName);
}

bool SavedFile::readAllHeaderData(BinaryFile& file)
{
    if(!readFileHeader(file))
        return false;
    if(!readExtHeader(file))
        return false;
    return true;
}

void SavedFile::writePlayerData(BinaryFile& file)
{
    Serializer ser;
    ser.PushUnsignedChar(players.size());
    for(const auto& player : players)
        player.serialize(ser, true);

    ser.WriteToFile(file);
}

void SavedFile::readPlayerData(BinaryFile& file)
{
    clearPlayers();
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
        BasePlayerInfo player(ser, getMinorVersion() >= 1 ? 1 : 0, true);
        // Temporary workaround: The random team was stored in the file but should not anymore, see PR #1331
        if(player.team > Team::Team4)
            player.team = Team(rttr::enumCast(player.team) - 3); // Was random team 2-4
        else if(player.team == Team::Random)
            player.team = Team::Team1; // Was random team 1
        addPlayer(player);
    }
}

/**
 *  schreibt die GlobalGameSettings in die Datei.
 */
void SavedFile::writeGGS(BinaryFile& file) const
{
    Serializer ser;
    ggs.serialize(ser);
    ser.WriteToFile(file);
}

/**
 *  liest die GlobalGameSettings aus der Datei.
 */
void SavedFile::readGGS(BinaryFile& file)
{
    Serializer ser;
    ser.ReadFromFile(file);
    ggs.deserialize(ser);
}

const BasePlayerInfo& SavedFile::getPlayer(unsigned idx) const
{
    return players[idx];
}

unsigned SavedFile::getNumPlayers()
{
    return players.size();
}

void SavedFile::addPlayer(const BasePlayerInfo& player)
{
    players.push_back(player);
    if(player.isUsed())
        playerNames_.push_back(player.name);
}

void SavedFile::clearPlayers()
{
    players.clear();
    playerNames_.clear();
}

std::string SavedFile::getRevision() const
{
    return std::string(revision.begin(), revision.end());
}
