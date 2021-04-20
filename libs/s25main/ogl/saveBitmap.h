// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <libsiedler2/PixelBufferBGRA.h>
#include <boost/filesystem/path.hpp>

void saveBitmap(const libsiedler2::PixelBufferBGRA&, const boost::filesystem::path&);
