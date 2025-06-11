#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <random>
#include <thread>
#include <mutex>
#include <queue>
#include <future>
#include <iomanip>
#include <cstring>
#include <list>
#include <condition_variable>
#include <functional>

using namespace std;
using namespace chrono;

// Configurações do sistema
const int NUM_FACTORS = 64;          // Número de fatores latentes
const int NUM_HASH_TABLES = 12;      // Tabelas LSH
const int NUM_PROJECTIONS = 64;      // Projeções LSH
const float LEARNING_RATE = 0.01f;   // Taxa de aprendizado
const float REGULARIZATION = 0.01f;  // Regularização
const int MAX_ITERATIONS = 5;        // Iterações do ALS
const int TOP_K = 20;                // Top K recomendações
const int NUM_THREADS = thread::hardware_concurrency();

// Estruturas de dados otimizadas
struct Rating {
    uint32_t userId;
    uint32_t movieId;
    float rating;
};

struct Movie {
    uint32_t id;
    string title;
    vector<string> genres;
    uint32_t genreBitmask;  // Representação binária dos gêneros
};

// Classe para matriz esparsa otimizada
class SparseMatrix {
private:
    vector<vector<pair<uint32_t, float>>> data;
    uint32_t rows, cols;
    
public:
    SparseMatrix(uint32_t r, uint32_t c) : rows(r), cols(c) {
        data.resize(r);
    }
    
    void set(uint32_t row, uint32_t col, float val) {
        if (row >= rows) data.resize(row + 1);
        data[row].push_back({col, val});
    }
    
    float get(uint32_t row, uint32_t col) const {
        if (row >= data.size()) return 0.0f;
        for (const auto& p : data[row]) {
            if (p.first == col) return p.second;
        }
        return 0.0f;
    }
    
    const vector<pair<uint32_t, float>>& getRow(uint32_t row) const {
        static vector<pair<uint32_t, float>> empty;
        return row < data.size() ? data[row] : empty;
    }
    
    void sortRows() {
        for (auto& row : data) {
            sort(row.begin(), row.end());
        }
    }
};

// Pool de memória para alocações frequentes
template<typename T>
class MemoryPool {
private:
    vector<T*> pool;
    queue<T*> available;
    mutex poolMutex;
    
public:
    MemoryPool(size_t initialSize = 1000) {
        for (size_t i = 0; i < initialSize; ++i) {
            T* obj = new T();
            pool.push_back(obj);
            available.push(obj);
        }
    }
    
    T* acquire() {
        lock_guard<mutex> lock(poolMutex);
        if (available.empty()) {
            T* obj = new T();
            pool.push_back(obj);
            return obj;
        }
        T* obj = available.front();
        available.pop();
        return obj;
    }
    
    void release(T* obj) {
        lock_guard<mutex> lock(poolMutex);
        available.push(obj);
    }
    
    ~MemoryPool() {
        for (T* obj : pool) delete obj;
    }
};

template<typename K, typename V>
class LRUCache {
private:
    unordered_map<K, pair<V, typename list<K>::iterator>> cache;
    list<K> lru;
    size_t capacity;
    mutable mutex cacheMutex; // OK manter mutável, pois mutex pode ser trancado em métodos const

public:
    LRUCache(size_t cap) : capacity(cap) {}

    bool get(const K& key, V& value) { // <<< const removido aqui
        lock_guard<mutex> lock(cacheMutex);
        auto it = cache.find(key);
        if (it == cache.end()) return false;

        // Move para o início (mais recente)
        lru.splice(lru.begin(), lru, it->second.second);
        value = it->second.first;
        return true;
    }

    void put(const K& key, const V& value) {
        lock_guard<mutex> lock(cacheMutex);
        auto it = cache.find(key);

        if (it != cache.end()) {
            lru.erase(it->second.second);
            cache.erase(it);
        }

        if (cache.size() >= capacity) {
            cache.erase(lru.back());
            lru.pop_back();
        }

        lru.push_front(key);
        cache[key] = {value, lru.begin()};
    }
};


