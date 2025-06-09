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
    bool CanRecruitSoldiers();
    /// Derived classes may use ordered wares immediately, e.g. for sending via ship instead of by road
    /// Return true if so and take ownership of the ware
    virtual bool UseWareAtOnce(std::unique_ptr<Ware>& /*ware*/, noBaseBuilding& /*goal*/) { return false; }
    /// Derived classes may use ordered figures immediately, e.g. for sending via ship instead of by road
    /// Return true if so and take ownership of the figure
    virtual bool UseFigureAtOnce(std::unique_ptr<noFigure>& /*fig*/, noRoadNode& /*goal*/) { return false; }
    /// Prüft verschiedene Verwendungszwecke für eine neuangekommende Ware
    void CheckUsesForNewWare(GoodType gt);
    /// Prüft verschiedene Sachen, falls ein neuer Mensch das Haus betreten hat
    void CheckJobsForNewFigure(Job job);

    friend class gc::SetInventorySetting;
    friend class gc::SetAllInventorySettings;
    /// Verändert Ein/Auslagerungseinstellungen
    void SetInventorySetting(const boost_variant2<GoodType, Job>& what, InventorySetting state);

    /// Verändert alle Ein/Auslagerungseinstellungen einer Kategorie (also Waren oder Figuren)(real)
    void SetAllInventorySettings(bool isJob, const std::vector<InventorySetting>& states);

    /// Lässt einen bestimmten Waren/Job-Typ ggf auslagern
    void CheckOuthousing(const boost_variant2<GoodType, Job>& what);
    void HandleCollectEvent();
    void HandleSendoutEvent();
    void HandleRecrutingEvent();
    void HandleProduceHelperEvent();
    void HandleLeaveEvent();

