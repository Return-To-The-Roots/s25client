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

#include "SerializedGameData.h"
#include "CatapultStone.h"
#include "EventManager.h"
#include "FOWObjects.h"
#include "Game.h"
#include "GameEvent.h"
#include "GameObject.h"
#include "GamePlayer.h"
#include "RoadSegment.h"
#include "Ware.h"
#include "buildings/BurnedWarehouse.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "buildings/nobStorehouse.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofArmorer.h"
#include "figures/nofAttacker.h"
#include "figures/nofBaker.h"
#include "figures/nofBrewer.h"
#include "figures/nofBuilder.h"
#include "figures/nofButcher.h"
#include "figures/nofCarpenter.h"
#include "figures/nofCarrier.h"
#include "figures/nofCatapultMan.h"
#include "figures/nofCharburner.h"
#include "figures/nofDefender.h"
#include "figures/nofDonkeybreeder.h"
#include "figures/nofFarmer.h"
#include "figures/nofFisher.h"
#include "figures/nofForester.h"
#include "figures/nofGeologist.h"
#include "figures/nofHunter.h"
#include "figures/nofIronfounder.h"
#include "figures/nofMetalworker.h"
#include "figures/nofMiller.h"
#include "figures/nofMiner.h"
#include "figures/nofMinter.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofPassiveWorker.h"
#include "figures/nofPigbreeder.h"
#include "figures/nofPlaner.h"
#include "figures/nofScout_Free.h"
#include "figures/nofScout_LookoutTower.h"
#include "figures/nofShipWright.h"
#include "figures/nofStonemason.h"
#include "figures/nofTradeDonkey.h"
#include "figures/nofTradeLeader.h"
#include "figures/nofWarehouseWorker.h"
#include "figures/nofWellguy.h"
#include "figures/nofWoodcutter.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "helpers/toString.h"
#include "world/MapSerializer.h"
#include "nodeObjs/noAnimal.h"
#include "nodeObjs/noCharburnerPile.h"
#include "nodeObjs/noDisappearingMapEnvObject.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noExtension.h"
#include "nodeObjs/noFighting.h"
#include "nodeObjs/noFire.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noGrainfield.h"
#include "nodeObjs/noGranite.h"
#include "nodeObjs/noShip.h"
#include "nodeObjs/noShipBuildingSite.h"
#include "nodeObjs/noSign.h"
#include "nodeObjs/noSkeleton.h"
#include "nodeObjs/noStaticObject.h"
#include "nodeObjs/noTree.h"
#include "s25util/Log.h"

// clang-format off
/// Version of the current game data
/// Usage: Always save for the most current version but include loading code that can cope with file format changes
/// If a format change occurred that can still be handled increase this version and handle it in the loading code.
/// If the change is to big to handle increase the version in Savegame.cpp and remove all code referencing GetGameDataVersion.
/// Then reset this number to 1.
/// TODO: Let GO_Type start at 0 again when resetting this
/// Changelog:
/// 2: All player buildings together, variable width size for containers and ship names
/// 3: Landscape and terrain names stored as strings
/// 4: HunterWaitingForAnimalReady introduced as sub-state of HunterFindingShootingpoint
/// 5: Make RoadPathDirection contiguous and use optional for ware in nofBuildingWorker
/// 6: Make TradeDirection contiguous, Serialize only nobUsuals in BuildingRegister::buildings,
///    include water and fish in geologists resourceFound
/// 7: Use helpers::push/popContainer (uses var size)
/// 8: noFlag::Wares converted to static_vector
static const unsigned currentGameDataVersion = 8;
// clang-format on

