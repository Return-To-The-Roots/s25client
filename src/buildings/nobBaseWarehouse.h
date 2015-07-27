// $Id: nobBaseWarehouse.h 9564 2014-12-30 10:53:04Z marcus $
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
#ifndef NOBBASEWAREHOUSE_H_INCLUDED
#define NOBBASEWAREHOUSE_H_INCLUDED

#pragma once

#include "nobBaseMilitary.h"
#include "gameData/GameConsts.h"
#include "EventManager.h"
#include "figures/noFigure.h"
#include <list>

class nofCarrier;
class nofWarehouseWorker;
class noFigure;
class Ware;
class nobMilitary;
class TradeRoute;

/// Ein/Auslagereinstellungsstruktur
struct InventorySettings
{
    unsigned char wares[WARE_TYPES_COUNT];
    unsigned char figures[JOB_TYPES_COUNT];

    InventorySettings()
    { memset(wares, 0, sizeof(wares)); memset(figures, 0, sizeof(figures)); }
};

/// Grundlegende Warenhausklasse, die alle Funktionen vereint, die fÃ¼r WarenhÃ¤user (HQ, Lagerhaus, HÃ¤fen)
/// wichtig sind.
class nobBaseWarehouse : public nobBaseMilitary
{
    protected:
        // Liste von Waren, die noch rausgebracht werden mÃ¼ssen, was im Moment aber nicht mÃ¶glich ist,
        // weil die Flagge voll ist vor dem Lagerhaus
        std::list<Ware*> waiting_wares;
        // verhindert doppeltes Holen von Waren
        bool fetch_double_protection;
        /// Liste von Figuren, die auf dem Weg zu dem Lagerhaus sind bzw. Soldaten die von ihm kommen
        std::list<noFigure*> dependent_figures;
        /// Liste von Waren, die auf dem Weg zum Lagerhaus sind
        std::list<Ware*> dependent_wares;
        /// Produzier-TrÃ¤ger-Event
        EventManager::EventPointer producinghelpers_event;
        /// Rekrutierungsevent fÃ¼r Soldaten
        EventManager::EventPointer recruiting_event;
        /// Auslagerevent fÃ¼r Waren und Figuren
        EventManager::EventPointer empty_event;
        /// Einlagerevent fÃ¼r Waren und Figuren
        EventManager::EventPointer store_event;


    protected:

        /// Soldaten-Reserve-Einstellung
        unsigned reserve_soldiers_available[5]; /// einkassierte Soldaten zur Reserve
        unsigned reserve_soldiers_claimed_visual[5]; /// geforderte Soldaten zur Reserve - visuell
        unsigned reserve_soldiers_claimed_real[5]; /// geforderte Soldaten zur Reserve - real

        /// Waren bzw. Menschenanzahl im GebÃ¤ude, real_goods ist die tatsÃ¤chliche Anzahl und wird zum berechnen verwendet, goods ist nur die, die auch angezeigt wird
        Goods goods, real_goods;
        InventorySettings inventory_settings_visual; ///< die Inventar-Einstellungen, visuell
        InventorySettings inventory_settings_real; ///< die Inventar-Einstellungen, real

    private:

        /// PrÃ¼ft, ob alle Bedingungen zum Rekrutieren erfÃ¼llt sind
        bool AreRecruitingConditionsComply();
        /// Abgeleitete kann eine gerade erzeugte Ware ggf. sofort verwenden
        /// (muss in dem Fall true zurÃ¼ckgeben)
        virtual bool UseWareAtOnce(Ware* ware, noBaseBuilding* const goal);
        /// Dasselbe fÃ¼r Menschen
        virtual bool UseFigureAtOnce(noFigure* fig, noRoadNode* const goal);
        /// PrÃ¼ft verschiedene Verwendungszwecke fÃ¼r eine neuangekommende Ware
        void CheckUsesForNewWare(const GoodType gt);
        /// PrÃ¼ft verschiedene Sachen, falls ein neuer Mensch das Haus betreten hat
        void CheckJobsForNewFigure(const Job job);


