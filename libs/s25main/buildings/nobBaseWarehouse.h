// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DataChangedObservable.h"
#include "nobBaseMilitary.h"
#include "variant.h"
#include "gameTypes/GoodsAndPeopleArray.h"
#include "gameTypes/InventorySetting.h"
#include "gameTypes/VirtualInventory.h"
#include <array>
#include <list>
#include <memory>

class nofCarrier;
class noFigure;
class Ware;
class nobMilitary;
class TradeRoute;

class RoadSegment;
class SerializedGameData;
class noBaseBuilding;
class noRoadNode;
class nofActiveSoldier;
class nofAggressiveDefender;
class nofAttacker;
class nofDefender;
class nofSoldier;
class GameEvent;

namespace gc {
class SetInventorySetting;
class SetAllInventorySettings;
} // namespace gc

/// Ein/Auslagereinstellungsstruktur
struct InventorySettings : GoodsAndPeopleArray<InventorySetting>
{};

/// Grundlegende Warenhausklasse, die alle Funktionen vereint, die für Warenhäuser (HQ, Lagerhaus, Häfen) wichtig sind.
/// Change events: 1=InventorySettings
class nobBaseWarehouse : public nobBaseMilitary, public DataChangedObservable
{
public:
    static constexpr bool isStorehouseGOT(const GO_Type got)
    {
        return got == GO_Type::NobStorehouse || got == GO_Type::NobHarborbuilding || got == GO_Type::NobHq;
    }

protected:
    // Liste von Waren, die noch rausgebracht werden müssen, was im Moment aber nicht möglich ist,
    // weil die Flagge voll ist vor dem Lagerhaus
    std::list<std::unique_ptr<Ware>> waiting_wares;
    // verhindert doppeltes Holen von Waren
    bool fetch_double_protection;
    /// Liste von Figuren, die auf dem Weg zu dem Lagerhaus sind bzw. Soldaten die von ihm kommen
    std::list<noFigure*> dependent_figures;
    /// Liste von Waren, die auf dem Weg zum Lagerhaus sind
    std::list<Ware*> dependent_wares;
    /// Produzier-Träger-Event
    const GameEvent* producinghelpers_event;
    /// Rekrutierungsevent für Soldaten
    const GameEvent* recruiting_event;
    /// Auslagerevent für Waren und Figuren
    const GameEvent* empty_event;
    /// Einlagerevent für Waren und Figuren
    const GameEvent* store_event;

    /// Soldaten-Reserve-Einstellung
    std::array<unsigned, 5> reserve_soldiers_available;            /// einkassierte Soldaten zur Reserve
    std::array<unsigned, 5> reserve_soldiers_available_with_armor; /// how many soldiers of the reserve have armor
    std::array<unsigned, 5> reserve_soldiers_claimed_visual;       /// geforderte Soldaten zur Reserve - visuell
    std::array<unsigned, 5> reserve_soldiers_claimed_real;         /// geforderte Soldaten zur Reserve - real

    /// Inventory of the building, real is the usable amount, visual is the total amount currently in the wh
    VirtualInventory inventory;
    InventorySettings inventorySettingsVisual; /// die Inventar-Einstellungen, visuell
    InventorySettings inventorySettings;       /// die Inventar-Einstellungen, real

private:
    /// Check if soldiers can be recruited
    bool canRecruitSoldiers();
    /// Derived classes may use ordered wares immediately, e.g. for sending via ship instead of by road
    /// Return true if so and take ownership of the ware
    virtual bool useWareAtOnce(std::unique_ptr<Ware>& /*ware*/, noBaseBuilding& /*goal*/) { return false; }
    /// Derived classes may use ordered figures immediately, e.g. for sending via ship instead of by road
    /// Return true if so and take ownership of the figure
    virtual bool useFigureAtOnce(std::unique_ptr<noFigure>& /*fig*/, noRoadNode& /*goal*/) { return false; }
    /// Prüft verschiedene Verwendungszwecke für eine neuangekommende Ware
    void checkUsesForNewWare(GoodType gt);
    /// Prüft verschiedene Sachen, falls ein neuer Mensch das Haus betreten hat
    void checkJobsForNewFigure(Job job);

    friend class gc::SetInventorySetting;
    friend class gc::SetAllInventorySettings;
    /// Verändert Ein/Auslagerungseinstellungen
    void SetInventorySetting(const boost_variant2<GoodType, Job>& what, InventorySetting state);

    /// Verändert alle Ein/Auslagerungseinstellungen einer Kategorie (also Waren oder Figuren)(real)
    void SetAllInventorySettings(bool isJob, const std::vector<InventorySetting>& states);

