#include "SimilarityCalculator.hpp"
#include "Config.hpp"
#include <cmath>
#include <algorithm>

using namespace std;

SimilarityCalculator::SimilarityCalculator(const unordered_map<uint32_t, UserProfile> &u)
    : users(u) {}

uint64_t SimilarityCalculator::makeKey(uint32_t user1, uint32_t user2) const
{
    return ((uint64_t)min(user1, user2) << 32) | max(user1, user2);
}

float SimilarityCalculator::calculateCosineSimilarity(uint32_t user1, uint32_t user2) const
{
    // Verifica cache
    uint64_t key = makeKey(user1, user2);
    {
        lock_guard<mutex> lock(cacheMutex);
        auto it = cache.find(key);
        if (it != cache.end())
            return it->second;
    }

    auto it1 = users.find(user1);
    auto it2 = users.find(user2);

    if (it1 == users.end() || it2 == users.end())
        return 0.0f;

    const auto &ratings1 = it1->second.ratings;
    const auto &ratings2 = it2->second.ratings;

    if (ratings1.size() < Config::MIN_COMMON_ITEMS ||
        ratings2.size() < Config::MIN_COMMON_ITEMS)
    {
        return 0.0f;
    }

    float dotProduct = 0.0f;
    float normA = 0.0f, normB = 0.0f;
    int commonItems = 0;

    // Encontra filmes em comum usando merge (arrays ordenados)
    size_t i = 0, j = 0;
    while (i < ratings1.size() && j < ratings2.size())
    {
        if (ratings1[i].first < ratings2[j].first)
        {
            i++;
        }
        else if (ratings1[i].first > ratings2[j].first)
        {
            j++;
        }
        else
        {
            float r1 = ratings1[i].second;
            float r2 = ratings2[j].second;

            // Cosine similarity calculation
            dotProduct += r1 * r2;
            normA += r1 * r1;
            normB += r2 * r2;
            commonItems++;

            i++;
            j++;
        }
    }

    if (commonItems < Config::MIN_COMMON_ITEMS)
        return 0.0f;

    // Calcula a similaridade do cosseno
    float denominator = sqrt(normA) * sqrt(normB);
    float similarity = (denominator == 0.0f) ? 0.0f : dotProduct / denominator;

    // Armazena no cache
    {
        lock_guard<mutex> lock(cacheMutex);
        cache[key] = similarity;
    }

    return similarity;
}