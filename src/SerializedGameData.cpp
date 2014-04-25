// $Id: SerializedGameData.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "SerializedGameData.h"
#include "GameWorld.h"
#include "noBase.h"
#include "GameObject.h"
#include "EventManager.h"
#include "GameClient.h"
#include "GameClientPlayer.h"

#include "nobHQ.h"
#include "nobMilitary.h"
#include "nobStorehouse.h"
#include "nobShipYard.h"
#include "nofAggressiveDefender.h"
#include "nofAttacker.h"
#include "nofDefender.h"
#include "nofPassiveSoldier.h"
#include "nofWellguy.h"
#include "nofCarrier.h"
#include "nofWoodcutter.h"
#include "nofFisher.h"
#include "nofForester.h"
#include "nofCarpenter.h"
#include "nofStonemason.h"
#include "nofHunter.h"
#include "nofFarmer.h"
#include "nofMiller.h"
#include "nofBaker.h"
#include "nofButcher.h"
#include "nofMiner.h"
#include "nofBrewer.h"
#include "nofPigbreeder.h"
#include "nofDonkeybreeder.h"
#include "nofIronfounder.h"
#include "nofMinter.h"
#include "nofMetalworker.h"
#include "nofArmorer.h"
#include "nofBuilder.h"
#include "nofPlaner.h"
#include "nofGeologist.h"
#include "nofShipWright.h"
#include "nofScout_Free.h"
#include "nofScout_LookoutTower.h"
#include "nofWarehouseWorker.h"
#include "nofPassiveWorker.h"
#include "nofCharburner.h"
#include "nofCatapultMan.h"
#include "nofTradeDonkey.h"
#include "nofTradeLeader.h"
#include "noExtension.h"
#include "noBuildingSite.h"
#include "noEnvObject.h"
#include "noFire.h"
#include "noFlag.h"
#include "noGrainfield.h"
#include "noGranite.h"
#include "noSign.h"
#include "noSkeleton.h"
#include "noStaticObject.h"
#include "noTree.h"
#include "noAnimal.h"
#include "noFighting.h"
#include "noDisappearingMapEnvObject.h"
#include "EventManager.h"
#include "RoadSegment.h"
#include "Ware.h"
#include "CatapultStone.h"
#include "FOWObjects.h"
#include "nobHarborBuilding.h"
#include "noShip.h"
#include "noShipBuildingSite.h"
#include "noCharburnerPile.h"
#include "BurnedWarehouse.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GameObject* SerializedGameData::Create_GameObject(const GO_Type got, const unsigned obj_id)
{
    switch(got)
    {
        default: return 0;
        case GOT_NOB_HQ: return new nobHQ(this, obj_id);
        case GOT_NOB_MILITARY: return new nobMilitary(this, obj_id);
        case GOT_NOB_STOREHOUSE: return new nobStorehouse(this, obj_id);
        case GOT_NOB_USUAL: return new nobUsual(this, obj_id);
        case GOT_NOB_SHIPYARD: return new nobShipYard(this, obj_id);
        case GOT_NOB_HARBORBUILDING: return new nobHarborBuilding(this, obj_id);
        case GOT_NOF_AGGRESSIVEDEFENDER: return new nofAggressiveDefender(this, obj_id);
        case GOT_NOF_ATTACKER: return new nofAttacker(this, obj_id);
        case GOT_NOF_DEFENDER: return new nofDefender(this, obj_id);
        case GOT_NOF_PASSIVESOLDIER: return new nofPassiveSoldier(this, obj_id);
        case GOT_NOF_PASSIVEWORKER: return new nofPassiveWorker(this, obj_id);
        case GOT_NOF_WELLGUY: return new nofWellguy(this, obj_id);
        case GOT_NOF_CARRIER: return new nofCarrier(this, obj_id);
        case GOT_NOF_WOODCUTTER: return new nofWoodcutter(this, obj_id);
        case GOT_NOF_FISHER: return new nofFisher(this, obj_id);
        case GOT_NOF_FORESTER: return new nofForester(this, obj_id);
        case GOT_NOF_CARPENTER: return new nofCarpenter(this, obj_id);
        case GOT_NOF_STONEMASON: return new nofStonemason(this, obj_id);
        case GOT_NOF_HUNTER: return new nofHunter(this, obj_id);
        case GOT_NOF_FARMER: return new nofFarmer(this, obj_id);
        case GOT_NOF_MILLER: return new nofMiller(this, obj_id);
        case GOT_NOF_BAKER: return new nofBaker(this, obj_id);
        case GOT_NOF_BUTCHER: return new nofButcher(this, obj_id);
        case GOT_NOF_MINER: return new nofMiner(this, obj_id);
        case GOT_NOF_BREWER: return new nofBrewer(this, obj_id);
        case GOT_NOF_PIGBREEDER: return new nofPigbreeder(this, obj_id);
        case GOT_NOF_DONKEYBREEDER: return new nofDonkeybreeder(this, obj_id);
        case GOT_NOF_IRONFOUNDER: return new nofIronfounder(this, obj_id);
        case GOT_NOF_MINTER: return new nofMinter(this, obj_id);
        case GOT_NOF_METALWORKER: return new nofMetalworker(this, obj_id);
        case GOT_NOF_ARMORER: return new nofArmorer(this, obj_id);
        case GOT_NOF_BUILDER: return new nofBuilder(this, obj_id);
        case GOT_NOF_PLANER: return new nofPlaner(this, obj_id);
        case GOT_NOF_GEOLOGIST: return new nofGeologist(this, obj_id);
        case GOT_NOF_SHIPWRIGHT: return new nofShipWright(this, obj_id);
        case GOT_NOF_SCOUT_FREE: return new nofScout_Free(this, obj_id);
        case GOT_NOF_SCOUT_LOOKOUTTOWER: return new nofScout_LookoutTower(this, obj_id);
        case GOT_NOF_WAREHOUSEWORKER: return new nofWarehouseWorker(this, obj_id);
        case GOT_NOF_CATAPULTMAN: return new nofCatapultMan(this, obj_id);
        case GOT_NOF_CHARBURNER: return new nofCharburner(this, obj_id);
        case GOT_NOF_TRADEDONKEY: return new nofTradeDonkey(this, obj_id);
        case GOT_NOF_TRADELEADER: return new nofTradeLeader(this, obj_id);
        case GOT_EXTENSION: return new noExtension(this, obj_id);
        case GOT_BUILDINGSITE: return new noBuildingSite(this, obj_id);
        case GOT_ENVOBJECT: return new noEnvObject(this, obj_id);
        case GOT_FIRE: return new noFire(this, obj_id);
        case GOT_BURNEDWAREHOUSE: return new BurnedWarehouse(this, obj_id);
        case GOT_FLAG: return new noFlag(this, obj_id);
        case GOT_GRAINFIELD: return new noGrainfield(this, obj_id);
        case GOT_GRANITE: return new noGranite(this, obj_id);
        case GOT_SIGN: return new noSign(this, obj_id);
        case GOT_SKELETON: return new noSkeleton(this, obj_id);
        case GOT_STATICOBJECT: return new noStaticObject(this, obj_id);
        case GOT_DISAPPEARINGMAPENVOBJECT: return new noDisappearingMapEnvObject(this, obj_id);
        case GOT_TREE: return new noTree(this, obj_id);
        case GOT_ANIMAL: return new noAnimal(this, obj_id);
        case GOT_FIGHTING: return new noFighting(this, obj_id);
        case GOT_EVENT: return em->AddEvent(this, obj_id);
        case GOT_ROADSEGMENT: return new RoadSegment(this, obj_id);
        case GOT_WARE: return new Ware(this, obj_id);
        case GOT_CATAPULTSTONE: return new CatapultStone(this, obj_id);
        case GOT_SHIP: return new noShip(this, obj_id);
        case GOT_SHIPBUILDINGSITE: return new noShipBuildingSite(this, obj_id);
        case GOT_CHARBURNERPILE: return new noCharburnerPile(this, obj_id);

    }
}