    /// Lässt einen bestimmten Waren/Job-Typ ggf auslagern
    void checkOuthousing(const boost_variant2<GoodType, Job>& what);
    void handleCollectEvent();
    void handleSendoutEvent();
    void handleRecrutingEvent();
    void handleProduceHelperEvent();
    void handleLeaveEvent();

protected:
    /// Stellt Verteidiger zur Verfügung
    std::unique_ptr<nofDefender> provideDefender(nofAttacker& attacker) override;

    void handleBaseEvent(unsigned id);

    /// Versucht ein Rekrutierungsevent anzumelden, falls ausreichend Waffen und Bier sowie genügend Gehilfen
    /// vorhanden sind (je nach Militäreinstellungen)
    void tryRecruiting();
    /// Versucht Rekrutierungsevent abzumeldne, falls die Bedingungen nicht mehr erfüllt sind (z.B. wenn Ware
    /// rausgetragen wurde o.ä.)
    void tryStopRecruiting();
    /// Recruts a worker of the given job if possible
    bool tryRecruitJob(Job job);

    void removeFromVisualInventory(const noFigure& figure);
    void addToVisualInventory(const noFigure& figure);
    void addToRealInventory(const noFigure& figure);

    nobBaseWarehouse(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    nobBaseWarehouse(SerializedGameData& sgd, unsigned obj_id);

public:
    void clear();

    ~nobBaseWarehouse() override;

protected:
    void DestroyBuilding() override;

public:
    void serialize(SerializedGameData& sgd) const override;

    const Inventory& getInventory() const;

    /// Adds specified goods and people. If addToPlayer is true,
    /// then they are also added to the owners inventory (for newly created/arrived goods)
    /// Use false for goods, that are only moved between players units
    void addToInventory(const GoodsAndPeopleCounts& what, bool addToPlayer);
    void addToInventory(const PeopleCounts& people, bool addToPlayer);
    void addToInventory(const GoodCounts& goods, bool addToPlayer);

    /// Gibt Anzahl der Waren bzw. Figuren zurück
    unsigned getNumRealWares(GoodType type) const { return inventory.real[type]; }
    unsigned getNumRealFigures(Job job) const { return inventory.real[job]; }
    unsigned getNumRealArmoredFigures(ArmoredSoldier job) const { return inventory.real[job]; }
    unsigned getNumVisualWares(GoodType type) const { return inventory.visual[type]; }
    unsigned getNumVisualFigures(Job job) const { return inventory.visual[job]; }
    unsigned getNumVisualArmoredFigures(ArmoredSoldier job) const { return inventory.visual[job]; }

    /// Gibt Ein/Auslagerungseinstellungen zurück
    InventorySetting getInventorySettingVisual(Job job) const;
    InventorySetting getInventorySettingVisual(GoodType ware) const;
    InventorySetting getInventorySetting(Job job) const;
    InventorySetting getInventorySetting(GoodType ware) const;
    // Convenience functions
    bool isInventorySettingVisual(const Job job, const EInventorySetting setting) const
    {
        return getInventorySettingVisual(job).isSet(setting);
    }
    bool isInventorySettingVisual(const GoodType ware, const EInventorySetting setting) const
    {
        return getInventorySettingVisual(ware).isSet(setting);
    }
    bool isInventorySetting(const Job job, const EInventorySetting setting) const
    {
        return getInventorySetting(job).isSet(setting);
    }
    bool isInventorySetting(const GoodType ware, const EInventorySetting setting) const
    {
        return getInventorySetting(ware).isSet(setting);
    }

    void setInventorySettingVisual(const boost_variant2<GoodType, Job>& what, InventorySetting state);

    /// Bestellt einen Träger
    void orderCarrier(noRoadNode& goal, RoadSegment& workplace);
    /// Bestellt irgendeinen Beruf (ggf. stellt er ihn noch mit einem Werkzeug her)
    bool orderJob(Job job, noRoadNode& goal, bool allow_recruiting);
    /// Bestellt einen Esel
    nofCarrier* orderDonkey(RoadSegment& road, noRoadNode& goal_flag);
    /// "Bestellt" eine Ware --> gibt den Pointer auf die Ware zurück
    Ware* orderWare(GoodType good, noBaseBuilding& goal);
    /// Returns true, if the given job can be recruited. Excludes soldiers and carriers!
    bool canRecruit(Job job) const;

    /// Wird von den Lagerhaus-Arbeitern aufgerufen, wenn sie ein Ware wieder zurückbringen, die sie vorne nicht ablegen
    /// konnten
    void addWaitingWare(std::unique_ptr<Ware> ware);
    /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
    bool freePlaceAtFlag() override;
    // Eine Ware liegt vor der Flagge des Warenhauses und will rein --> ein Warenhausmitarbeiter muss kommen und sie
    // holen
    void fetchWare();
    // Soll die nächste Ware nicht holen
    void dontFetchNextWare() { fetch_double_protection = true; }

    /// Legt eine Ware im Lagerhaus ab
    void addWare(std::unique_ptr<Ware> ware) override;
    /// Eine Figur geht ins Lagerhaus
    virtual void addFigure(std::unique_ptr<noFigure> figure, bool increase_visual_counts = true);

    /// Eine bestellte Ware konnte doch nicht kommen
    void wareLost(Ware& ware) override;
    /// Bestellte Ware, die sich noch hier drin befindet, storniert ihre Auslieferung
    void cancelWare(Ware*& ware);
    /// Bestellte Figur, die sich noch ind er Warteschlange befindet, kommt nicht mehr und will rausgehauen werden
    virtual void cancelFigure(noFigure* figure);

    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
    void takeWare(Ware* ware) override;

    /// Fügt eine Figur hinzu, die auf dem Weg zum Lagerhaus ist
    void addDependentFigure(noFigure& figure)
    {
        RTTR_Assert(!isDependentFigure(figure));
        dependent_figures.push_back(&figure);
    }
    //// Entfernt eine abhängige Figur wieder aus der Liste
    virtual void removeDependentFigure(noFigure& figure)
    {
        RTTR_Assert(isDependentFigure(figure));
        dependent_figures.remove(&figure);
    }
    /// Wird aufgerufen, wenn ein Arbeiter hierher kommt
    void gotWorker(Job /*job*/, noFigure& worker) override
    {
        RTTR_Assert(!isDependentFigure(worker));
        dependent_figures.push_back(&worker);
    }

    //// Entfernt eine abhängige Ware wieder aus der Liste (wird mit TakeWare hinzugefügt)
    void removeDependentWare(Ware& ware)
    {
        RTTR_Assert(isWareDependent(ware));
        dependent_wares.remove(&ware);
    }
    /// Überprüft, ob Ware abhängig ist
    bool isWareDependent(const Ware& ware);
    /// Prüft, ob es Waren zum Auslagern gibt
    bool areWaresToEmpty() const;

    void figureLeft(const noFigure& fig) override;
    /// Fügt aktiven Soldaten (der aus von einer Mission) zum Militärgebäude hinzu
    void addActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier) override;
    /// Gibt Gesamtanzahl aller im Lager befindlichen Soldaten zurück
    unsigned getNumSoldiers() const
    {
        return getNumRealFigures(Job::Private) + getNumRealFigures(Job::PrivateFirstClass)
               + getNumRealFigures(Job::Sergeant) + getNumRealFigures(Job::Officer) + getNumRealFigures(Job::General);
    }
    /// Order troops of each rank according to `counts` without exceeding `max` in total. The number of soldiers
    /// of each rank that is sent out is subtracted from the corresponding count in `counts` and from `max`.
    void orderTroops(nobMilitary& goal, std::array<unsigned, NUM_SOLDIER_RANKS>& counts, unsigned& max);

