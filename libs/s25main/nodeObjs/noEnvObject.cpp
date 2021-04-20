// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noEnvObject.h"

/**
 *  Konstruktor von @p noEnvObject.
 *
 *  @param[in] id Nr der Grafik
 *  @param[in] file Nr der Datei (0xFFFF map_?_z.lst, 0-5 mis?bobs.lst)
 */
noEnvObject::noEnvObject(const MapPoint pos, unsigned short id, unsigned short file)
    : noStaticObject(pos, id, file, 0, NodalObjectType::Environment)
{}

noEnvObject::noEnvObject(SerializedGameData& sgd, const unsigned obj_id) : noStaticObject(sgd, obj_id) {}
