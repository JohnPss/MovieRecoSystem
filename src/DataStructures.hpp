#ifndef DATA_STRUCTURES_HPP
#define DATA_STRUCTURES_HPP

#include <vector>
#include <string>
#include <cstdint>

// Estrutura para armazenar informações de um filme
struct Movie
{
    std::string title;
    uint32_t genreBitmask;
    std::vector<std::string> genres;
};

// Estrutura para o perfil de um usuário
struct UserProfile
{
    std::vector<std::pair<uint32_t, float>> ratings; // (movieId, rating)
    float avgRating;
    uint32_t preferredGenres; // Bitmask dos gêneros preferidos
};

struct Recommendation
{
    uint32_t movieId;
    float score;

    // Construtor padrão — permite que std::vector<Recommendation>::resize() funcione
    Recommendation()
        : movieId(0), score(0.0f)
    {
    }

    // Construtor que você já tinha
    Recommendation(uint32_t id, float s) : movieId(id),
                                           score(s)
    {
    }
};

#endif // DATA_STRUCTURES_H