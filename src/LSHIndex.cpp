#include "LSHIndex.hpp"

// O include e o namespace para 'cout' e 'chrono' foram removidos.
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
    // Gerações de hash compartilhadas
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

    vector<pair<uint32_t, const vector<pair<uint32_t, float>> *>> userVector;
    userVector.reserve(userRatings.size());
    for (const auto &[userId, ratings] : userRatings)
    {
        userVector.emplace_back(userId, &ratings);
    }

    // Processamento em paralelo
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
                    
                    MinHashSignature sig(userId);
                    
                    for (int h = 0; h < Config::NUM_HASH_FUNCTIONS; h++) {
                        sig.signature[h] = UINT32_MAX;
                    }
                    
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
}

void LSHIndex::indexSignatures()
{
    for (auto &table : tables)
    {
        table.clear();
        table.reserve(signatures.size() * Config::NUM_BANDS / 100);
    }

    const int BANDS_PER_TABLE = 3;

    for (const auto &[userId, sig] : signatures)
    {
        for (int tableIdx = 0; tableIdx < Config::NUM_TABLES; tableIdx++)
        {
            int startBand = (tableIdx * BANDS_PER_TABLE) % Config::NUM_BANDS;
            size_t combinedHash = 0;
            for (int i = 0; i < BANDS_PER_TABLE; i++)
            {
                int bandIdx = (startBand + i) % Config::NUM_BANDS;
                size_t bandHash = hashBand(sig, bandIdx, tableIdx);
                combinedHash = (combinedHash << 16) ^ bandHash;
            }

            size_t finalHash = combinedHash % 4000;
            tables[tableIdx][finalHash].push_back(userId);
        }
    }
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

    const int BANDS_PER_TABLE = 3;

    for (int tableIdx = 0; tableIdx < Config::NUM_TABLES; tableIdx++)
    {
        int startBand = (tableIdx * BANDS_PER_TABLE) % Config::NUM_BANDS;
        size_t combinedHash = 0;
        for (int i = 0; i < BANDS_PER_TABLE; i++)
        {
            int bandIdx = (startBand + i) % Config::NUM_BANDS;
            size_t bandHash = hashBand(querySignature, bandIdx, tableIdx);
            combinedHash = (combinedHash << 16) ^ bandHash;
        }

        size_t finalHash = combinedHash % 4000;
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

    if (candidateCount.size() < 50)
    {
        for (int tableIdx = 0; tableIdx < min(3, Config::NUM_TABLES); tableIdx++)
        {
            for (int probe = 1; probe <= 2; probe++)
            {
                int startBand = (tableIdx * BANDS_PER_TABLE) % Config::NUM_BANDS;
                size_t combinedHash = 0;

                for (int i = 0; i < BANDS_PER_TABLE; i++)
                {
                    int bandIdx = (startBand + i) % Config::NUM_BANDS;
                    size_t bandHash = hashBand(querySignature, bandIdx, tableIdx);
                    bandHash = (bandHash + probe) % Config::LARGE_PRIME;
                    combinedHash = (combinedHash << 16) ^ bandHash;
                }

                size_t finalHash = combinedHash % 4000;
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

    vector<pair<int, uint32_t>> scoredCandidates;
    scoredCandidates.reserve(candidateCount.size());

    for (const auto &[candidateId, count] : candidateCount)
    {
        float similarity = estimateJaccardSimilarity(userId, candidateId);
        float score = count * 0.3f + similarity * 0.7f;
        scoredCandidates.push_back({(int)(score * 1000), candidateId});
    }

    sort(scoredCandidates.begin(), scoredCandidates.end(), greater<pair<int, uint32_t>>());

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

    for (int i = 0; i < Config::ROWS_PER_BAND; i++)
    {
        uint32_t val = sig.signature[startIdx + i];
        uint64_t temp = (uint64_t)bandHashParams[tableIdx][bandIdx].a * val +
                        bandHashParams[tableIdx][bandIdx].b;
        hash = (hash * 37 + (temp % 20000)) % Config::LARGE_PRIME;
    }

    return hash % 20000;
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
    // O corpo desta função foi esvaziado para remover as impressões de estatísticas.
}