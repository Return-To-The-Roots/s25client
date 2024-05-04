// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaignDataLoader.h"
#include "CheckedLuaTable.h"
#include "RttrConfig.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "mygettext/mygettext.h"
#include "gameData/CampaignDescription.h"
#include "s25util/Log.h"
#include <kaguya/kaguya.hpp>
#include <boost/filesystem.hpp>
#include <stdexcept>

unsigned CampaignDataLoader::GetVersion()
{
    return 2;
}

CampaignDataLoader::CampaignDataLoader(CampaignDescription& campaignDesc, const boost::filesystem::path& basePath)
    : campaignDesc_(campaignDesc), basePath_(basePath.lexically_normal().make_preferred())
{
    Register(lua);
    lua["rttr"] = this;
}

CampaignDataLoader::~CampaignDataLoader() = default;

bool CampaignDataLoader::CheckScriptVersion()
{
    kaguya::LuaRef func = lua["getRequiredLuaVersion"];
    if(func.type() == LUA_TFUNCTION)
    {
        const auto scriptVersion = func.call<unsigned>();
        if(scriptVersion == GetVersion())
            return true;
        else
        {
            LOG.write(_("Wrong lua script version: %1%. Current version: %2%.\n")) % scriptVersion % GetVersion();
            return false;
        }
    } else
    {
        LOG.write(_("Lua script did not provide the function getRequiredLuaVersion()! It is probably outdated.\n"));
        return false;
    }
}

bool CampaignDataLoader::Load()
{
    auto curFile_ = basePath_ / "campaign.lua";
    try
    {
        if(!loadScript(curFile_))
            return false;

        if(!CheckScriptVersion())
            throw std::runtime_error("Wrong lua script version.");

        kaguya::LuaRef entry = lua["campaign"];
        if(entry.type() != LUA_TTABLE)
            throw std::runtime_error("Campaign table variable missing.");

        campaignDesc_ = CampaignDescription(entry);
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
    state["RTTRCampaignData"].setClass(kaguya::UserdataMetatable<CampaignDataLoader, LuaInterfaceBase>());
}
