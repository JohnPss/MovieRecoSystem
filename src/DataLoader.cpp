
#include "DataLoader.hpp"




using std::atomic;
using std::make_pair;
using std::make_unique;
using std::max;
using std::min;
using std::move;
using std::pair;
using std::string;
using std::thread;
using std::unordered_map;
using std::vector;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

class DataLoader::Impl
{
public:
    unordered_map<uint32_t, UserProfile> &users;
    unordered_map<uint32_t, Movie> &movies;
    unordered_map<string, int> &genreToId;
    unordered_map<uint32_t, vector<pair<uint32_t, float>>> &movieToUsers;
    unordered_map<uint32_t, vector<uint32_t>> &genreToMovies;
    float &globalAvgRating;
    unordered_map<uint32_t, float> &movieAvgRatings;
    unordered_map<uint32_t, int> &moviePopularity;

    Impl(unordered_map<uint32_t, UserProfile> &u,
         unordered_map<uint32_t, Movie> &m,
         unordered_map<string, int> &g,
         unordered_map<uint32_t, vector<pair<uint32_t, float>>> &mtu,
         unordered_map<uint32_t, vector<uint32_t>> &gtm,
         float &gar,
         unordered_map<uint32_t, float> &mar,
         unordered_map<uint32_t, int> &mp)
        : users(u), movies(m), genreToId(g), movieToUsers(mtu),
          genreToMovies(gtm), globalAvgRating(gar),
          movieAvgRatings(mar), moviePopularity(mp) {}

    void loadRatings(const string &filename);
    void loadMovies(const string &filename);
    void calculateUserPreferences();
    vector<uint32_t> loadUsersToRecommend(const string &filename);

private:
    struct alignas(64) ThreadData
    {
        unordered_map<uint32_t, UserProfile> users;
        unordered_map<uint32_t, vector<pair<uint32_t, float>>> movieToUsers;
        unordered_map<uint32_t, float> movieSums;
        unordered_map<uint32_t, int> movieCounts;
        double ratingSum = 0.0;
        uint64_t ratingCount = 0;

        ThreadData()
        {
            users.reserve(10000);
            movieToUsers.reserve(10000);
            movieSums.reserve(10000);
            movieCounts.reserve(10000);
        }
    };

    inline const char *skipWhitespace(const char *p, const char *end)
    {
        while (p < end && (*p == ' ' || *p == '\t'))
            ++p;
        return p;
    }

    inline const char *skipToNext(const char *p, const char *end)
    {
        while (p < end && *p != '\n' && *p != '\r')
            ++p;
        while (p < end && (*p == '\n' || *p == '\r'))
            ++p;
        return p;
    }
};

DataLoader::DataLoader(
    unordered_map<uint32_t, UserProfile> &u,
    unordered_map<uint32_t, Movie> &m,
    unordered_map<string, int> &g,
    unordered_map<uint32_t, vector<pair<uint32_t, float>>> &mtu,
    unordered_map<uint32_t, vector<uint32_t>> &gtm,
    float &gar,
    unordered_map<uint32_t, float> &mar,
    unordered_map<uint32_t, int> &mp)
    : pimpl(make_unique<Impl>(u, m, g, mtu, gtm, gar, mar, mp)) {}

DataLoader::~DataLoader() = default;

void DataLoader::loadRatings(const string &filename)
{
    pimpl->loadRatings(filename);
}

void DataLoader::loadMovies(const string &filename)
{
    pimpl->loadMovies(filename);
}

vector<uint32_t> DataLoader::loadUsersToRecommend(const string &filename)
{
    return pimpl->loadUsersToRecommend(filename);
}

