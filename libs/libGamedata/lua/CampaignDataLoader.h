// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "LuaInterfaceBase.h"
#include <boost/filesystem/path.hpp>

namespace kaguya {
class State;
class LuaTable;
} // namespace kaguya
struct CampaignDescription;

class CampaignDataLoader : public LuaInterfaceBase
{
public:
    CampaignDataLoader(CampaignDescription& campaignDesc, const boost::filesystem::path& basePath);
    ~CampaignDataLoader() override;

    bool Load();

    static void Register(kaguya::State& state);

private:
    void AddCampaign(const kaguya::LuaTable& data);

    CampaignDescription& campaignDesc_;
    boost::filesystem::path basePath_;
};