// Classe principal do sistema de recomendação
class RecommendationSystem {
private:
    // Dados
    SparseMatrix* userItemMatrix;
    unordered_map<uint32_t, Movie> movies;
    unordered_map<string, uint32_t> genreToId;
    unordered_map<uint32_t, uint32_t> userIdMap;
    unordered_map<uint32_t, uint32_t> movieIdMap;
    vector<uint32_t> reverseUserMap;
    vector<uint32_t> reverseMovieMap;
    
    // Matrix Factorization
    vector<vector<float>> userFactors;
    vector<vector<float>> itemFactors;
    
    // LSH
    vector<vector<vector<float>>> lshProjections;
    vector<unordered_map<size_t, vector<uint32_t>>> lshTables;
    
    // Cache
    LRUCache<uint32_t, vector<pair<uint32_t, float>>>* recommendationCache;
    
    // Thread pool
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex queueMutex;
    condition_variable cv;
    bool stop = false;
    
    // Estatísticas
    float avgRating = 0.0f;
    
public:
    RecommendationSystem() {
        userItemMatrix = nullptr;
        recommendationCache = new LRUCache<uint32_t, vector<pair<uint32_t, float>>>(10000);
        initializeThreadPool();
    }
    
    ~RecommendationSystem() {
        stop = true;
        cv.notify_all();
        for (auto& worker : workers) {
            worker.join();
        }
        delete userItemMatrix;
        delete recommendationCache;
    }
    
    void initializeThreadPool() {
        for (int i = 0; i < NUM_THREADS; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(queueMutex);
                        cv.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }
    
    template<typename F>
    future<void> enqueueTask(F&& f) {
        auto task = make_shared<packaged_task<void()>>(forward<F>(f));
        future<void> res = task->get_future();
        {
            unique_lock<mutex> lock(queueMutex);
            tasks.emplace([task]() { (*task)(); });
        }
        cv.notify_one();
        return res;
    }
    
    // Carrega os dados de rating
    void loadRatings(const string& filename) {
        cout << "Carregando ratings..." << endl;
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Erro ao abrir " << filename << endl;
            return;
        }
        
        string line;
        uint32_t userCounter = 0, movieCounter = 0;
        vector<Rating> allRatings;
        
        while (getline(file, line)) {
            istringstream iss(line);
            uint32_t userId;
            iss >> userId;
            
            if (userIdMap.find(userId) == userIdMap.end()) {
                userIdMap[userId] = userCounter++;
                reverseUserMap.push_back(userId);
            }
            
            string ratingPair;
            while (iss >> ratingPair) {
                size_t colonPos = ratingPair.find(':');
                if (colonPos != string::npos) {
                    uint32_t movieId = stoul(ratingPair.substr(0, colonPos));
                    float rating = stof(ratingPair.substr(colonPos + 1));
                    
                    if (movieIdMap.find(movieId) == movieIdMap.end()) {
                        movieIdMap[movieId] = movieCounter++;
                        reverseMovieMap.push_back(movieId);
                    }
                    
                    allRatings.push_back({userIdMap[userId], movieIdMap[movieId], rating});
                    avgRating += rating;
                }
            }
        }
        
        avgRating /= allRatings.size();
        
        // Cria matriz esparsa
        userItemMatrix = new SparseMatrix(userCounter, movieCounter);
        for (const auto& r : allRatings) {
            userItemMatrix->set(r.userId, r.movieId, r.rating - avgRating);
        }
        userItemMatrix->sortRows();
        
        cout << "Carregados " << userCounter << " usuários e " 
             << movieCounter << " filmes" << endl;
    }
    
    // Carrega informações dos filmes
    void loadMovies(const string& filename) {
        cout << "Carregando filmes..." << endl;
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Erro ao abrir " << filename << endl;
            return;
        }
        
        string line;
        getline(file, line); // Pula header
        
        uint32_t genreCounter = 0;
        
        while (getline(file, line)) {
            istringstream iss(line);
            string movieIdStr, title, genres;
            
            getline(iss, movieIdStr, ',');
            getline(iss, title, ',');
            getline(iss, genres);
            
            // Remove aspas do título
            if (title.front() == '"') title = title.substr(1, title.length() - 2);
            
            uint32_t movieId = stoul(movieIdStr);
            Movie movie;
            movie.id = movieId;
            movie.title = title;
            movie.genreBitmask = 0;
            
            // Parse dos gêneros
            istringstream genreStream(genres);
            string genre;
            while (getline(genreStream, genre, '|')) {
                movie.genres.push_back(genre);
                
                if (genreToId.find(genre) == genreToId.end()) {
                    genreToId[genre] = genreCounter++;
                }
                movie.genreBitmask |= (1 << genreToId[genre]);
            }
            
            movies[movieId] = movie;
        }
        
