#ifndef GAME_COMMANDS_H_
#define GAME_COMMANDS_H_

#include "GameCommand.h"
#include "Serializer.h"
#include "gameTypes/MapTypes.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/PactTypes.h"
#include "gameTypes/SettingsTypes.h"
#include "gameTypes/InventorySetting.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/ToolConsts.h"
#include <vector>

class GameWorldGame;
class GamePlayer;

namespace gc{

/// Basisklasse für sämtliche GameCommands mit Koordinaten
    class Coords : public GameCommand
    {
        GC_FRIEND_DECL;
        private: MapPoint PopMapPoint(Serializer& ser)
                 {
                     MapPoint pt;
                     pt.x = ser.PopUnsignedShort();
                     pt.y = ser.PopUnsignedShort();
                     return pt;
                 }
        protected:
            /// Koordinaten auf der Map, die dieses Command betreffen
            const MapPoint pt_;
            Coords(const Type gst, const MapPoint pt)
                : GameCommand(gst), pt_(pt) {}
            Coords(const Type gst, Serializer& ser)
                : GameCommand(gst), pt_(PopMapPoint(ser)){}

        public:
            void Serialize(Serializer& ser) const override
            {
                ser.PushUnsignedShort(pt_.x);
                ser.PushUnsignedShort(pt_.y);
            }

    };

/// Flagge setzen
    class SetFlag : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            SetFlag(const MapPoint pt)
                : Coords(SETFLAG, pt) {}
            SetFlag(Serializer& ser)
                : Coords(SETFLAG, ser) {}
        public:

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Flagge zerstören
    class DestroyFlag : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            DestroyFlag(const MapPoint pt)
                : Coords(DESTROYFLAG, pt) {}
            DestroyFlag(Serializer& ser)
                : Coords(DESTROYFLAG, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Straße bauen
    class BuildRoad : public Coords
    {
        GC_FRIEND_DECL;
            /// Boot-Straße oder nicht?
            const bool boat_road;
            /// Beschreibung der Straße mittels einem Array aus Richtungen
            std::vector<unsigned char> route;
        protected:
            BuildRoad(const MapPoint pt, const bool boat_road, const std::vector<unsigned char>& route)
                : Coords(BUILDROAD, pt), boat_road(boat_road), route(route) {}
            BuildRoad(Serializer& ser)
                : Coords(BUILDROAD, ser),
                  boat_road(ser.PopBool()),
                  route(ser.PopUnsignedInt())
            {
                for(unsigned i = 0; i < route.size(); ++i)
                    route[i] = ser.PopUnsignedChar();
            }
        public:

            void Serialize(Serializer& ser) const override
            {
                Coords::Serialize(ser);

                ser.PushBool(boat_road);
                ser.PushUnsignedInt(route.size());
                for(unsigned i = 0; i < route.size(); ++i)
                    ser.PushUnsignedChar(route[i]);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Straße zerstören
    class DestroyRoad : public Coords
    {
        GC_FRIEND_DECL;
            /// Richtung in der von der Flagge an x;y aus gesehen die Straße zerstört werden soll
            const unsigned char start_dir;
        protected:
            DestroyRoad(const MapPoint pt, const unsigned char start_dir)
                : Coords(DESTROYROAD, pt), start_dir(start_dir) {}
            DestroyRoad(Serializer& ser)
                : Coords(DESTROYROAD, ser),
                  start_dir(ser.PopUnsignedChar()) {}
        public:
            void Serialize(Serializer& ser) const override
            {
                Coords::Serialize(ser);

                ser.PushUnsignedChar(start_dir);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Straße aufwerten
    class UpgradeRoad : public Coords
    {
        GC_FRIEND_DECL;
            /// Richtung in der von der Flagge an x;y aus gesehen die Straße zerstört werden soll
            const unsigned char start_dir;
        protected:
            UpgradeRoad(const MapPoint pt, const unsigned char start_dir)
                : Coords(UPGRADEROAD, pt), start_dir(start_dir) {}
            UpgradeRoad(Serializer& ser)
                : Coords(UPGRADEROAD, ser), start_dir(ser.PopUnsignedChar()) {}
        public:
            void Serialize(Serializer& ser) const override
            {
                Coords::Serialize(ser);
                ser.PushUnsignedChar(start_dir);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Warenverteilung ändern
    class ChangeDistribution : public GameCommand
    {
        GC_FRIEND_DECL;
            /// Daten der Distribution (einzelne Prozente der Waren in Gebäuden)
            Distributions data;
        protected:
            ChangeDistribution(const Distributions& data)
                : GameCommand(CHANGEDISTRIBUTION), data(data) {}
            ChangeDistribution(Serializer& ser)
                : GameCommand(CHANGEDISTRIBUTION)
            {
                for(unsigned i = 0; i < data.size(); ++i)
                    data[i] = ser.PopUnsignedChar();
            }
        public:
            void Serialize(Serializer& ser) const override
            {
                for(unsigned i = 0; i < data.size(); ++i)
                    ser.PushUnsignedChar(data[i]);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
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
                : GameCommand(CHANGEBUILDORDER), useCustomBuildOrder(useCustomBuildOrder), data(data) {}
            ChangeBuildOrder(Serializer& ser)
                : GameCommand(CHANGEBUILDORDER), useCustomBuildOrder(ser.PopBool())
            {
                for(unsigned i = 0; i < data.size(); ++i)
                    data[i] = BuildingType(ser.PopUnsignedChar());
            }
        public:
            void Serialize(Serializer& ser) const override
            {
                ser.PushBool(useCustomBuildOrder);
                for(unsigned i = 0; i < data.size(); ++i)
                    ser.PushUnsignedChar(data[i]);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };


/// Baustelle setzen
    class SetBuildingSite : public Coords
    {
        GC_FRIEND_DECL;
            /// Art des Gebäudes, was gebaut werden soll
            const BuildingType bt;
        protected:
            SetBuildingSite(const MapPoint pt, const BuildingType bt)
                : Coords(SETBUILDINGSITE, pt), bt(bt) {}
            SetBuildingSite(Serializer& ser)
                : Coords(SETBUILDINGSITE, ser),
                  bt(BuildingType(ser.PopUnsignedChar())) {}
        public:
            void Serialize(Serializer& ser) const override
            {
                Coords::Serialize(ser);

                ser.PushUnsignedChar(static_cast<unsigned char>(bt));
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Gebäude zerstören
    class DestroyBuilding : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            DestroyBuilding(const MapPoint pt)
                : Coords(DESTROYBUILDING, pt) {}
            DestroyBuilding(Serializer& ser)
                : Coords(DESTROYBUILDING, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Send all highest rank soldiers home (used by ai to upgrade troops instead of changing mil settings all the time)
    class SendSoldiersHome : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            SendSoldiersHome(const MapPoint pt)
                : Coords(SENDSOLDIERSHOME, pt) {}
            SendSoldiersHome(Serializer& ser)
                : Coords(SENDSOLDIERSHOME, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// call for new min rank soldiers (used by ai to upgrade troops instead of changing mil settings all the time)
    class OrderNewSoldiers : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            OrderNewSoldiers(const MapPoint pt)
                : Coords(ORDERNEWSOLDIERS, pt) {}
            OrderNewSoldiers(Serializer& ser)
                : Coords(ORDERNEWSOLDIERS, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };


/// Transportreihenfolge ändern
    class ChangeTransport : public GameCommand
    {
        GC_FRIEND_DECL;
            /// Daten der Distribution (einzelne Prozente der Waren in Gebäuden)
            TransportOrders data;
        protected:
            ChangeTransport(const TransportOrders& data)
                : GameCommand(CHANGETRANSPORT), data(data) {}
            ChangeTransport(Serializer& ser)
                : GameCommand(CHANGETRANSPORT)
            {
                for(unsigned i = 0; i < data.size(); ++i)
                    data[i] = ser.PopUnsignedChar();
            }
        public:
            void Serialize(Serializer& ser) const override
            {
                for(unsigned i = 0; i < data.size(); ++i)
                    ser.PushUnsignedChar(data[i]);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Transportreihenfolge ändern
    class ChangeMilitary : public GameCommand
    {
        GC_FRIEND_DECL;
            /// Daten der Distribution (einzelne Prozente der Waren in Gebäuden)
            MilitarySettings data;
        protected:
            ChangeMilitary(const MilitarySettings& data)
                : GameCommand(CHANGEMILITARY), data(data) {}
            ChangeMilitary(Serializer& ser)
                : GameCommand(CHANGEMILITARY)
            {
                for(unsigned i = 0; i < data.size(); ++i)
                    data[i] = ser.PopUnsignedChar();
            }
        public:
            void Serialize(Serializer& ser) const override
            {
                for(unsigned i = 0; i < data.size(); ++i)
                    ser.PushUnsignedChar(data[i]);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Werkzeugeinstellungen ändern
    class ChangeTools : public GameCommand
    {
        GC_FRIEND_DECL;
            /// Daten der Distribution (einzelne Prozente der Waren in Gebäuden)
            ToolSettings data;

            boost::array<signed char, TOOL_COUNT> orders;
        protected:
            ChangeTools(const ToolSettings& data, const signed char* order_delta = 0)
                : GameCommand(CHANGETOOLS), data(data)
            {
                if (order_delta != 0)
                {
                    for (unsigned i = 0; i < TOOL_COUNT; ++i)
                        orders[i] = order_delta[i];
                }
                else
                {
                    for (unsigned i = 0; i < TOOL_COUNT; ++i)
                        orders[i] = 0;
                }
            }

            ChangeTools(Serializer& ser)
                : GameCommand(CHANGETOOLS)
            {
                for(unsigned i = 0; i < data.size(); ++i)
                    data[i] = ser.PopUnsignedChar();

                for (unsigned i = 0; i < TOOL_COUNT; ++i)
                    orders[i] = ser.PopSignedChar();
            }
        public:
            void Serialize(Serializer& ser) const override
            {
                for(unsigned i = 0; i < data.size(); ++i)
                    ser.PushUnsignedChar(data[i]);

                for (unsigned i = 0; i < TOOL_COUNT; ++i)
                    ser.PushSignedChar(orders[i]);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Geologen rufen
    class CallGeologist : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            CallGeologist(const MapPoint pt)
                : Coords(CALLGEOLOGIST, pt) {}
            CallGeologist(Serializer& ser)
                : Coords(CALLGEOLOGIST, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Späher rufen
    class CallScout : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            CallScout(const MapPoint pt)
                : Coords(CALLSCOUT, pt) {}
            CallScout(Serializer& ser)
                : Coords(CALLSCOUT, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Basisklasse für beide Angriffstypen
    class BaseAttack : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            /// Anzahl der Soldaten
            const unsigned soldiers_count;
            /// Starke Soldaten oder schwache Soldaten?
            const bool strong_soldiers;

        protected:
            BaseAttack(const Type gst, const MapPoint pt, const unsigned soldiers_count, const bool strong_soldiers)
                : Coords(gst, pt), soldiers_count(soldiers_count), strong_soldiers(strong_soldiers) {}
            BaseAttack(const Type gst, Serializer& ser)
                : Coords(gst, ser),
                  soldiers_count(ser.PopUnsignedInt()), strong_soldiers(ser.PopBool()) {}
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
            Attack(const MapPoint pt, const unsigned soldiers_count, const bool strong_soldiers)
                : BaseAttack(ATTACK, pt, soldiers_count, strong_soldiers) {}
            Attack(Serializer& ser)
                : BaseAttack(ATTACK, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// See-Angriff starten
    class SeaAttack : public BaseAttack
    {
        GC_FRIEND_DECL;
        protected:
            SeaAttack(const MapPoint pt, const unsigned soldiers_count, const bool strong_soldiers)
                : BaseAttack(SEAATTACK, pt, soldiers_count, strong_soldiers) {}
            SeaAttack(Serializer& ser)
                : BaseAttack(SEAATTACK, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Goldzufuhr in einem Gebäude stoppen/erlauben
    class SetCoinsAllowed : public Coords
    {
        GC_FRIEND_DECL;
        const bool enabled;
        protected:
            SetCoinsAllowed(const MapPoint pt, const bool enabled): Coords(SET_COINS_ALLOWED, pt), enabled(enabled) {}
            SetCoinsAllowed(Serializer& ser): Coords(SET_COINS_ALLOWED, ser), enabled(ser.PopBool()) {}
        public:
            void Serialize(Serializer& ser) const override
            {
                Coords::Serialize(ser);
                ser.PushBool(enabled);
            }
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Produktivität in einem Gebäude deaktivieren/aktivieren
    class SetProductionEnabled : public Coords
    {
        GC_FRIEND_DECL;
        const bool enabled;
        protected:
            SetProductionEnabled(const MapPoint pt, const bool enabled): Coords(SET_PRODUCTION_ENABLED, pt), enabled(enabled) {}
            SetProductionEnabled(Serializer& ser): Coords(SET_PRODUCTION_ENABLED, ser), enabled(ser.PopBool()) {}
        public:
            void Serialize(Serializer& ser) const override
            {
                Coords::Serialize(ser);
                ser.PushBool(enabled);
            }
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Produktivität in einem Gebäude deaktivieren/aktivieren
    class NotifyAlliesOfLocation : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            NotifyAlliesOfLocation(const MapPoint pt)
                : Coords(NOTIFYALLIESOFLOCATION, pt) {}
            NotifyAlliesOfLocation(Serializer& ser)
                : Coords(NOTIFYALLIESOFLOCATION, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Einlagerungseinstellungen von einem Lagerhaus verändern
    class SetInventorySetting : public Coords
    {
        GC_FRIEND_DECL;
            /// Kategorie (Waren, Menschen), Status (Einlagern/Auslagern), type (welche Ware, welcher Mensch)
            const bool isJob;
            const unsigned char type;
            const InventorySetting state;
        protected:
            SetInventorySetting(const MapPoint pt, const bool isJob, const unsigned char type, const InventorySetting state)
                : Coords(SET_INVENTORY_SETTING, pt), isJob(isJob), type(type), state(state) {}
            SetInventorySetting(Serializer& ser)
                : Coords(SET_INVENTORY_SETTING, ser),
                  isJob(ser.PopBool()),
                  type(ser.PopUnsignedChar()),
                  state(static_cast<InventorySetting>(ser.PopUnsignedChar()))
                {}
        public:
            void Serialize(Serializer& ser) const override
            {
                Coords::Serialize(ser);

                ser.PushBool(isJob);
                ser.PushUnsignedChar(type);
                ser.PushUnsignedChar(state.ToUnsignedChar());
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Alle Einlagerungseinstellungen (für alle Menschen oder Waren) von einem Lagerhaus verändern
    class SetAllInventorySettings : public Coords
    {
        GC_FRIEND_DECL;
            /// Kategorie (Waren, Menschen), Status (Einlagern/Auslagern), type (welche Ware, welcher Mensch)
            const bool isJob;
            std::vector<InventorySetting> states;
        protected:
            SetAllInventorySettings(const MapPoint pt, const bool isJob, const std::vector<InventorySetting>& states)
                : Coords(SET_ALL_INVENTORY_SETTINGS, pt), isJob(isJob), states(states) {}
            SetAllInventorySettings(Serializer& ser): Coords(SET_ALL_INVENTORY_SETTINGS, ser),
                isJob(ser.PopBool())
            {
                const unsigned numStates = (isJob ? JOB_TYPES_COUNT : WARE_TYPES_COUNT);
                states.reserve(numStates);
                for(unsigned i = 0; i < numStates; i++)
                    states.push_back(static_cast<InventorySetting>(ser.PopUnsignedChar()));
            }
        public:
            void Serialize(Serializer& ser) const override
            {
                Coords::Serialize(ser);

                ser.PushBool(isJob);
                RTTR_Assert(states.size() == (isJob ? JOB_TYPES_COUNT : WARE_TYPES_COUNT));
                for(std::vector<InventorySetting>::const_iterator it = states.begin(); it != states.end(); ++it)
                    ser.PushUnsignedChar(it->ToUnsignedChar());
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Verändert die Reserve im HQ auf einen bestimmten Wert
    class ChangeReserve : public Coords
    {
        GC_FRIEND_DECL;
            /// Rang des Soldaten, der verändert werden soll
            const unsigned char rank;
            /// Anzahl der Reserve für diesen Rang
            const unsigned char count;
        protected:
            ChangeReserve(const MapPoint pt, const unsigned char rank, const unsigned char count)
                : Coords(CHANGERESERVE, pt), rank(rank), count(count) {}
            ChangeReserve(Serializer& ser)
                : Coords(CHANGERESERVE, ser),
                  rank(ser.PopUnsignedChar()),
                  count(ser.PopUnsignedChar())
            {}
        public:
            void Serialize(Serializer& ser) const override
            {
                Coords::Serialize(ser);

                ser.PushUnsignedChar(rank);
                ser.PushUnsignedChar(count);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Alle Fahnen zerstören
    class CheatArmageddon : public GameCommand
    {
        GC_FRIEND_DECL;
        protected:
            CheatArmageddon()
                : GameCommand(CHEAT_ARMAGEDDON) {}
            CheatArmageddon(Serializer&  /*ser*/)
                : GameCommand(CHEAT_ARMAGEDDON) {}
        public:
            void Serialize(Serializer&  /*ser*/) const override
            {}

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Aufgeben
    class Surrender : public GameCommand
    {
        GC_FRIEND_DECL;
        protected:
            Surrender()
                : GameCommand(SURRENDER) {}
            Surrender(Serializer&  /*ser*/)
                : GameCommand(SURRENDER) {}
        public:
            void Serialize(Serializer&  /*ser*/) const override
            {}

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Alle eigenen Fahnen zerstören
    class DestroyAll : public GameCommand
    {
        GC_FRIEND_DECL;
        protected:
            DestroyAll()
                : GameCommand(DESTROYALL) {}
            DestroyAll(Serializer&  /*ser*/)
                : GameCommand(DESTROYALL) {}
        public:
            void Serialize(Serializer&  /*ser*/) const override
            {}

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Unterbreitet anderen Spielern einen Bündnisvertrag
    class SuggestPact : public GameCommand
    {
        GC_FRIEND_DECL;
            /// Player to whom we suggest the pact
            const unsigned char targetPlayer;
            /// Art des Vertrages
            const PactType pt;
            /// Dauer des Vertrages
            const unsigned duration;

        protected:
            SuggestPact(const unsigned char targetPlayer, const PactType pt, const unsigned duration) : GameCommand(SUGGESTPACT),
                targetPlayer(targetPlayer), pt(pt), duration(duration) {}
            SuggestPact(Serializer& ser) : GameCommand(SUGGESTPACT),
                targetPlayer(ser.PopUnsignedChar()), pt(PactType(ser.PopUnsignedChar())), duration(ser.PopUnsignedInt()) {}
        public:

            void Serialize(Serializer& ser) const override
            {
                ser.PushUnsignedChar(targetPlayer);
                ser.PushUnsignedChar(static_cast<unsigned char>(pt));
                ser.PushUnsignedInt(duration);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };


/// Antwortet auf einen Bündnisvorschlag mit Annehmen oder Ablehnung
    class AcceptPact : public GameCommand
    {
        GC_FRIEND_DECL;
            /// ID des Vertrages
            const unsigned id;
            /// Art des Vertrages
            const PactType pt;
            /// Player who offered the pact
            const unsigned char fromPlayer;

        protected:
            AcceptPact(const unsigned id, const PactType pt, const unsigned char fromPlayer) : GameCommand(ACCEPTPACT),
                id(id), pt(pt), fromPlayer(fromPlayer) {}
            AcceptPact(Serializer& ser) : GameCommand(ACCEPTPACT),
                id(ser.PopUnsignedInt()), pt(PactType(ser.PopUnsignedChar())), fromPlayer(ser.PopUnsignedChar()) {}
        public:

            void Serialize(Serializer& ser) const override
            {
                ser.PushUnsignedInt(id);
                ser.PushUnsignedChar(static_cast<unsigned char>(pt));
                ser.PushUnsignedChar(fromPlayer);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };


/// Bündnis abbrechen bzw. das Angebot zurücknehmen, falls dieses schon gestellt wurde
    class CancelPact : public GameCommand
    {
        GC_FRIEND_DECL;
            /// Vertragsart
            const PactType pt;
            /// Anderen Spieler, den dies noch betrifft
            const unsigned char player;

        protected:
            CancelPact(const PactType pt, const unsigned char player) : GameCommand(CANCELPACT),
                pt(pt), player(player) {}
            CancelPact(Serializer& ser) : GameCommand(CANCELPACT),
                pt(PactType(ser.PopUnsignedChar())), player(ser.PopUnsignedChar()) {}
        public:

            void Serialize(Serializer& ser) const override
            {
                ser.PushUnsignedChar(static_cast<unsigned char>(pt));
                ser.PushUnsignedChar(player);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Zwischen Boote und Schiffen beim Schiffsbauer hin- und herschalten
    class ToggleShipYardMode : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            ToggleShipYardMode(const MapPoint pt)
                : Coords(TOGGLESHIPYARDMODE, pt) {}
            ToggleShipYardMode(Serializer& ser)
                : Coords(TOGGLESHIPYARDMODE, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Expedition starten
    class StartExpedition : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            StartExpedition(const MapPoint pt)
                : Coords(STARTEXPEDITION, pt) {}
            StartExpedition(Serializer& ser)
                : Coords(STARTEXPEDITION, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Expedition starten
    class StartExplorationExpedition : public Coords
    {
        GC_FRIEND_DECL;
        protected:
            StartExplorationExpedition(const MapPoint pt)
                : Coords(STARTEXPLORATIONEXPEDITION, pt) {}
            StartExplorationExpedition(Serializer& ser)
                : Coords(STARTEXPLORATIONEXPEDITION, ser) {}
        public:
            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };

/// Wartendes Schiff einer Expedition Befehle geben
    class ExpeditionCommand : public GameCommand
    {
        GC_FRIEND_DECL;
        protected:

            /// Aktion, die ausgeführt wird
            enum Action
            {
                FOUNDCOLONY = 0,
                CANCELEXPEDITION,
                NORTH,
                NORTHEAST,
                SOUTHEAST,
                SOUTH,
                SOUTHWEST,
                NORTHWEST
            };

            ExpeditionCommand(const Action action, const unsigned ship_id)
                : GameCommand(EXPEDITION_COMMAND), action(action), ship_id(ship_id) {}

            ExpeditionCommand(Serializer& ser)
                : GameCommand(EXPEDITION_COMMAND),
                  action(Action(ser.PopUnsignedChar())),
                  ship_id(ser.PopUnsignedInt()) {}
        public:
            void Serialize(Serializer& ser) const override
            {
                ser.PushUnsignedChar(static_cast<unsigned char>(action));
                ser.PushUnsignedInt(ship_id);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;

        private:
            /// Die Aktion, die ausgeführt werden soll
            Action action;
            /// Schiff, welches dieses Command betrifft
            unsigned ship_id;
    };


    /// Send wares or figures to another allied player
    class TradeOverLand : public Coords
    {
        GC_FRIEND_DECL;
            GoodType gt;
            Job job;
            /// Number of wares/figures we want to trade
            unsigned count;

        protected:
            /// Note: Can only trade wares or figures!
            TradeOverLand(const MapPoint pt, const GoodType gt, const Job job, const unsigned count)
                : Coords(TRADEOVERLAND, pt), gt(gt), job(job), count(count)
            {
                RTTR_Assert((gt == GD_NOTHING) != (job == JOB_NOTHING));
            }
            TradeOverLand(Serializer& ser)
                : Coords(TRADEOVERLAND, ser),
                  gt(GoodType(ser.PopUnsignedChar())),
                  job(Job(ser.PopUnsignedChar())),
                  count(ser.PopUnsignedInt())
            {}
        public:
            void Serialize(Serializer& ser) const override
            {
                Coords::Serialize(ser);

                ser.PushUnsignedChar(static_cast<unsigned char>(gt));
                ser.PushUnsignedChar(static_cast<unsigned char>(job));
                ser.PushUnsignedInt(count);
            }

            /// Führt das GameCommand aus
            void Execute(GameWorldGame& gwg, unsigned char playerId) override;
    };


}

#endif



