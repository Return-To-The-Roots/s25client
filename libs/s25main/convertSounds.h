//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem/path.hpp>

namespace libsiedler2 {
class Archiv;
}

void convertSounds(libsiedler2::Archiv& sounds, const boost::filesystem::path& scriptPath);