std::unique_ptr<GameObject> SerializedGameData::Create_GameObject(const GO_Type got, const unsigned obj_id)
{
    switch(got)
    {
#define RTTR_CREATE_GO(GOT, CLASS) \
    case GOT: return std::unique_ptr<GameObject>(new CLASS(*this, obj_id))
        RTTR_CREATE_GO(GO_Type::NobHq, nobHQ);
        RTTR_CREATE_GO(GO_Type::NobMilitary, nobMilitary);
        RTTR_CREATE_GO(GO_Type::NobStorehouse, nobStorehouse);
        RTTR_CREATE_GO(GO_Type::NobUsual, nobUsual);
        RTTR_CREATE_GO(GO_Type::NobShipyard, nobShipYard);
        RTTR_CREATE_GO(GO_Type::NobHarborbuilding, nobHarborBuilding);
        RTTR_CREATE_GO(GO_Type::NofAggressivedefender, nofAggressiveDefender);
        RTTR_CREATE_GO(GO_Type::NofAttacker, nofAttacker);
        RTTR_CREATE_GO(GO_Type::NofDefender, nofDefender);
        RTTR_CREATE_GO(GO_Type::NofPassivesoldier, nofPassiveSoldier);
        RTTR_CREATE_GO(GO_Type::NofPassiveworker, nofPassiveWorker);
        RTTR_CREATE_GO(GO_Type::NofWellguy, nofWellguy);
        RTTR_CREATE_GO(GO_Type::NofCarrier, nofCarrier);
        RTTR_CREATE_GO(GO_Type::NofWoodcutter, nofWoodcutter);
        RTTR_CREATE_GO(GO_Type::NofFisher, nofFisher);
        RTTR_CREATE_GO(GO_Type::NofForester, nofForester);
        RTTR_CREATE_GO(GO_Type::NofCarpenter, nofCarpenter);
        RTTR_CREATE_GO(GO_Type::NofStonemason, nofStonemason);
        RTTR_CREATE_GO(GO_Type::NofHunter, nofHunter);
        RTTR_CREATE_GO(GO_Type::NofFarmer, nofFarmer);
        RTTR_CREATE_GO(GO_Type::NofMiller, nofMiller);
        RTTR_CREATE_GO(GO_Type::NofBaker, nofBaker);
        RTTR_CREATE_GO(GO_Type::NofButcher, nofButcher);
        RTTR_CREATE_GO(GO_Type::NofMiner, nofMiner);
        RTTR_CREATE_GO(GO_Type::NofBrewer, nofBrewer);
        RTTR_CREATE_GO(GO_Type::NofPigbreeder, nofPigbreeder);
        RTTR_CREATE_GO(GO_Type::NofDonkeybreeder, nofDonkeybreeder);
        RTTR_CREATE_GO(GO_Type::NofIronfounder, nofIronfounder);
        RTTR_CREATE_GO(GO_Type::NofMinter, nofMinter);
        RTTR_CREATE_GO(GO_Type::NofMetalworker, nofMetalworker);
        RTTR_CREATE_GO(GO_Type::NofArmorer, nofArmorer);
        RTTR_CREATE_GO(GO_Type::NofBuilder, nofBuilder);
        RTTR_CREATE_GO(GO_Type::NofPlaner, nofPlaner);
        RTTR_CREATE_GO(GO_Type::NofGeologist, nofGeologist);
        RTTR_CREATE_GO(GO_Type::NofShipwright, nofShipWright);
        RTTR_CREATE_GO(GO_Type::NofScoutFree, nofScout_Free);
        RTTR_CREATE_GO(GO_Type::NofScoutLookouttower, nofScout_LookoutTower);
        RTTR_CREATE_GO(GO_Type::NofWarehouseworker, nofWarehouseWorker);
        RTTR_CREATE_GO(GO_Type::NofCatapultman, nofCatapultMan);
        RTTR_CREATE_GO(GO_Type::NofCharburner, nofCharburner);
        RTTR_CREATE_GO(GO_Type::NofTradedonkey, nofTradeDonkey);
        RTTR_CREATE_GO(GO_Type::NofTradeleader, nofTradeLeader);
        RTTR_CREATE_GO(GO_Type::Extension, noExtension);
        RTTR_CREATE_GO(GO_Type::Buildingsite, noBuildingSite);
        RTTR_CREATE_GO(GO_Type::Envobject, noEnvObject);
        RTTR_CREATE_GO(GO_Type::Fire, noFire);
        RTTR_CREATE_GO(GO_Type::Burnedwarehouse, BurnedWarehouse);
        RTTR_CREATE_GO(GO_Type::Flag, noFlag);
        RTTR_CREATE_GO(GO_Type::Grainfield, noGrainfield);
        RTTR_CREATE_GO(GO_Type::Granite, noGranite);
        RTTR_CREATE_GO(GO_Type::Sign, noSign);
        RTTR_CREATE_GO(GO_Type::Skeleton, noSkeleton);
        RTTR_CREATE_GO(GO_Type::Staticobject, noStaticObject);
        RTTR_CREATE_GO(GO_Type::Disappearingmapenvobject, noDisappearingMapEnvObject);
        RTTR_CREATE_GO(GO_Type::Tree, noTree);
        RTTR_CREATE_GO(GO_Type::Animal, noAnimal);
        RTTR_CREATE_GO(GO_Type::Fighting, noFighting);
        RTTR_CREATE_GO(GO_Type::Roadsegment, RoadSegment);
        RTTR_CREATE_GO(GO_Type::Ware, Ware);
        RTTR_CREATE_GO(GO_Type::Catapultstone, CatapultStone);
        RTTR_CREATE_GO(GO_Type::Ship, noShip);
        RTTR_CREATE_GO(GO_Type::Shipbuildingsite, noShipBuildingSite);
        RTTR_CREATE_GO(GO_Type::Charburnerpile, noCharburnerPile);
        RTTR_CREATE_GO(GO_Type::Economymodehandler, EconomyModeHandler);
        case GO_Type::Nothing: RTTR_Assert(false); break;
#undef RTTR_CREATE_GO
    }
    throw Error("Invalid GameObjectType " + helpers::toString(got) + " for objId=" + helpers::toString(obj_id)
                + " found!");
}

