#include "FastRecommendationSystem.hpp"


using namespace std;

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

void FastRecommendationSystem::loadData()
{
    dataLoader->loadRatings(Config::RATINGS_FILE);
    dataLoader->loadMovies(Config::MOVIES_FILE);

    auto userRatingsForLSH = make_unique<unordered_map<uint32_t, vector<pair<uint32_t, float>>>>();
    userRatingsForLSH->reserve(users.size());

    for (const auto &[userId, profile] : users)
    {
        (*userRatingsForLSH)[userId] = profile.ratings;
    }

    lshIndex->buildSignatures(*userRatingsForLSH, Config::NUM_THREADS);
    lshIndex->indexSignatures();
}

void FastRecommendationSystem::processRecommendations(const string &filename)
{
    vector<uint32_t> userIds = dataLoader->loadUsersToRecommend(filename);

    filesystem::create_directory("outcome");
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
}

vector<Recommendation> FastRecommendationSystem::recommendForUser(uint32_t userId)
{
    return recommendationEngine->recommendForUser(userId);
}

void FastRecommendationSystem::printRecommendations(
    uint32_t userId,
    const vector<Recommendation> &recommendations)
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
    }
}