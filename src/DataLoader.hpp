#ifndef DATA_LOADER_HPP
#define DATA_LOADER_HPP

#include "Config.hpp"
#include "DataLoader.hpp"
#include "DataStructures.hpp"

struct UserProfile;
struct Movie;

class DataLoader
{
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

    ~DataLoader();

    void loadRatings(const std::string &filename);
    void loadMovies(const std::string &filename);
    std::vector<uint32_t> loadUsersToRecommend(const std::string &filename);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

#endif