void DataLoader::Impl::loadRatings(const string &filename)
{
    const int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1)
    {
        return;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        close(fd);
        return;
    }

    const char *const file_data = static_cast<const char *>(
        mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (file_data == MAP_FAILED)
    {
        close(fd);
        return;
    }
    close(fd);

    madvise(const_cast<char *>(file_data), sb.st_size, MADV_SEQUENTIAL);

    const int num_threads = min(static_cast<int>(thread::hardware_concurrency()),
                                max(1, static_cast<int>(sb.st_size / 5000000)));

    vector<ThreadData> threadData(num_threads);
    vector<thread> threads;
    threads.reserve(num_threads);

    const size_t chunk_size = sb.st_size / num_threads;
    const char *const file_end = file_data + sb.st_size;

    for (int t = 0; t < num_threads; ++t)
    {
        const char *chunk_start = file_data + t * chunk_size;
        const char *chunk_end = (t == num_threads - 1) ? file_end : (chunk_start + chunk_size);

        if (t > 0)
        {
            while (chunk_start < chunk_end && *(chunk_start - 1) != '\n')
            {
                ++chunk_start;
            }
        }

        threads.emplace_back([this, chunk_start, chunk_end, t, &threadData]()
                             {
            ThreadData& data = threadData[t];
            const char* p = chunk_start;
            
            while (p < chunk_end) {
                p = skipToNext(p, chunk_end);
                if (p >= chunk_end) break;
                
                uint32_t userId;
                const auto [p1, ec1] = std::from_chars(p, chunk_end, userId);
                if (ec1 != std::errc{}) continue;
                p = skipWhitespace(p1, chunk_end);

                UserProfile& user = data.users[userId];
                user.ratings.reserve(100); 
                
                float sumRatings = 0.0f;
                int ratingsCount = 0;

                while (p < chunk_end && *p != '\n' && *p != '\r') {
                    uint32_t movieId;
                    const auto [p2, ec2] = std::from_chars(p, chunk_end, movieId);
                    if (ec2 != std::errc{}) break;
                    p = p2;
                    if (p >= chunk_end || *p != ':') break;
                    ++p; 
                    
                    float rating;
                    const auto [p3, ec3] = std::from_chars(p, chunk_end, rating);
                    if (ec3 != std::errc{}) break;
                    p = skipWhitespace(p3, chunk_end);
                    
                    user.ratings.emplace_back(movieId, rating);
                    data.movieToUsers[movieId].emplace_back(userId, rating);
                    sumRatings += rating;
                    data.movieSums[movieId] += rating;
                    ++data.movieCounts[movieId];
                    ++ratingsCount;
                }
                
                if (ratingsCount > 0) {
                    data.ratingSum += sumRatings;
                    data.ratingCount += ratingsCount;
                    user.avgRating = sumRatings / ratingsCount;
                    
                    if (user.ratings.size() > 1) {
                        std::sort(user.ratings.begin(), user.ratings.end());
                    }
                }
            } });
    }

    for (auto &t : threads)
        t.join();
    munmap(const_cast<char *>(file_data), sb.st_size);

    size_t totalUsers = 0;
    unordered_map<uint32_t, size_t> movieRatingCounts;
    for (const auto &data : threadData)
    {
        totalUsers += data.users.size();
        for (const auto &[movieId, ratings] : data.movieToUsers)
        {
            movieRatingCounts[movieId] += ratings.size();
        }
    }

    users.reserve(totalUsers);
    movieToUsers.reserve(movieRatingCounts.size());
    movieAvgRatings.reserve(movieRatingCounts.size());
    moviePopularity.reserve(movieRatingCounts.size());

    for (const auto &[movieId, count] : movieRatingCounts)
    {
        movieToUsers[movieId].reserve(count);
    }

    uint64_t totalRatings = 0;
    double totalSum = 0.0;

    for (auto &data : threadData)
    {
        for (auto &[userId, profile] : data.users)
        {
            users[userId] = move(profile);
        }

        for (auto &[movieId, ratings] : data.movieToUsers)
        {
            auto &target = movieToUsers[movieId];
            target.insert(target.end(),
                          std::make_move_iterator(ratings.begin()),
                          std::make_move_iterator(ratings.end()));
        }

        totalSum += data.ratingSum;
        totalRatings += data.ratingCount;

        for (const auto &[movieId, sum] : data.movieSums)
        {
            movieAvgRatings[movieId] += sum;
        }
        for (const auto &[movieId, count] : data.movieCounts)
        {
            moviePopularity[movieId] += count;
        }
    }

    globalAvgRating = totalRatings > 0 ? static_cast<float>(totalSum / totalRatings) : 0.0f;

    for (auto &[movieId, sum] : movieAvgRatings)
    {
        const int count = moviePopularity[movieId];
        if (count > 0)
            sum /= count;
    }
}