std::unique_ptr<FOWObject> SerializedGameData::Create_FOWObject(const FoW_Type fowtype)
{
    switch(fowtype)
    {
        default: return nullptr;
        case FoW_Type::Building: return std::make_unique<fowBuilding>(*this);
        case FoW_Type::Buildingsite: return std::make_unique<fowBuildingSite>(*this);
        case FoW_Type::Flag: return std::make_unique<fowFlag>(*this);
        case FoW_Type::Tree: return std::make_unique<fowTree>(*this);
        case FoW_Type::Granite: return std::make_unique<fowGranite>(*this);
    }
}

SerializedGameData::SerializedGameData()
    : debugMode(false), expectedNumObjects(0), em(nullptr), writeEm(nullptr), isReading(false)
{}

void SerializedGameData::Prepare(bool reading)
{
    static const std::array<char, 4> versionID = {"VER"};
    if(reading)
    {
        std::array<char, 4> versionIDRead;
        PopRawData(&versionIDRead.front(), versionIDRead.size());
        if(versionIDRead != versionID)
            throw Error("Invalid file format!");
        gameDataVersion = PopUnsignedInt();
    } else
    {
        Clear();
        PushRawData(&versionID.front(), versionID.size());
        PushUnsignedInt(currentGameDataVersion);
        gameDataVersion = currentGameDataVersion;
    }
    writtenObjIds.clear();
    readObjects.clear();
    expectedNumObjects = 0;
    isReading = reading;
}

void SerializedGameData::MakeSnapshot(const Game& game)
{
    Prepare(false);

    const GameWorldBase& gw = game.world_;
    writeEm = &gw.GetEvMgr();

    // Anzahl Objekte reinschreiben (used for safety checks only)
    expectedNumObjects = GameObject::GetNumObjs();
    PushUnsignedInt(expectedNumObjects);

    // World and objects
    MapSerializer::Serialize(gw, *this);
    // EventManager
    writeEm->Serialize(*this);
    if(game.ggs_.objective == GameObjective::EconomyMode)
    {
        PushObject(gw.getEconHandler(), true);
    }
    // Spieler serialisieren
    for(unsigned i = 0; i < gw.GetNumPlayers(); ++i)
    {
        if(debugMode)
            LOG.write("Start serializing player %1% at %2%\n") % i % GetLength();
        gw.GetPlayer(i).Serialize(*this);
        if(debugMode)
            LOG.write("Done serializing player %1% at %2%\n") % i % GetLength();
    }

    if(writtenEventIds.size() != writeEm->GetNumActiveEvents())
    {
        throw Error(helpers::format("Event count mismatch. Expected: %1%, written: %2%", writeEm->GetNumActiveEvents(),
                                    writtenEventIds.size()));
    }
    // If this check fails, we missed some objects or some objects were destroyed without decreasing the obj count
    if(expectedNumObjects != writtenObjIds.size() + 1) // "Nothing" nodeObj does not get serialized
    {
        throw Error(helpers::format("Object count mismatch. Expected: %1%, written: %2%", expectedNumObjects,
                                    writtenObjIds.size() + 1));
    }

    writeEm = nullptr;
    writtenObjIds.clear();
    writtenEventIds.clear();
}

