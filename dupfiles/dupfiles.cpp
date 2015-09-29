#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <dupfiles.hpp>
#ifdef _MSC_VER
#include <iso646.h>
#endif
#include <boost/filesystem.hpp>
#include <boost/cstdint.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include "hash_digest.hpp"

namespace dupfiles {

static HashDigest hash_file(const boost::filesystem::directory_entry & entry)
{
    boost::iostreams::mapped_file_source map(entry.path());
    auto hash = sha512(static_cast<const void *>(map.data()), map.size());
    map.close();
    return hash;
}

std::vector<std::vector<std::string>> findDuplicates(const std::string & path, const ErrorCallback & error_callback)
{
    if (not boost::filesystem::is_directory(path)) {
        throw NotADirectory();
    }

    std::vector<std::vector<std::string>> result;
    std::unordered_map<
        boost::uintmax_t,
        std::unordered_map<
            HashDigest,
            std::vector<boost::filesystem::directory_entry>>> map;

    boost::filesystem::recursive_directory_iterator iter(path);
    std::vector<std::string> group;
    for (const auto entry : iter) {
        if (boost::filesystem::is_directory(entry)) {
            continue;
        }

        auto size = boost::filesystem::file_size(entry);

	try {
            auto entry_hash = size == 0 ? HashDigest() : hash_file(entry);
            map[size][entry_hash].push_back(std::move(entry));
        } catch (const std::exception & e) {
            error_callback(e.what());
            continue;
        }
    }

    for (const auto iter1 : map) {
        for (const auto iter2 : iter1.second) {
            std::vector<std::string> paths;
            if (iter2.second.size() > 1) {
                for (const auto path_iter : iter2.second) {
                    paths.push_back(std::move(path_iter.path().string()));
                }
                result.push_back(std::move(paths));
            }
        }
    }

    return result;
}

}
