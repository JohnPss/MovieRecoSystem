#include "LSHIndex.hpp"
#include "DataStructures.hpp"
#include <algorithm>
#include <thread>
#include <future>
#include <iostream>
#include <chrono>
#include <cmath>

using namespace std;

LSHIndex::LSHIndex() : rng(std::random_device{}()) {
    tables.resize(LSHConfig::NUM_TABLES);
    
    // Inicializa parâmetros das funções hash para cada tabela e band
    bandHashParams.resize(LSHConfig::NUM_TABLES);
    for (int t = 0; t < LSHConfig::NUM_TABLES; t++) {
        bandHashParams[t].resize(LSHConfig::NUM_BANDS);
        for (int b = 0; b < LSHConfig::NUM_BANDS; b++) {
            uniform_int_distribution<uint32_t> dist(1, LSHConfig::LARGE_PRIME - 1);
            bandHashParams[t][b].a = dist(rng);
            bandHashParams[t][b].b = dist(rng);
        }
    }
}

void LSHIndex::buildSignatures(
    const unordered_map<uint32_t, vector<pair<uint32_t, float>>>& userRatings,
    int numThreads) {
    
    cout << "Construindo assinaturas MinHash para " << userRatings.size() 
         << " usuários..." << endl;
    auto start = chrono::high_resolution_clock::now();
    
    // Coleta todos os userIds
    vector<uint32_t> userIds;
    userIds.reserve(userRatings.size());
    for (const auto& [userId, _] : userRatings) {
        userIds.push_back(userId);
    }
    
    // Processa em paralelo
    const size_t chunkSize = (userIds.size() + numThreads - 1) / numThreads;
    vector<future<vector<MinHashSignature>>> futures;
    
    // Gera funções hash uma vez (compartilhadas entre threads)
    auto hashFunctions = generateHashFunctions();
    
    for (int t = 0; t < numThreads; t++) {
        size_t start = t * chunkSize;
        size_t end = min(start + chunkSize, userIds.size());
        
        if (start < userIds.size()) {
            futures.push_back(async(launch::async, [&, start, end, hashFunctions]() {
                vector<MinHashSignature> localSignatures;
                localSignatures.reserve(end - start);
                
                for (size_t i = start; i < end; i++) {
                    uint32_t userId = userIds[i];
                    auto it = userRatings.find(userId);
                    if (it != userRatings.end()) {
                        // Extrai apenas movieIds (ignora ratings para MinHash)
                        vector<uint32_t> movies;
                        movies.reserve(it->second.size());
                        for (const auto& [movieId, _] : it->second) {
                            movies.push_back(movieId);
                        }
                        
                        // Computa MinHash
                        MinHashSignature sig(userId);
                        
                        // Para cada função hash, encontra o mínimo
                        for (int h = 0; h < LSHConfig::NUM_HASH_FUNCTIONS; h++) {
                            uint32_t minHash = UINT32_MAX;
                            for (uint32_t movie : movies) {
                                uint32_t hash = (hashFunctions[h].first * movie + 
                                               hashFunctions[h].second) % LSHConfig::LARGE_PRIME;
                                minHash = min(minHash, hash);
                            }
                            sig.signature[h] = minHash;
                        }
                        
                        localSignatures.push_back(move(sig));
                    }
                }
                return localSignatures;
            }));
        }
    }
    
    // Coleta resultados
    {
        lock_guard<mutex> lock(indexMutex);
        for (auto& future : futures) {
            auto localSigs = future.get();
            for (auto& sig : localSigs) {
                signatures[sig.userId] = move(sig);
            }
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Assinaturas construídas em " << duration.count() << "ms" << endl;
}

void LSHIndex::indexSignatures() {
    cout << "Indexando " << signatures.size() << " assinaturas no LSH..." << endl;
    auto start = chrono::high_resolution_clock::now();
    
    // Limpa tabelas anteriores
    for (auto& table : tables) {
        table.clear();
    }
    
    // Indexa cada assinatura em todas as tabelas
    for (const auto& [userId, sig] : signatures) {
        for (int tableIdx = 0; tableIdx < LSHConfig::NUM_TABLES; tableIdx++) {
            // Para cada band nesta tabela
            for (int bandIdx = 0; bandIdx < LSHConfig::NUM_BANDS; bandIdx++) {
                size_t bucketHash = hashBand(sig, bandIdx, tableIdx);
                tables[tableIdx][bucketHash].push_back(userId);
            }
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Indexação concluída em " << duration.count() << "ms" << endl;
    
    // Estatísticas
    size_t totalBuckets = 0;
    size_t totalEntries = 0;
    for (const auto& table : tables) {
        totalBuckets += table.size();
        for (const auto& [_, bucket] : table) {
            totalEntries += bucket.size();
        }
    }
    cout << "Total de buckets: " << totalBuckets 
         << ", Média de entradas/bucket: " << totalEntries / (double)totalBuckets << endl;
}

vector<uint32_t> LSHIndex::findSimilarCandidates(uint32_t userId, int maxCandidates) const {
    lock_guard<mutex> lock(indexMutex);
    
    auto it = signatures.find(userId);
    if (it == signatures.end()) {
        return {};
    }
    
    const MinHashSignature& querySignature = it->second;
    unordered_set<uint32_t> candidateSet;
    
    // Busca em todas as tabelas
    for (int tableIdx = 0; tableIdx < LSHConfig::NUM_TABLES; tableIdx++) {
        for (int bandIdx = 0; bandIdx < LSHConfig::NUM_BANDS; bandIdx++) {
            size_t bucketHash = hashBand(querySignature, bandIdx, tableIdx);
            
            auto bucketIt = tables[tableIdx].find(bucketHash);
            if (bucketIt != tables[tableIdx].end()) {
                for (uint32_t candidateId : bucketIt->second) {
                    if (candidateId != userId) {
                        candidateSet.insert(candidateId);
                    }
                }
            }
        }
    }
    
    // Converte para vetor e limita tamanho
    vector<uint32_t> candidates(candidateSet.begin(), candidateSet.end());
    
    // Se temos muitos candidatos, podemos fazer uma pré-filtragem baseada em Jaccard estimado
    if (candidates.size() > maxCandidates * 2) {
        vector<pair<float, uint32_t>> scoredCandidates;
        scoredCandidates.reserve(candidates.size());
        
        for (uint32_t candidateId : candidates) {
            float similarity = estimateJaccardSimilarity(userId, candidateId);
            scoredCandidates.push_back({similarity, candidateId});
        }
        
        partial_sort(scoredCandidates.begin(), 
                    scoredCandidates.begin() + min((int)scoredCandidates.size(), maxCandidates),
                    scoredCandidates.end(),
                    greater<pair<float, uint32_t>>());
        
        candidates.clear();
        for (int i = 0; i < min((int)scoredCandidates.size(), maxCandidates); i++) {
            candidates.push_back(scoredCandidates[i].second);
        }
    }
    
    return candidates;
}

float LSHIndex::estimateJaccardSimilarity(uint32_t user1, uint32_t user2) const {
    auto it1 = signatures.find(user1);
    auto it2 = signatures.find(user2);
    
    if (it1 == signatures.end() || it2 == signatures.end()) {
        return 0.0f;
    }
    
    const auto& sig1 = it1->second.signature;
    const auto& sig2 = it2->second.signature;
    
    int matches = 0;
    for (int i = 0; i < LSHConfig::NUM_HASH_FUNCTIONS; i++) {
        if (sig1[i] == sig2[i]) {
            matches++;
        }
    }
    
    return (float)matches / LSHConfig::NUM_HASH_FUNCTIONS;
}

size_t LSHIndex::hashBand(const MinHashSignature& sig, int bandIdx, int tableIdx) const {
    size_t hash = 0;
    int startIdx = bandIdx * LSHConfig::ROWS_PER_BAND;
    
    // Combina valores da band usando hash universal
    for (int i = 0; i < LSHConfig::ROWS_PER_BAND; i++) {
        uint32_t val = sig.signature[startIdx + i];
        hash ^= (bandHashParams[tableIdx][bandIdx].a * val + 
                bandHashParams[tableIdx][bandIdx].b) % LSHConfig::LARGE_PRIME;
        hash = (hash << 1) | (hash >> 31); // Rotate
    }
    
    return hash;
}

vector<pair<uint32_t, uint32_t>> LSHIndex::generateHashFunctions() {
    vector<pair<uint32_t, uint32_t>> functions;
    uniform_int_distribution<uint32_t> dist(1, LSHConfig::LARGE_PRIME - 1);
    
    for (int i = 0; i < LSHConfig::NUM_HASH_FUNCTIONS; i++) {
        functions.push_back({dist(rng), dist(rng)});
    }
    
    return functions;
}

void LSHIndex::printStatistics() const {
    lock_guard<mutex> lock(indexMutex);
    
    cout << "\n=== Estatísticas do LSH ===" << endl;
    cout << "Usuários indexados: " << signatures.size() << endl;
    cout << "Tabelas: " << LSHConfig::NUM_TABLES << endl;
    cout << "Bands por tabela: " << LSHConfig::NUM_BANDS << endl;
    
    // Análise de distribuição
    size_t totalBuckets = 0;
    size_t emptyBuckets = 0;
    size_t maxBucketSize = 0;
    double avgBucketSize = 0;
    
    for (const auto& table : tables) {
        for (const auto& [_, bucket] : table) {
            totalBuckets++;
            if (bucket.empty()) emptyBuckets++;
            maxBucketSize = max(maxBucketSize, bucket.size());
            avgBucketSize += bucket.size();
        }
    }
    
    avgBucketSize /= totalBuckets;
    
    cout << "Total de buckets: " << totalBuckets << endl;
    cout << "Buckets vazios: " << emptyBuckets << endl;
    cout << "Tamanho médio do bucket: " << avgBucketSize << endl;
    cout << "Maior bucket: " << maxBucketSize << " usuários" << endl;
    cout << "=========================" << endl;
}