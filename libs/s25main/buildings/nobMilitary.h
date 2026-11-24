// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/nofSoldier.h"
#include "nobBaseMilitary.h"
#include <boost/container/flat_set.hpp>
#include <list>
#include <vector>

class GameEvent;
class nobChurch;
class nobHarborBuilding;
class nofActiveSoldier;
class nofAggressiveDefender;
class nofAttacker;
class nofChurchVisitor;
class nofDefender;
class noFigure;
class nofPassiveSoldier;
class SerializedGameData;
class Ware;

/// Distance to the next enemy border
enum class FrontierDistance : uint8_t
{
    Far,    /// next military building is far away
    Mid,    /// Next military building is in reachable range
    Harbor, /// Military building is near a harbor
    Near    /// Military building is next to a border
};
constexpr auto maxEnumValue(FrontierDistance)
{
    return FrontierDistance::Near;
}

/// Stellt ein Militärgebäude beliebiger Größe (also von Baracke bis Festung) dar
class nobMilitary : public nobBaseMilitary
{
    using OwnedSortedTroops = boost::container::flat_set<std::unique_ptr<nofPassiveSoldier>, ComparatorSoldiersByRank>;
    using SortedTroops = boost::container::flat_set<nofPassiveSoldier*, ComparatorSoldiersByRank>;

private:
    /// wurde das Gebäude gerade neu gebaut (muss also die Landgrenze beim Eintreffen von einem Soldaten neu berechnet
    /// werden?)
    bool new_built;
    /// Anzahl der Goldmünzen im Gebäude
    unsigned char numCoins;
    /// Gibt an, ob Goldmünzen gesperrt worden (letzteres nur visuell, um Netzwerk-Latenzen zu verstecken)
    bool coinsDisabled, coinsDisabledVirtual;
    unsigned char numArmor;
    bool armorAllowed, armorAllowedVirtual;
    /// Entfernung zur freindlichen Grenze (woraus sich dann die Besatzung ergibt) von 0-3, 0 fern, 3 nah, 2 Hafen!
    FrontierDistance frontier_distance;
    /// Größe bzw Typ des Militärgebäudes (0 = Baracke, 3 = Festung)
    unsigned char size;
    /// Bestellte Soldaten
    SortedTroops ordered_troops;
    /// Bestellter Goldmünzen
    std::list<Ware*> ordered_coins;
    std::list<Ware*> ordered_armor;
    /// Gibt an, ob gerade die Eroberer in das Gebäude gehen (und es so nicht angegegriffen werden sollte)
    bool capturing;
    /// Anzahl der Soldaten, die das Militärgebäude gerade noch einnehmen
    unsigned capturing_soldiers;
    /// List of soldiers who are on the way to capture the military building
    /// but who are still quite far away (didn't stand around the building)
    std::list<nofAttacker*> far_away_capturers;
    /// Gold-Bestell-Event
    const GameEvent* gold_order_event;
    const GameEvent* armor_order_event;
    /// Beförderung-Event
    const GameEvent* upgrade_event;
    const GameEvent* armor_upgrade_event;
    /// Is the military building regulating its troops at the moment? (then block furthere RegulateTroop calls)
    bool is_regulating_troops;
    /// Soldatenbesatzung
    OwnedSortedTroops troops;
    std::array<uint8_t, NUM_SOLDIER_RANKS> troop_limits;
    /// Soldiers currently out attending a church service
    std::list<nofChurchVisitor*> churchVisitors_;

    /// Bestellungen (sowohl Truppen als auch Goldmünzen) zurücknehmen
    void cancelOrders();
    /// Wählt je nach Militäreinstellungen (Verteidigerstärke) einen passenden Soldaten aus
    nofPassiveSoldier* chooseSoldier();
    /// Stellt Verteidiger zur Verfügung
    std::unique_ptr<nofDefender> provideDefender(nofAttacker& attacker) override;
    void figureLeft(const noFigure& /*fig*/) override {}
    /// Will/kann das Gebäude noch Münzen bekommen?
    bool wantCoins() const;
    bool wantArmor() const;
    /// Prüft, ob Goldmünzen und Soldaten, die befördert werden können, vorhanden sind und meldet ggf. ein
    /// Beförderungsevent an
    void prepareUpgrading();
    void prepareArmorUpgrading();
    /// Gets the total amount of soldiers (ordered, stationed, on mission)
    size_t getTotalSoldiers() const;
    std::array<unsigned, NUM_SOLDIER_RANKS> getTotalSoldiersByRank() const;
    /// Looks for the next far-away-capturer waiting around and calls it to the flag
    void callNextFarAwayCapturer(nofAttacker& attacker);

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobMilitary(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    nobMilitary(SerializedGameData& sgd, unsigned obj_id);

public:
    ~nobMilitary() override;

protected:
    void DestroyBuilding() override;

public:
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NobMilitary; }

