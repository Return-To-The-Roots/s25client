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

///////////////////////////////////////////////////////////////////////////////
// Header


#include "defines.h"
#include "noEnvObject.h"
#include "SerializedGameData.h"

// Include last!
#include "DebugNew.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p noEnvObject.
 *
 *  @param[in] id Nr der Grafik
 *  @param[in] file Nr der Datei (0xFFFF map_?_z.lst, 0-5 mis?bobs.lst)
 *
 *  @author FloSoft
 */
noEnvObject::noEnvObject(const MapPoint pos, unsigned short id, unsigned short file)
    : noStaticObject(pos, id, file, 0, NOP_ENVIRONMENT)
{
}

void noEnvObject::Serialize_noEnvObject(SerializedGameData& sgd) const
{
    Serialize_noStaticObject(sgd);


}

noEnvObject::noEnvObject(SerializedGameData& sgd, const unsigned obj_id) : noStaticObject(sgd, obj_id)
{
}