void DataLoader::Impl::loadMovies(const string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        return;
    }

    movies.reserve(65000);
    genreToMovies.reserve(25);

    string line;
    line.reserve(256);
    std::getline(file, line); 

    int genreCounter = 0;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        const auto first_comma = line.find(',');
        if (first_comma == string::npos)
            continue;

        const auto last_comma = line.rfind(',');
        if (last_comma == string::npos || last_comma <= first_comma)
        {
            const string movieIdStr = line.substr(0, first_comma);
            const uint32_t movieId = std::stoul(movieIdStr);
            Movie &movie = movies[movieId];
            movie.genreBitmask = 0;
            continue;
        }

        const string movieIdStr = line.substr(0, first_comma);
        const string genres = line.substr(last_comma + 1);

        const uint32_t movieId = std::stoul(movieIdStr);
        Movie &movie = movies[movieId];
        movie.genreBitmask = 0;
        movie.genres.reserve(5);

        std::string_view genreView(genres);
        size_t pos = 0;
        while (pos < genreView.length())
        {
            const size_t endPos = genreView.find('|', pos);
            const size_t actualEnd = (endPos == string::npos) ? genreView.length() : endPos;

            const string genre(genreView.substr(pos, actualEnd - pos));
            movie.genres.push_back(genre);

            if (genreToId.find(genre) == genreToId.end())
            {
                genreToId[genre] = genreCounter++;
            }

            const int genreId = genreToId[genre];
            movie.genreBitmask |= (1U << genreId);
            genreToMovies[genreId].push_back(movieId);

            pos = actualEnd + 1;
        }
    }

    calculateUserPreferences();
}

void DataLoader::Impl::calculateUserPreferences()
{
    vector<UserProfile *> userPtrs;
    userPtrs.reserve(users.size());
    for (auto &[userId, profile] : users)
    {
        userPtrs.push_back(&profile);
    }

    const int num_threads = min(static_cast<int>(thread::hardware_concurrency()),
                                max(1, static_cast<int>(userPtrs.size() / 5000)));
    vector<thread> threads;
    threads.reserve(num_threads);
    const size_t chunk_size = userPtrs.size() / num_threads;

    for (int t = 0; t < num_threads; ++t)
    {
        const size_t start_idx = t * chunk_size;
        const size_t end_idx = (t == num_threads - 1) ? userPtrs.size() : (t + 1) * chunk_size;

        threads.emplace_back([this, &userPtrs, start_idx, end_idx]()
                             {
            for (size_t i = start_idx; i < end_idx; ++i) {
                UserProfile& user = *userPtrs[i];
                unordered_map<int, float> genreScores;
                genreScores.reserve(20);
                
                for (const auto& [movieId, rating] : user.ratings) {
                    if (rating >= Config::MIN_RATING) {
                        const auto movieIt = movies.find(movieId);
                        if (movieIt != movies.end()) {
                            uint32_t movieGenres = movieIt->second.genreBitmask;
                            for (int g = 0; g < 32 && movieGenres; ++g) {
                                if (movieGenres & (1U << g)) {
                                    genreScores[g] += rating - Config::MIN_RATING;
                                    movieGenres &= ~(1U << g);
                                }
                            }
                        }
                    }
                }
                
                if (!genreScores.empty()) {
                    vector<pair<float, int>> sortedGenres;
                    sortedGenres.reserve(genreScores.size());
                    for (const auto& [genreId, score] : genreScores) {
                        sortedGenres.emplace_back(score, genreId);
                    }
                    
                    const int topN = min(5, static_cast<int>(sortedGenres.size()));
                    std::partial_sort(sortedGenres.begin(), sortedGenres.begin() + topN, 
                                    sortedGenres.end(), std::greater<pair<float, int>>());
                    
                    user.preferredGenres = 0;
                    for (int j = 0; j < topN; ++j) {
                        user.preferredGenres |= (1U << sortedGenres[j].second);
                    }
                }
            } });
    }

    for (auto &t : threads)
        t.join();
}

vector<uint32_t> DataLoader::Impl::loadUsersToRecommend(const string &filename)
{
    vector<uint32_t> userIds;
    userIds.reserve(1000);

    std::ifstream file(filename);
    if (!file.is_open())
    {
        return userIds;
    }

    string line;
    line.reserve(32);
    while (std::getline(file, line))
    {
        if (!line.empty())
        {
            userIds.push_back(std::stoul(line));
        }
    }

    return userIds;
}