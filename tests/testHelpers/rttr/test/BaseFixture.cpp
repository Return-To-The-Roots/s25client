// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "BaseFixture.hpp"
#include "BufferedWriter.hpp"
#include "s25util/AvoidDuplicatesWriter.h"
#include "s25util/LocaleHelper.h"
#include "s25util/Log.h"
#include "s25util/NullWriter.h"
#include "s25util/StdStreamWriter.h"
#include <cstdlib>
#include <ctime>

rttr::test::BaseFixture::BaseFixture()
{
    if(!LocaleHelper::init())
        throw std::runtime_error("Could not init locale");
    // Don't write to file
    LOG.setWriter(new NullWriter(), LogTarget::File);
    // Filter everything so FileAndStdout won't result in duplicate lines and store text for tests
    LOG.setWriter(new AvoidDuplicatesWriter(std::make_shared<BufferedWriter>(std::make_shared<StdStreamWriter>(true))),
                  LogTarget::StdoutAndStderr);
    srand(static_cast<unsigned>(time(nullptr)));
}
