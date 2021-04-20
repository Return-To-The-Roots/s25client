// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SoundItem.h"

class MusicItem : public SoundItem
{
public:
    /// Plays the music repeating it if repeats is greater than zero or indefinitely when repeats is less than zero
    void Play(int repeats = 0);
};