void SerializedGameData::ReadSnapshot(Game& game, ILocalGameState& localGameState)
{
    Prepare(true);

    GameWorld& gw = game.world_;
    em = &gw.GetEvMgr();

    expectedNumObjects = PopUnsignedInt();

    MapSerializer::Deserialize(gw, *this, game, localGameState);
    em->Deserialize(*this);
    if(gw.GetGGS().objective == GameObjective::EconomyMode)
    {
        gw.setEconHandler(
          std::unique_ptr<EconomyModeHandler>(PopObject<EconomyModeHandler>(GO_Type::Economymodehandler)));
    }

    for(unsigned i = 0; i < gw.GetNumPlayers(); ++i)
        gw.GetPlayer(i).Deserialize(*this);

    // If this check fails, we did not serialize all objects or there was an async
    if(readEvents.size() != em->GetNumActiveEvents())
    {
        throw Error(helpers::format("Event count mismatch. Expected: %1%, read: %2%", em->GetNumActiveEvents(),
                                    readEvents.size()));
    }
    if(expectedNumObjects != GameObject::GetNumObjs())
    {
        throw Error(helpers::format("Object count mismatch. Expected: %1%, Existing: %2%", expectedNumObjects,
                                    GameObject::GetNumObjs()));
    }
    if(expectedNumObjects != readObjects.size() + 1) // "Nothing" nodeObj does not get serialized
    {
        throw Error(helpers::format("Object count mismatch. Expected: %1%, read: %2%", expectedNumObjects,
                                    readObjects.size() + 1));
    }

    em = nullptr;
    readObjects.clear();
    readEvents.clear();
}

void SerializedGameData::PushObject_(const GameObject* go, const bool known)
{
    RTTR_Assert(!isReading);

    // Gibts das Objekt gar nich?
    if(!go)
    {
        // Null draufschreiben
        PushUnsignedInt(0);
        return;
    }

    const unsigned objId = go->GetObjId();

    RTTR_Assert(objId <= GameObject::GetObjIDCounter());
    if(objId > GameObject::GetObjIDCounter())
    {
        LOG.write("%s\n") % _("An error occured while saving which was suppressed!");
        PushUnsignedInt(0);
        return;
    }

    PushUnsignedInt(objId);

    // If the object was already serialized skip the data
    if(IsObjectSerialized(objId))
    {
        if(debugMode)
            LOG.write("Saved known objId %u\n") % objId;
        return;
    }

    if(debugMode)
        LOG.write("Saving objId %u, obj#=%u\n") % objId % writtenObjIds.size();

    // Objekt merken
    writtenObjIds.insert(objId);

    RTTR_Assert(writtenObjIds.size() < GameObject::GetNumObjs());

    // Objekt nich bekannt? Dann Type-ID noch mit drauf
    if(!known)
        PushEnum<uint16_t>(go->GetGOT());

    // Objekt serialisieren
    if(debugMode)
        LOG.write("Start serializing %1% at %2%\n") % objId % GetLength();
    go->Serialize(*this);
    if(debugMode)
        LOG.write("Done serializing %1% at %2%\n") % objId % GetLength();

    // Sicherheitscode reinschreiben
    PushUnsignedShort(GetSafetyCode(*go));
}

void SerializedGameData::PushEvent(const GameEvent* event)
{
    if(!event)
    {
        PushUnsignedInt(0);
        return;
    }

    unsigned instanceId = event->GetInstanceId();
    PushUnsignedInt(instanceId);
    if(IsEventSerialized(instanceId))
        return;
    writtenEventIds.insert(instanceId);
    if(debugMode)
        LOG.write("Start serializing event %1% at %2%\n") % instanceId % GetLength();
    event->Serialize(*this);
    if(debugMode)
        LOG.write("Done serializing event %1% at %2%\n") % instanceId % GetLength();
    PushUnsignedShort(GetSafetyCode(*event));
}

