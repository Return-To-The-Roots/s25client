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

#ifndef NOB_USUAL_H_
#define NOB_USUAL_H_

#include "noBuilding.h"
#include "EventManager.h"
#include <boost/array.hpp>
#include <vector>
#include <list>

class Ware;
class nofBuildingWorker;
class SerializedGameData;
class noFigure;
class noRoadNode;

// Gewöhnliches Gebäude mit einem Arbeiter und Waren
class nobUsual : public noBuilding
{
        /// Der Typ, der hier arbeitet
        nofBuildingWorker* worker;
        /// Produktivität
        unsigned short productivity;
        /// Produktion eingestellt? (letzteres nur visuell, um Netzwerk-Latenzen zu verstecken)
        bool disable_production, disable_production_virtual;
        /// Warentyp, den er zuletzt bestellt hatte (bei >1 Waren)
        unsigned char last_ordered_ware;
        /// Rohstoffe, die zur Produktion benötigt werden
        boost::array<unsigned char, 3> wares;
        /// Bestellte Waren
        std::vector< std::list<Ware*> > ordered_wares;
        /// Bestell-Ware-Event
        EventManager::EventPointer orderware_ev;
        /// Rechne-Produktivität-aus-Event
        EventManager::EventPointer productivity_ev;
        /// Letzte Produktivitäten (Durschnitt = Gesamtproduktivität), vorne das neuste !
        static const unsigned LAST_PRODUCTIVITIES_COUNT = 6;
        boost::array<unsigned short, LAST_PRODUCTIVITIES_COUNT> last_productivities;

protected:
        friend class SerializedGameData;
        friend class BuildingFactory;
        nobUsual(const BuildingType type, const MapPoint pt, const unsigned char player, const Nation nation);
        nobUsual(SerializedGameData& sgd, const unsigned obj_id);
    public:

        /// Wird gerade gearbeitet oder nicht?
        bool is_working;

        ~nobUsual() override;

        /// Aufräummethoden
    protected:  void Destroy_nobUsual();
    public:     void Destroy() override { Destroy_nobUsual(); }

        /// Serialisierungsfunktionen
    protected: void Serialize_nobUsual(SerializedGameData& sgd) const;
    public: void Serialize(SerializedGameData& sgd) const override { Serialize_nobUsual(sgd); }

        GO_Type GetGOT() const override { return GOT_NOB_USUAL; }

        void Draw(int x, int y) override;

        bool HasWorker() const;

        /// Event-Handler
        void HandleEvent(const unsigned int id) override;
        /// Legt eine Ware am Objekt ab (an allen Straßenknoten (Gebäude, Baustellen und Flaggen) kann man Waren ablegen
        void AddWare(Ware*& ware) override;
        /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
        bool FreePlaceAtFlag() override;
        /// Eine bestellte Ware konnte doch nicht kommen
        void WareLost(Ware* ware) override;
        /// Wird aufgerufen, wenn ein Arbeiter für das Gebäude gefunden werden konnte
        void GotWorker(Job job, noFigure* worker) override;
        /// Wird vom Arbeiter aufgerufen, wenn er im Gebäude angekommen ist
        void WorkerArrived();
        /// Wird vom Arbeiter aufgerufen, wenn er nicht (mehr) zur Arbeit kommen kann
        void WorkerLost();

        /// Gibt den Warenbestand (eingehende Waren - Rohstoffe) zurück
        unsigned char GetWares(const unsigned int id) const { return wares[id]; }
        /// Prüft, ob Waren für einen Arbeitsschritt vorhanden sind
        bool WaresAvailable();
        /// Verbraucht Waren
        void ConsumeWares();

        /// Berechnet Punktewertung für Ware type, start ist der Produzent, von dem die Ware kommt
        unsigned CalcDistributionPoints(noRoadNode* start, const GoodType type);

        /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
        void TakeWare(Ware* ware) override;

        /// Bestellte Waren
        inline bool AreThereAnyOrderedWares() const {
            for(std::vector< std::list<Ware*> >::const_iterator it = ordered_wares.begin(); it != ordered_wares.end(); ++it)
                if(!it->empty())
                    return true;
            return false;
        }

        /// Gibt Pointer auf Produktivität zurück
        const unsigned short* GetProductivityPointer() const { return &productivity; }
        const unsigned short GetProductivity() const { return productivity; }

        /// Ermittelt, ob es sich bei diesem Gebäude um ein Bergwerk handelt
        bool IsMine() const { return type_ >= BLD_GRANITEMINE && type_ <= BLD_GOLDMINE; }

        /// Stoppt/Erlaubt Produktion (visuell)
        void ToggleProductionVirtual() { disable_production_virtual = !disable_production_virtual; }
        /// Stoppt/Erlaubt Produktion (real)
        void SetProductionEnabled(const bool enabled);
        /// Fragt ab, ob Produktion ausgeschaltet ist (visuell)
        bool IsProductionDisabledVirtual() const { return disable_production_virtual; }
        /// Fragt ab, ob Produktion ausgeschaltet ist (real)
        bool IsProductionDisabled() const { return disable_production; }
        /// Setzt Produktivität instant auf 0 (Keine Ressourcen mehr)
        void SetProductivityToZero();

};


#endif
