#ifndef LSH_INDEX_HPP
#define LSH_INDEX_HPP

#include "Config.hpp"
#include "DataStructures.hpp"


struct MinHashSignature
{
    std::vector<uint32_t> signature;
    uint32_t userId;

    MinHashSignature() : signature(Config::NUM_HASH_FUNCTIONS), userId(0) {}
    MinHashSignature(uint32_t id) : signature(Config::NUM_HASH_FUNCTIONS), userId(id) {}
};

class LSHIndex
{
private:
    std::vector<std::unordered_map<size_t, std::vector<uint32_t>>> tables;

    std::unordered_map<uint32_t, MinHashSignature> signatures;

    struct HashParams
    {
        uint32_t a, b;
    };
    std::vector<std::vector<HashParams>> bandHashParams; 

    mutable std::mutex indexMutex;

    std::mt19937 rng;

public:
    LSHIndex();

    void buildSignatures(
        const std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, float>>> &userRatings,
        int numThreads = 8);

    void indexSignatures();

    std::vector<uint32_t> findSimilarCandidates(
        uint32_t userId,
        int maxCandidates = 500) const;

    float estimateJaccardSimilarity(uint32_t user1, uint32_t user2) const;


private:

    MinHashSignature computeMinHash(
        const std::vector<uint32_t> &movies,
        uint32_t userId);

    size_t hashBand(
        const MinHashSignature &sig,
        int bandIdx,
        int tableIdx) const;

    std::vector<std::pair<uint32_t, uint32_t>> generateHashFunctions();
};

#endif 