#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <thread>

// Configurações do sistema de recomendação
namespace Config
{
    // Parâmetros de recomendação
    const int TOP_K = 20;              // Número de recomendações a retornar
    const int MAX_SIMILAR_USERS = 500; // Máximo de usuários similares a considerar
    const int MIN_COMMON_ITEMS = 5;    // Mínimo de filmes em comum para calcular similaridade
    const float MIN_RATING = 3.5f;     // Rating mínimo para considerar positivo
    const float MIN_SIMILARITY = 0.1f; // Similaridade mínima para considerar usuário

    // Parâmetros de performance
    const int NUM_THREADS = std::thread::hardware_concurrency();
    const int BATCH_SIZE = 100;      // Tamanho do batch para processamento paralelo
    const int MAX_CANDIDATES = 5000; // Máximo de candidatos a processar

    // Pesos do sistema híbrido
    const float CF_WEIGHT = 0.7f; // Peso do collaborative filtering
    const float CB_WEIGHT = 0.3f; // Peso do content-based filtering

    // // Arquivos de entrada
    // const char *RATINGS_FILE = "datasets/input.dat";
    // const char *MOVIES_FILE = "ml-25m/movies.csv";
    // const char *USERS_FILE = "datasets/explore.dat";

    inline static const std::string USERS_FILE = "datasets/explore.dat";
    inline static const std::string MOVIES_FILE = "ml-25m/movies.csv";
    inline static const std::string RATINGS_FILE = "datasets/input.dat";
}

#endif // CONFIG_H