        cout << "Carregados " << movies.size() << " filmes com " 
             << genreToId.size() << " gêneros" << endl;
    }
    
    // Treina o modelo usando ALS (Alternating Least Squares)
    void trainModel() {
        cout << "Treinando modelo..." << endl;
        auto start = high_resolution_clock::now();
        
        uint32_t numUsers = reverseUserMap.size();
        uint32_t numItems = reverseMovieMap.size();
        
        // Inicializa fatores aleatoriamente
        random_device rd;
        mt19937 gen(rd());
        normal_distribution<float> dist(0.0f, 0.1f);
        
        userFactors.resize(numUsers, vector<float>(NUM_FACTORS));
        itemFactors.resize(numItems, vector<float>(NUM_FACTORS));
        
        for (auto& factors : userFactors) {
            for (auto& f : factors) f = dist(gen);
        }
        for (auto& factors : itemFactors) {
            for (auto& f : factors) f = dist(gen);
        }
        
        // ALS iterations
        for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
            cout << "Iteração " << (iter + 1) << "/" << MAX_ITERATIONS << endl;
            
            // Atualiza fatores dos usuários em paralelo
            vector<future<void>> futures;
            size_t batchSize = numUsers / NUM_THREADS;
            
            for (int t = 0; t < NUM_THREADS; ++t) {
                size_t start = t * batchSize;
                size_t end = (t == NUM_THREADS - 1) ? numUsers : (t + 1) * batchSize;
                
                futures.push_back(enqueueTask([this, start, end]() {
                    updateUserFactors(start, end);
                }));
            }
            
            for (auto& f : futures) f.wait();
            futures.clear();
            
            // Atualiza fatores dos itens em paralelo
            batchSize = numItems / NUM_THREADS;
            
            for (int t = 0; t < NUM_THREADS; ++t) {
                size_t start = t * batchSize;
                size_t end = (t == NUM_THREADS - 1) ? numItems : (t + 1) * batchSize;
                
                futures.push_back(enqueueTask([this, start, end]() {
                    updateItemFactors(start, end);
                }));
            }
            
            for (auto& f : futures) f.wait();
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(end - start);
        cout << "Modelo treinado em " << duration.count() << " segundos" << endl;
        
        // Constrói índice LSH
        buildLSHIndex();
    }
    
    void updateUserFactors(size_t startUser, size_t endUser) {
        for (size_t u = startUser; u < endUser; ++u) {
            const auto& userRatings = userItemMatrix->getRow(u);
            if (userRatings.empty()) continue;
            
            // Matriz A = I^T * I + λI
            vector<vector<float>> A(NUM_FACTORS, vector<float>(NUM_FACTORS, 0.0f));
            vector<float> b(NUM_FACTORS, 0.0f);
            
            // Adiciona regularização
            for (int i = 0; i < NUM_FACTORS; ++i) {
                A[i][i] = REGULARIZATION;
            }
            
            // Calcula A e b
            for (const auto& [itemId, rating] : userRatings) {
                for (int i = 0; i < NUM_FACTORS; ++i) {
                    for (int j = 0; j < NUM_FACTORS; ++j) {
                        A[i][j] += itemFactors[itemId][i] * itemFactors[itemId][j];
                    }
                    b[i] += rating * itemFactors[itemId][i];
                }
            }
            
            // Resolve o sistema linear (método de eliminação Gaussiana simplificado)
            solveLinearSystem(A, b, userFactors[u]);
        }
    }
    
    void updateItemFactors(size_t startItem, size_t endItem) {
        for (size_t i = startItem; i < endItem; ++i) {
            vector<pair<uint32_t, float>> itemRatings;
            
            // Coleta ratings para este item
            for (uint32_t u = 0; u < userFactors.size(); ++u) {
                float rating = userItemMatrix->get(u, i);
                if (rating != 0.0f) {
                    itemRatings.push_back({u, rating});
                }
            }
            
            if (itemRatings.empty()) continue;
            
            // Matriz A = U^T * U + λI
            vector<vector<float>> A(NUM_FACTORS, vector<float>(NUM_FACTORS, 0.0f));
            vector<float> b(NUM_FACTORS, 0.0f);
            
            // Adiciona regularização
            for (int k = 0; k < NUM_FACTORS; ++k) {
                A[k][k] = REGULARIZATION;
            }
            
            // Calcula A e b
            for (const auto& [userId, rating] : itemRatings) {
                for (int k = 0; k < NUM_FACTORS; ++k) {
                    for (int j = 0; j < NUM_FACTORS; ++j) {
                        A[k][j] += userFactors[userId][k] * userFactors[userId][j];
                    }
                    b[k] += rating * userFactors[userId][k];
                }
            }
            
            // Resolve o sistema linear
            solveLinearSystem(A, b, itemFactors[i]);
        }
    }
    
    void solveLinearSystem(vector<vector<float>>& A, vector<float>& b, vector<float>& x) {
        int n = A.size();
        
        // Eliminação Gaussiana com pivoteamento parcial
        for (int i = 0; i < n; ++i) {
            // Encontra pivô
            int maxRow = i;
            for (int k = i + 1; k < n; ++k) {
                if (abs(A[k][i]) > abs(A[maxRow][i])) {
                    maxRow = k;
                }
            }
            
            // Troca linhas
            swap(A[i], A[maxRow]);
            swap(b[i], b[maxRow]);
            
            // Eliminação
            for (int k = i + 1; k < n; ++k) {
                float factor = A[k][i] / A[i][i];
                for (int j = i; j < n; ++j) {
                    A[k][j] -= factor * A[i][j];
                }
                b[k] -= factor * b[i];
            }
        }
        
        // Substituição reversa
        for (int i = n - 1; i >= 0; --i) {
            x[i] = b[i];
            for (int j = i + 1; j < n; ++j) {
                x[i] -= A[i][j] * x[j];
            }
            x[i] /= A[i][i];
        }
    }
    
    // Constrói índice LSH para busca rápida
    void buildLSHIndex() {
        cout << "Construindo índice LSH..." << endl;
        
        random_device rd;
        mt19937 gen(rd());
        normal_distribution<float> dist(0.0f, 1.0f);
        
        // Gera projeções aleatórias
        lshProjections.resize(NUM_HASH_TABLES);
        for (int t = 0; t < NUM_HASH_TABLES; ++t) {
            lshProjections[t].resize(NUM_PROJECTIONS);
            for (int p = 0; p < NUM_PROJECTIONS; ++p) {
                lshProjections[t][p].resize(NUM_FACTORS);
                for (int f = 0; f < NUM_FACTORS; ++f) {
                    lshProjections[t][p][f] = dist(gen);
                }
            }
        }
        
        // Constrói tabelas hash
        lshTables.resize(NUM_HASH_TABLES);
        
        for (uint32_t itemId = 0; itemId < itemFactors.size(); ++itemId) {
            for (int t = 0; t < NUM_HASH_TABLES; ++t) {
                size_t hashValue = computeLSHHash(itemFactors[itemId], t);
                lshTables[t][hashValue].push_back(itemId);
            }
        }
        
        cout << "Índice LSH construído" << endl;
    }
    
    size_t computeLSHHash(const vector<float>& factors, int tableIndex) {
        size_t hash = 0;
        for (int p = 0; p < NUM_PROJECTIONS; ++p) {
            float projection = 0.0f;
            for (int f = 0; f < NUM_FACTORS; ++f) {
                projection += factors[f] * lshProjections[tableIndex][p][f];
            }
            if (projection >= 0) {
                hash |= (1ULL << p);
            }
        }
        return hash;
    }
    
    // Encontra itens similares usando LSH
    vector<uint32_t> findSimilarItemsLSH(const vector<float>& queryFactors, int maxCandidates) {
        unordered_set<uint32_t> candidates;
        
        for (int t = 0; t < NUM_HASH_TABLES; ++t) {
            size_t hash = computeLSHHash(queryFactors, t);
            
            // Busca em buckets vizinhos também (hamming distance = 1)
            for (int flip = -1; flip < NUM_PROJECTIONS; ++flip) {
                size_t searchHash = hash;
                if (flip >= 0) {
                    searchHash ^= (1ULL << flip);
                }
                
                auto it = lshTables[t].find(searchHash);
                if (it != lshTables[t].end()) {
                    for (uint32_t itemId : it->second) {
                        candidates.insert(itemId);
                        if (candidates.size() >= static_cast<size_t>(maxCandidates)) {
                            goto end_search;
                        }
                    }
                }
            }
        }
        end_search:
        
        return vector<uint32_t>(candidates.begin(), candidates.end());
    }
    
    // Calcula similaridade por cosseno
    float cosineSimilarity(const vector<float>& a, const vector<float>& b) {
        float dot = 0.0f, normA = 0.0f, normB = 0.0f;
        for (int i = 0; i < NUM_FACTORS; ++i) {
            dot += a[i] * b[i];
            normA += a[i] * a[i];
            normB += b[i] * b[i];
        }
        return dot / (sqrt(normA) * sqrt(normB) + 1e-8f);
    }
    
    // Calcula similaridade de gêneros (Jaccard)
    float genreSimilarity(uint32_t movie1, uint32_t movie2) {
        auto it1 = movies.find(reverseMovieMap[movie1]);
        auto it2 = movies.find(reverseMovieMap[movie2]);
        
        if (it1 == movies.end() || it2 == movies.end()) return 0.0f;
        
        uint32_t intersection = it1->second.genreBitmask & it2->second.genreBitmask;
        uint32_t unionSet = it1->second.genreBitmask | it2->second.genreBitmask;
        
        if (unionSet == 0) return 0.0f;
        
        return __builtin_popcount(intersection) / (float)__builtin_popcount(unionSet);
    }
    
    // Gera recomendações para um usuário
    vector<pair<uint32_t, float>> recommendForUser(uint32_t originalUserId) {
        auto it = userIdMap.find(originalUserId);
        if (it == userIdMap.end()) {
            return {};
        }
        
        uint32_t userId = it->second;
        
        // Verifica cache
        vector<pair<uint32_t, float>> cached;
        if (recommendationCache->get(userId, cached)) {
            return cached;
        }
        
        // Obtém filmes já assistidos
        unordered_set<uint32_t> watchedMovies;
        const auto& userRatings = userItemMatrix->getRow(userId);
        for (const auto& [movieId, _] : userRatings) {
            watchedMovies.insert(movieId);
        }
        
        // Estratégia híbrida: collaborative filtering + content-based
        vector<pair<uint32_t, float>> recommendations;
        
        // 1. Collaborative Filtering usando Matrix Factorization + LSH
        if (userId < userFactors.size()) {
            // Encontra itens candidatos usando LSH
            vector<uint32_t> candidates = findSimilarItemsLSH(userFactors[userId], 1000);
            
            // Calcula scores
            for (uint32_t itemId : candidates) {
                if (watchedMovies.find(itemId) != watchedMovies.end()) continue;
                
                float score = 0.0f;
                for (int f = 0; f < NUM_FACTORS; ++f) {
                    score += userFactors[userId][f] * itemFactors[itemId][f];
                }
                score += avgRating;
                
                recommendations.push_back({itemId, score});
            }
        }
        
        // 2. Content-based filtering para cold start ou diversificação
        if (recommendations.size() < TOP_K * 2) {
            unordered_map<uint32_t, float> contentScores;
            
            // Calcula perfil de gêneros do usuário
            unordered_map<uint32_t, float> genrePreferences;
            for (const auto& [movieId, rating] : userRatings) {
                auto it = movies.find(reverseMovieMap[movieId]);
                if (it != movies.end()) {
                    for (const auto& genre : it->second.genres) {
                        genrePreferences[genreToId[genre]] += rating;
                    }
                }
            }
            
            // Normaliza preferências
            float totalPref = 0.0f;
            for (auto& [_, pref] : genrePreferences) totalPref += abs(pref);
            if (totalPref > 0) {
                for (auto& [_, pref] : genrePreferences) pref /= totalPref;
            }
            
            // Calcula scores baseados em conteúdo
            for (uint32_t itemId = 0; itemId < reverseMovieMap.size(); ++itemId) {
                if (watchedMovies.find(itemId) != watchedMovies.end()) continue;
                
                auto it = movies.find(reverseMovieMap[itemId]);
                if (it == movies.end()) continue;
                
                float contentScore = 0.0f;
                for (const auto& genre : it->second.genres) {
                    auto genreIt = genreToId.find(genre);
                    if (genreIt != genreToId.end()) {
                        contentScore += genrePreferences[genreIt->second];
                    }
                }
                
                if (contentScore > 0) {
                    contentScores[itemId] = contentScore;
                }
            }
            
            // Adiciona top content-based recommendations
            vector<pair<uint32_t, float>> contentRecs;
            for (const auto& [itemId, score] : contentScores) {
                contentRecs.push_back({itemId, score});
            }
            sort(contentRecs.begin(), contentRecs.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
            
            // Mescla com collaborative filtering (70% CF, 30% CB)
            for (size_t i = 0; i < min(contentRecs.size(), (size_t)TOP_K); ++i) {
                bool found = false;
                for (auto& [itemId, score] : recommendations) {
                    if (itemId == contentRecs[i].first) {
                        score = 0.7f * score + 0.3f * (contentRecs[i].second * 5.0f);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    recommendations.push_back({contentRecs[i].first, contentRecs[i].second * 5.0f * 0.3f});
                }
            }
        }
        
        // Ordena e seleciona top K
        sort(recommendations.begin(), recommendations.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
        
        if (recommendations.size() > TOP_K) {
            recommendations.resize(TOP_K);
        }
        
        // Converte IDs de volta
        vector<pair<uint32_t, float>> finalRecs;
        for (const auto& [itemId, score] : recommendations) {
            finalRecs.push_back({reverseMovieMap[itemId], score});
        }
        
        // Armazena no cache
        recommendationCache->put(userId, finalRecs);
        
        return finalRecs;
    }
    
    // Processa lista de usuários para recomendação
    void processRecommendations(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Erro ao abrir " << filename << endl;
            return;
        }
        
        string line;
        vector<uint32_t> userIds;
        while (getline(file, line)) {
            userIds.push_back(stoul(line));
        }
        
        cout << "\nGerando recomendações para " << userIds.size() << " usuários..." << endl;
        
        auto totalStart = high_resolution_clock::now();
        
        for (uint32_t userId : userIds) {
            auto start = high_resolution_clock::now();
            
            vector<pair<uint32_t, float>> recommendations = recommendForUser(userId);
            
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start);
            
            cout << "\nUsuário " << userId << " (" << duration.count() << "ms):" << endl;
            
            for (int i = 0; i < min((int)recommendations.size(), 10); ++i) {
                auto movieIt = movies.find(recommendations[i].first);
                if (movieIt != movies.end()) {
                    cout << "  " << (i + 1) << ". " << movieIt->second.title 
                         << " (Score: " << fixed << setprecision(2) 
                         << recommendations[i].second << ")" << endl;
                }
            }
        }
        
        auto totalEnd = high_resolution_clock::now();
        auto totalDuration = duration_cast<milliseconds>(totalEnd - totalStart);
        cout << "\nTempo total: " << totalDuration.count() << "ms" << endl;
        cout << "Tempo médio por usuário: " << totalDuration.count() / userIds.size() << "ms" << endl;
    }
};

int main() {
    cout << "=== Sistema de Recomendação MovieLens 25M ===" << endl;
    cout << "Threads disponíveis: " << NUM_THREADS << endl;
    
    RecommendationSystem* system = new RecommendationSystem();
    
    try {
        // Carrega dados
        auto start = high_resolution_clock::now();
        
        system->loadRatings("datasets/input.dat");
        system->loadMovies("ml-25m/movies.csv");
        
        auto loadEnd = high_resolution_clock::now();
        auto loadDuration = duration_cast<seconds>(loadEnd - start);
        cout << "Dados carregados em " << loadDuration.count() << " segundos" << endl;
        
        // Treina modelo
        system->trainModel();
        
        // Processa recomendações
        system->processRecommendations("datasets/explore.dat");
        
        auto totalEnd = high_resolution_clock::now();
        auto totalDuration = duration_cast<seconds>(totalEnd - start);
        cout << "\nTempo total de execução: " << totalDuration.count() << " segundos" << endl;
        
    } catch (const exception& e) {
        cerr << "Erro: " << e.what() << endl;
    }
    
    delete system;
    return 0;
}
