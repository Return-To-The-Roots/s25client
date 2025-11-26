------------------------------------------------------------------------------
-- LUA-Script for AFRICA.WLD (mission of the original "Continent Campaign") --
--                                                                          --
-- Author: Michael Ott (v1.0)                                               --
------------------------------------------------------------------------------


-------------------------------- TODO -----------------------------------------
-- EnableNextMissions()
-- Set Portraits
-- Set AI Agression Level
-------------------------------------------------------------------------------


-------------------------------- Lua Version used -----------------------------
function getRequiredLuaVersion()
    return 1
end

function isMapPreviewEnabled()
    return false;
end

-------------------------------- general settings -----------------------------
function onSettingsReady()
    rttr:Log("-----------------------\n AFRICA.lua loaded... \n-----------------------\n")
    rttr:ResetAddons()
    rttr:SetAddon(ADDON_FRONTIER_DISTANCE_REACHABLE, true)
    rttr:SetGameSettings({
        ["fow"] = EXP_CLASSIC,
        ["teamView"] = false,
        ["lockedTeams"] = false,
        ["objective"] = GO_TOTALDOMINATION
    })

    rttr:GetPlayer(0):SetNation(NAT_ROMANS)
    rttr:GetPlayer(0):SetColor(0)
    rttr:GetPlayer(0):SetPortrait(0)

    rttr:GetPlayer(1):SetAI(3)
    rttr:GetPlayer(1):SetNation(NAT_AFRICANS)
    rttr:GetPlayer(1):SetColor(1)
    rttr:GetPlayer(1):SetName('Shaka')
    rttr:GetPlayer(1):SetPortrait(9)
    rttr:GetPlayer(1):SetTeam(TM_TEAM1)

    rttr:GetPlayer(2):SetAI(3)
    rttr:GetPlayer(2):SetNation(NAT_AFRICANS)
    rttr:GetPlayer(2):SetColor(2)
    rttr:GetPlayer(2):SetName('Todo')
    rttr:GetPlayer(2):SetPortrait(10)
    rttr:GetPlayer(2):SetTeam(TM_TEAM1)

    rttr:GetPlayer(3):SetAI(3)
    rttr:GetPlayer(3):SetNation(NAT_AFRICANS)
    rttr:GetPlayer(3):SetColor(3)
    rttr:GetPlayer(3):SetName('Mnga Tscha')
    rttr:GetPlayer(3):SetPortrait(11)
    rttr:GetPlayer(3):SetTeam(TM_TEAM1)
end

function getAllowedChanges()
    return {
        ["addonsAll"]   = false,
        ["ownNation"]   = false,
        ["ownColor"]    = false,
        ["ownTeam"]     = false,
        ["ownPortrait"] = false,
        ["aiNation"]    = false,
        ["aiColor"]     = false,
        ["aiTeam"]      = false,
        ["aiPortrait"]  = false
    }
end