    void draw(DrawPoint drawPt) override;
    void handleEvent(unsigned id) override;

    /// Wurde das Militärgebäude neu gebaut und noch nicht besetzt und kann somit abgerissen werden bei Land-verlust?
    bool isNewBuilt() const { return new_built; }

    /// Liefert Militärradius des Gebäudes
    unsigned getMilitaryRadius() const override;
    unsigned getMaxCoinCt() const;
    unsigned getMaxArmorCt() const;
    unsigned getMaxTroopsCt() const;

    /// Sucht feindliche Miitärgebäude im Umkreis und setzt die frontier_distance entsprechend (sowohl selber als
    /// auch von den feindlichen Gebäuden) und bestellt somit ggf. neue Soldaten, exception wird nicht mit einbezogen
    void lookForEnemyBuildings(const nobBaseMilitary* exception = nullptr);

    /// Wird von gegnerischem Gebäude aufgerufen, wenn sie neu gebaut worden sind und es so ein neues Gebäude im Umkreis
    /// gibt setzt frontier_distance neu falls möglich und sendet ggf. Verstärkung
    void newEnemyMilitaryBuilding(FrontierDistance distance);
    bool isUseless() const;
    bool isAttackable(unsigned playerIdx) const override;
    /// Gibt Distanz zurück
    FrontierDistance getFrontierDistance() const { return frontier_distance; }

    /// Berechnet die gewünschte Besatzung je nach Grenznähe
    unsigned calcRequiredNumTroops() const;
    /// Calculate the required troop count for the given setting
    unsigned calcRequiredNumTroops(FrontierDistance assumedFrontierDistance, unsigned settingValue) const;
    /// Reguliert die Besatzung des Gebäudes je nach Grenznähe, bestellt neue Soldaten und schickt überflüssige raus
    void regulateTroops();
    /// Gibt aktuelle Besetzung zurück
    unsigned getNumTroops() const { return troops.size(); }
    auto getTroops() const { return helpers::nonNullPtrSpan(troops); }
    bool isInTroops(const nofPassiveSoldier& soldier) const;

    /// Get/Set the maximum number of soldiers of rank `rank` allowed in this building
    unsigned getTroopLimit(const unsigned rank) const { return troop_limits[rank]; }
    void SetTroopLimit(unsigned rank, unsigned limit);

    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (in dem Fall nur Goldstücke)
    void takeWare(Ware* ware) override;
    /// Legt eine Ware am Objekt ab (an allen Straßenknoten (Gebäude, Baustellen und Flaggen) kann man Waren ablegen
    void addWare(std::unique_ptr<Ware> ware) override;
    /// Eine bestellte Ware konnte doch nicht kommen
    void wareLost(Ware& ware) override;
    /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
    bool freePlaceAtFlag() override;

    /// Berechnet, wie dringend eine Goldmünze gebraucht wird, in Punkten, je höher desto dringender
    unsigned calcCoinsPoints() const;
    unsigned calcArmorPoints() const;

