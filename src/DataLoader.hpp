#ifndef DATA_LOADER_HPP
#define DATA_LOADER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include "DataStructures.hpp"

// Para Memory-Mapped Files (Unix/Linux)
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

class DataLoader
{
private:
    std::unordered_map<uint32_t, UserProfile> &users;
    std::unordered_map<uint32_t, Movie> &movies;
    std::unordered_map<std::string, int> &genreToId;
    std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, float>>> &movieToUsers;
    std::unordered_map<uint32_t, std::vector<uint32_t>> &genreToMovies;

    float &globalAvgRating;
    std::unordered_map<uint32_t, float> &movieAvgRatings;
    std::unordered_map<uint32_t, int> &moviePopularity;

public:
    DataLoader(
        std::unordered_map<uint32_t, UserProfile> &u,
        std::unordered_map<uint32_t, Movie> &m,
        std::unordered_map<std::string, int> &g,
        std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, float>>> &mtu,
        std::unordered_map<uint32_t, std::vector<uint32_t>> &gtm,
        float &gar,
        std::unordered_map<uint32_t, float> &mar,
        std::unordered_map<uint32_t, int> &mp);

    void loadRatings(const std::string &filename);
    void loadMovies(const std::string &filename);
    std::vector<uint32_t> loadUsersToRecommend(const std::string &filename);

private:
    void calculateUserPreferences();
};

#endif // DATA_LOADER_H