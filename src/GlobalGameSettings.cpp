// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "GlobalGameSettings.h"

#include "Settings.h"

#include "addons/AddonLimitCatapults.h"
#include "addons/AddonInexhaustibleMines.h"
#include "addons/AddonRefundMaterials.h"
#include "addons/AddonExhaustibleWells.h"
#include "addons/AddonRefundOnEmergency.h"
#include "addons/AddonManualRoadEnlargement.h"
#include "addons/AddonCatapultGraphics.h"
#include "addons/AddonMetalworksBehaviorOnZero.h"

#include "addons/AddonDemolitionProhibition.h"
#include "addons/AddonCharburner.h"
#include "addons/AddonTrade.h"

#include "addons/AddonChangeGoldDeposits.h"
#include "addons/AddonMaxWaterwayLength.h"
#include "addons/AddonCustomBuildSequence.h"
#include "addons/AddonStatisticsVisibility.h"

#include "addons/AddonDefenderBehavior.h"
#include "addons/AddonAIDebugWindow.h"

#include "addons/AddonNoCoinsDefault.h"

#include "addons/AddonAdjustMilitaryStrength.h"

#include "addons/AddonToolOrdering.h"

#include "addons/AddonMilitaryAid.h"
#include "addons/AddonInexhaustibleGraniteMines.h"
#include "addons/AddonMaxRank.h"
#include "addons/AddonSeaAttack.h"
#include "addons/AddonInexhaustibleFish.h"

#include "addons/AddonShipSpeed.h"
#include "addons/AddonMoreAnimals.h"
#include "addons/AddonBurnDuration.h"
#include "addons/AddonNoAlliedPush.h"
#include "addons/AddonBattlefieldPromotion.h"
#include "addons/AddonHalfCostMilEquip.h"
#include "addons/AddonMilitaryControl.h"

#include "addons/AddonMilitaryHitpoints.h"

#include "Serializer.h"
#include "Log.h"
#include "gameData/MilitaryConsts.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author FloSoft
 */
GlobalGameSettings::GlobalGameSettings() : game_speed(GS_FAST), game_objective(GO_NONE), start_wares(SWR_NORMAL), lock_teams(false), exploration(EXP_FOGOFWAR), team_view(true), random_location(false)
{
    // register addons
    reset();
}

GlobalGameSettings::GlobalGameSettings(const GlobalGameSettings& ggs)
{
    Serializer ser;
    ggs.Serialize(ser);
    Deserialize(ser);
}

