#include "RecommendationEngine.hpp"
#include "Config.hpp"
#include <algorithm>
#include <cmath>
#include <future>

using namespace std;

RecommendationEngine::RecommendationEngine(
    const unordered_map<uint32_t, UserProfile> &u,
    const unordered_map<uint32_t, Movie> &m,
    const unordered_map<uint32_t, vector<pair<uint32_t, float>>> &mtu,
    const unordered_map<uint32_t, vector<uint32_t>> &gtm,
    const unordered_map<uint32_t, float> &mar,
    const unordered_map<uint32_t, int> &mp,
    float gar,
    SimilarityCalculator &sc,
    LSHIndex &lsh) : users(u), movies(m), movieToUsers(mtu), genreToMovies(gtm),
                     movieAvgRatings(mar), moviePopularity(mp), globalAvgRating(gar),
                     similarityCalc(sc), lshIndex(lsh) {}

vector<Recommendation> RecommendationEngine::recommendForUser(uint32_t userId)
{
    auto it = users.find(userId);
    if (it == users.end())
    {
        return {};
    }

    const UserProfile &user = it->second;

    // Obtém filmes já assistidos
    unordered_set<uint32_t> watchedMovies;
    for (const auto &[movieId, _] : user.ratings)
    {
        watchedMovies.insert(movieId);
    }

    // 1. Encontra candidatos similares
    // 1. Encontra candidatos similares (PONTO DA OTIMIZAÇÃO)
    vector<pair<uint32_t, int>> candidates;
    if (Config::USE_LSH)
    {
        // NOVO: Chama a versão rápida com LSH
        candidates = findCandidateUsersLSH(userId, user);
    }
    else
    {
        // Mantém o método antigo como fallback
        candidates = findCandidateUsers(userId, user);
    }

    // 2. Calcula similaridades
    auto similarUsers = calculateSimilarities(userId, candidates);

    // 3. Collaborative filtering
    auto scores = collaborativeFiltering(user, similarUsers, watchedMovies);

    // 4. Content-based boost
    contentBasedBoost(user, watchedMovies, scores);

    // 5. Popularity fallback
    if (scores.size() < Config::TOP_K)
    {
        popularityFallback(watchedMovies, scores);
    }

    // Converte para vetor e ordena
    vector<Recommendation> recommendations;
    recommendations.reserve(scores.size());

    for (const auto &[movieId, score] : scores)
    {
        recommendations.emplace_back(movieId, score);
    }

    sort(recommendations.begin(), recommendations.end(),
         [](const auto &a, const auto &b)
         { return a.score > b.score; });

    if (recommendations.size() > Config::TOP_K)
    {
        recommendations.resize(Config::TOP_K);
    }

    return recommendations;
}

vector<pair<uint32_t, int>> RecommendationEngine::findCandidateUsers(
    uint32_t userId,
    const UserProfile &user)
{
    unordered_map<uint32_t, int> candidateCount;

    for (const auto &[movieId, _] : user.ratings)
    {
        auto it = movieToUsers.find(movieId);
        if (it != movieToUsers.end())
        {
            for (const auto &[otherUser, _] : it->second)
            {
                if (otherUser != userId)
                {
                    candidateCount[otherUser]++;
                }
            }
        }
    }

    // Filtra candidatos com pelo menos MIN_COMMON_ITEMS filmes em comum
    vector<pair<uint32_t, int>> filteredCandidates;
    for (const auto &[candidateId, count] : candidateCount)
    {
        if (count >= Config::MIN_COMMON_ITEMS)
        {
            filteredCandidates.push_back({candidateId, count});
        }
    }

    // Ordena por número de filmes em comum
    sort(filteredCandidates.begin(), filteredCandidates.end(),
         [](const auto &a, const auto &b)
         { return a.second > b.second; });

    if (filteredCandidates.size() > Config::MAX_CANDIDATES)
    {
        filteredCandidates.resize(Config::MAX_CANDIDATES);
    }

    return filteredCandidates;
}

