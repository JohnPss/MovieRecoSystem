#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <thread>
#include <cstdint> // Necessário para uint32_t

// Configurações do sistema de recomendação
namespace Config
{
    // Parâmetros de recomendação
    const int TOP_K = 20;               // Número de recomendações a retornar
    const int MAX_SIMILAR_USERS = 500;  // Máximo de usuários similares a considerar
    const int MIN_COMMON_ITEMS = 1;     // MUDANÇA: Aceita 1+ filme em comum (era 2)
    const float MIN_RATING = 3.5f;      // Rating mínimo para considerar positivo
    const float MIN_SIMILARITY = 0.01f; // MUDANÇA: Muito mais permissivo (era 0.05f)
    const int MAX_CANDIDATES = 1000;    // MUDANÇA: Mais candidatos (era 800)

    // --- Parâmetros LSH OTIMIZADOS ---
    const int NUM_HASH_FUNCTIONS = 96; // MUDANÇA: Reduzido para permitir bands menores
    const int NUM_BANDS = 24;          // MUDANÇA: Mais bands = mais chances de colidir
    const int ROWS_PER_BAND = 4;       // MUDANÇA: 96/24 = 4 (muito menos rigoroso!)
    const int NUM_TABLES = 8;          // MUDANÇA: Menos tabelas (era 12, performance vs recall)
    const uint32_t LARGE_PRIME = 4294967291u;

    // Parâmetros de performance
    const int NUM_THREADS = std::thread::hardware_concurrency() - 2;
    const int BATCH_SIZE = 100; // Tamanho do batch para processamento paralelo

    // Pesos do sistema híbrido OTIMIZADOS
    const float CF_WEIGHT = 1.0f;         // MUDANÇA: Reduzido (CF está falhando)
    const float CB_WEIGHT = 1.0f;         // MUDANÇA: CB muito mais forte para compensar
    const float POPULARITY_WEIGHT = 3.0f; // ← NOVO!

    // NOVAS CONFIGURAÇÕES DE FALLBACK
    const int MIN_CANDIDATES_FOR_CF = 50;        // Se < 50 candidatos, força fallback
    const int EMERGENCY_FALLBACK_THRESHOLD = 10; // Se < 10 candidatos, modo emergência
    const float POPULARITY_BOOST_WEIGHT = 1.5f;  // Peso do boost de popularidade

    inline static const std::string USERS_FILE = "datasets/explore.dat";
    inline static const std::string MOVIES_FILE = "ml-25m/movies.csv";
    inline static const std::string RATINGS_FILE = "datasets/input.dat";
    inline static const std::string OUTPUT_FILE = "outcome/output.dat";
    inline static const std::string DEBUG_OUTPUT_FILE = "outcome/debug_recommendations.txt";

}

#endif // CONFIG_H

/*
 ==================================================================================
 CONFIGURAÇÃO OTIMIZADA PARA RESOLVER PROBLEMA DE BUCKETS PEQUENOS
 ----------------------------------------------------------------------------------
 MUDANÇAS PRINCIPAIS:

 1. LSH MENOS RIGOROSO:
    - NUM_BANDS = 24 (mais chances de colidir)
    - ROWS_PER_BAND = 4 (era 8, muito menos rigoroso!)
    - NUM_HASH_FUNCTIONS = 96 (reduzido para permitir bands menores)

 2. FILTROS ULTRA-PERMISSIVOS:
    - MIN_COMMON_ITEMS = 1 (aceita qualquer overlap)
    - MIN_SIMILARITY = 0.01 (quase qualquer similaridade)
    - MAX_CANDIDATES = 1000 (mais material para trabalhar)

 3. CONTENT-BASED DOMINANTE:
    - CB_WEIGHT = 3.0 (era 2.0, compensa CF fraco)
    - CF_WEIGHT = 0.8 (reduzido, CF não está funcionando bem)

 4. FALLBACKS AGRESSIVOS:
    - Novos thresholds para forçar modos de emergência
    - Boost de popularidade mais forte

 PROBABILIDADE DE COLISÃO COM ROWS_PER_BAND = 4:
 - Usuários 80% similares: 0.8^4 = 0.41 por band
 - Com 24 bands: 1-(1-0.41)^24 ≈ 99.999% chance de colidir!
 - Usuários 50% similares: 0.5^4 = 0.0625 por band
 - Com 24 bands: 1-(1-0.0625)^24 ≈ 78% chance de colidir

 EXPECTATIVA: Buckets com 50-200 usuários, Hit Rate 15-30%
 ==================================================================================
*/