    /// Schickt einen Verteidiger raus, der einem Angreifer in den Weg rennt
    nofAggressiveDefender* sendAggressiveDefender(nofAttacker& attacker) override;
    /// Wird aufgerufen, wenn ein Soldat nicht mehr kommen kann
    void soldierLost(nofSoldier* soldier) override;

    /// Sind noch Truppen drinne, die dieses Gebäude verteidigen könnten?
    bool defendersAvailable() const override;

    /// Verändert Reserveeinstellung
    void setReserveVisual(unsigned rank, unsigned count);
    void setRealReserve(unsigned rank, unsigned count);

    /// Versucht, die geforderten Reserve-Soldaten bereitzustellen
    void refreshReserve(unsigned rank);

    /// Gibt Zeiger auf dir Reserve zurück für das GUI
    const unsigned* getReserveAvailablePointer(unsigned rank) const { return &reserve_soldiers_available[rank]; }
    const unsigned* getReserveArmoredAvailablePointer(unsigned rank) const
    {
        return &reserve_soldiers_available_with_armor[rank];
    }
    const unsigned* getReserveClaimedVisualPointer(unsigned rank) const
    {
        return &reserve_soldiers_claimed_visual[rank];
    }
    unsigned getReserveClaimed(unsigned rank) const { return reserve_soldiers_claimed_real[rank]; }

    /// Available goods of a specific type that can be used for trading
    unsigned getAvailableWaresForTrading(GoodType gt) const;
    /// Available figures of a specific type that can be used for trading
    unsigned getAvailableFiguresForTrading(Job job) const;
    /// Starts a trade caravane from this warehouse
    void startTradeCaravane(const boost_variant2<GoodType, Job>& what, unsigned count, const TradeRoute& tr,
                            nobBaseWarehouse* goal);

    /// For debug only
    bool isDependentFigure(const noFigure& fig) const;
};