vector<pair<uint32_t, float>> RecommendationEngine::calculateSimilarities(
    uint32_t userId,
    const vector<pair<uint32_t, int>> &candidates)
{
    vector<pair<uint32_t, float>> similarUsers;

    // Processa em batches para paralelização controlada
    for (size_t i = 0; i < candidates.size(); i += Config::BATCH_SIZE)
    {
        size_t end = min(i + Config::BATCH_SIZE, candidates.size());
        vector<future<pair<uint32_t, float>>> futures;

        // Processa batch em paralelo
        for (size_t j = i; j < end; ++j)
        {
            uint32_t candidateId = candidates[j].first;
            futures.push_back(async(launch::async,
                                    [this, userId, candidateId]()
                                    {
                                        float sim = similarityCalc.calculatePearsonCorrelation(userId, candidateId);
                                        return make_pair(candidateId, sim);
                                    }));
        }

        // Coleta resultados
        for (auto &f : futures)
        {
            auto result = f.get();
            if (result.second > Config::MIN_SIMILARITY)
            {
                similarUsers.push_back(result);
            }
        }
    }

    // Ordena por similaridade
    sort(similarUsers.begin(), similarUsers.end(),
         [](const auto &a, const auto &b)
         { return a.second > b.second; });

    if (similarUsers.size() > Config::MAX_SIMILAR_USERS)
    {
        similarUsers.resize(Config::MAX_SIMILAR_USERS);
    }

    return similarUsers;
}

unordered_map<uint32_t, float> RecommendationEngine::collaborativeFiltering(
    const UserProfile &user,
    const vector<pair<uint32_t, float>> &similarUsers,
    const unordered_set<uint32_t> &watchedMovies)
{
    (void)user;
    unordered_map<uint32_t, float> scores;
    float totalSim = 0;

    for (const auto &[simUserId, similarity] : similarUsers)
    {
        totalSim += similarity;
        auto it = users.find(simUserId);
        if (it == users.end())
            continue;

        const auto &simUserRatings = it->second.ratings;
        float simUserAvg = it->second.avgRating;

        for (const auto &[movieId, rating] : simUserRatings)
        {
            if (watchedMovies.find(movieId) == watchedMovies.end())
            {
                scores[movieId] += similarity * (rating - simUserAvg);
            }
        }
    }

    // Normaliza e adiciona baseline
    if (totalSim > 0)
    {
        for (auto &[movieId, score] : scores)
        {
            score = score / totalSim;
            auto it = movieAvgRatings.find(movieId);
            if (it != movieAvgRatings.end())
            {
                score += it->second;
            }
        }
    }

    return scores;
}

void RecommendationEngine::contentBasedBoost(
    const UserProfile &user,
    const unordered_set<uint32_t> &watchedMovies,
    unordered_map<uint32_t, float> &scores)
{
    if (user.preferredGenres == 0)
        return;

    for (int i = 0; i < 32; ++i)
    {
        if (user.preferredGenres & (1 << i))
        {
            auto it = genreToMovies.find(i);
            if (it != genreToMovies.end())
            {
                for (uint32_t movieId : it->second)
                {
                    if (watchedMovies.find(movieId) == watchedMovies.end())
                    {
                        auto avgIt = movieAvgRatings.find(movieId);
                        auto popIt = moviePopularity.find(movieId);

                        if (avgIt != movieAvgRatings.end() && popIt != moviePopularity.end())
                        {
                            float boost = 0.5f * (avgIt->second / 5.0f) +
                                          0.5f * min(1.0f, static_cast<float>(log(popIt->second + 1) / 10.0));
                            scores[movieId] += boost * Config::CB_WEIGHT;
                        }
                    }
                }
            }
        }
    }
}

void RecommendationEngine::popularityFallback(
    const unordered_set<uint32_t> &watchedMovies,
    unordered_map<uint32_t, float> &scores)
{
    vector<pair<uint32_t, float>> popularMovies;

    for (const auto &[movieId, popularity] : moviePopularity)
    {
        if (watchedMovies.find(movieId) == watchedMovies.end())
        {
            auto it = movieAvgRatings.find(movieId);
            if (it != movieAvgRatings.end() && it->second >= Config::MIN_RATING)
            {
                popularMovies.push_back({movieId, popularity * it->second});
            }
        }
    }

    sort(popularMovies.begin(), popularMovies.end(),
         [](const auto &a, const auto &b)
         { return a.second > b.second; });

    int needed = Config::TOP_K - scores.size();
    for (int i = 0; i < min(needed, (int)popularMovies.size()); ++i)
    {
        if (scores.find(popularMovies[i].first) == scores.end())
        {
            scores[popularMovies[i].first] = popularMovies[i].second / 100.0f;
        }
    }
}

