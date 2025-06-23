#ifndef FAST_RECOMMENDATION_SYSTEM_H
#define FAST_RECOMMENDATION_SYSTEM_H

#include <unordered_map>
#include <vector>
#include <string>
#include "DataStructures.hpp"
#include "DataLoader.hpp"
#include "SimilarityCalculator.hpp"
#include "RecommendationEngine.hpp"
#include "LSHIndex.hpp" 


class FastRecommendationSystem
{
private:
    // Dados principais
    std::unordered_map<uint32_t, UserProfile> users;
    std::unordered_map<uint32_t, Movie> movies;
    std::unordered_map<std::string, int> genreToId;

    // Índices invertidos
    std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, float>>> movieToUsers;
    std::unordered_map<uint32_t, std::vector<uint32_t>> genreToMovies;

    // Estatísticas
    float globalAvgRating;
    std::unordered_map<uint32_t, float> movieAvgRatings;
    std::unordered_map<uint32_t, int> moviePopularity;

    // Componentes
    DataLoader *dataLoader;
    SimilarityCalculator *similarityCalculator;
    RecommendationEngine *recommendationEngine;
    LSHIndex* lshIndex; 

public:
    FastRecommendationSystem();
    ~FastRecommendationSystem();

    // Carrega dados dos arquivos
    void loadData();

    // Processa recomendações para lista de usuários
    void processRecommendations(const std::string &filename);

    // Gera recomendações para um usuário específico
    std::vector<Recommendation> recommendForUser(uint32_t userId);

private:
    void printRecommendations(uint32_t userId, const std::vector<Recommendation> &recommendations);
};

#endif // FAST_RECOMMENDATION_SYSTEM_H