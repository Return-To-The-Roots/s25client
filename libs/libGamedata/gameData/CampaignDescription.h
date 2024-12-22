// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "SelectionMapInputData.h"
#include <boost/filesystem/path.hpp>
#include <optional>
#include <string>
#include <vector>

namespace kaguya {
class LuaRef;
} // namespace kaguya

struct CampaignDescription
{
    std::string uid;
    std::string version;
    std::string author;
    std::string name;
    std::string shortDescription;
    std::string longDescription;
    std::optional<std::string> image;
    unsigned maxHumanPlayers = 1;
    std::string difficulty;
    std::string defaultChaptersEnabled;
    std::optional<SelectionMapInputData> selectionMapData;

    CampaignDescription() = default;
    explicit CampaignDescription(const boost::filesystem::path& campaignPath, const kaguya::LuaRef& table);
    size_t getNumMaps() const { return mapNames_.size(); }
    const std::string& getMapName(const size_t idx) const { return mapNames_.at(idx); }
    boost::filesystem::path getLuaFilePath(size_t idx) const;
    boost::filesystem::path getMapFilePath(size_t idx) const;

private:
    boost::filesystem::path mapFolder_, luaFolder_;
    std::vector<std::string> mapNames_;
};
