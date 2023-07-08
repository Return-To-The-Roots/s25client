// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaignDataLoader.h"
#include "CheckedLuaTable.h"
#include "RttrConfig.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "gameData/CampaignDescription.h"
#include "s25util/Log.h"
#include <kaguya/kaguya.hpp>
#include <boost/filesystem.hpp>
#include <stdexcept>

namespace bfs = boost::filesystem;

CampaignDataLoader::CampaignDataLoader(CampaignDescription& campaignDesc, const boost::filesystem::path& basePath)
    : campaignDesc_(campaignDesc), basePath_(basePath.lexically_normal().make_preferred())
{
    Register(lua);
    lua["rttr"] = this;
}

CampaignDataLoader::~CampaignDataLoader() = default;

bool CampaignDataLoader::Load()
{
    auto curFile_ = basePath_ / "campaign.lua";
    try
    {
        if(!loadScript(curFile_))
            return false;
    } catch(std::exception& e)
    {
        LOG.write("Failed to load campaign data!\nReason: %1%\nCurrent file being processed: %2%\n") % e.what()
          % curFile_;
        return false;
    }
    return true;
}

void CampaignDataLoader::Register(kaguya::State& state)
{
    state["RTTRCampaignData"].setClass(kaguya::UserdataMetatable<CampaignDataLoader, LuaInterfaceBase>().addFunction(
      "AddCampaign", &CampaignDataLoader::AddCampaign));
}

void CampaignDataLoader::AddCampaign(const kaguya::LuaTable& data)
{
    CampaignDescription(data, campaignDesc_);
}
