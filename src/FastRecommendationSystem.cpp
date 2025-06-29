#include "FastRecommendationSystem.hpp"
#include "Config.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <memory>
#include <fstream>
#include <thread>
#include <mutex>
#include <algorithm>
#include <filesystem>

using namespace std;
using namespace chrono;

// Construtor e Destrutor permanecem os mesmos
FastRecommendationSystem::FastRecommendationSystem() : globalAvgRating(0.0f)
{
    dataLoader = new DataLoader(
        users, movies, genreToId, movieToUsers, genreToMovies,
        globalAvgRating, movieAvgRatings, moviePopularity);
    similarityCalculator = new SimilarityCalculator(users);
    lshIndex = new LSHIndex();
    recommendationEngine = new RecommendationEngine(
        users, movies, movieToUsers, genreToMovies,
        movieAvgRatings, moviePopularity, globalAvgRating,
        *similarityCalculator, *lshIndex);
}

FastRecommendationSystem::~FastRecommendationSystem()
{
    delete dataLoader;
    delete similarityCalculator;
    delete recommendationEngine;
    delete lshIndex;
}

// loadData permanece o mesmo
void FastRecommendationSystem::loadData()
{
    dataLoader->loadRatings(Config::RATINGS_FILE);
    dataLoader->loadMovies(Config::MOVIES_FILE);

    cout << "\n--- Iniciando construção do índice LSH ---" << endl;

    auto userRatingsForLSH = make_unique<unordered_map<uint32_t, vector<pair<uint32_t, float>>>>();
    userRatingsForLSH->reserve(users.size());

    for (const auto &[userId, profile] : users)
    {
        (*userRatingsForLSH)[userId] = profile.ratings;
    }

    lshIndex->buildSignatures(*userRatingsForLSH, Config::NUM_THREADS);
    lshIndex->indexSignatures();
    lshIndex->printStatistics();

    cout << "--- Construção do índice LSH concluída ---\n"
         << endl;
}

// processRecommendations permanece o mesmo
void FastRecommendationSystem::processRecommendations(const string &filename)
{
    vector<uint32_t> userIds = dataLoader->loadUsersToRecommend(filename);

    cout << "\nGerando recomendações para " << userIds.size() << " usuários..." << endl;
    auto totalStart = high_resolution_clock::now();

    filesystem::create_directory("outcome");
    // Usaremos um arquivo diferente para a saída de debug para não bagunçar a saída oficial
    ofstream resultFile("outcome/debug_recommendations.txt", ios::trunc);
    resultFile.close();

    mutex fileMutex;
    vector<thread> threads;
    const unsigned int num_threads = std::max(1u, thread::hardware_concurrency());
    threads.reserve(num_threads);
    const size_t batch_size = userIds.size() / num_threads;

    for (unsigned int i = 0; i < num_threads; ++i)
    {
        size_t start_idx = i * batch_size;
        size_t end_idx = (i == num_threads - 1) ? userIds.size() : (i + 1) * batch_size;
        threads.emplace_back([this, &userIds, &fileMutex, start_idx, end_idx]()
                             {
            for (size_t j = start_idx; j < end_idx; ++j) {
                uint32_t userId = userIds[j];
                vector<Recommendation> recommendations = recommendForUser(userId);
                lock_guard<mutex> lock(fileMutex);
                printRecommendations(userId, recommendations);
            } });
    }

    for (auto &t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    auto totalEnd = high_resolution_clock::now();
    auto totalDuration = duration_cast<milliseconds>(totalEnd - totalStart);
    cout << "\nTempo total de recomendações: " << totalDuration.count() << "ms" << endl;
    cout << "Tempo médio por usuário: " << (userIds.empty() ? 0 : totalDuration.count() / (double)userIds.size()) << "ms" << endl;
}

// recommendForUser permanece o mesmo
vector<Recommendation> FastRecommendationSystem::recommendForUser(uint32_t userId)
{
    return recommendationEngine->recommendForUser(userId);
}

// *** MUDANÇA: Função restaurada para a versão de debug com scores e títulos ***
void FastRecommendationSystem::printRecommendations(
    uint32_t userId,
    const vector<Recommendation> &recommendations)
{
    // --- 1. Saída Oficial para o Trabalho (formato: ID_USUARIO ID_FILME1 ID_FILME2 ...) ---
    {
        ofstream resultFile(Config::OUTPUT_FILE, ios::app);
        if (resultFile.is_open())
        {
            resultFile << userId;
            for (const auto &rec : recommendations)
            {
                resultFile << " " << rec.movieId;
            }
            resultFile << endl;
        }
        else
        {
            cerr << "ERRO: Não foi possível abrir o arquivo de saída oficial '" << Config::OUTPUT_FILE << "'!" << endl;
        }
    }

    // --- 2. Saída de Debug com Scores (fácil de remover) ---
    // Para desativar a geração deste arquivo para a entrega final,
    // basta comentar o bloco de código abaixo (da chave '{' até a chave '}').
    {
        ofstream debugFile(Config::DEBUG_OUTPUT_FILE, ios::app);
        if (debugFile.is_open())
        {
            debugFile << "Recomendações para User " << userId << ":" << endl;
            if (recommendations.empty())
            {
                debugFile << "  Nenhuma recomendação encontrada." << endl;
            }
            else
            {
                int count = 0;
                for (const auto &rec : recommendations)
                {
                    auto movieIt = movies.find(rec.movieId);
                    if (movieIt != movies.end() && count < Config::TOP_K)
                    {
                        debugFile << "  " << (count + 1) << ". " << movieIt->second.title
                                  << " (MovieID: " << rec.movieId << ", Score: " << fixed << setprecision(4)
                                  << rec.score << ")" << endl;
                        count++;
                    }
                }
            }
            debugFile << endl;
        }
        else
        {
            cerr << "ERRO: Não foi possível abrir o arquivo de debug '" << Config::DEBUG_OUTPUT_FILE << "'!" << endl;
        }
    }
}