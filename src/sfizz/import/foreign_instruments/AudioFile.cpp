// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "AudioFile.h"
#include "utility/U8Strings.h"
#include <absl/strings/match.h>
#include <absl/strings/string_view.h>
#include <absl/memory/memory.h>
#include <locale>
#include <sstream>

namespace sfz {

static const char* kRecognizedAudioExtensions[] = {
    ".wav", ".flac", ".ogg", ".mp3", ".aif", ".aiff", ".aifc", ".wv",
};

///
AudioFileInstrumentFormat& AudioFileInstrumentFormat::getInstance()
{
    static AudioFileInstrumentFormat format;
    return format;
}

const char* AudioFileInstrumentFormat::name() const noexcept
{
    return "Audio file";
}

bool AudioFileInstrumentFormat::matchesFilePath(const fs::path& path) const
{
    const std::string ext = u8EncodedString(path.extension());
    for (absl::string_view knownExt : kRecognizedAudioExtensions) {
        if (absl::EqualsIgnoreCase(ext, knownExt))
            return true;
    }

    return false;
}

std::unique_ptr<InstrumentImporter> AudioFileInstrumentFormat::createImporter() const
{
    return absl::make_unique<AudioFileInstrumentImporter>();
}

///
std::string AudioFileInstrumentImporter::convertToSfz(const fs::path& path) const
{
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << "<region>sample=" << u8EncodedString(path.filename());
    return os.str();
}

const InstrumentFormat* AudioFileInstrumentImporter::getFormat() const noexcept
{
    return &AudioFileInstrumentFormat::getInstance();
}

} // namespace sfz
