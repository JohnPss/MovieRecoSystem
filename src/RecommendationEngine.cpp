#include "RecommendationEngine.hpp"

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

    unordered_set<uint32_t> watchedMovies;
    for (const auto &[movieId, _] : user.ratings)
    {
        watchedMovies.insert(movieId);
    }

    vector<pair<uint32_t, int>> candidates = findCandidateUsersLSH(userId, user);

    auto similarUsers = calculateSimilarities(userId, candidates);
    auto scores = collaborativeFiltering(user, similarUsers, watchedMovies);
    contentBasedBoost(user, watchedMovies, scores);

    if (scores.size() < Config::TOP_K)
    {
        popularityFallback(watchedMovies, scores);
    }

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

    vector<pair<uint32_t, int>> filteredCandidates;
    for (const auto &[candidateId, count] : candidateCount)
    {
        if (count >= Config::MIN_COMMON_ITEMS)
        {
            filteredCandidates.push_back({candidateId, count});
        }
    }

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
    for (size_t i = 0; i < candidates.size(); i += Config::BATCH_SIZE)
    {
        size_t end = min(i + Config::BATCH_SIZE, candidates.size());
        vector<future<pair<uint32_t, float>>> futures;
        for (size_t j = i; j < end; ++j)
        {
            uint32_t candidateId = candidates[j].first;
            futures.push_back(async(launch::async,
                                    [this, userId, candidateId]()
                                    {
                                        float sim = similarityCalc.calculateCosineSimilarity(userId, candidateId);
                                        return make_pair(candidateId, sim);
                                    }));
        }
        for (auto &f : futures)
        {
            auto result = f.get();
            if (result.second > Config::MIN_SIMILARITY)
            {
                similarUsers.push_back(result);
            }
        }
    }

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

    for (auto &[movieId, score] : scores)
    {
        auto popIt = moviePopularity.find(movieId);
        if (popIt != moviePopularity.end())
        {
            float popularity_boost = log(popIt->second + 1) / 15.0f;
            score += popularity_boost * Config::POPULARITY_WEIGHT;
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
                            float quality = avgIt->second / 5.0f;
                            float popularity = min(1.0f, static_cast<float>(log(popIt->second + 1) / 10.0));

                            float combined_boost = (0.3f * quality + 0.7f * popularity) * Config::CB_WEIGHT +
                                                   popularity * Config::POPULARITY_WEIGHT;

                            scores[movieId] += combined_boost;
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
                float weighted_score = popularity * it->second * Config::POPULARITY_WEIGHT;
                popularMovies.push_back({movieId, weighted_score});
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
            scores[popularMovies[i].first] = popularMovies[i].second / 50.0f; 
        }
    }
}

vector<pair<uint32_t, int>> RecommendationEngine::findCandidateUsersLSH(
    uint32_t userId,
    const UserProfile &user)
{
    vector<uint32_t> lshCandidates = lshIndex.findSimilarCandidates(userId, Config::MAX_CANDIDATES * 3);


    vector<pair<uint32_t, int>> allFoundCandidates;
    allFoundCandidates.reserve(lshCandidates.size());

    for (uint32_t candidateId : lshCandidates)
    {
        auto it = users.find(candidateId);
        if (it == users.end())
            continue;

        const auto &candidateRatings = it->second.ratings;
        int commonCount = 0;
        size_t i = 0, j = 0;
        while (i < user.ratings.size() && j < candidateRatings.size())
        {
            if (user.ratings[i].first < candidateRatings[j].first)
                i++;
            else if (user.ratings[i].first > candidateRatings[j].first)
                j++;
            else
            {
                commonCount++;
                i++;
                j++;
            }
        }
        if (commonCount > 0)
        {
            allFoundCandidates.push_back({candidateId, commonCount});
        }
    }

    vector<pair<uint32_t, int>> highQualityCandidates;
    for (const auto &candidate : allFoundCandidates)
    {
        if (candidate.second >= Config::MIN_COMMON_ITEMS)
        {
            highQualityCandidates.push_back(candidate);
        }
    }

    sort(highQualityCandidates.begin(), highQualityCandidates.end(),
         [](const auto &a, const auto &b)
         { return a.second > b.second; });

    const size_t MINIMUM_CANDIDATES = 20;
    if (highQualityCandidates.size() < MINIMUM_CANDIDATES)
    {
        sort(allFoundCandidates.begin(), allFoundCandidates.end(),
             [](const auto &a, const auto &b)
             { return a.second > b.second; });


        for (const auto &fallbackCandidate : allFoundCandidates)
        {
            if (highQualityCandidates.size() >= MINIMUM_CANDIDATES)
                break;

            bool already_included = false;
            for (const auto &hq_cand : highQualityCandidates)
            {
                if (hq_cand.first == fallbackCandidate.first)
                {
                    already_included = true;
                    break;
                }
            }

            if (!already_included)
            {
                highQualityCandidates.push_back(fallbackCandidate);
            }
        }
    }
    
    if (highQualityCandidates.size() > Config::MAX_CANDIDATES)
    {
        highQualityCandidates.resize(Config::MAX_CANDIDATES);
    }

    return highQualityCandidates;
}