void GlobalGameSettings::operator=(const GlobalGameSettings& ggs)
{
    Serializer ser;
    ggs.Serialize(ser);
    Deserialize(ser);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author FloSoft
 */
GlobalGameSettings::~GlobalGameSettings()
{
    // clear memory and dont register addons again
    reset(false);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  clears the addon memory.
 *
 *  if @p recreate is @p true then the addons are re-registered
 *  and set to defaults
 *
 *  @author FloSoft
 */
void GlobalGameSettings::reset(bool recreate)
{
    for( std::vector<item>::iterator it = addons.begin(); it != addons.end(); ++it)
        delete it->addon;

    addons.clear();

    if(recreate)
    {
        registerAddon(new AddonLimitCatapults);
        registerAddon(new AddonInexhaustibleMines);
        registerAddon(new AddonRefundMaterials);
        registerAddon(new AddonExhaustibleWells);
        registerAddon(new AddonRefundOnEmergency);
        registerAddon(new AddonManualRoadEnlargement);
        registerAddon(new AddonCatapultGraphics);
        registerAddon(new AddonMetalworksBehaviorOnZero);

        registerAddon(new AddonDemolitionProhibition);
        registerAddon(new AddonCharburner);
        registerAddon(new AddonTrade);

        registerAddon(new AddonChangeGoldDeposits);
        registerAddon(new AddonMaxWaterwayLength);
        registerAddon(new AddonCustomBuildSequence);
        registerAddon(new AddonStatisticsVisibility);

        registerAddon(new AddonDefenderBehavior);
        registerAddon(new AddonAIDebugWindow);

        registerAddon(new AddonNoCoinsDefault);

        registerAddon(new AddonAdjustMilitaryStrength);

        registerAddon(new AddonToolOrdering);

        registerAddon(new AddonMilitaryAid);
        registerAddon(new AddonInexhaustibleGraniteMines);
        registerAddon(new AddonMaxRank);
        registerAddon(new AddonSeaAttack);
        registerAddon(new AddonInexhaustibleFish);

        registerAddon(new AddonShipSpeed);
		registerAddon(new AddonMoreAnimals);
		registerAddon(new AddonBurnDuration);
		registerAddon(new AddonNoAlliedPush);
		registerAddon(new AddonBattlefieldPromotion);
		registerAddon(new AddonHalfCostMilEquip);
		registerAddon(new AddonMilitaryControl);

		registerAddon(new AddonMilitaryHitpoints);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  loads the saved addon configuration from the SETTINGS.
 *
 *  @author FloSoft
 */
void GlobalGameSettings::LoadSettings()
{
    reset();

    for( std::map<unsigned int, unsigned int>::iterator it = SETTINGS.addons.configuration.begin(); it != SETTINGS.addons.configuration.end(); ++it)
        setSelection((AddonId)it->first, it->second);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  saves the current addon configuration to the SETTINGS.
 *
 *  @author FloSoft
 */
void GlobalGameSettings::SaveSettings() const
{
    SETTINGS.addons.configuration.clear();
    for( std::vector<item>::const_iterator it = addons.begin(); it != addons.end(); ++it)
        SETTINGS.addons.configuration.insert(std::make_pair(it->addon->getId(), it->status));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  saves the current addon configuration to a serializer object.
 *
 *  @author FloSoft
 */
void GlobalGameSettings::Serialize(Serializer& ser) const
{
    LOG.write(">>> Addon Status:\n");

    ser.PushUnsignedChar(static_cast<unsigned char>(game_speed));
    ser.PushUnsignedChar(static_cast<unsigned char>(game_objective));
    ser.PushUnsignedChar(static_cast<unsigned char>(start_wares));
    ser.PushBool(lock_teams);
    ser.PushUnsignedChar(static_cast<unsigned char>(exploration));
    ser.PushBool(team_view);
    ser.PushBool(random_location);

    ser.PushUnsignedInt(addons.size());
    for( std::vector<item>::const_iterator it = addons.begin(); it != addons.end(); ++it)
    {
        ser.PushUnsignedInt(it->addon->getId());
        ser.PushUnsignedInt(it->status);

        LOG.write("\t0x%08X=%d\n", it->addon->getId(), it->status);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  reads the current addon configuration from a serializer object.
 *
 *  @author FloSoft
 */
void GlobalGameSettings::Deserialize(Serializer& ser)
{
    game_speed = static_cast<GameSpeed>(ser.PopUnsignedChar());
    game_objective = static_cast<GameObjective>(ser.PopUnsignedChar());
    start_wares = static_cast<StartWares>(ser.PopUnsignedChar());
    lock_teams = ser.PopBool();
    exploration = static_cast<Exploration>(ser.PopUnsignedChar());
    team_view = ser.PopBool();
    random_location = ser.PopBool();

    unsigned int count = ser.PopUnsignedInt();

    reset();

    LOG.write("<<< Addon Status:\n");

    for(unsigned int i = 0; i < count; ++i)
    {
        AddonId addon = AddonId(ser.PopUnsignedInt());
        unsigned int status = ser.PopUnsignedInt();
        setSelection(addon, status);

        LOG.write("\t0x%08X=%d\n", addon, status);
    }
}


void GlobalGameSettings::setSelection(AddonId id, unsigned int selection)
{
    std::vector<item>::iterator it = std::find(addons.begin(), addons.end(), id);
    if(it == addons.end())
    {
        std::cout << "Addon 0x" << std::hex << id << std::dec << " not found!" << std::endl;
        return;
    }

    it->status = selection;
}

unsigned GlobalGameSettings::GetMaxMilitaryRank() const
{
    unsigned selection = getSelection(ADDON_MAX_RANK);
    RTTR_Assert(selection <= MAX_MILITARY_RANK);
    return MAX_MILITARY_RANK - selection;
}