    protected:

        /// Stellt Verteidiger zur VerfÃ¼gung
        virtual nofDefender* ProvideDefender(nofAttacker* const attacker);

        void HandleBaseEvent(const unsigned int id);
        /// Versucht ein Rekrutierungsevent anzumelden, falls ausreichend Waffen und Bier sowie genÃ¼gend Gehilfen
        /// vorhanden sind (je nach MilitÃ¤reinstellungen)
        void TryRecruiting();
        /// Versucht Rekrutierungsevent abzumeldne, falls die Bedingungen nicht mehr erfÃ¼llt sind (z.B. wenn Ware
        /// rausgetragen wurde o.Ã„.)
        void TryStopRecruiting();
        /// Aktuellen Warenbestand zur aktuellen Inventur dazu addieren
        void AddToInventory();

        nobBaseWarehouse(const BuildingType type, const MapPoint pt, const unsigned char player, const Nation nation);
        nobBaseWarehouse(SerializedGameData* sgd, const unsigned obj_id);
    public:

        void Clear();
        
        virtual ~nobBaseWarehouse();

        /// AufrÃ¤ummethoden
    protected:
        void Destroy_nobBaseWarehouse();
    public:
        void Destroy() { Destroy_nobBaseWarehouse(); }

        /// Serialisierungsfunktionen
    protected: void Serialize_nobBaseWarehouse(SerializedGameData* sgd) const;
    public: void Serialize(SerializedGameData* sgd) const { Serialize_nobBaseWarehouse(sgd); }

        const Goods* GetInventory() const;

        /// FÃ¼gt einige GÃ¼ter hinzu
        void AddGoods(const Goods goods);


        /// Gibt Anzahl der Waren bzw. Figuren zurÃ¼ck
        unsigned GetRealWaresCount(GoodType type) const { return real_goods.goods[type]; }
        unsigned GetRealFiguresCount(Job type) const { return real_goods.people[type]; }
        unsigned GetVisualWaresCount(GoodType type) const { return goods.goods[type]; }
        unsigned GetVisualFiguresCount(Job type) const { return goods.people[type]; }


        /// VerÃ¤ndert Ein/Auslagerungseinstellungen (visuell)
        void ChangeVisualInventorySettings(unsigned char category, unsigned char state, unsigned char type);
        /// Gibt Ein/Auslagerungseinstellungen zurÃ¼ck (visuell)
        bool CheckVisualInventorySettings(unsigned char category, unsigned char state, unsigned char type) const;
        ///// Generiert einen NC-Befehl fÃ¼r eine Inventory Settings Ã„nderung und fÃ¼hrt noch entsprechend eigene Ã„nderungen aus
        //void SubmitInventorySettings();

        /// VerÃ¤ndert Ein/Auslagerungseinstellungen (real)
        void ChangeRealInventorySetting(unsigned char category, unsigned char state, unsigned char type);
        /// VerÃ¤ndert alle Ein/Auslagerungseinstellungen einer Kategorie (also Waren oder Figuren)(real)
        void ChangeAllRealInventorySettings(unsigned char category, unsigned char state);
        /// Gibt Ein/Auslagerungseinstellungen zurÃ¼ck (real), cannot check for state 0
        bool CheckRealInventorySettings(unsigned char category, unsigned char state, unsigned char type) const
        { return ((((category == 0) ? inventory_settings_real.wares[type] : inventory_settings_real.figures[type]) & state) == state); }

        /// LÃ¤sst einen bestimmten Waren/Job-Typ ggf auslagern
        void CheckOuthousing(unsigned char category, unsigned job_ware_id);


        /// Bestellt einen TrÃ¤ger
        void OrderCarrier(noRoadNode* const goal, RoadSegment* workplace);
        /// Bestellt irgendeinen Beruf (ggf. stellt er ihn noch mit einem Werkzeug her)
        bool OrderJob(const Job job, noRoadNode* const goal, const bool allow_recruiting);
        /// Bestellt einen Esel
        nofCarrier* OrderDonkey(RoadSegment* road, noRoadNode* const goal_flag);

