#ifndef SIMILARITY_CALCULATOR_HPP
#define SIMILARITY_CALCULATOR_HPP

#include <unordered_map>
#include <mutex>
#include "DataStructures.hpp"

class SimilarityCalculator
{
private:
    const std::unordered_map<uint32_t, UserProfile> &users;
    mutable std::unordered_map<uint64_t, float> cache;
    mutable std::mutex cacheMutex;

public:
    SimilarityCalculator(const std::unordered_map<uint32_t, UserProfile> &u);

    // Calcula similaridade de Pearson entre dois usuários
    float calculatePearsonCorrelation(uint32_t user1, uint32_t user2) const;

    // Calcula similaridade de cosseno (alternativa)
    float calculateCosineSimilarity(uint32_t user1, uint32_t user2) const;

    // Limpa o cache
    void clearCache();

private:
    uint64_t makeKey(uint32_t user1, uint32_t user2) const;
};

#endif // SIMILARITY_CALCULATOR_H