    /// Wird aufgerufen, wenn ein Soldat kommt
    void gotWorker(Job job, noFigure& worker) override;
    /// Fügt aktiven Soldaten (der aus von einer Mission) zum Militärgebäude hinzu
    void addActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier) override;
    /// Fügt passiven Soldaten (der aus einem Lagerhaus kommt) zum Militärgebäude hinzu
    void addPassiveSoldier(std::unique_ptr<nofPassiveSoldier> soldier);
    /// Soldat konnte nicht kommen
    void soldierLost(nofSoldier* soldier) override;
    /// Send the given passive soldier from this building to attack the goal, optionally via the given harbor
    void sendAttacker(nofPassiveSoldier*& passiveSoldier, nobBaseMilitary& goal,
                      const nobHarborBuilding* harbor = nullptr);

    /// Schickt einen Verteidiger raus, der einem Angreifer in den Weg rennt
    nofAggressiveDefender* sendAggressiveDefender(nofAttacker& attacker) override;

    /// Gibt die Anzahl der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
    unsigned getNumSoldiersForAttack(MapPoint dest) const;
    /// Gibt die Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
    std::vector<nofPassiveSoldier*> getSoldiersForAttack(MapPoint dest) const;
    /// Gibt die Stärke der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
    unsigned getSoldiersStrengthForAttack(MapPoint dest, unsigned& soldiers_count) const;
    /// Gibt die Stärke eines Militärgebäudes zurück
    unsigned getSoldiersStrength() const;

    /// Gebäude wird vom Gegner eingenommen, player ist die neue Spieler-ID
    void capture(unsigned char new_owner);
    /// Das Gebäude wurde bereits eingenommen, hier wird geprüft, ob noch weitere Soldaten für die Besetzung
    /// notwendig sind, wenn ja wird ein neuer Soldat gerufen, wenn nein, werden alle restlichen nach Hause geschickt
    void needOccupyingTroops();
    /// Sagt dem Gebäude schonmal, dass es eingenommen wird, wenn er erste Eroberer gerade in das Gebäude reinläuft
    /// (also noch bevor er drinnen ist!) - damit da nicht zusätzliche Soldaten reinlaufen
    void prepareCapturing()
    {
        RTTR_Assert(!isBeingCaptured());
        capturing = true;
        ++capturing_soldiers;
    }

    /// Wird das Gebäude gerade eingenommen?
    bool isBeingCaptured() const { return capturing; }
    /// Gebäude wird nicht mehr eingenommen (falls anderer Soldat zuvor reingekommen ist beim Einnehmen)
    void stopCapturing()
    {
        capturing_soldiers = 0;
        capturing = false;
    }
    /// Sagt, dass ein erobernder Soldat das Militärgebäude erreicht hat
    void capturingSoldierArrived();
    /// A far-away capturer arrived at the flag and starts the capturing or is waiting around it
    void farAwayCapturerReachedGoal(nofAttacker& attacker, bool walkingIntoBld);

    bool isFarAwayCapturer(const nofAttacker& attacker);

    /// Stoppt/Erlaubt Goldzufuhr (visuell)
    void toggleCoinsVirtual() { coinsDisabledVirtual = !coinsDisabledVirtual; }
    /// Stoppt/Erlaubt Goldzufuhr (real)
    void SetCoinsAllowed(bool enabled);
    /// Fragt ab, ob Goldzufuhr ausgeschaltet ist (visuell)
    bool isGoldDisabledVirtual() const { return coinsDisabledVirtual; }
    /// Fragt ab, ob Goldzufuhr ausgeschaltet ist (real)
    bool isGoldDisabled() const { return coinsDisabled; }
    unsigned char getNumCoins() const { return numCoins; }

    void toggleArmorVirtual() { armorAllowedVirtual = !armorAllowedVirtual; }
    void SetArmorAllowed(bool enabled);
    bool isArmorAllowedVirtual() const { return armorAllowedVirtual; }
    bool isArmorAllowed() const { return armorAllowed; }
    unsigned char getNumArmor() const { return numArmor; }

    /// is there a max rank soldier in the building?
    bool hasMaxRankSoldier() const;

    /// Sucht sämtliche Lagerhäuser nach Goldmünzen ab und bestellt ggf. eine, falls eine gebraucht wird
    void searchCoins();
    void searchArmor();

    /// Gebäude wird von einem Katapultstein getroffen
    void hitOfCatapultStone();

    /// Sind noch Truppen drinne, die dieses Gebäude verteidigen können
    bool defendersAvailable() const override { return getNumTroops() > 0; }

    /// Darf das Militärgebäude abgerissen werden (Abriss-Verbot berücksichtigen)?
    bool isDemolitionAllowed() const;

    void unlinkAggressor(nofAttacker& soldier) override;

    /// Send up to 20% of stationed soldiers (always keeping at least 1) to attend a church service
    std::vector<nofChurchVisitor*> sendChurchVisitors(nobChurch& church);
    /// Return a soldier back to the building after church service
    void returnFromChurch(nofChurchVisitor& visitor, std::unique_ptr<nofPassiveSoldier> soldier);
    /// Detach a visitor that did not return (abrogated road, died, or church destroyed)
    void churchVisitorLost(nofChurchVisitor& visitor);
};
