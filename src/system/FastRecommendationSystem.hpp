#ifndef FAST_RECOMMENDATION_SYSTEM_H
#define FAST_RECOMMENDATION_SYSTEM_H

#include "../core/Config.hpp"
#include "../data/DataLoader.hpp"
#include "../algorithms/SimilarityCalculator.hpp"
#include "../algorithms/RecommendationEngine.hpp"
#include "../algorithms/LSHIndex.hpp"

class FastRecommendationSystem
{
private:
    
    std::unordered_map<uint32_t, UserProfile> users;
    std::unordered_map<uint32_t, Movie> movies;
    std::unordered_map<std::string, int> genreToId;

    
    std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, float>>> movieToUsers;
    std::unordered_map<uint32_t, std::vector<uint32_t>> genreToMovies;

    
    float globalAvgRating;
    std::unordered_map<uint32_t, float> movieAvgRatings;
    std::unordered_map<uint32_t, int> moviePopularity;

    
    DataLoader *dataLoader;
    SimilarityCalculator *similarityCalculator;
    RecommendationEngine *recommendationEngine;
    LSHIndex *lshIndex;

public:
    FastRecommendationSystem();
    ~FastRecommendationSystem();

    
    void loadData();

    
    void processRecommendations(const std::string &filename);

    
    std::vector<Recommendation> recommendForUser(uint32_t userId);

private:
    void printRecommendations(uint32_t userId, const std::vector<Recommendation> &recommendations);
};

#endif 