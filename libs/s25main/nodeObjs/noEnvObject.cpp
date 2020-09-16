// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "noEnvObject.h"

/**
 *  Konstruktor von @p noEnvObject.
 *
 *  @param[in] id Nr der Grafik
 *  @param[in] file Nr der Datei (0xFFFF map_?_z.lst, 0-5 mis?bobs.lst)
 */
noEnvObject::noEnvObject(const MapPoint pos, unsigned short id, unsigned short file)
    : noStaticObject(pos, id, file, 0, NOP_ENVIRONMENT)
{}

noEnvObject::noEnvObject(SerializedGameData& sgd, const unsigned obj_id) : noStaticObject(sgd, obj_id) {}