FOWObject* SerializedGameData::Create_FOWObject(const FOW_Type fowtype)
{
    switch(fowtype)
    {
        default: return 0;
        case FOW_BUILDING: return new fowBuilding(this);
        case FOW_BUILDINGSITE: return new fowBuildingSite(this);
        case FOW_FLAG: return new fowFlag(this);
        case FOW_TREE: return new fowTree(this);
        case FOW_GRANITE: return new fowGranite(this);
    }
}

SerializedGameData::SerializedGameData() : objects_write(0), objects_count(0)
{
}


void SerializedGameData::MakeSnapshot(GameWorld* const gw, EventManager* const em)
{
    // Buffer erzeugen
    Clear();

    // Objektreferenzen reservieren
    objects_write = new const GameObject*[GameObject::GetObjCount()];
    memset(objects_write, 0, GameObject::GetObjCount()*sizeof(GameObject*));

    // Anzahl Objekte reinschreiben
    PushUnsignedInt(GameObject::GetObjCount());

    // Objektmanager serialisieren
    gw->Serialize(this);
    // EventManager serialisieren
    em->Serialize(this);
    // Spieler serialisieren
    for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
        GAMECLIENT.GetPlayer(i)->Serialize(this);

    delete [] objects_write;
}

void SerializedGameData::ReadFromFile(BinaryFile& file)
{
    Serializer::ReadFromFile(file);

    total_objects_count = PopUnsignedInt();
    objects_count = 0;
    objects_read = new GameObject*[total_objects_count];
    GameObject::SetObjCount(total_objects_count);
}

