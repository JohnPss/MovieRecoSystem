#include "FastRecommendationSystem.hpp"
#include "Config.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <memory> // NOVO: Necessário para std::unique_ptr

using namespace std;
using namespace chrono;

FastRecommendationSystem::FastRecommendationSystem() : globalAvgRating(0.0f)
{
    dataLoader = new DataLoader(
        users, movies, genreToId, movieToUsers, genreToMovies,
        globalAvgRating, movieAvgRatings, moviePopularity);

    similarityCalculator = new SimilarityCalculator(users);
    
    // Instancia o índice LSH
    lshIndex = new LSHIndex();

    // Passa a referência do LSH para o motor de recomendação
    recommendationEngine = new RecommendationEngine(
        users, movies, movieToUsers, genreToMovies,
        movieAvgRatings, moviePopularity, globalAvgRating,
        *similarityCalculator, 
        *lshIndex 
    );
}

FastRecommendationSystem::~FastRecommendationSystem()
{
    delete dataLoader;
    delete similarityCalculator;
    delete recommendationEngine;
    delete lshIndex;
}

void FastRecommendationSystem::loadData()
{
    dataLoader->loadRatings(Config::RATINGS_FILE);
    dataLoader->loadMovies(Config::MOVIES_FILE);
    
    if (Config::USE_LSH) {
        cout << "\n--- Iniciando construção do índice LSH ---" << endl;
        
        // --- MODIFICAÇÃO PRINCIPAL: Alocando o mapa no HEAP ---
        // Em vez de um objeto na pilha, usamos um ponteiro inteligente para evitar stack overflow.
        auto userRatingsForLSH = make_unique<unordered_map<uint32_t, vector<pair<uint32_t, float>>>>();
        userRatingsForLSH->reserve(users.size());
        
        for (const auto& [userId, profile] : users) {
            (*userRatingsForLSH)[userId] = profile.ratings;
        }

        // Passamos o mapa para a função LSH usando o desreferenciamento do ponteiro.
        lshIndex->buildSignatures(*userRatingsForLSH, Config::NUM_THREADS);
        
        lshIndex->indexSignatures();
        lshIndex->printStatistics();

        cout << "--- Construção do índice LSH concluída ---\n" << endl;
    }
}

void FastRecommendationSystem::processRecommendations(const string &filename)
{
    vector<uint32_t> userIds = dataLoader->loadUsersToRecommend(filename);

    cout << "\nGerando recomendações para " << userIds.size() << " usuários..." << endl;
    auto totalStart = high_resolution_clock::now();

    for (uint32_t userId : userIds)
    {
        auto start = high_resolution_clock::now();
        vector<Recommendation> recommendations = recommendForUser(userId);
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        cout << "\nUsuário " << userId << " (" << duration.count() << "ms):" << endl;
        printRecommendations(userId, recommendations);
    }

    auto totalEnd = high_resolution_clock::now();
    auto totalDuration = duration_cast<milliseconds>(totalEnd - totalStart);
    cout << "\nTempo de recomendações: " << totalDuration.count() << "ms" << endl;
    cout << "Tempo médio por usuário: " << (userIds.empty() ? 0 : totalDuration.count() / userIds.size()) << "ms" << endl;
}

vector<Recommendation> FastRecommendationSystem::recommendForUser(uint32_t userId)
{
    return recommendationEngine->recommendForUser(userId);
}

void FastRecommendationSystem::printRecommendations(
    uint32_t userId,
    const vector<Recommendation> &recommendations)
{
    int count = 0;
    cout << "  Recomendações:" << endl;
    for (const auto &rec : recommendations)
    {
        auto movieIt = movies.find(rec.movieId);
        if (movieIt != movies.end() && count < 10)
        {
            cout << "    " << (count + 1) << ". " << movieIt->second.title
                 << " (Score: " << fixed << setprecision(2)
                 << rec.score << ")" << endl;
            count++;
        }
    }
}
