// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GoodTypes.h"
#include "JobTypes.h"
#include "helpers/EnumArray.h"

/// Array for people with typed accessors
template<typename T>
struct PeopleArray
{
    helpers::EnumArray<T, Job> people = {};
    helpers::EnumArray<T, ArmoredSoldier> armoredSoldiers = {};

    const T& operator[](Job job) const { return people[job]; }
    T& operator[](Job job) { return people[job]; }
    const T& operator[](ArmoredSoldier soldier) const { return armoredSoldiers[soldier]; }
    T& operator[](ArmoredSoldier soldier) { return armoredSoldiers[soldier]; }
};

/// Combined array for goods and people with typed accessors
template<typename T>
struct GoodsAndPeopleArray : PeopleArray<T>
{
    using PeopleArray<T>::operator[];
    helpers::EnumArray<T, GoodType> goods = {};

    const T& operator[](GoodType good) const { return goods[good]; }
    T& operator[](GoodType good) { return goods[good]; }
};
