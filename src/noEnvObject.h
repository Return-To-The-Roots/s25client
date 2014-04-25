// $Id: noEnvObject.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef NOENVOBJECT_H_INCLUDED
#define NOENVOBJECT_H_INCLUDED

#pragma once

#include "noStaticObject.h"

class noEnvObject : public noStaticObject
{
    public:
        noEnvObject(unsigned short x, unsigned short y, unsigned short id, unsigned short file = 0xFFFF);
        noEnvObject(SerializedGameData* sgd, const unsigned obj_id);

        void Destroy() { Destroy_noEnvObject(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_noEnvObject(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_noEnvObject(sgd); }

        GO_Type GetGOT() const { return GOT_ENVOBJECT; }

        virtual BlockingManner GetBM() const { return BM_NOTBLOCKING; }

    protected:
        void Destroy_noEnvObject() { Destroy_noStaticObject(); }
};

#endif // !NOENVOBJECT_H_INCLUDED
