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
#include <boost/nowide/fstream.hpp>
#include <cmath>
#include <samplerate.hpp>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace bnw = boost::nowide;

bool convertSounds(libsiedler2::Archiv& sounds, const bfs::path& scriptPath)
{
    bnw::ifstream file(scriptPath); // script
    samplerate::State converter(samplerate::Converter::SincFastest, 1);
    std::vector<float> input, output;
    std::string line;
    const unsigned targetFrequency = 44100;
    while(std::getline(file, line))
    {
        if(line.empty() || line[0] == '#' || line == "empty")
            continue;
        int item, frequency;
        std::istringstream ss(line);
        if(!(ss >> item >> frequency))
            continue;
        auto* sound = dynamic_cast<libsiedler2::ArchivItem_Sound_Wave*>(sounds[item]);
        if(!sound)
            return false;
        auto header = sound->getHeader();
        if(header.samplesPerSec == targetFrequency)
            continue;
        if(header.numChannels != 1)
            throw std::runtime_error("Unexpected number of channels for item " + std::to_string(item));
        if(header.frameSize != 1 || header.bitsPerSample != 8)
            throw std::runtime_error("Unsupported format for item " + std::to_string(item));
        converter.reset();
        double rate = static_cast<double>(targetFrequency) / sound->getHeader().samplesPerSec;
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
    }
    return true;
}
