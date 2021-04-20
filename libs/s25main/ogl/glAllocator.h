// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "libsiedler2/StandardAllocator.h"

class GlAllocator : public libsiedler2::StandardAllocator
{
public:
    std::unique_ptr<libsiedler2::ArchivItem>
    create(libsiedler2::BobType type, libsiedler2::SoundType subtype = libsiedler2::SoundType::None) const override;
};
