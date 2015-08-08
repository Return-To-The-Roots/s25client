// $Id: SerializedGameData.h 9357 2014-04-25 15:35:25Z FloSoft $
//
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
#ifndef SERIALIZED_GAME_DATA_H_
#define SERIALIZED_GAME_DATA_H_

#pragma once

#include <memory.h>
#include <set>
#include <list>
#include <vector>
#include "BinaryFile.h"
#include "GameObject.h"
#include "FOWObjects.h"
#include "Serializer.h"
#include "Point.h"
#include "helpers/traits.h"

class noBase;
class GameObject;
class GameWorld;


/// Kümmert sich um das Serialisieren der GameDaten fürs Speichern und Resynchronisieren
class SerializedGameData : public Serializer
{
        /// Objektreferenzen
        union
        {
            const GameObject** objects_write;
            GameObject** objects_read;
        };
        /// Voraussichtliche Gesamtanzahl an Objekten (nur beim Laden)
        unsigned total_objects_count;
        /// Aktuelle Anzahl an Objekten
        unsigned objects_count;

        EventManager* em;

    private:

        /// Objekt(referenzen) lesen
        GameObject* PopObject_(GO_Type got);

        /// Reserves space if possible
        template<class T, bool T_hasReserve = helpers::has_member_function_reserve<void (T::*)(size_t)>::value>
        struct ReserveElements
        {
            static void reserve(T& gos, unsigned size)
            {
                gos.reserve(size);
            }
        };
        template<class T>
        struct ReserveElements<T, false>
        {
            static void reserve(T& gos, unsigned size)
            {}
        };

        /// Returns the most efficient insert operator defining its type as "iterator"
        template<class T, bool T_hasPushBack = helpers::has_member_function_push_back<void (T::*)(const typename T::value_type&)>::value>
        struct GetInsertIterator
        {
            typedef std::back_insert_iterator<T> iterator;
            static iterator get(T& gos)
            {
                return iterator(gos);
            }
        };
        template<class T>
        struct GetInsertIterator<T, false>
        {
            typedef std::insert_iterator<T> iterator;
            static iterator get(T& gos)
            {
                return iterator(gos, gos.end());
            }
        };

    public:

        SerializedGameData();

        /// Nimmt das gesamte Spiel auf und speichert es im Buffer
        void MakeSnapshot(GameWorld* const gw, EventManager* const em);
        /// Liest den Buffer aus einer Datei
        void ReadFromFile(BinaryFile& file);

        void PrepareDeserialization(EventManager* const em) { this->em = em; }

        /// Erzeugt GameObject
        GameObject* Create_GameObject(const GO_Type got, const unsigned obj_id);
        /// Erzeugt FOWObject
        FOWObject* Create_FOWObject(const FOW_Type fowtype);

        /// Kopiermethoden

        /// Objekt(referenzen) kopieren
        void PushObject(const GameObject* go, const bool known);

        /// Copies a container of GameObjects
        template <typename T>
        void PushObjectContainer(const T& gos, const bool known)
        {
            // Anzahl
            PushUnsignedInt(gos.size());
            // einzelne Objekte
            for(typename T::const_iterator it = gos.begin(); it != gos.end(); ++it)
                PushObject(*it, known);
        }

        /// FoW-Objekt
        void PushFOWObject(const FOWObject* fowobj);

        /// Point of map coords
        void PushMapPoint(const MapPoint p)
        {
            PushUnsignedShort(p.x);
            PushUnsignedShort(p.y);
        }

        //////////////////////////////////////////////////////////////////////////
        //Lesemethoden
        //////////////////////////////////////////////////////////////////////////

        /// Point of map coords
        MapPoint PopMapPoint()
        {
            MapPoint p;
            p.x = PopUnsignedShort();
            p.y = PopUnsignedShort();
            return p;
        }

        /// Objekt(referenzen) lesen
        template <typename T>
        T* PopObject(GO_Type got) { return static_cast<T*>(PopObject_(got)); }

        /// FoW-Objekt
        FOWObject* PopFOWObject();


        /// Liest einen Vektor von GameObjects
        template <typename T>
        void PopObjectContainer(T& gos, GO_Type got)
        {
            typedef typename T::value_type ObjectPtr;
            typedef typename helpers::remove_pointer<ObjectPtr>::type Object;

            unsigned size = PopUnsignedInt();
            ReserveElements<T>::reserve(gos, size);
            typename GetInsertIterator<T>::iterator it = GetInsertIterator<T>::get(gos);
            for(unsigned i = 0; i < size; ++i)
                *it = PopObject<Object>(got);
        }

        /// Fügt ein gelesenes Objekt zur globalen Objektliste dazu
        void AddObject(GameObject* go);


        /// Sucht ein Objekt, falls vorhanden
        const GameObject* GetConstGameObject(const unsigned obj_id) const;
        GameObject* GetGameObject(const unsigned obj_id) const;
};


#endif // !SERIALIZED_GAME_DATA_H_