protected:
    /// Stellt Verteidiger zur Verfügung
    std::unique_ptr<nofDefender> ProvideDefender(nofAttacker& attacker) override;

    void HandleBaseEvent(unsigned id);

    /// Versucht ein Rekrutierungsevent anzumelden, falls ausreichend Waffen und Bier sowie genügend Gehilfen
    /// vorhanden sind (je nach Militäreinstellungen)
    void TryRecruiting();
    /// Versucht Rekrutierungsevent abzumeldne, falls die Bedingungen nicht mehr erfüllt sind (z.B. wenn Ware
    /// rausgetragen wurde o.ä.)
    void TryStopRecruiting();
    /// Aktuellen Warenbestand zur aktuellen Inventur dazu addieren
    void AddToInventory();
    /// Recruts a worker of the given job if possible
    bool TryRecruitJob(Job job);

    void RemoveArmoredFigurFromVisualInventory(noFigure* figure);
    void AddArmoredFigurToVisualInventory(noFigure* figure);

    nobBaseWarehouse(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    nobBaseWarehouse(SerializedGameData& sgd, unsigned obj_id);

public:
    void Clear();

    ~nobBaseWarehouse() override;

protected:
    void DestroyBuilding() override;

public:
    void Serialize(SerializedGameData& sgd) const override;

    const Inventory& GetInventory() const;

    /// Adds specified goods. If addToPlayer is true,
    /// then they are also added to the owners inventory (for newly created/arrived goods)
    /// Use false for goods, that are only moved between players units
    void AddGoods(const Inventory& goods, bool addToPlayer);

    /// Gibt Anzahl der Waren bzw. Figuren zurück
    unsigned GetNumRealWares(GoodType type) const { return inventory.real[type]; }
    unsigned GetNumRealFigures(Job job) const { return inventory.real[job]; }
    unsigned GetNumVisualWares(GoodType type) const { return inventory.visual[type]; }
    unsigned GetNumVisualFigures(Job job) const { return inventory.visual[job]; }

    /// Gibt Ein/Auslagerungseinstellungen zurück
    InventorySetting GetInventorySettingVisual(Job job) const;
    InventorySetting GetInventorySettingVisual(GoodType ware) const;
    InventorySetting GetInventorySetting(Job job) const;
    InventorySetting GetInventorySetting(GoodType ware) const;
    // Convenience functions
    bool IsInventorySettingVisual(const Job job, const EInventorySetting setting) const
    {
        return GetInventorySettingVisual(job).IsSet(setting);
    }
    bool IsInventorySettingVisual(const GoodType ware, const EInventorySetting setting) const
    {
        return GetInventorySettingVisual(ware).IsSet(setting);
    }
    bool IsInventorySetting(const Job job, const EInventorySetting setting) const
    {
        return GetInventorySetting(job).IsSet(setting);
    }
    bool IsInventorySetting(const GoodType ware, const EInventorySetting setting) const
    {
        return GetInventorySetting(ware).IsSet(setting);
    }

    void SetInventorySettingVisual(const boost_variant2<GoodType, Job>& what, InventorySetting state);

    /// Bestellt einen Träger
    void OrderCarrier(noRoadNode& goal, RoadSegment& workplace);
    /// Bestellt irgendeinen Beruf (ggf. stellt er ihn noch mit einem Werkzeug her)
    bool OrderJob(Job job, noRoadNode& goal, bool allow_recruiting);
    /// Bestellt einen Esel
    nofCarrier* OrderDonkey(RoadSegment& road, noRoadNode& goal_flag);
    /// "Bestellt" eine Ware --> gibt den Pointer auf die Ware zurück
    Ware* OrderWare(GoodType good, noBaseBuilding& goal);
    /// Returns true, if the given job can be recruited. Excludes soldiers and carriers!
    bool CanRecruit(Job job) const;

    /// Wird von den Lagerhaus-Arbeitern aufgerufen, wenn sie ein Ware wieder zurückbringen, die sie vorne nicht ablegen
    /// konnten
    void AddWaitingWare(std::unique_ptr<Ware> ware);
    /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
    bool FreePlaceAtFlag() override;
    // Eine Ware liegt vor der Flagge des Warenhauses und will rein --> ein Warenhausmitarbeiter muss kommen und sie
    // holen
    void FetchWare();
    // Soll die nächste Ware nicht holen
    void DontFetchNextWare() { fetch_double_protection = true; }

    /// Legt eine Ware im Lagerhaus ab
    void AddWare(std::unique_ptr<Ware> ware) override;
    /// Eine Figur geht ins Lagerhaus
    virtual void AddFigure(std::unique_ptr<noFigure> figure, bool increase_visual_counts = true);

    /// Eine bestellte Ware konnte doch nicht kommen
    void WareLost(Ware& ware) override;
    /// Bestellte Ware, die sich noch hier drin befindet, storniert ihre Auslieferung
    void CancelWare(Ware*& ware);
    /// Bestellte Figur, die sich noch inder Warteschlange befindet, kommt nicht mehr und will rausgehauen werden
    virtual void CancelFigure(noFigure* figure);

    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
    void TakeWare(Ware* ware) override;

    /// Fügt eine Figur hinzu, die auf dem Weg zum Lagerhaus ist
    void AddDependentFigure(noFigure& figure)
    {
        RTTR_Assert(!IsDependentFigure(figure));
        dependent_figures.push_back(&figure);
    }
    //// Entfernt eine abhängige Figur wieder aus der Liste
    virtual void RemoveDependentFigure(noFigure& figure)
    {
        RTTR_Assert(IsDependentFigure(figure));
        dependent_figures.remove(&figure);
    }
    /// Wird aufgerufen, wenn ein Arbeiter hierher kommt
    void GotWorker(Job /*job*/, noFigure& worker) override
    {
        RTTR_Assert(!IsDependentFigure(worker));
        dependent_figures.push_back(&worker);
    }

    //// Entfernt eine abhängige Ware wieder aus der Liste (wird mit TakeWare hinzugefügt)
    void RemoveDependentWare(Ware& ware)
    {
        RTTR_Assert(IsWareDependent(ware));
        dependent_wares.remove(&ware);
    }
    /// Überprüft, ob Ware abhängig ist
    bool IsWareDependent(const Ware& ware);
    /// Prüft, ob es Waren zum Auslagern gibt
    bool AreWaresToEmpty() const;

    /// Fügt aktiven Soldaten (der aus von einer Mission) zum Militärgebäude hinzu
    void AddActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier) override;
    /// Gibt Gesamtanzahl aller im Lager befindlichen Soldaten zurück
    unsigned GetNumSoldiers() const
    {
        return GetNumRealFigures(Job::Private) + GetNumRealFigures(Job::PrivateFirstClass)
               + GetNumRealFigures(Job::Sergeant) + GetNumRealFigures(Job::Officer) + GetNumRealFigures(Job::General);
    }
    /// Order troops of each rank according to `counts` without exceeding `max` in total. The number of soldiers
    /// of each rank that is sent out is subtracted from the corresponding count in `counts` and from `max`.
    void OrderTroops(nobMilitary& goal, std::array<unsigned, NUM_SOLDIER_RANKS>& counts, unsigned& max);

    /// Schickt einen Verteidiger raus, der einem Angreifer in den Weg rennt
    nofAggressiveDefender* SendAggressiveDefender(nofAttacker& attacker) override;
    /// Wird aufgerufen, wenn ein Soldat nicht mehr kommen kann
    void SoldierLost(nofSoldier* soldier) override;

    /// Sind noch Truppen drinne, die dieses Gebäude verteidigen könnten?
    bool DefendersAvailable() const override;

    /// Verändert Reserveeinstellung
    void SetReserveVisual(unsigned rank, unsigned count);
    void SetRealReserve(unsigned rank, unsigned count);

    /// Versucht, die geforderten Reserve-Soldaten bereitzustellen
    void RefreshReserve(unsigned rank);

    /// Gibt Zeiger auf dir Reserve zurück für das GUI
    const unsigned* GetReserveAvailablePointer(unsigned rank) const { return &reserve_soldiers_available[rank]; }
    const unsigned* GetReserveClaimedVisualPointer(unsigned rank) const
    {
        return &reserve_soldiers_claimed_visual[rank];
    }
    unsigned GetReserveClaimed(unsigned rank) const { return reserve_soldiers_claimed_real[rank]; }

    /// Available goods of a specific type that can be used for trading
    unsigned GetAvailableWaresForTrading(GoodType gt) const;
    /// Available figures of a specific type that can be used for trading
    unsigned GetAvailableFiguresForTrading(Job job) const;
    /// Starts a trade caravane from this warehouse
    void StartTradeCaravane(const boost_variant2<GoodType, Job>& what, unsigned count, const TradeRoute& tr,
                            nobBaseWarehouse* goal);

    /// For debug only
    bool IsDependentFigure(const noFigure& fig) const;
};
