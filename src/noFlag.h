// $Id: noFlag.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef NO_FLAG_H_INCLUDED
#define NO_FLAG_H_INCLUDED

#include "noRoadNode.h"
#include "RoadSegment.h"
#include "MapConsts.h"
#include "Ware.h"

class noFlag : public noRoadNode
{
    public:
        noFlag(const unsigned short x, const unsigned short y, const unsigned char player, const unsigned char dis_dir = 0xFF);
        noFlag(SerializedGameData* sgd, const unsigned int obj_id);
        ~noFlag();

        void Destroy()  {   Destroy_noFlag();   }
        void Serialize(SerializedGameData* sgd) const { Serialize_noFlag(sgd);  }

        inline GO_Type GetGOT() const { return GOT_FLAG; }
        inline FlagType GetFlagType() const { return flagtype; }
        /// Gibt Auskunft darüber, ob noch Platz für eine Ware an der Flagge ist.
        inline bool IsSpaceForWare() const { return (GetWareCount() < 8); }

        void Draw(int x, int y);

        BlockingManner GetBM() const { return BM_FLAG; }

        /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War.
        FOWObject* CreateFOWObject() const;
        /// Legt eine Ware an der Flagge ab.
        void AddWare(Ware* ware);
        /// Gibt die Anzahl der Waren zurück, die an der Flagge liegen.
        unsigned GetWareCount() const;
        /// Wählt eine Ware von einer Flagge aus (anhand der Transportreihenfolge), entfernt sie von der Flagge und gibt sie zurück.
        Ware* SelectWare(const unsigned char dir, const bool swap_wares, const noFigure* const carrier);
        /// Prüft, ob es Waren gibt, die auf den Weg in Richtung dir getragen werden müssen.
        inline unsigned short GetWaresCountForRoad(const unsigned char dir) const
        {
            unsigned short ret = 0;

            if (wares[0] && (wares[0]->GetNextDir() == dir))
                ret++;
            if (wares[1] && (wares[1]->GetNextDir() == dir))
                ret++;
            if (wares[2] && (wares[2]->GetNextDir() == dir))
                ret++;
            if (wares[3] && (wares[3]->GetNextDir() == dir))
                ret++;
            if (wares[4] && (wares[4]->GetNextDir() == dir))
                ret++;
            if (wares[5] && (wares[5]->GetNextDir() == dir))
                ret++;
            if (wares[6] && (wares[6]->GetNextDir() == dir))
                ret++;
            if (wares[7] && (wares[7]->GetNextDir() == dir))
                ret++;

            return(ret);
        }
        /// Gibt Wegstrafpunkte für das Pathfinden für Waren, die in eine bestimmte Richtung noch transportiert werden müssen.
        unsigned short GetPunishmentPoints(const unsigned char dir) const;
        /// Zerstört evtl. vorhandenes Gebäude bzw. Baustelle vor der Flagge.
        void DestroyAttachedBuilding();
        /// Baut normale Flaggen zu "gloriösen" aus bei Eselstraßen.
        void Upgrade();
        /// Feind übernimmt die Flagge.
        void Capture(const unsigned char new_owner);
        /// Ist diese Flagge für eine bestimmte Lagerhausflüchtlingsgruppe (BWU) nicht zugänglich?
        bool IsImpossibleForBWU(const unsigned int bwu_id) const;
        /// Hinzufügen, dass diese Flagge für eine bestimmte Lagerhausgruppe nicht zugänglich ist.
        void ImpossibleForBWU(const unsigned int bwu_id);

    protected:
        void Destroy_noFlag();
        void Serialize_noFlag(SerializedGameData* sgd) const;

    private:
        unsigned short ani_offset;
        FlagType flagtype;

        /// Die Waren, die an dieser Flagge liegen
        Ware* wares[8];

        /// Wieviele BWU-Teile es maximal geben soll, also wieviele abgebrannte Lagerhausgruppen
        /// gleichzeitig die Flagge als nicht begehbar deklarieren können.
        static const unsigned int MAX_BWU = 4;

        /// Nicht erreichbar für Massenflüchtlinge
        struct BurnedWarehouseUnit
        {
            unsigned int id;        ///< ID der Gruppe
            unsigned int last_gf;   ///< letzter TÜV, ob man auch nicht hinkommt, in GF
        } bwus[MAX_BWU];
};

#endif // !NO_FLAG_H_INCLUDED
