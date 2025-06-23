#ifndef RECOMMENDATION_ENGINE_H
#define RECOMMENDATION_ENGINE_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <future>
#include "DataStructures.hpp"
#include "SimilarityCalculator.hpp"
#include "LSHIndex.hpp"

using namespace std;

class RecommendationEngine
{
private:
    const std::unordered_map<uint32_t, UserProfile> &users;
    const std::unordered_map<uint32_t, Movie> &movies;
    const std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, float>>> &movieToUsers;
    const std::unordered_map<uint32_t, std::vector<uint32_t>> &genreToMovies;
    const std::unordered_map<uint32_t, float> &movieAvgRatings;
    const std::unordered_map<uint32_t, int> &moviePopularity;
    float globalAvgRating;

    SimilarityCalculator &similarityCalc;
    LSHIndex& lshIndex;


public:
    RecommendationEngine(
        const std::unordered_map<uint32_t, UserProfile> &u,
        const std::unordered_map<uint32_t, Movie> &m,
        const std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, float>>> &mtu,
        const std::unordered_map<uint32_t, std::vector<uint32_t>> &gtm,
        const std::unordered_map<uint32_t, float> &mar,
        const std::unordered_map<uint32_t, int> &mp,
        float gar,
        SimilarityCalculator &sc,
        LSHIndex& lshIndex);

    // Gera recomendações para um usuário
    std::vector<Recommendation> recommendForUser(uint32_t userId);

private:
    // Encontra usuários candidatos similares
    std::vector<std::pair<uint32_t, int>> findCandidateUsers(
        uint32_t userId,
        const UserProfile &user);

    // Calcula similaridades com candidatos
    std::vector<std::pair<uint32_t, float>> calculateSimilarities(
        uint32_t userId,
        const std::vector<std::pair<uint32_t, int>> &candidates);

    // Gera scores usando collaborative filtering
    std::unordered_map<uint32_t, float> collaborativeFiltering(
        const UserProfile &user,
        const std::vector<std::pair<uint32_t, float>> &similarUsers,
        const std::unordered_set<uint32_t> &watchedMovies);

    // Adiciona scores baseados em conteúdo
    void contentBasedBoost(
        const UserProfile &user,
        const std::unordered_set<uint32_t> &watchedMovies,
        std::unordered_map<uint32_t, float> &scores);

    // Adiciona filmes populares como fallback
    void popularityFallback(
        const std::unordered_set<uint32_t> &watchedMovies,
        std::unordered_map<uint32_t, float> &scores);

    std::vector<pair<uint32_t, int>> findCandidateUsersLSH(uint32_t userId);

};

#endif // RECOMMENDATION_ENGINE_H