// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <s25util/TextWriterInterface.h>
#include <memory>
#include <string>
#include <utility>

namespace rttr::test {
/// Adapter buffers the current text. If it isn't cleared till the end of the lifetime it will be written to the
/// orig writer
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

inline void BufferedWriter::writeText(const std::string& txt, unsigned /*color*/)
{
    curText += txt;
}
} // namespace rttr::test
