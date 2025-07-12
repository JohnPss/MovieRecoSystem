#ifndef LSH_INDEX_HPP
#define LSH_INDEX_HPP

#include "Config.hpp"
#include "DataStructures.hpp"


// Configurações do LSH
// namespace LSHConfig {
//     const int NUM_HASH_FUNCTIONS = 128;  // Tamanho da assinatura MinHash
//     const int NUM_BANDS = 32;            // Número de bands
//     const int ROWS_PER_BAND = 4;         // NUM_HASH_FUNCTIONS / NUM_BANDS
//     const int NUM_TABLES = 10;           // Múltiplas tabelas para aumentar recall
//     const uint32_t LARGE_PRIME = 4294967291u;  // Primo grande para hash
// }

// Assinatura MinHash de um usuário
struct MinHashSignature
{
    std::vector<uint32_t> signature;
    uint32_t userId;

    MinHashSignature() : signature(Config::NUM_HASH_FUNCTIONS), userId(0) {}
    MinHashSignature(uint32_t id) : signature(Config::NUM_HASH_FUNCTIONS), userId(id) {}
};

// Índice LSH para busca eficiente de usuários similares
class LSHIndex
{
private:
    // Cada tabela tem múltiplos buckets, cada bucket tem lista de userIds
    std::vector<std::unordered_map<size_t, std::vector<uint32_t>>> tables;

    // Armazena assinaturas MinHash de todos os usuários
    std::unordered_map<uint32_t, MinHashSignature> signatures;

    // Parâmetros das funções hash universais
    struct HashParams
    {
        uint32_t a, b;
    };
    std::vector<std::vector<HashParams>> bandHashParams; // [table][band]

    // Mutex para operações thread-safe
    mutable std::mutex indexMutex;

    // Gerador de números aleatórios
    std::mt19937 rng;

public:
    LSHIndex();

    // Constrói assinaturas MinHash para todos os usuários
    void buildSignatures(
        const std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, float>>> &userRatings,
        int numThreads = 8);

    // Indexa todas as assinaturas nas tabelas LSH
    void indexSignatures();

    // Encontra candidatos similares para um usuário
    std::vector<uint32_t> findSimilarCandidates(
        uint32_t userId,
        int maxCandidates = 500) const;

    // Calcula similaridade Jaccard entre dois usuários (baseado em MinHash)
    float estimateJaccardSimilarity(uint32_t user1, uint32_t user2) const;

    // Estatísticas do índice
    void printStatistics() const;

private:
    // Gera assinatura MinHash para um conjunto de filmes
    MinHashSignature computeMinHash(
        const std::vector<uint32_t> &movies,
        uint32_t userId);

    // Computa hash de uma band
    size_t hashBand(
        const MinHashSignature &sig,
        int bandIdx,
        int tableIdx) const;

    // Funções hash universais para MinHash
    std::vector<std::pair<uint32_t, uint32_t>> generateHashFunctions();
};

#endif // LSH_INDEX_HPP