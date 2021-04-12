// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzFileScan.h"
#include "SfizzForeignPaths.h"
#include "SfizzSettings.h"
#include "NativeHelpers.h"
#include <absl/strings/ascii.h>
#include <absl/algorithm/container.h>
#include <memory>
#if defined(_WIN32)
#include <windows.h>
#endif

// wait at least this much before refreshing the file rescan
// it permits to not repeat the operation many times if many searches are
// requested at once, eg. on session loading with multiple plugin instances
static const std::chrono::seconds expiration_time { 10 };

SfzFileScan& SfzFileScan::getInstance()
{
    static SfzFileScan instance;
    return instance;
}

bool SfzFileScan::locateRealFile(const fs::path& pathOrig, fs::path& pathFound)
{
    if (pathOrig.empty())
        return false;

    std::unique_lock<std::mutex> lock { mutex };
    refreshScan();

    auto it = file_index_.find(keyOf(pathOrig.filename()));
    if (it == file_index_.end())
        return false;

    const std::list<size_t>& candidateIndices = it->second;
    std::vector<fs::path> candidates;
    candidates.reserve(candidateIndices.size());
    for (const size_t index : candidateIndices)
        candidates.push_back(file_trie_[index]);

    lock.unlock();

    pathFound = electBestMatch(pathOrig, candidates);
    return true;
}

bool SfzFileScan::isExpired() const
{
    return !completion_time_ ||
        (clock::now() - *completion_time_) > expiration_time;
}

void SfzFileScan::refreshScan(bool force)
{
    if (!force && !isExpired())
        return;

    file_trie_.clear();
    file_index_.clear();

    FileTrieBuilder builder;

    for (const fs::path& dirPath : SfizzPaths::getSfzSearchPaths()) {
        std::error_code ec;
        const fs::directory_options dirOpts =
            fs::directory_options::skip_permission_denied;

        for (fs::recursive_directory_iterator it(dirPath, dirOpts, ec);
             !ec && it != fs::recursive_directory_iterator();
             it.increment(ec))
        {
            const fs::directory_entry& ent = *it;
            const fs::path& filePath = ent.path();
            std::error_code ec;
            if (ent.is_regular_file(ec) /*&& pathIsSfz(filePath)*/) {
                size_t fileIndex = builder.addFile(filePath);
                file_index_[keyOf(filePath.filename())].push_back(fileIndex);
            }
        }
    }

    file_trie_ = builder.build();
    completion_time_ = clock::now();
}

std::string SfzFileScan::keyOf(const fs::path& path)
{
    std::string key = path.u8string();
    absl::AsciiStrToLower(&key);
    return key;
}

namespace SfzFileScanImpl {
template <class T>
bool asciiCaseEqual(const std::basic_string<T>& a, const std::basic_string<T>& b)
{
    const size_t n = a.size();
    if (n != b.size())
        return false;

    auto lower = [](T c) -> T {
        return (c >= T('A') && c <= T('Z')) ? (c - T('A') + T('a')) : c;
    };

    for (size_t i = 0; i < n; ++i)
        if (lower(a[i]) != lower(b[i]))
            return false;

    return true;
}
} // namespace SfzFileScanImpl

bool SfzFileScan::pathIsSfz(const fs::path& path)
{
    const fs::path::string_type& str = path.native();
    using char_type = fs::path::value_type;
    const size_t n = str.size();
    return n > 4 &&
        str[n - 4] == char_type('.') &&
        (str[n - 3] == char_type('s') || str[n - 3] == char_type('S')) &&
        (str[n - 2] == char_type('f') || str[n - 2] == char_type('F')) &&
        (str[n - 1] == char_type('z') || str[n - 1] == char_type('Z'));
}