        /// Reiht einen Beruf sofort in die Warteschlange zum Rausgehen rein, wenn er da ist und gibt den Pointer auf
        /// ihn zurÃ¼ck, wenn keiner da ist, wird 0 zurÃ¼ckgegeben
        noFigure* OrderDefender();
        /// "Bestellt" eine Ware --> gibt den Pointer auf die Ware zurÃ¼ck
        Ware* OrderWare(const GoodType good, noBaseBuilding* const goal);
        /// Wird von den Lagerhaus-Arbeitern aufgerufen, wenn sie ein Ware wieder zurÃ¼ckbringen, die sie vorne nicht ablegen konnten
        void AddWaitingWare(Ware* ware);
        /// Wird aufgerufen, wenn von der Fahne vor dem GebÃ¤ude ein Rohstoff aufgenommen wurde
        bool FreePlaceAtFlag();
        // Eine Ware liegt vor der Flagge des Warenhauses und will rein --> ein Warenhausmitarbeiter muss kommen und sie holen
        void FetchWare();
        // Soll die nÃ¤chste Ware nicht holen
        void DontFetchNextWare() {fetch_double_protection = true;}

        /// Legt eine Ware im Lagerhaus ab
        virtual void AddWare(Ware* ware);
        /// Eine Figur geht ins Lagerhaus
        virtual void AddFigure(noFigure* figure, const bool increase_visual_counts = true);

        void CarryOutWare();
        /// Eine bestellte Ware konnte doch nicht kommen
        virtual void WareLost(Ware* ware);
        /// Bestellte Ware, die sich noch hier drin befindet, storniert ihre Auslieferung
        void CancelWare(Ware* ware);
        /// Bestellte Figur, die sich noch inder Warteschlange befindet, kommt nicht mehr und will rausgehauen werden
        virtual void CancelFigure(noFigure* figure);

        /// Sowas ist bei WarenhÃ¤usern nicht nÃ¶tig
        unsigned CalcDistributionPoints(noRoadNode* start, const GoodType type) { return 0; }
        /// Wird aufgerufen, wenn eine neue Ware zum dem GebÃ¤ude geliefert wird (nicht wenn sie bestellt wurde vom GebÃ¤ude!)
        void TakeWare(Ware* ware);

        /// FÃ¼gt eine Figur hinzu, die auf dem Weg zum Lagerhaus ist
        void AddDependentFigure(noFigure* figure) { assert(!CheckDependentFigure(figure)); dependent_figures.push_back(figure); }
        //// Entfernt eine abhÃ¤ngige Figur wieder aus der Liste
        virtual void RemoveDependentFigure(noFigure* figure) { dependent_figures.remove(figure); }
        /// Wird aufgerufen, wenn ein Arbeiter hierher kommt
        void GotWorker(Job job, noFigure* worker)
        { assert(!CheckDependentFigure(worker)); dependent_figures.push_back(worker); }

        //// Entfernt eine abhÃ¤ngige Ware wieder aus der Liste (wird mit TakeWare hinzugefÃ¼gt)
		void RemoveDependentWare(Ware* ware) { dependent_wares.remove(ware); }
        /// ÃœberprÃ¼ft, ob Ware abhÃ¤ngig ist
		bool IsWareDependent(Ware* ware) { return std::find(dependent_wares.begin(),dependent_wares.end(),ware)!=dependent_wares.end(); }
		std::list<Ware*> GetDependentWares() {return dependent_wares;}
        /// PrÃ¼ft, ob es Waren zum Auslagern gibt
        bool AreWaresToEmpty() const;

