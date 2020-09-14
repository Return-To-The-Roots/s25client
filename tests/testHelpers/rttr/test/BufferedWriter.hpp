// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include <s25util/TextWriterInterface.h>
#include <memory>
#include <string>
#include <utility>

namespace rttr { namespace test {
    /// Adapter buffers the current text. If it isn't cleared till the end of the lifetime it will be written to the orig writer
    class BufferedWriter : public TextWriterInterface
    {
    public:
        explicit BufferedWriter(std::shared_ptr<TextWriterInterface> writer) noexcept : origWriter(std::move(writer)) {}
        ~BufferedWriter() override { flush(); }
        void writeText(const std::string& txt, unsigned color) override;
        void flush();

        std::shared_ptr<TextWriterInterface> origWriter;
        std::string curText;
    };

    inline void BufferedWriter::flush()
    {
        // Flush remaining txt
        if(!curText.empty())
        {
            origWriter->writeText(curText, 0);
            curText.clear();
        }
    }

    inline void BufferedWriter::writeText(const std::string& txt, unsigned /*color*/) { curText += txt; }
}} // namespace rttr::test
