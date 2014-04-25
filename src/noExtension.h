// $Id: noExtension.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef NOEXTENSION_H_INCLUDED
#define NOEXTENSION_H_INCLUDED

#pragma once

#include "noBase.h"

// Große Gebäude erstrecken sich über 4 Felder, die restlichen 3 werden mit dieser Klasse gefüllt
class noExtension : public noBase
{
    public:
        noExtension(noBase* const base) : noBase(NOP_EXTENSION), base(base) {}
        noExtension(SerializedGameData* sgd, const unsigned obj_id);
        ~noExtension();

        void Destroy() { Destroy_noExtension(); }
        noBase* GetBaseObject() const { return base; }
        void Draw(int x, int y) {}

        /// Serialisierungsfunktionen
    protected:  void Serialize_noExtension(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_noExtension(sgd); }

        GO_Type GetGOT() const { return GOT_EXTENSION; }

        BlockingManner GetBM() const { return BM_SINGLEBLOCKING; }

    protected:
        void Destroy_noExtension() { Destroy_noBase(); }

    private:
        noBase* base;
};

#endif // !NOEXTENSION_H_INCLUDED
