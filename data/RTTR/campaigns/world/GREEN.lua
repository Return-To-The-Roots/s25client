------------------------------------------------------------------------------
-- LUA-Script for GREEN.WLD (mission of the original "Continent Campaign")  --
--                                                                          --
-- Author: Michael Ott (v1.0)                                                    --
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
    rttr:Log("-----------------------\n GREEN.lua loaded... \n-----------------------\n")
    rttr:ResetAddons()
    rttr:SetAddon(ADDON_FRONTIER_DISTANCE_REACHABLE, true)
    rttr:SetGameSettings({
        ["fow"] = EXP_CLASSIC,
        ["teamView"] = false,
        ["lockedTeams"] = false,
        ["objective"] = GO_TOTALDOMINATION
    })

    rttr:GetPlayer(0):SetNation(NAT_ROMANS)     -- nation
    rttr:GetPlayer(0):SetColor(0)               -- blue

    rttr:GetPlayer(1):SetAI(3)                  -- hard AI
    rttr:GetPlayer(1):SetNation(NAT_VIKINGS)    -- nation
    rttr:GetPlayer(1):SetColor(1)               -- yellow
    rttr:GetPlayer(1):SetName('Olof')           -- Enemy Name
    rttr:GetPlayer(1):SetTeam(TM_TEAM1)
end

function getAllowedChanges()
    return {
        ["addonsAll"]   = false,
        ["ownNation"]   = false,
        ["ownColor"]    = false,
        ["ownTeam"]     = false,
        ["aiNation"]    = false, 
        ["aiColor"]     = false,
        ["aiTeam"]      = false
    }
end
