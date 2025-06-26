#include "FastRecommendationSystem.hpp"
#include "Config.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <memory>
#include <fstream>
#include <thread>       // ADICIONADO: Para usar std::thread
#include <mutex>        // ADICIONADO: Para usar std::mutex e proteger a escrita no arquivo
#include <algorithm>    // ADICIONADO: Para usar std::max

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

// loadData permanece o mesmo
void FastRecommendationSystem::loadData()
{
    dataLoader->loadRatings(Config::RATINGS_FILE);
    dataLoader->loadMovies(Config::MOVIES_FILE);
    
    if (Config::USE_LSH) {
        cout << "\n--- Iniciando construção do índice LSH ---" << endl;
        
        auto userRatingsForLSH = make_unique<unordered_map<uint32_t, vector<pair<uint32_t, float>>>>();
        userRatingsForLSH->reserve(users.size());
        
        for (const auto& [userId, profile] : users) {
            (*userRatingsForLSH)[userId] = profile.ratings;
        }

        lshIndex->buildSignatures(*userRatingsForLSH, Config::NUM_THREADS);
        lshIndex->indexSignatures();
        lshIndex->printStatistics();

        cout << "--- Construção do índice LSH concluída ---\n" << endl;
    }
}

// --- processRecommendations MODIFICADO PARA PARALELISMO ---
void FastRecommendationSystem::processRecommendations(const string &filename)
{
    vector<uint32_t> userIds = dataLoader->loadUsersToRecommend(filename);

    cout << "\nGerando recomendações para " << userIds.size() << " usuários..." << endl;
    auto totalStart = high_resolution_clock::now();

    // Limpa/Sobrescreve o arquivo de resultados uma vez no início.
    ofstream resultFile("result", ios::trunc);
    resultFile.close();

    // 1. Determina o número de threads a serem usados com base no hardware.
    const unsigned int num_threads = std::max(1u, thread::hardware_concurrency());
    cout << "Utilizando " << num_threads << " threads para o processamento." << endl;

    // 2. Cria um mutex para garantir que apenas um thread escreva no arquivo por vez.
    mutex fileMutex;

    // 3. Cria um vetor para guardar nossos threads.
    vector<thread> threads;
    threads.reserve(num_threads);

    // 4. Calcula o tamanho do lote de usuários para cada thread.
    const size_t batch_size = userIds.size() / num_threads;

    // 5. Substitui o loop 'for' sequencial pela criação de threads.
    for (unsigned int i = 0; i < num_threads; ++i)
    {
        size_t start_idx = i * batch_size;
        size_t end_idx = (i == num_threads - 1) ? userIds.size() : (i + 1) * batch_size;

        // Lança um novo thread que executará o código da lambda a seguir.
        threads.emplace_back([this, &userIds, &fileMutex, start_idx, end_idx]() {
            // Cada thread executa este loop para o seu próprio lote de usuários.
            for (size_t j = start_idx; j < end_idx; ++j)
            {
                uint32_t userId = userIds[j];
                
                // A parte de cálculo pesado (CPU-bound) acontece aqui, em paralelo.
                vector<Recommendation> recommendations = recommendForUser(userId);

                // --- SEÇÃO CRÍTICA ---
                // O lock_guard trava o mutex. Apenas um thread pode estar aqui dentro por vez.
                lock_guard<mutex> lock(fileMutex);
                // A escrita no arquivo (I/O-bound) é protegida, evitando corrupção de dados.
                printRecommendations(userId, recommendations);
                // O mutex é liberado automaticamente quando 'lock' sai de escopo.
            }
        });
    }

    // 6. O thread principal espera que todos os outros threads terminem seu trabalho.
    for (auto &t : threads)
    {
        if (t.joinable()) {
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

// printRecommendations permanece o mesmo. A proteção é feita ANTES de chamá-la.
void FastRecommendationSystem::printRecommendations(
    uint32_t userId,
    const vector<Recommendation> &recommendations)
{
    ofstream resultFile("result", ios::app);
    if (!resultFile.is_open()) {
        cerr << "Erro ao abrir arquivo de resultados!" << endl;
        return;
    }

    // Escreve o cabeçalho do usuário no arquivo
    resultFile << "Recomendações para User " << userId << ":" << endl;
    
    int count = 0;
    for (const auto &rec : recommendations)
    {
        auto movieIt = movies.find(rec.movieId);
        if (movieIt != movies.end() && count < 10)
        {
            resultFile << "  " << (count + 1) << ". " << movieIt->second.title
                       << " (Score: " << fixed << setprecision(2)
                       << rec.score << ")" << endl;
            count++;
        }
    }
    resultFile << endl;
    resultFile.close();
}