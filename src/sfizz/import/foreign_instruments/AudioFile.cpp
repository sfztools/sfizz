// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "AudioFile.h"
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
#if __cplusplus >= 202002L
    auto ext_u8 = path.extension().u8string();
    auto ext = (const char*)ext_u8.c_str();
#else
    const std::string ext = path.extension().u8string();
#endif

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
#if __cplusplus >= 202002L
    os << "<region>sample=" << (const char*)path.filename().u8string().c_str();
#else
    os << "<region>sample=" << path.filename().u8string();
#endif
    return os.str();
}

const InstrumentFormat* AudioFileInstrumentImporter::getFormat() const noexcept
{
    return &AudioFileInstrumentFormat::getInstance();
}

} // namespace sfz