// FUNÇÃO TOTALMENTE NOVA: Busca candidatos usando o índice LSH
vector<pair<uint32_t, int>> RecommendationEngine::findCandidateUsersLSH(
    uint32_t userId,
    const UserProfile &user)
{

    // Obtém candidatos do LSH
    vector<uint32_t> lshCandidates = lshIndex.findSimilarCandidates(userId, Config::MAX_CANDIDATES * 2);

    // IMPORTANTE: Precisamos calcular quantos filmes em comum cada candidato tem
    // Isso é CRÍTICO para o algoritmo funcionar corretamente
    vector<pair<uint32_t, int>> candidatesWithCount;
    candidatesWithCount.reserve(lshCandidates.size());

    for (uint32_t candidateId : lshCandidates)
    {
        auto it = users.find(candidateId);
        if (it == users.end())
            continue;

        const auto &candidateRatings = it->second.ratings;

        // Conta filmes em comum usando merge (arrays ordenados)
        int commonCount = 0;
        size_t i = 0, j = 0;
        while (i < user.ratings.size() && j < candidateRatings.size())
        {
            if (user.ratings[i].first < candidateRatings[j].first)
            {
                i++;
            }
            else if (user.ratings[i].first > candidateRatings[j].first)
            {
                j++;
            }
            else
            {
                commonCount++;
                i++;
                j++;
            }
        }

        // Só adiciona se tem filmes suficientes em comum
        if (commonCount >= Config::MIN_COMMON_ITEMS)
        {
            candidatesWithCount.push_back({candidateId, commonCount});
        }
    }

    // CRÍTICO: Ordena por número de filmes em comum (não por ID!)
    sort(candidatesWithCount.begin(), candidatesWithCount.end(),
         [](const auto &a, const auto &b)
         { return a.second > b.second; });

    // Se temos poucos candidatos após filtrar, busca mais via força bruta parcial
    if (candidatesWithCount.size() < 20)
    {
        // Fallback: busca nos filmes mais populares do usuário
        unordered_map<uint32_t, int> additionalCandidates;

        // Pega os 10 filmes mais bem avaliados do usuário
        vector<pair<float, uint32_t>> topRatedMovies;
        for (const auto &[movieId, rating] : user.ratings)
        {
            if (rating >= 4.0f)
            {
                topRatedMovies.push_back({rating, movieId});
            }
        }
        sort(topRatedMovies.begin(), topRatedMovies.end(), greater<pair<float, uint32_t>>());

        // Para cada filme top, adiciona alguns usuários que também gostaram
        int moviesChecked = 0;
        for (const auto &[rating, movieId] : topRatedMovies)
        {
            if (moviesChecked++ >= 10)
                break;

            auto movieIt = movieToUsers.find(movieId);
            if (movieIt != movieToUsers.end())
            {
                // Pega usuários que deram nota alta para este filme
                for (const auto &[otherUser, otherRating] : movieIt->second)
                {
                    if (otherUser != userId && otherRating >= 4.0f)
                    {
                        additionalCandidates[otherUser]++;
                    }
                }
            }
        }

        // Adiciona candidatos adicionais que não estavam no LSH
        for (const auto &[candidateId, sharedHighRated] : additionalCandidates)
        {
            // Verifica se já não está na lista
            bool alreadyIncluded = false;
            for (const auto &[existingId, _] : candidatesWithCount)
            {
                if (existingId == candidateId)
                {
                    alreadyIncluded = true;
                    break;
                }
            }

            if (!alreadyIncluded && sharedHighRated >= 3)
            {
                // Conta filmes em comum
                auto it = users.find(candidateId);
                if (it != users.end())
                {
                    const auto &candidateRatings = it->second.ratings;
                    int commonCount = 0;
                    size_t i = 0, j = 0;
                    while (i < user.ratings.size() && j < candidateRatings.size())
                    {
                        if (user.ratings[i].first < candidateRatings[j].first)
                        {
                            i++;
                        }
                        else if (user.ratings[i].first > candidateRatings[j].first)
                        {
                            j++;
                        }
                        else
                        {
                            commonCount++;
                            i++;
                            j++;
                        }
                    }

                    if (commonCount >= Config::MIN_COMMON_ITEMS)
                    {
                        candidatesWithCount.push_back({candidateId, commonCount});
                    }
                }
            }
        }

        // Re-ordena com os novos candidatos
        sort(candidatesWithCount.begin(), candidatesWithCount.end(),
             [](const auto &a, const auto &b)
             { return a.second > b.second; });
    }

    // Limita ao máximo configurado
    if (candidatesWithCount.size() > Config::MAX_SIMILAR_USERS)
    {
        candidatesWithCount.resize(Config::MAX_SIMILAR_USERS);
    }

    return candidatesWithCount;
}