// $Id: nofWarehouseWorker.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NOF_WAREHOUSEWORKER_H_
#define NOF_WAREHOUSEWORKER_H_

#include "noFigure.h"


// Enumforwarddeklaration bei VC nutzen
#ifdef _MSC_VER
enum GoodType;
#else
#include "GameConsts.h"
#endif

class Ware;
class nobBaseWarehouse;


/// Der "Warehouse-Worker" ist ein einfacher(er) Träger, der die Waren aus dem Lagerhaus holt
class nofWarehouseWorker : public noFigure
{
        // Mein Lagerhaus, in dem ich arbeite, darf auch mal ein bisschen was an mir ändern
        friend class nobBaseWarehouse;

    private:

        // Die Ware, die er gerade trägt (GD_NOTHING wenn er nix trägt?)
        Ware* carried_ware;

        // Aufgabe, die der Warenhaustyp hat (Ware raustragen (0) oder reinholen)
        const bool task;

        // Bin ich fett? (werde immer mal dünn oder fett, damits nicht immer gleich aussieht, wenn jemand rauskommt)
        bool fat;


    private:

        void GoalReached();
        void Walked();
        /// wenn man beim Arbeitsplatz "kündigen" soll, man das Laufen zum Ziel unterbrechen muss (warum auch immer)
        void AbrogateWorkplace();

        void HandleDerivedEvent(const unsigned int id);



    public:

        nofWarehouseWorker(const unsigned short x, const unsigned short y, const unsigned char player, Ware* ware, const bool task);
        nofWarehouseWorker(SerializedGameData* sgd, const unsigned obj_id);

        ~nofWarehouseWorker();

        /// Aufräummethoden
    protected:  void Destroy_nofWarehouseWorker();
    public:     void Destroy() { Destroy_nofWarehouseWorker(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofWarehouseWorker(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofWarehouseWorker(sgd); }

        GO_Type GetGOT() const { return GOT_NOF_WAREHOUSEWORKER; }

        void Draw(int x, int y);

        // Ware nach draußen bringen (von Lagerhaus aus aufgerufen)
        void CarryWare(Ware* ware);

        /// Mitglied von nem Lagerhaus(Lagerhausarbeiter, die die Träger-Bestände nicht beeinflussen?)
        bool MemberOfWarehouse() const { return true; }

};



#endif
