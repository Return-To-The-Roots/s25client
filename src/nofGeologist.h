// $Id: nofGeologist.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NOF_GEOLOGIST_H_
#define NOF_GEOLOGIST_H_

#include "nofFlagWorker.h"

class noFlag;

class nofGeologist : public nofFlagWorker
{
    private:

        /// Schilder, die er noch aufstellen sollte (max 15 abarbeiten)
        unsigned short signs;
        /// Punkte, die noch in Frage kommen
        struct Point { unsigned short x, y; };
        list<Point> available_nodes;
        /// Punkt, zu dem er gerade geht
        Point node_goal;

        /// maximaler Radius wie weit die Geologen sich von der Flagge entfernen würde
        static const unsigned short MAX_RADIUS = 10;

        std::vector<bool> resAlreadyFound;

    private:

        void GoalReached();
        void Walked();
        void HandleDerivedEvent(const unsigned int id);

        /// Kann man an diesem Punkt ein Schild aufstellen?
        bool IsNodeGood(const unsigned short x, const unsigned short y);
        /// Sucht im Umkreis von der Flagge neue Punkte wo man graben könnte
        void LookForNewNodes();
        /// Nimmt den Punkt mit in die Liste auf, wenn er geeignet ist
        void TestNode(const unsigned short x, const unsigned short y);
        /// Bestimmt einen neuen Punkt,wo man hingehen kann, falls es keinen mehr gibt, wird ein ungültiger
        /// Iterator gesetzt, liefert die Richtung in die man zum Punkt gehen muss, zurück
        unsigned char GetNextNode();
        /// Sucht sich einen neuen Punkt und geht dorthin oder geht wieder nach Hause wenn alle Schilder aufgestellt wurden
        /// oder es keinen Punkt mehr gibt
        void GoToNextNode();
        /// Setzt das Schild, wenn noch was frei ist
        void SetSign(const unsigned char resources);

        bool IsSignInArea(unsigned char type) const;

    public:

        nofGeologist(const unsigned short x, const unsigned short y, const unsigned char player, noRoadNode* goal);
        nofGeologist(SerializedGameData* sgd, const unsigned obj_id);

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofGeologist(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofGeologist(sgd); }

        GO_Type GetGOT() const { return GOT_NOF_GEOLOGIST; }

        void Draw(int x, int y);

        /// Wird aufgerufen, wenn die Flagge abgerissen wurde
        void LostWork();

};

#endif