        /// FÃ¼gt aktiven Soldaten (der aus von einer Mission) zum MilitÃ¤rgebÃ¤ude hinzu
        void AddActiveSoldier(nofActiveSoldier* soldier);
        /// Gibt Gesamtanzahl aller im Lager befindlichen Soldaten zurÃ¼ck
        unsigned GetSoldiersCount() const
        {
            return real_goods.people[JOB_PRIVATE] + real_goods.people[JOB_PRIVATEFIRSTCLASS] +
                   real_goods.people[JOB_SERGEANT] + real_goods.people[JOB_OFFICER] + real_goods.people[JOB_GENERAL];
        }
        /// Bestellt Soldaten
        void OrderTroops(nobMilitary* goal, unsigned count,bool ignoresettingsendweakfirst=false);

        /// Schickt einen Verteidiger raus, der einem Angreifer in den Weg rennt
        nofAggressiveDefender* SendDefender(nofAttacker* attacker);
        /// Wird aufgerufen, wenn ein Soldat nicht mehr kommen kann
        void SoldierLost(nofSoldier* soldier);

        /// Sind noch Truppen drinne, die dieses GebÃ¤ude verteidigen kÃ¶nnten?
        bool DefendersAvailable() const;

        /// VerÃ¤ndert Reserveeinstellung - visuell (nur das geforderte natÃ¼rlich) und gibt neue Anzahl zurÃ¼ck
        unsigned IncreaseReserveVisual(unsigned rank);
        unsigned DecreaseReserveVisual(unsigned rank);
        void SetRealReserve(const unsigned rank, const unsigned count);

        /// Versucht, die geforderten Reserve-Soldaten bereitzustellen
        void RefreshReserve(unsigned rank);

        /// Gibt Zeiger auf dir Reserve zurÃ¼ck fÃ¼r das GUI
        const unsigned* GetReservePointerAvailable(unsigned rank) const { return &reserve_soldiers_available[rank]; }
        const unsigned* GetReservePointerClaimed(unsigned rank) const { return &reserve_soldiers_claimed_visual[rank]; }

        /// Available goods of a speciefic type that can be used for trading
        unsigned GetAvailableWaresForTrading(const GoodType gt) const;
        /// Available figures of a speciefic type that can be used for trading
        unsigned GetAvailableFiguresForTrading(const Job job) const;
        /// Starts a trade caravane from this warehouse
        void StartTradeCaravane(const GoodType gt,  Job job, const unsigned count, const TradeRoute& tr, nobBaseWarehouse* goal);

        /// For debug only
        bool CheckDependentFigure(noFigure* fig);
};



/// Vorgefertigte Bedingungsfunktionen fÃ¼r FindWarehouse, param jeweils Pointer auf die einzelnen Strukturen
namespace FW
{
    struct Param_Ware { GoodType type; unsigned count; };
    bool Condition_Ware(nobBaseWarehouse* wh, const void* param);
    struct Param_Job { Job type; unsigned count; };
    bool Condition_Job(nobBaseWarehouse* wh, const void* param);
    struct Param_WareAndJob { Param_Ware ware; Param_Job job; };
    bool Condition_WareAndJob(nobBaseWarehouse* wh, const void* param);

    bool Condition_Troops(nobBaseWarehouse* wh, const void* param);   // param = &unsigned --> count
    bool Condition_StoreWare(nobBaseWarehouse* wh, const void* param);   // param = &GoodType -> Warentyp
    bool Condition_StoreFigure(nobBaseWarehouse* wh, const void* param);   // param = &Job -> Jobtyp

    // Die LagerhÃ¤user lagern die jeweiligen Waren ein
    bool Condition_WantStoreWare(nobBaseWarehouse* wh, const void* param);   // param = &GoodType -> Warentyp
    bool Condition_WantStoreFigure(nobBaseWarehouse* wh, const void* param);   // param = &Job -> Jobtyp

    // LagerhÃ¤user enthalten die jeweilien Waren, liefern sie aber NICHT gleichzeitig ein
    bool Condition_StoreAndDontWantWare(nobBaseWarehouse* wh, const void* param);   // param = &GoodType -> Warentyp
    bool Condition_StoreAndDontWantFigure(nobBaseWarehouse* wh, const void* param);   // param = &Job -> Jobtyp


    bool NoCondition(nobBaseWarehouse* wh, const void* param);
}

#endif
