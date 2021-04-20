// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
