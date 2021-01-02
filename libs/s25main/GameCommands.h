// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "GameCommand.h"
#include "helpers/serializeEnums.h"
#include "variant.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/InventorySetting.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/PactTypes.h"
#include "gameTypes/SettingsTypes.h"
#include "s25util/Serializer.h"
#include <cstdint>
#include <utility>
#include <vector>
class GameWorldGame;

namespace gc {

/// Basisklasse für sämtliche GameCommands mit Koordinaten
class Coords : public GameCommand
{
    GC_FRIEND_DECL;

private:
    static MapPoint PopMapPoint(Serializer& ser)
    {
        MapPoint pt;
        pt.x = ser.PopUnsignedShort();
        pt.y = ser.PopUnsignedShort();
        return pt;
    }

protected:
    /// Koordinaten auf der Map, die dieses Command betreffen
    const MapPoint pt_;
    Coords(const Type gst, const MapPoint pt) : GameCommand(gst), pt_(pt) {}
    Coords(const Type gst, Serializer& ser) : GameCommand(gst), pt_(PopMapPoint(ser)) {}

public:
    void Serialize(Serializer& ser) const override
    {
        GameCommand::Serialize(ser);
        ser.PushUnsignedShort(pt_.x);
        ser.PushUnsignedShort(pt_.y);
    }
};

/// Flagge setzen
class SetFlag : public Coords
{
    GC_FRIEND_DECL;

protected:
    SetFlag(const MapPoint pt) : Coords(SET_FLAG, pt) {}
    SetFlag(Serializer& ser) : Coords(SET_FLAG, ser) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Flagge zerstören
class DestroyFlag : public Coords
{
    GC_FRIEND_DECL;

protected:
    DestroyFlag(const MapPoint pt) : Coords(DESTROY_FLAG, pt) {}
    DestroyFlag(Serializer& ser) : Coords(DESTROY_FLAG, ser) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Straße bauen
class BuildRoad : public Coords
{
    GC_FRIEND_DECL;
    /// Boot-Straße oder nicht?
    const bool boat_road;
    /// Beschreibung der Straße mittels einem Array aus Richtungen
    std::vector<Direction> route;

protected:
    BuildRoad(const MapPoint pt, bool boat_road, std::vector<Direction> route)
        : Coords(BUILD_ROAD, pt), boat_road(boat_road), route(std::move(route))
    {}
    BuildRoad(Serializer& ser);

public:
    void Serialize(Serializer& ser) const override;

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Straße zerstören
class DestroyRoad : public Coords
{
    GC_FRIEND_DECL;
    /// Richtung in der von der Flagge an x;y aus gesehen die Straße zerstört werden soll
    const Direction start_dir;

protected:
    DestroyRoad(const MapPoint pt, const Direction start_dir) : Coords(DESTROY_ROAD, pt), start_dir(start_dir) {}
    DestroyRoad(Serializer& ser);

public:
    void Serialize(Serializer& ser) const override;

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Straße aufwerten
class UpgradeRoad : public Coords
{
    GC_FRIEND_DECL;
    /// Richtung in der von der Flagge an x;y aus gesehen die Straße zerstört werden soll
    const Direction start_dir;

protected:
    UpgradeRoad(const MapPoint pt, const Direction start_dir) : Coords(UPGRADE_ROAD, pt), start_dir(start_dir) {}
    UpgradeRoad(Serializer& ser);

public:
    void Serialize(Serializer& ser) const override;

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Warenverteilung ändern
class ChangeDistribution : public GameCommand
{
    GC_FRIEND_DECL;
    /// Daten der Distribution (einzelne Prozente der Waren in Gebäuden)
    Distributions data;

protected:
    ChangeDistribution(const Distributions& data) : GameCommand(CHANGE_DISTRIBUTION), data(data) {}
    ChangeDistribution(Serializer& ser) : GameCommand(CHANGE_DISTRIBUTION)
    {
        for(unsigned char& i : data)
            i = ser.PopUnsignedChar();
    }

public:
    void Serialize(Serializer& ser) const override
    {
        GameCommand::Serialize(ser);
        for(unsigned char i : data)
            ser.PushUnsignedChar(i);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Baureihenfolge ändern
class ChangeBuildOrder : public GameCommand
{
    GC_FRIEND_DECL;
    /// Ordnungs-Typ
    const bool useCustomBuildOrder;
    /// Daten der BuildOrder
    BuildOrders data;

protected:
    ChangeBuildOrder(const bool useCustomBuildOrder, const BuildOrders& data)
        : GameCommand(CHANGE_BUILDORDER), useCustomBuildOrder(useCustomBuildOrder), data(data)
    {}
    ChangeBuildOrder(Serializer& ser) : GameCommand(CHANGE_BUILDORDER), useCustomBuildOrder(ser.PopBool())
    {
        for(auto& i : data)
            i = helpers::popEnum<BuildingType>(ser);
    }

public:
    void Serialize(Serializer& ser) const override
    {
        GameCommand::Serialize(ser);
        ser.PushBool(useCustomBuildOrder);
        for(auto i : data)
            helpers::pushEnum<uint8_t>(ser, i);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Baustelle setzen
class SetBuildingSite : public Coords
{
    GC_FRIEND_DECL;
    /// Art des Gebäudes, was gebaut werden soll
    const BuildingType bt;

protected:
    SetBuildingSite(const MapPoint pt, const BuildingType bt) : Coords(SET_BUILDINGSITE, pt), bt(bt) {}
    SetBuildingSite(Serializer& ser) : Coords(SET_BUILDINGSITE, ser), bt(helpers::popEnum<BuildingType>(ser)) {}

public:
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);

        helpers::pushEnum<uint8_t>(ser, bt);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Gebäude zerstören
class DestroyBuilding : public Coords
{
    GC_FRIEND_DECL;

protected:
    DestroyBuilding(const MapPoint pt) : Coords(DESTROY_BUILDING, pt) {}
    DestroyBuilding(Serializer& ser) : Coords(DESTROY_BUILDING, ser) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Send all highest rank soldiers home (used by ai to upgrade troops instead of changing mil settings all the time)
class SendSoldiersHome : public Coords
{
    GC_FRIEND_DECL;

protected:
    SendSoldiersHome(const MapPoint pt) : Coords(SEND_SOLDIERS_HOME, pt) {}
    SendSoldiersHome(Serializer& ser) : Coords(SEND_SOLDIERS_HOME, ser) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// call for new min rank soldiers (used by ai to upgrade troops instead of changing mil settings all the time)
class OrderNewSoldiers : public Coords
{
    GC_FRIEND_DECL;

protected:
    OrderNewSoldiers(const MapPoint pt) : Coords(ORDER_NEW_SOLDIERS, pt) {}
    OrderNewSoldiers(Serializer& ser) : Coords(ORDER_NEW_SOLDIERS, ser) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Transportreihenfolge ändern
class ChangeTransport : public GameCommand
{
    GC_FRIEND_DECL;
    /// Daten der Distribution (einzelne Prozente der Waren in Gebäuden)
    TransportOrders data;

protected:
    ChangeTransport(const TransportOrders& data) : GameCommand(CHANGE_TRANSPORT), data(data) {}
    ChangeTransport(Serializer& ser) : GameCommand(CHANGE_TRANSPORT)
    {
        for(unsigned char& i : data)
            i = ser.PopUnsignedChar();
    }

public:
    void Serialize(Serializer& ser) const override
    {
        GameCommand::Serialize(ser);
        for(unsigned char i : data)
            ser.PushUnsignedChar(i);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Transportreihenfolge ändern
class ChangeMilitary : public GameCommand
{
    GC_FRIEND_DECL;
    /// Daten der Distribution (einzelne Prozente der Waren in Gebäuden)
    MilitarySettings data;

protected:
    ChangeMilitary(const MilitarySettings& data) : GameCommand(CHANGE_MILITARY), data(data) {}
    ChangeMilitary(Serializer& ser) : GameCommand(CHANGE_MILITARY)
    {
        for(unsigned char& i : data)
            i = ser.PopUnsignedChar();
    }

public:
    void Serialize(Serializer& ser) const override
    {
        GameCommand::Serialize(ser);
        for(unsigned char i : data)
            ser.PushUnsignedChar(i);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Werkzeugeinstellungen ändern
class ChangeTools : public GameCommand
{
    GC_FRIEND_DECL;
    /// Daten der Distribution (einzelne Prozente der Waren in Gebäuden)
    ToolSettings data;

    std::array<int8_t, NUM_TOOLS> orders;

protected:
    ChangeTools(const ToolSettings& data, const int8_t* order_delta = nullptr) : GameCommand(CHANGE_TOOLS), data(data)
    {
        if(order_delta != nullptr)
        {
            for(unsigned i = 0; i < NUM_TOOLS; ++i)
                orders[i] = order_delta[i];
        } else
        {
            for(unsigned i = 0; i < NUM_TOOLS; ++i)
                orders[i] = 0;
        }
    }

    ChangeTools(Serializer& ser) : GameCommand(CHANGE_TOOLS)
    {
        for(unsigned char& i : data)
            i = ser.PopUnsignedChar();

        for(unsigned i = 0; i < NUM_TOOLS; ++i)
            orders[i] = ser.PopSignedChar();
    }

public:
    void Serialize(Serializer& ser) const override
    {
        GameCommand::Serialize(ser);
        for(unsigned char i : data)
            ser.PushUnsignedChar(i);

        for(unsigned i = 0; i < NUM_TOOLS; ++i)
            ser.PushSignedChar(orders[i]);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Geologen rufen
class CallSpecialist : public Coords
{
    GC_FRIEND_DECL;
    const Job job;

protected:
    CallSpecialist(const MapPoint pt, Job job) : Coords(CALL_SPECIALIST, pt), job(job) {}
    CallSpecialist(Serializer& ser) : Coords(CALL_SPECIALIST, ser), job(helpers::popEnum<Job>(ser)) {}

public:
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);
        helpers::pushEnum<uint8_t>(ser, job);
    }
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Basisklasse für beide Angriffstypen
class BaseAttack : public Coords
{
    GC_FRIEND_DECL;

protected:
    /// Anzahl der Soldaten
    const uint32_t soldiers_count;
    /// Starke Soldaten oder schwache Soldaten?
    const bool strong_soldiers;

    BaseAttack(const Type gst, const MapPoint pt, const uint32_t soldiers_count, bool strong_soldiers)
        : Coords(gst, pt), soldiers_count(soldiers_count), strong_soldiers(strong_soldiers)
    {}
    BaseAttack(const Type gst, Serializer& ser)
        : Coords(gst, ser), soldiers_count(ser.PopUnsignedInt()), strong_soldiers(ser.PopBool())
    {}

public:
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);

        ser.PushUnsignedInt(soldiers_count);
        ser.PushBool(strong_soldiers);
    }
};

/// Angriff starten
class Attack : public BaseAttack
{
    GC_FRIEND_DECL;

protected:
    Attack(const MapPoint pt, const uint32_t soldiers_count, bool strong_soldiers)
        : BaseAttack(ATTACK, pt, soldiers_count, strong_soldiers)
    {}
    Attack(Serializer& ser) : BaseAttack(ATTACK, ser) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// See-Angriff starten
class SeaAttack : public BaseAttack
{
    GC_FRIEND_DECL;

protected:
    SeaAttack(const MapPoint pt, const uint32_t soldiers_count, bool strong_soldiers)
        : BaseAttack(SEA_ATTACK, pt, soldiers_count, strong_soldiers)
    {}
    SeaAttack(Serializer& ser) : BaseAttack(SEA_ATTACK, ser) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Goldzufuhr in einem Gebäude stoppen/erlauben
class SetCoinsAllowed : public Coords
{
    GC_FRIEND_DECL;
    const bool enabled;

protected:
    SetCoinsAllowed(const MapPoint pt, bool enabled) : Coords(SET_COINS_ALLOWED, pt), enabled(enabled) {}
    SetCoinsAllowed(Serializer& ser) : Coords(SET_COINS_ALLOWED, ser), enabled(ser.PopBool()) {}

public:
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);
        ser.PushBool(enabled);
    }
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Produktivität in einem Gebäude deaktivieren/aktivieren
class SetProductionEnabled : public Coords
{
    GC_FRIEND_DECL;
    const bool enabled;

protected:
    SetProductionEnabled(const MapPoint pt, bool enabled) : Coords(SET_PRODUCTION_ENABLED, pt), enabled(enabled) {}
    SetProductionEnabled(Serializer& ser) : Coords(SET_PRODUCTION_ENABLED, ser), enabled(ser.PopBool()) {}

public:
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);
        ser.PushBool(enabled);
    }
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Produktivität in einem Gebäude deaktivieren/aktivieren
class NotifyAlliesOfLocation : public Coords
{
    GC_FRIEND_DECL;

protected:
    NotifyAlliesOfLocation(const MapPoint pt) : Coords(NOTIFY_ALLIES_OF_LOCATION, pt) {}
    NotifyAlliesOfLocation(Serializer& ser) : Coords(NOTIFY_ALLIES_OF_LOCATION, ser) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Einlagerungseinstellungen von einem Lagerhaus verändern
class SetInventorySetting : public Coords
{
    GC_FRIEND_DECL;
    boost::variant<GoodType, Job> what;
    InventorySetting state;

protected:
    SetInventorySetting(const MapPoint pt, boost::variant<GoodType, Job> what, const InventorySetting state)
        : Coords(SET_INVENTORY_SETTING, pt), what(std::move(what)), state(state)
    {}
    SetInventorySetting(Serializer& ser) : Coords(SET_INVENTORY_SETTING, ser)

    {
        if(ser.PopBool())
            what = helpers::popEnum<Job>(ser);
        else
            what = helpers::popEnum<GoodType>(ser);
        state = static_cast<InventorySetting>(ser.PopUnsignedChar());
    }

public:
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);

        ser.PushBool(holds_alternative<Job>(what));
        boost::apply_visitor([&ser](auto type) { helpers::pushEnum<uint8_t>(ser, type); }, what);
        ser.PushUnsignedChar(static_cast<uint8_t>(state));
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Alle Einlagerungseinstellungen (für alle Menschen oder Waren) von einem Lagerhaus verändern
class SetAllInventorySettings : public Coords
{
    GC_FRIEND_DECL;
    /// Kategorie (Waren, Menschen), Status (Einlagern/Auslagern), type (welche Ware, welcher Mensch)
    const bool isJob;
    std::vector<InventorySetting> states;

protected:
    SetAllInventorySettings(const MapPoint pt, bool isJob, std::vector<InventorySetting> states)
        : Coords(SET_ALL_INVENTORY_SETTINGS, pt), isJob(isJob), states(std::move(states))
    {}
    SetAllInventorySettings(Serializer& ser) : Coords(SET_ALL_INVENTORY_SETTINGS, ser), isJob(ser.PopBool())
    {
        const uint32_t numStates = (isJob ? helpers::NumEnumValues_v<Job> : helpers::NumEnumValues_v<GoodType>);
        states.reserve(numStates);
        for(unsigned i = 0; i < numStates; i++)
            states.push_back(static_cast<InventorySetting>(ser.PopUnsignedChar()));
    }

public:
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);

        ser.PushBool(isJob);
        RTTR_Assert(states.size() == (isJob ? helpers::NumEnumValues_v<Job> : helpers::NumEnumValues_v<GoodType>));
        for(auto state : states)
            ser.PushUnsignedChar(static_cast<uint8_t>(state));
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Verändert die Reserve im HQ auf einen bestimmten Wert
class ChangeReserve : public Coords
{
    GC_FRIEND_DECL;
    /// Rang des Soldaten, der verändert werden soll
    const uint8_t rank;
    /// Anzahl der Reserve für diesen Rang
    const uint32_t count;

protected:
    ChangeReserve(const MapPoint pt, const uint8_t rank, const uint32_t count)
        : Coords(CHANGE_RESERVE, pt), rank(rank), count(count)
    {}
    ChangeReserve(Serializer& ser)
        : Coords(CHANGE_RESERVE, ser), rank(ser.PopUnsignedChar()), count(ser.PopUnsignedInt())
    {}

public:
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);

        ser.PushUnsignedChar(rank);
        ser.PushUnsignedInt(count);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Alle Fahnen zerstören
class CheatArmageddon : public GameCommand
{
    GC_FRIEND_DECL;

protected:
    CheatArmageddon() : GameCommand(CHEAT_ARMAGEDDON) {}
    CheatArmageddon(Serializer& /*ser*/) : GameCommand(CHEAT_ARMAGEDDON) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Aufgeben
class Surrender : public GameCommand
{
    GC_FRIEND_DECL;

protected:
    Surrender() : GameCommand(SURRENDER) {}
    Surrender(Serializer& /*ser*/) : GameCommand(SURRENDER) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Alle eigenen Fahnen zerstören
class DestroyAll : public GameCommand
{
    GC_FRIEND_DECL;

protected:
    DestroyAll() : GameCommand(DESTROY_ALL) {}
    DestroyAll(Serializer& /*ser*/) : GameCommand(DESTROY_ALL) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Unterbreitet anderen Spielern einen Bündnisvertrag
class SuggestPact : public GameCommand
{
    GC_FRIEND_DECL;
    /// Player to whom we suggest the pact
    const uint8_t targetPlayer;
    /// Art des Vertrages
    const PactType pt;
    /// Dauer des Vertrages
    const uint32_t duration;

protected:
    SuggestPact(const uint8_t targetPlayer, const PactType pt, const uint32_t duration)
        : GameCommand(SUGGEST_PACT), targetPlayer(targetPlayer), pt(pt), duration(duration)
    {}
    SuggestPact(Serializer& ser)
        : GameCommand(SUGGEST_PACT), targetPlayer(ser.PopUnsignedChar()), pt(helpers::popEnum<PactType>(ser)),
          duration(ser.PopUnsignedInt())
    {}

public:
    void Serialize(Serializer& ser) const override
    {
        GameCommand::Serialize(ser);
        ser.PushUnsignedChar(targetPlayer);
        helpers::pushEnum<uint8_t>(ser, pt);
        ser.PushUnsignedInt(duration);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Antwortet auf einen Bündnisvorschlag mit Annehmen oder Ablehnung
class AcceptPact : public GameCommand
{
    GC_FRIEND_DECL;
    /// ID des Vertrages
    const uint32_t id;
    /// Art des Vertrages
    const PactType pt;
    /// Player who offered the pact
    const uint8_t fromPlayer;

protected:
    AcceptPact(const uint32_t id, const PactType pt, const uint8_t fromPlayer)
        : GameCommand(ACCEPT_PACT), id(id), pt(pt), fromPlayer(fromPlayer)
    {}
    AcceptPact(Serializer& ser)
        : GameCommand(ACCEPT_PACT), id(ser.PopUnsignedInt()), pt(helpers::popEnum<PactType>(ser)),
          fromPlayer(ser.PopUnsignedChar())
    {}

public:
    void Serialize(Serializer& ser) const override
    {
        GameCommand::Serialize(ser);
        ser.PushUnsignedInt(id);
        helpers::pushEnum<uint8_t>(ser, pt);
        ser.PushUnsignedChar(fromPlayer);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Bündnis abbrechen bzw. das Angebot zurücknehmen, falls dieses schon gestellt wurde
class CancelPact : public GameCommand
{
    GC_FRIEND_DECL;
    /// Vertragsart
    const PactType pt;
    /// Anderen Spieler, den dies noch betrifft
    const uint8_t otherPlayer;

protected:
    CancelPact(const PactType pt, const uint8_t otherPlayer)
        : GameCommand(CANCEL_PACT), pt(pt), otherPlayer(otherPlayer)
    {}
    CancelPact(Serializer& ser)
        : GameCommand(CANCEL_PACT), pt(helpers::popEnum<PactType>(ser)), otherPlayer(ser.PopUnsignedChar())
    {}

public:
    void Serialize(Serializer& ser) const override
    {
        GameCommand::Serialize(ser);
        helpers::pushEnum<uint8_t>(ser, pt);
        ser.PushUnsignedChar(otherPlayer);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

/// Zwischen Boote und Schiffen beim Schiffsbauer hin- und herschalten
class SetShipYardMode : public Coords
{
    GC_FRIEND_DECL;
    const bool buildShips;

protected:
    SetShipYardMode(const MapPoint pt, bool buildShips) : Coords(SET_SHIPYARD_MODE, pt), buildShips(buildShips) {}
    SetShipYardMode(Serializer& ser) : Coords(SET_SHIPYARD_MODE, ser), buildShips(ser.PopBool()) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);
        ser.PushBool(buildShips);
    }
};

/// Expedition starten
class StartStopExpedition : public Coords
{
    GC_FRIEND_DECL;
    const bool start;

protected:
    StartStopExpedition(const MapPoint pt, bool start) : Coords(START_STOP_EXPEDITION, pt), start(start) {}
    StartStopExpedition(Serializer& ser) : Coords(START_STOP_EXPEDITION, ser), start(ser.PopBool()) {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);
        ser.PushBool(start);
    }
};

/// Expedition starten
class StartStopExplorationExpedition : public Coords
{
    GC_FRIEND_DECL;
    const bool start;

protected:
    StartStopExplorationExpedition(const MapPoint pt, bool start)
        : Coords(START_STOP_EXPLORATION_EXPEDITION, pt), start(start)
    {}
    StartStopExplorationExpedition(Serializer& ser)
        : Coords(START_STOP_EXPLORATION_EXPEDITION, ser), start(ser.PopBool())
    {}

public:
    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);
        ser.PushBool(start);
    }
};

/// Wartendes Schiff einer Expedition Befehle geben
class ExpeditionCommand : public GameCommand
{
    GC_FRIEND_DECL;

protected:
    /// Aktion, die ausgeführt wird
    enum class Action : uint8_t
    {
        FOUNDCOLONY,
        CANCELEXPEDITION,
        NORTH,
        NORTHEAST,
        SOUTHEAST,
        SOUTH,
        SOUTHWEST,
        NORTHWEST
    };
    friend constexpr auto maxEnumValue(Action) { return Action::NORTHWEST; }

    ExpeditionCommand(const Action action, const uint32_t ship_id)
        : GameCommand(EXPEDITION_COMMAND), action(action), ship_id(ship_id)
    {}

    ExpeditionCommand(Serializer& ser)
        : GameCommand(EXPEDITION_COMMAND), action(helpers::popEnum<Action>(ser)), ship_id(ser.PopUnsignedInt())
    {}

public:
    void Serialize(Serializer& ser) const override
    {
        GameCommand::Serialize(ser);
        helpers::pushEnum<uint8_t>(ser, action);
        ser.PushUnsignedInt(ship_id);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;

private:
    /// Die Aktion, die ausgeführt werden soll
    Action action;
    /// Schiff, welches dieses Command betrifft
    uint32_t ship_id;
};

/// Send wares or figures to another allied player
class TradeOverLand : public Coords
{
    GC_FRIEND_DECL;
    boost::variant<GoodType, Job> what;
    /// Number of wares/figures we want to trade
    uint32_t count;

protected:
    /// Note: Can only trade wares or figures!
    TradeOverLand(const MapPoint pt, boost::variant<GoodType, Job> what, const uint32_t count)
        : Coords(TRADE, pt), what(std::move(what)), count(count)
    {}
    TradeOverLand(Serializer& ser) : Coords(TRADE, ser)
    {
        if(ser.PopBool())
            what = helpers::popEnum<Job>(ser);
        else
            what = helpers::popEnum<GoodType>(ser);
        count = ser.PopUnsignedInt();
    }

public:
    void Serialize(Serializer& ser) const override
    {
        Coords::Serialize(ser);

        ser.PushBool(holds_alternative<Job>(what));
        boost::apply_visitor([&ser](auto type) { helpers::pushEnum<uint8_t>(ser, type); }, what);
        ser.PushUnsignedInt(count);
    }

    void Execute(GameWorldGame& gwg, uint8_t playerId) override;
};

} // namespace gc
