// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glArchivItem_Sound_Other.h"
#include "drivers/AudioDriverWrapper.h"

SoundHandle glArchivItem_Sound_Other::Load()
{
    std::string extension = ".tmp";
    if(getType() == libsiedler2::SoundType::OGG)
        extension = ".ogg";
    else if(getType() == libsiedler2::SoundType::MP3)
        extension = ".mp3";

    return AUDIODRIVER.LoadMusic(*this, extension);
}