const fs::path& SfzFileScan::electBestMatch(const fs::path& path, absl::Span<const fs::path> candidates)
{
    if (candidates.empty())
        return path;

    if (candidates.size() == 1)
        return candidates.front();

    struct Score {
        size_t components = 0;
        size_t exact = 0;
        bool operator<(const Score& other) const noexcept
        {
            return (components != other.components) ?
                (components < other.components) : (exact < other.exact);
        }
    };

    std::vector<Score> scores;
    scores.reserve(candidates.size());

    for (size_t i = 0, n = candidates.size(); i < n; ++i) {
        scores.emplace_back();
        Score& score = scores.back();

        const fs::path& p1 = path;
        const fs::path& p2 = candidates[i];
        auto it1 = p1.end();
        auto it2 = p2.end();

        bool matching = true;
        while (matching && it1-- != p1.begin() && it2-- != p2.begin()) {
            const fs::path& c1 = *it1;
            const fs::path& c2 = *it2;
            if (c1 == c2) {
                score.components += 1;
                score.exact += 1;
            }
            else if (SfzFileScanImpl::asciiCaseEqual(c1.native(), c2.native()))
                score.components += 1;
            else
                matching = false;
        }
    }

    size_t best = 0;
    for (size_t i = 1, n = scores.size(); i < n; ++i) {
        if (scores[best] < scores[i])
            best = i;
    }

    return candidates[best];
}

//------------------------------------------------------------------------------

namespace SfizzPaths {

std::vector<fs::path> getSfzSearchPaths()
{
    std::vector<fs::path> paths;
    paths.reserve(8);
    auto addPath = [&paths](const fs::path& newPath) {
        if (absl::c_find(paths, newPath) == paths.end())
            paths.push_back(newPath);
    };

    absl::optional<fs::path> configDefaultPath = getSfzConfigDefaultPath();
    fs::path fallbackDefaultPath = getSfzFallbackDefaultPath();

    if (configDefaultPath)
        addPath(*configDefaultPath);
    addPath(fallbackDefaultPath);

    for (const fs::path& path : getEnvironmentSfzPaths())
        addPath(path);

    for (const fs::path& foreign : {
            getAriaPathSetting("user_files_dir"),
            getAriaPathSetting("Converted_path") })
        if (!foreign.empty() && foreign.is_absolute())
            addPath(foreign);

    paths.shrink_to_fit();
    return paths;
}

absl::optional<fs::path> getSfzConfigDefaultPath()
{
    SfizzSettings settings;
    fs::path path = fs::u8path(settings.load_or("user_files_dir", {}));
    if (path.empty() || !path.is_absolute())
        return {};
    return path;
}

void setSfzConfigDefaultPath(const fs::path& path)
{
    if (path.empty() || !path.is_absolute())
        return;
    SfizzSettings settings;
    settings.store("user_files_dir", path.u8string());
}

fs::path getSfzFallbackDefaultPath()
{
    return getUserDocumentsDirectory() / "SFZ instruments";
}

std::vector<fs::path> getEnvironmentSfzPaths()
{
    std::vector<fs::path> paths;

#if defined(_WIN32)
    std::unique_ptr<WCHAR[]> buf;

    DWORD bufsize = GetEnvironmentVariableW(L"SFZ_PATH", nullptr, 0);
    if (bufsize == 0)
        return {};

    buf.reset(new WCHAR[bufsize]);
    if (GetEnvironmentVariableW(L"SFZ_PATH", buf.get(), bufsize) != bufsize - 1)
        return {};

    paths.reserve(8);

    const WCHAR* env = buf.get();
    while (*env) {
        const WCHAR* endp;
        for (endp = env; *endp && *endp != L';'; ++endp);
        fs::path path = fs::path(env, endp);
        if (!path.empty() && path.is_absolute())
            paths.push_back(std::move(path));
        env = *endp ? (endp + 1) : endp;
    }
#else
    const char* env = getenv("SFZ_PATH");
    if (!env)
        return {};

    paths.reserve(8);

    while (*env) {
        const char* endp;
        for (endp = env; *endp != ':' && *endp != '\0'; ++endp);
        fs::path path = fs::u8path(env, endp);
        if (!path.empty() && path.is_absolute())
            paths.push_back(std::move(path));
        env = *endp ? (endp + 1) : endp;
    }
#endif

    return paths;
}

} // namespace SfizzPaths
