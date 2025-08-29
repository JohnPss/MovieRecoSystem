#ifndef DATA_STRUCTURES_HPP
#define DATA_STRUCTURES_HPP

#include "../core/Config.hpp"

struct Movie
{
    uint32_t genreBitmask;
    std::vector<std::string> genres;
};

struct UserProfile
{
    std::vector<std::pair<uint32_t, float>> ratings; 
    float avgRating;
    uint32_t preferredGenres; 
};

struct Recommendation
{
    uint32_t movieId;
    float score;

    Recommendation()
        : movieId(0), score(0.0f)
    {
    }

    
    Recommendation(uint32_t id, float s) : movieId(id),
                                           score(s)
    {
    }
};

#endif 