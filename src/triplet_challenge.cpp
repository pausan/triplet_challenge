/**
 * triplet_challenge is an application that extracts the top 3 triplet words of a file
 *
 * Copyright (C) 2018 Pablo Marcos Oltra
 *
 * This file is part of gencc.
 *
 * gencc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gencc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gencc.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "triplet_challenge.h"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <utility>
#include <thread>
#include <atomic>
#include <vector>

//#define logDebug(...) fprintf(stderr, "debug: " __VA_ARGS__)
#define logDebug(...)

bool shouldSkipCharacter(const char c) {
    return !(std::isalnum(c) || c == '\'');
}

std::size_t findGeneric(std::string_view buffer, FindType type) {
    auto method = [type](auto c) {
        if (type == FindType::FIRST_CHARACTER) {
            return shouldSkipCharacter(c);
        } else {
            return !shouldSkipCharacter(c);
        }
    };

    std::size_t offset = 0;
    for (const auto& c : buffer) {
        if (method(c)) {
            offset++;
        } else {
            break;
        }
    }

    return offset;
}

std::size_t findFirstCharacter(std::string_view buffer) {
    return findGeneric(buffer, FindType::FIRST_CHARACTER);
}

std::size_t findFirstNonCharacter(std::string_view buffer) {
    return findGeneric(buffer, FindType::FIRST_NON_CHARACTER);
}

std::size_t getNextWord(std::string_view buffer, std::string& word) {
    word.clear();

    // Find the current word and store it in the variable
    auto offset = findFirstNonCharacter(buffer);
    word = buffer.substr(0, offset);

    // Skip characters until the beginning of the next word
    if (offset <= buffer.size()) {
        buffer = buffer.substr(offset);
    }
    offset += findFirstCharacter(buffer);;

    return offset;
}

std::size_t getTripletIndex(ssize_t firstWord, ssize_t offset) {
    firstWord %= 3;
    offset %= 3;
    return (firstWord + (offset < 0 ? offset + 3 : offset)) % 3;
}

using TripletMap = std::unordered_map<std::string, std::size_t>;
static std::atomic<std::size_t> g_numberOfWords = 0;
static std::size_t g_numThreads = std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 1;
static std::vector<TripletMap> g_map(g_numThreads);
void calculateTripletsForOneThread(size_t threadNumber, std::string_view buffer) {
    std::array<std::string, 3> words;
    std::size_t firstWord = 0;
    std::size_t offset = 0;
    std::unordered_map<std::string, std::size_t> map;
    std::string tripletKey;
    std::size_t numberOfWords = 0;

    logDebug("Buffer passed with size %zu bytes\n", buffer.size());

    while (offset < buffer.size()) {
        auto& word = words[firstWord];
        offset += getNextWord(buffer.substr(offset), word);
        std::transform(word.begin(), word.end(), word.begin(), tolower);
        logDebug("[%zu] Word extracted: %.*s\n", threadNumber, static_cast<int>(word.size()), word.data());

        const auto& prevWord = words[getTripletIndex(firstWord, -1)];
        const auto& prevPrevWord = words[getTripletIndex(firstWord, -2)];

        if (!prevPrevWord.empty() && !prevWord.empty() && !word.empty()) {
            tripletKey = prevPrevWord + " " + prevWord + " " + word;
            std::size_t count = 0;
            (void)count; // get rid of warning of unused variable
            count = ++g_map[threadNumber][tripletKey];
            logDebug("[%zu] Increasing triplet \"%s\" count to %zu\n", threadNumber, tripletKey.c_str(), count);
        }

        firstWord = (firstWord+1) % 3;
        numberOfWords++;
    }

    g_numberOfWords += numberOfWords;
}

TripletResult calculateTriplets(std::string_view buffer) {
    TripletResult result;
    std::size_t startOffset, endOffset = 0;
    std::size_t chunkSize = buffer.size() / g_numThreads;
    std::vector<std::unique_ptr<std::thread>> threads(g_numThreads);
    TripletMap finalMap;

    logDebug("Buffer passed with size %zu bytes\n", buffer.size());

    // Split buffer into chunks of similar size
    for (std::size_t nThread = 0; nThread < g_numThreads; ++nThread) {
        startOffset = endOffset;
        endOffset += chunkSize;
        std::size_t chunkOffset = endOffset;
        if (endOffset < buffer.size()) {
            std::string dummy;
            endOffset += findFirstNonCharacter(buffer.substr(endOffset, buffer.size() - endOffset));
            chunkOffset = endOffset;
            for (std::size_t i = 0; i < 3; ++i) {
                chunkOffset += getNextWord(buffer.substr(chunkOffset, buffer.size() - chunkOffset), dummy);
            }
        } else {
            endOffset = buffer.size();
            chunkOffset = endOffset;
        }
        const auto bufferSize = chunkOffset - startOffset;
        if (bufferSize > 0) {
            auto threadBuffer = buffer.substr(startOffset, bufferSize);
            logDebug("Chunk for thread %zu: %.*s\n", nThread, static_cast<int>(threadBuffer.size()), threadBuffer.data());
            threads[nThread] = std::make_unique<std::thread>([nThread, threadBuffer] {
                calculateTripletsForOneThread(nThread, threadBuffer);
            });
        }
    }

    for (auto& thread : threads) {
        thread->join();
    }

    // Collect data of all threads into the final map
    for (const auto& map : g_map) {
        for (const auto& triplet : map) {
            const auto& tripletKey = triplet.first;
            auto& count = finalMap[tripletKey];
            count += triplet.second;

            // Update the result
            for (std::size_t i = 0; i < result.size(); ++i) {
                if (count > result[i].count) {
                    auto updateTriplet = Triplet{tripletKey, count};
                    logDebug("Updating top triplet \"%s\" with count %zu\n", tripletKey.c_str(), count);
                    for (auto j = i; j < result.size(); ++j) {
                        updateTriplet = std::exchange(result[j], updateTriplet);
                        if (updateTriplet.words == tripletKey)
                            break;
                    }
                    /*for (const auto& triplet : result) {
                    logDebug("\t\t\t%s:%zu\n", triplet.words.c_str(), triplet.count);
                    }*/
                    break;
                }
            }
        }
    }

    fprintf(stderr, "Used %zu threads\nNumber of words: %zu\nNumber of triplets: %zu\n\n", g_numThreads,
            g_numberOfWords.load(), finalMap.size());

    return result;
}