const GameEvent* SerializedGameData::PopEvent()
{
    unsigned instanceId = PopUnsignedInt();
    if(!instanceId)
        return nullptr;

    // Note: em->GetEventInstanceCtr() might not be set yet
    const auto foundObj = readEvents.find(instanceId);
    if(foundObj != readEvents.end())
        return foundObj->second;
    std::unique_ptr<GameEvent> ev = std::make_unique<GameEvent>(*this, instanceId);

    unsigned short safety_code = PopUnsignedShort();

    if(safety_code != GetSafetyCode(*ev))
    {
        LOG.write("SerializedGameData::PopEvent: ERROR: After loading Event(instanceId = %1%); Code is wrong!\n")
          % instanceId;
        throw Error("Invalid safety code after PopEvent");
    }
    return ev.release();
}

/// FoW-Objekt
void SerializedGameData::PushFOWObject(const FOWObject* fowobj)
{
    // Gibts das Objekt gar nich?
    if(!fowobj)
    {
        // Null draufschreiben
        PushUnsignedChar(0);
        return;
    }

    // Objekt-Typ
    PushEnum<uint8_t>(fowobj->GetType());

    // Objekt serialisieren
    fowobj->Serialize(*this);
}

std::unique_ptr<FOWObject> SerializedGameData::PopFOWObject()
{
    // Typ auslesen
    auto type = Pop<FoW_Type>();

    // Kein Objekt?
    if(type == FoW_Type::Nothing)
        return nullptr;

    // entsprechendes Objekt erzeugen
    return Create_FOWObject(type);
}

GameObject* SerializedGameData::PopObject_(helpers::OptionalEnum<GO_Type> got)
{
    RTTR_Assert(isReading);
    // Obj-ID holen
    const unsigned objId = PopUnsignedInt();

    // Obj-ID = 0 ? Dann Null-Pointer zurueckgeben
    if(!objId)
        return nullptr;

    if(GameObject* go = GetReadGameObject(objId))
        return go;

    // Objekt nich bekannt? Dann in den heiligen Schriften lesen
    if(!got)
        got = Pop<GO_Type>();

    // und erzeugen
    std::unique_ptr<GameObject> go = Create_GameObject(*got, objId);

    // Sicherheitscode auslesen
    unsigned short safety_code = PopUnsignedShort();

    if(safety_code != GetSafetyCode(*go))
    {
        LOG.write(
          "SerializedGameData::PopObject_: ERROR: After loading Object(obj_id = %u, got = %u); Code is wrong!\n")
          % objId % rttr::enum_cast(*got);
        throw Error("Invalid safety code after PopObject");
    }

    return go.release();
}

unsigned short SerializedGameData::GetSafetyCode(const GameObject& go)
{
    return 0xFFFF ^ rttr::enum_cast(go.GetGOT()) ^ go.GetObjId();
}

unsigned short SerializedGameData::GetSafetyCode(const GameEvent& ev)
{
    return 0xFFFF ^ ev.GetInstanceId();
}

SerializedGameData::Error SerializedGameData::makeOutOfRange(unsigned value, unsigned maxValue)
{
    return Error(helpers::format("%s is out of range. Maximum allowed value: %s", value, maxValue));
}

void SerializedGameData::AddObject(GameObject* go)
{
    RTTR_Assert(isReading);
    RTTR_Assert(!readObjects[go->GetObjId()]); // Do not call this multiple times per GameObject
    readObjects[go->GetObjId()] = go;
    RTTR_Assert(readObjects.size() < expectedNumObjects);
}

unsigned SerializedGameData::AddEvent(unsigned instanceId, GameEvent* ev)
{
    RTTR_Assert(isReading);
    RTTR_Assert(!readEvents[instanceId]); // Do not call this multiple times per GameObject
    readEvents[instanceId] = ev;
    return instanceId;
}

bool SerializedGameData::IsObjectSerialized(unsigned obj_id) const
{
    RTTR_Assert(!isReading);
    RTTR_Assert(obj_id <= GameObject::GetObjIDCounter());
    return helpers::contains(writtenObjIds, obj_id);
}

bool SerializedGameData::IsEventSerialized(unsigned evInstanceid) const
{
    RTTR_Assert(!isReading);
    RTTR_Assert(evInstanceid < writeEm->GetEventInstanceCtr());
    return helpers::contains(writtenEventIds, evInstanceid);
}

GameObject* SerializedGameData::GetReadGameObject(const unsigned obj_id) const
{
    RTTR_Assert(isReading);
    RTTR_Assert(obj_id <= GameObject::GetObjIDCounter());
    auto foundObj = readObjects.find(obj_id);
    if(foundObj == readObjects.end())
        return nullptr;
    else
        return foundObj->second;
}
