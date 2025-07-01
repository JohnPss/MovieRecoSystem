#include "LSHIndex.hpp"
#include "DataStructures.hpp"
#include <algorithm>
#include <thread>
#include <future>
#include <iostream>
#include <chrono>
#include <cmath>

using namespace std;

LSHIndex::LSHIndex() : rng(std::random_device{}())
{
    tables.resize(Config::NUM_TABLES);

    // Inicializa parâmetros das funções hash para cada tabela e band
    bandHashParams.resize(Config::NUM_TABLES);
    for (int t = 0; t < Config::NUM_TABLES; t++)
    {
        bandHashParams[t].resize(Config::NUM_BANDS);
        for (int b = 0; b < Config::NUM_BANDS; b++)
        {
            uniform_int_distribution<uint32_t> dist(1, Config::LARGE_PRIME - 1);
            bandHashParams[t][b].a = dist(rng);
            bandHashParams[t][b].b = dist(rng);
        }
    }
}

void LSHIndex::buildSignatures(
    const unordered_map<uint32_t, vector<pair<uint32_t, float>>> &userRatings,
    int numThreads)
{
    cout << "Construindo assinaturas MinHash para " << userRatings.size()
         << " usuários..." << endl;
    auto start = chrono::high_resolution_clock::now();

    // Gera funções hash uma vez (compartilhadas entre threads)
    auto hashFunctions = generateHashFunctions();

    // Pré-computa hashes para todos os filmes únicos
    unordered_set<uint32_t> allMovies;
    for (const auto &[userId, ratings] : userRatings)
    {
        for (const auto &[movieId, _] : ratings)
        {
            allMovies.insert(movieId);
        }
    }

    cout << "Pré-computando hashes para " << allMovies.size() << " filmes..." << endl;
    unordered_map<uint32_t, vector<uint32_t>> precomputedHashes;
    precomputedHashes.reserve(allMovies.size());

    for (uint32_t movieId : allMovies)
    {
        vector<uint32_t> hashes(Config::NUM_HASH_FUNCTIONS);
        for (int h = 0; h < Config::NUM_HASH_FUNCTIONS; h++)
        {
            hashes[h] = (hashFunctions[h].first * movieId +
                         hashFunctions[h].second) %
                        Config::LARGE_PRIME;
        }
        precomputedHashes[movieId] = move(hashes);
    }

    // Converte para vector de pares para divisão mais eficiente
    vector<pair<uint32_t, const vector<pair<uint32_t, float>> *>> userVector;
    userVector.reserve(userRatings.size());
    for (const auto &[userId, ratings] : userRatings)
    {
        userVector.emplace_back(userId, &ratings);
    }

    // Processa em paralelo
    const size_t chunkSize = (userVector.size() + numThreads - 1) / numThreads;
    vector<future<vector<MinHashSignature>>> futures;

    for (int t = 0; t < numThreads; t++)
    {
        size_t startIdx = t * chunkSize;
        size_t endIdx = min(startIdx + chunkSize, userVector.size());

        if (startIdx < userVector.size())
        {
            futures.push_back(async(launch::async, [&, startIdx, endIdx]()
                                    {
                vector<MinHashSignature> localSignatures;
                localSignatures.reserve(endIdx - startIdx);
                
                for (size_t i = startIdx; i < endIdx; i++) {
                    uint32_t userId = userVector[i].first;
                    const auto& ratings = *userVector[i].second;
                    
                    // Computa MinHash
                    MinHashSignature sig(userId);
                    
                    // Inicializa com valores máximos
                    for (int h = 0; h < Config::NUM_HASH_FUNCTIONS; h++) {
                        sig.signature[h] = UINT32_MAX;
                    }
                    
                    // Para cada filme, atualiza o mínimo usando hashes pré-computados
                    for (const auto& [movieId, _] : ratings) {
                        const auto& movieHashes = precomputedHashes.at(movieId);
                        for (int h = 0; h < Config::NUM_HASH_FUNCTIONS; h++) {
                            sig.signature[h] = min(sig.signature[h], movieHashes[h]);
                        }
                    }
                    
                    localSignatures.push_back(move(sig));
                }
                return localSignatures; }));
        }
    }

    // Coleta resultados
    {
        lock_guard<mutex> lock(indexMutex);
        for (auto &future : futures)
        {
            auto localSigs = future.get();
            for (auto &sig : localSigs)
            {
                signatures[sig.userId] = move(sig);
            }
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Assinaturas construídas em " << duration.count() << "ms" << endl;
}

void LSHIndex::indexSignatures()
{
    cout << "Indexando " << signatures.size() << " assinaturas no LSH..." << endl;
    auto start = chrono::high_resolution_clock::now();

    // Limpa tabelas anteriores e reserva espaço
    for (auto &table : tables)
    {
        table.clear();
        // Estima número de buckets baseado em probabilidade
        table.reserve(signatures.size() * Config::NUM_BANDS / 100);
    }

    // CORREÇÃO: Usa apenas ALGUMAS bands por tabela para criar buckets maiores
    const int BANDS_PER_TABLE = 3; // Usar apenas 3 bands por vez cria buckets com ~50-100 usuários

    // Para cada assinatura
    for (const auto &[userId, sig] : signatures)
    {
        // Para cada tabela
        for (int tableIdx = 0; tableIdx < Config::NUM_TABLES; tableIdx++)
        {
            // Seleciona quais bands usar para esta tabela
            int startBand = (tableIdx * BANDS_PER_TABLE) % Config::NUM_BANDS;

            // Calcula hash combinando apenas BANDS_PER_TABLE bands
            size_t combinedHash = 0;
            for (int i = 0; i < BANDS_PER_TABLE; i++)
            {
                int bandIdx = (startBand + i) % Config::NUM_BANDS;
                size_t bandHash = hashBand(sig, bandIdx, tableIdx);
                combinedHash = (combinedHash << 16) ^ bandHash; // Melhor combinação
            }

            // Adiciona ao bucket
            size_t finalHash = combinedHash % 5000; // ← ADICIONE ESTA LINHA
            tables[tableIdx][finalHash].push_back(userId);
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Indexação concluída em " << duration.count() << "ms" << endl;

    // Estatísticas
    size_t totalBuckets = 0;
    size_t totalEntries = 0;
    size_t largeBuckets = 0;
    size_t maxBucket = 0;

    for (const auto &table : tables)
    {
        totalBuckets += table.size();
        for (const auto &[_, bucket] : table)
        {
            totalEntries += bucket.size();
            if (bucket.size() > 100)
                largeBuckets++;
            maxBucket = max(maxBucket, bucket.size());
        }
    }

    cout << "Total de buckets: " << totalBuckets
         << ", Média de entradas/bucket: " << (totalBuckets > 0 ? totalEntries / (double)totalBuckets : 0)
         << ", Buckets grandes (>100): " << largeBuckets
         << ", Maior bucket: " << maxBucket << endl;
}

vector<uint32_t> LSHIndex::findSimilarCandidates(uint32_t userId, int maxCandidates) const
{
    lock_guard<mutex> lock(indexMutex);

    auto it = signatures.find(userId);
    if (it == signatures.end())
    {
        return {};
    }

    const MinHashSignature &querySignature = it->second;
    unordered_map<uint32_t, int> candidateCount;

    const int BANDS_PER_TABLE = 3; // Mesmo valor usado na indexação

    // Para cada tabela
    for (int tableIdx = 0; tableIdx < Config::NUM_TABLES; tableIdx++)
    {
        // Usa as mesmas bands que foram usadas na indexação
        int startBand = (tableIdx * BANDS_PER_TABLE) % Config::NUM_BANDS;

        // Calcula o mesmo hash combinado
        size_t combinedHash = 0;
        for (int i = 0; i < BANDS_PER_TABLE; i++)
        {
            int bandIdx = (startBand + i) % Config::NUM_BANDS;
            size_t bandHash = hashBand(querySignature, bandIdx, tableIdx);
            combinedHash = (combinedHash << 16) ^ bandHash;
        }

        // Busca o bucket
        size_t finalHash = combinedHash % 20000;          // ← ADICIONE
        auto bucketIt = tables[tableIdx].find(finalHash); // ← USE finalHash
        if (bucketIt != tables[tableIdx].end())
        {
            for (uint32_t candidateId : bucketIt->second)
            {
                if (candidateId != userId)
                {
                    candidateCount[candidateId]++;
                }
            }
        }
    }

    // Se temos poucos candidatos, relaxa a busca
    if (candidateCount.size() < 50)
    {
        // Busca em buckets vizinhos (técnica de multi-probe LSH)
        for (int tableIdx = 0; tableIdx < min(3, Config::NUM_TABLES); tableIdx++)
        {
            // Para cada band, tenta pequenas variações
            for (int probe = 1; probe <= 2; probe++)
            {
                int startBand = (tableIdx * BANDS_PER_TABLE) % Config::NUM_BANDS;
                size_t combinedHash = 0;

                for (int i = 0; i < BANDS_PER_TABLE; i++)
                {
                    int bandIdx = (startBand + i) % Config::NUM_BANDS;
                    size_t bandHash = hashBand(querySignature, bandIdx, tableIdx);
                    // Adiciona pequena perturbação
                    bandHash = (bandHash + probe) % Config::LARGE_PRIME;
                    combinedHash = (combinedHash << 16) ^ bandHash;
                }

                size_t finalHash = combinedHash % 20000;
                auto bucketIt = tables[tableIdx].find(finalHash);

                if (bucketIt != tables[tableIdx].end())
                {
                    for (uint32_t candidateId : bucketIt->second)
                    {
                        if (candidateId != userId)
                        {
                            candidateCount[candidateId]++;
                        }
                    }
                }
            }
        }
    }

    // Converte para vetor, priorizando candidatos que aparecem em múltiplas tabelas
    vector<pair<int, uint32_t>> scoredCandidates;
    scoredCandidates.reserve(candidateCount.size());

    for (const auto &[candidateId, count] : candidateCount)
    {
        // Score baseado em: frequência + similaridade estimada
        float similarity = estimateJaccardSimilarity(userId, candidateId);
        float score = count * 0.3f + similarity * 0.7f;
        scoredCandidates.push_back({score * 1000, candidateId}); // Multiplica para usar como int
    }

    // Ordena por score
    sort(scoredCandidates.begin(), scoredCandidates.end(), greater<pair<int, uint32_t>>());

    // Extrai os top candidatos
    vector<uint32_t> candidates;
    candidates.reserve(min((int)scoredCandidates.size(), maxCandidates));

    for (int i = 0; i < min((int)scoredCandidates.size(), maxCandidates); i++)
    {
        candidates.push_back(scoredCandidates[i].second);
    }

    return candidates;
}

float LSHIndex::estimateJaccardSimilarity(uint32_t user1, uint32_t user2) const
{
    auto it1 = signatures.find(user1);
    auto it2 = signatures.find(user2);

    if (it1 == signatures.end() || it2 == signatures.end())
    {
        return 0.0f;
    }

    const auto &sig1 = it1->second.signature;
    const auto &sig2 = it2->second.signature;

    int matches = 0;
    for (int i = 0; i < Config::NUM_HASH_FUNCTIONS; i++)
    {
        if (sig1[i] == sig2[i])
        {
            matches++;
        }
    }

    return (float)matches / Config::NUM_HASH_FUNCTIONS;
}

size_t LSHIndex::hashBand(const MinHashSignature &sig, int bandIdx, int tableIdx) const
{
    size_t hash = 0;
    int startIdx = bandIdx * Config::ROWS_PER_BAND;

    // Hash mais robusto para bands
    for (int i = 0; i < Config::ROWS_PER_BAND; i++)
    {
        uint32_t val = sig.signature[startIdx + i];
        uint64_t temp = (uint64_t)bandHashParams[tableIdx][bandIdx].a * val +
                        bandHashParams[tableIdx][bandIdx].b;
        // Reduz o espaço de hash para criar mais colisões (buckets maiores)
        hash = (hash * 37 + (temp % 20000)) % Config::LARGE_PRIME;
    }

    return hash % 20000; // Limita o espaço de hash para garantir buckets maiores
}

vector<pair<uint32_t, uint32_t>> LSHIndex::generateHashFunctions()
{
    vector<pair<uint32_t, uint32_t>> functions;
    uniform_int_distribution<uint32_t> dist(1, Config::LARGE_PRIME - 1);

    for (int i = 0; i < Config::NUM_HASH_FUNCTIONS; i++)
    {
        functions.push_back({dist(rng), dist(rng)});
    }

    return functions;
}

void LSHIndex::printStatistics() const
{
    lock_guard<mutex> lock(indexMutex);

    cout << "\n=== Estatísticas do LSH ===" << endl;
    cout << "Usuários indexados: " << signatures.size() << endl;
    cout << "Tabelas: " << Config::NUM_TABLES << endl;
    cout << "Bands por tabela: " << Config::NUM_BANDS << endl;

    // Análise detalhada de distribuição
    size_t totalBuckets = 0;
    size_t emptyBuckets = 0;
    size_t smallBuckets = 0;  // < 10 usuários
    size_t mediumBuckets = 0; // 10-100 usuários
    size_t largeBuckets = 0;  // > 100 usuários
    size_t maxBucketSize = 0;
    double avgBucketSize = 0;

    for (const auto &table : tables)
    {
        for (const auto &[_, bucket] : table)
        {
            totalBuckets++;
            size_t size = bucket.size();

            if (size == 0)
                emptyBuckets++;
            else if (size < 10)
                smallBuckets++;
            else if (size <= 100)
                mediumBuckets++;
            else
                largeBuckets++;

            maxBucketSize = max(maxBucketSize, size);
            avgBucketSize += size;
        }
    }

    if (totalBuckets > 0)
    {
        avgBucketSize /= totalBuckets;
    }

    cout << "Total de buckets: " << totalBuckets << endl;
    cout << "Distribuição de buckets:" << endl;
    cout << "  Vazios: " << emptyBuckets << endl;
    cout << "  Pequenos (<10): " << smallBuckets << endl;
    cout << "  Médios (10-100): " << mediumBuckets << endl;
    cout << "  Grandes (>100): " << largeBuckets << endl;
    cout << "Tamanho médio: " << avgBucketSize << endl;
    cout << "Maior bucket: " << maxBucketSize << " usuários" << endl;
    cout << "=========================" << endl;
}