void SerializedGameData::PushObject(const GameObject* go, const bool known)
{
    if(go)
    {
        //assert(go->GetObjId() < GameObject::GetObjIDCounter());
        if(go->GetObjId() >= GameObject::GetObjIDCounter())
        {
            LOG.lprintf("%s\n", _("An error occured while saving which was suppressed!"));
            go = NULL;
        }
    }

    // Gibts das Objekt gar nich?
    if(!go)
    {
        // Null draufschreiben
        PushUnsignedInt(0);
    }
    // Ist das Objekt schon vorhanden?
    else if(GetConstGameObject(go->GetObjId()))
    {
        // Dann nur die Obj-ID draufpushen
        PushUnsignedInt(go->GetObjId());
    }
    else
    {
        // Obj-ID
        PushUnsignedInt(go->GetObjId());

        // Objekt nich bekannt? Dann Type-ID noch mit drauf
        if(!known)
            PushUnsignedShort(go->GetGOT());

        // Objekt merken
        objects_write[objects_count++] = go;

        assert(objects_count <= GameObject::GetObjCount());

        // Objekt serialisieren
        go->Serialize(this);

        // Sicherheitscode reinschreiben
        PushUnsignedShort(0xFFFF);
    }
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
    PushUnsignedChar(static_cast<unsigned char>(fowobj->GetType()));

    // Objekt serialisieren
    fowobj->Serialize(this);
}

FOWObject* SerializedGameData::PopFOWObject()
{
    // Typ auslesen
    FOW_Type type = FOW_Type(PopUnsignedChar());

    // Kein Objekt?
    if(type == FOW_NOTHING)
        return NULL;

    // entsprechendes Objekt erzeugen
    return Create_FOWObject(type);
}

GameObject* SerializedGameData::PopObject_(GO_Type got)
{
    // Obj-ID holen
    unsigned obj_id = PopUnsignedInt();

    // Obj-ID = 0 ? Dann Null-Pointer zurueckgeben
    if(!obj_id)
        return 0;

    GameObject* go;

    // Schon vorhanden?
    if( (go = GetGameObject(obj_id)))
        // dann das nehmen
        return go;

    // Objekt nich bekannt? Dann in den heiligen Schriften lesen
    if(got == GOT_UNKNOWN)
        got = GO_Type(PopUnsignedShort());

    // und erzeugen
    go = Create_GameObject(got, obj_id);

    // Sicherheitscode auslesen
    unsigned short safety_code = PopUnsignedShort();

    if(safety_code != 0xFFFF)
    {
        LOG.lprintf("SerializedGameData::PopObject_: ERROR: After loading Object(obj_id = %u, got = %u); Code is wrong!\n",
                    obj_id, got);
        assert(false);
        return 0;
    }

    return go;
}

void SerializedGameData::AddObject(GameObject* go)
{
    objects_read[objects_count++] = go;
}

const GameObject* SerializedGameData::GetConstGameObject(const unsigned obj_id) const
{
    // Objekt suchen
    for(unsigned i = 0; i < objects_count; ++i)
    {
        if(objects_write[i]->GetObjId() == obj_id)
            return objects_write[i];
    }

    return 0;
}

GameObject* SerializedGameData::GetGameObject(const unsigned obj_id) const
{
    // Objekt suchen
    for(unsigned i = 0; i < objects_count; ++i)
    {
        if(objects_read[i]->GetObjId() == obj_id)
            return objects_read[i];
    }

    return 0;
}
