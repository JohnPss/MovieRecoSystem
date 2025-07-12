#ifndef SIMILARITY_CALCULATOR_HPP
#define SIMILARITY_CALCULATOR_HPP

#include "Config.hpp"
#include "DataStructures.hpp"

class SimilarityCalculator
{
private:
    const std::unordered_map<uint32_t, UserProfile> &users;
    mutable std::unordered_map<uint64_t, float> cache;
    mutable std::mutex cacheMutex;

public:
    SimilarityCalculator(const std::unordered_map<uint32_t, UserProfile> &u);

    float calculateCosineSimilarity(uint32_t user1, uint32_t user2) const;

private:
    uint64_t makeKey(uint32_t user1, uint32_t user2) const;
};

#endif 