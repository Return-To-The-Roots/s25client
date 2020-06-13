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

#include "convertSounds.h"
#include <libsiedler2/Archiv.h>
#include <libsiedler2/ArchivItem_Sound_Wave.h>
#include <libsiedler2/loadMapping.h>
#include <s25util/StringConversion.h>
#include <cmath>
#include <samplerate.hpp>
#include <sstream>
#include <stdexcept>
#include <vector>

void convertSounds(libsiedler2::Archiv& sounds, const boost::filesystem::path& scriptPath)
{
    samplerate::State converter(samplerate::Converter::SincFastest, 1);
    std::vector<float> input, output;
    libsiedler2::loadMapping(scriptPath, [&sounds, &converter, &input, &output](unsigned idx, const std::string& sFrequency) {
        constexpr unsigned targetFrequency = 44100;

        const auto frequency = s25util::fromStringClassic<unsigned>(sFrequency);
        auto* sound = dynamic_cast<libsiedler2::ArchivItem_Sound_Wave*>(sounds[idx]);
        if(!sound)
            throw std::runtime_error("No wave sound at index " + std::to_string(idx));
        auto header = sound->getHeader();
        if(header.numChannels != 1)
            throw std::runtime_error("Unexpected number of channels for item " + std::to_string(idx));
        if(header.frameSize != 1 || header.bitsPerSample != 8)
            throw std::runtime_error("Unsupported format for item " + std::to_string(idx));
        converter.reset();
        const double rate = static_cast<double>(targetFrequency) / frequency;
        input.resize(sound->getData().size());
        std::transform(sound->getData().begin(), sound->getData().end(), input.begin(),
                       [](uint8_t value) { return static_cast<float>(value) / std::numeric_limits<uint8_t>::max() * 2.f - 1.f; });
        output.resize(static_cast<size_t>(std::ceil(input.size() * rate)));
        const auto result = converter.process(samplerate::Data(input.data(), input.size(), output.data(), output.size(), rate));
        std::vector<uint8_t> data(result.output_frames_gen);
        std::transform(output.begin(), output.begin() + result.output_frames_gen, data.begin(), [](float value) {
            int converted = std::lrint((value + 1.f) / 2.f * std::numeric_limits<uint8_t>::max());
            return static_cast<uint8_t>(std::min<int>(std::numeric_limits<uint8_t>::max(), std::max(0, converted)));
        });
        header.samplesPerSec = targetFrequency;
        header.bytesPerSec = targetFrequency;
        header.frameSize = 1;
        header.bitsPerSample = 8;
        header.dataSize = data.size();
        header.fileSize = data.size() + sizeof(header);
        sound->setHeader(header);
        sound->setData(data);
    });
}
