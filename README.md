# 🎬 Sistema de Recomendação MovieLens - Workflow Detalhado

## 📋 Índice
1. [Visão Geral da Arquitetura](#visão-geral-da-arquitetura)
2. [Carregamento e Estruturação dos Dados](#carregamento-e-estruturação-dos-dados)
3. [Sistema de Categorização e Preferências](#sistema-de-categorização-e-preferências)
4. [LSH - Locality-Sensitive Hashing](#lsh---locality-sensitive-hashing)
5. [Cálculos de Similaridade](#cálculos-de-similaridade)
6. [Motor de Recomendação Híbrido](#motor-de-recomendação-híbrido)
7. [Fluxo Completo de Execução](#fluxo-completo-de-execução)

---

## 🏗️ Visão Geral da Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│                   SISTEMA DE RECOMENDAÇÃO                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  [Dados] → [Categorização] → [LSH Index] → [Similaridade]  │
│                                     ↓                       │
│                            [Motor de Recomendação]          │
│                                     ↓                       │
│                          [Recomendações Híbridas]           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Componentes Principais:
- **DataLoader**: Carrega ratings e filmes, estrutura dados em memória
- **LSHIndex**: Cria índice para busca rápida de usuários similares
- **SimilarityCalculator**: Calcula correlação de Pearson entre usuários
- **RecommendationEngine**: Combina CF + CB + Popularity

---

## 📊 Carregamento e Estruturação dos Dados

### 1. **Estruturas de Dados Principais**

```cpp
// Perfil de cada usuário
UserProfile {
    vector<pair<movieId, rating>> ratings;  // ORDENADO por movieId!
    float avgRating;                        // Média pré-calculada
    uint32_t preferredGenres;              // Bitmask dos top 5 gêneros
}

// Informações de cada filme
Movie {
    string title;
    uint32_t genreBitmask;     // Gêneros como bits (eficiente!)
    vector<string> genres;      // Lista legível
}
```

### 2. **Índices Invertidos**

```cpp
// Filme → Usuários que assistiram
movieToUsers[movieId] = [(userId, rating), ...]

// Gênero → Filmes desse gênero
genreToMovies[genreId] = [movieId1, movieId2, ...]
```

### 3. **Por que isso importa?**
- Arrays **ordenados** permitem merge O(n) ao invés de O(n²)
- **Bitmasks** permitem operações de conjunto em O(1)
- **Índices invertidos** aceleram buscas reversas

---

## 🎯 Sistema de Categorização e Preferências

### 1. **Mapeamento de Gêneros para Bits**

```
Gênero        → ID  → Bit
Action        → 0   → 0b00000001
Adventure     → 1   → 0b00000010
Animation     → 2   → 0b00000100
Comedy        → 3   → 0b00001000
...
```

### 2. **Cálculo de Preferências do Usuário**

```python
Para cada usuário:
    score_por_genero = {}
    
    Para cada filme assistido:
        Se rating >= 3.5:  # Gostou do filme
            Para cada gênero do filme:
                score_por_genero[gênero] += (rating - 3.5)
    
    # Pega top 5 gêneros
    top_5 = sorted(score_por_genero, by=score)[:5]
    
    # Cria bitmask
    user.preferredGenres = genre[0] | genre[1] | ... | genre[4]
```

### 3. **Exemplo Prático**

```
Usuário assistiu:
- Matrix (Action|Sci-Fi): 5.0 ⭐
- Toy Story (Animation): 4.5 ⭐
- Inception (Action|Sci-Fi): 5.0 ⭐

Scores:
- Sci-Fi: (5.0-3.5) + (5.0-3.5) = 3.0
- Action: (5.0-3.5) + (5.0-3.5) = 3.0
- Animation: (4.5-3.5) = 1.0

preferredGenres = 0b00010001 (Sci-Fi | Action)
```

---

## 🔍 LSH - Locality-Sensitive Hashing

### 1. **O Problema que Resolve**

```
Sem LSH: Comparar 1 usuário com 300k outros = 300k operações
Com LSH: Comparar 1 usuário com ~500 candidatos = 500 operações!
```

### 2. **Como Funciona - A Analogia da Gincana**

Imagine uma **gincana de compatibilidade** com várias provas:

```
GINCANA 1:
├── Prova 1: "Qual seu filme favorito dos anos 80?"
├── Prova 2: "Prefere ação ou romance?"
├── Prova 3: "Gosta de ficção científica?"
└── Prova 4: "Assiste documentários?"

Se duas pessoas dão respostas IDÊNTICAS numa prova,
elas vão para o mesmo "grupo" (bucket)
```

### 3. **Implementação Técnica: MinHash**

#### Passo 1: Criar Assinatura MinHash
```python
Para cada usuário:
    signature = []
    
    Para cada função hash h (temos 128):
        min_hash = INFINITO
        
        Para cada filme que o usuário viu:
            hash_value = h(movieId)
            min_hash = min(min_hash, hash_value)
        
        signature.append(min_hash)
```

#### Passo 2: Dividir em Bands
```
Assinatura de 128 valores → 32 bands × 4 valores cada

Band 0: [sig[0], sig[1], sig[2], sig[3]]
Band 1: [sig[4], sig[5], sig[6], sig[7]]
...
Band 31: [sig[124], sig[125], sig[126], sig[127]]
```

#### Passo 3: Hash de cada Band
```python
Para cada band b:
    hash = combineHash(sig[b*4], sig[b*4+1], sig[b*4+2], sig[b*4+3])
    tables[t][hash].append(userId)
```

### 4. **Parâmetros e Trade-offs**

```
NUM_HASH_FUNCTIONS = 128   # Precisão da assinatura
NUM_BANDS = 32            # Número de "chances" de colidir
ROWS_PER_BAND = 4         # Rigor de cada comparação
NUM_TABLES = 10           # Redundância para não perder similares

Trade-offs:
- Mais bands → Mais recall (acha mais similares)
- Mais rows/band → Mais precision (menos falsos positivos)
- Mais tables → Mais garantia mas mais memória
```

### 5. **Por que Funciona?**

**Probabilidade de colisão em uma band:**
- Usuários 80% similares: P = 0.8⁴ = 0.41
- Usuários 50% similares: P = 0.5⁴ = 0.06
- Usuários 20% similares: P = 0.2⁴ = 0.0016

Com 32 bands, a chance de colidir em ALGUMA:
- 80% similares: 1-(1-0.41)³² ≈ 0.999 ✓
- 50% similares: 1-(1-0.06)³² ≈ 0.87 ✓
- 20% similares: 1-(1-0.0016)³² ≈ 0.05 ✗

---

## 📐 Cálculos de Similaridade

### 1. **Correlação de Pearson**

```python
def pearson_correlation(user1, user2):
    # Encontra filmes em comum (merge de arrays ordenados)
    common_movies = merge_common(user1.ratings, user2.ratings)
    
    if len(common_movies) < 5:
        return 0  # Poucos dados
    
    # Cálculo otimizado (evita calcular médias)
    n = len(common_movies)
    sum1 = sum(r1 for _, r1, _ in common_movies)
    sum2 = sum(r2 for _, _, r2 in common_movies)
    sum1_sq = sum(r1*r1 for _, r1, _ in common_movies)
    sum2_sq = sum(r2*r2 for _, _, r2 in common_movies)
    sum_prod = sum(r1*r2 for _, r1, r2 in common_movies)
    
    num = sum_prod - (sum1 * sum2 / n)
    den = sqrt((sum1_sq - sum1²/n) * (sum2_sq - sum2²/n))
    
    return num/den if den != 0 else 0
```

### 2. **Por que Pearson ao invés de Cosseno?**

```
User A (generoso): [5, 5, 5, 5]
User B (crítico):  [3, 3, 3, 3]

Cosseno: 1.0 (máxima similaridade) ❌
Pearson: undefined (sem variância) ✅
```

### 3. **Cache de Similaridades**

```cpp
// Chave: min(user1,user2) << 32 | max(user1,user2)
unordered_map<uint64_t, float> cache;

// Thread-safe com mutex
lock_guard<mutex> lock(cacheMutex);
cache[key] = similarity;
```

---

## 🎯 Motor de Recomendação Híbrido

### 1. **Arquitetura: 70% CF + 30% CB + Fallback**

```
┌─────────────────────────────────────────┐
│         Collaborative Filtering         │
│              (70% peso)                 │
│   "Usuários como você também gostam"    │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│          Content-Based                  │
│            (30% peso)                   │
│    "Baseado nos seus gêneros"          │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│         Popularity Fallback             │
│      (se < 20 recomendações)           │
│         "Os mais populares"            │
└─────────────────────────────────────────┘
```

### 2. **Fase 1: Encontrar Candidatos (com LSH)**

```python
def find_candidates_lsh(userId):
    # Busca no LSH
    candidates = lsh_index.find_similar(userId, max=1000)
    
    # Filtra por filmes em comum
    filtered = []
    for candidate in candidates:
        common = count_common_movies(userId, candidate)
        if common >= 5:
            filtered.append((candidate, common))
    
    # Ordena por número de filmes em comum
    return sorted(filtered, by=common_count)[:500]
```

### 3. **Fase 2: Collaborative Filtering**

```python
def collaborative_filtering(user, similar_users):
    scores = {}
    
    for similar_user, similarity in similar_users:
        if similarity < 0.1:
            continue
            
        for movieId, rating in similar_user.ratings:
            if movieId not in user.watched:
                # Pondera pela similaridade e remove viés
                contribution = similarity * (rating - similar_user.avg)
                scores[movieId] += contribution
    
    # Normaliza e adiciona baseline
    for movieId in scores:
        scores[movieId] = scores[movieId] / sum_similarities
        scores[movieId] += movie_avg_rating[movieId]
    
    return scores
```

### 4. **Fase 3: Content-Based Boost**

```python
def content_based_boost(user, scores):
    for genreId in user.preferred_genres:
        for movieId in genre_to_movies[genreId]:
            if movieId not in user.watched:
                # Qualidade × Popularidade
                quality = movie_avg_rating[movieId] / 5.0
                popularity = min(1.0, log(movie_views[movieId]) / 10)
                
                boost = 0.5 * quality + 0.5 * popularity
                scores[movieId] += boost * 0.3  # 30% peso
```

### 5. **Fase 4: Popularity Fallback**

```python
def popularity_fallback(user, scores):
    if len(scores) >= 20:
        return  # Já tem recomendações suficientes
    
    popular_movies = sorted(movies, by=views*rating)
    
    for movie in popular_movies:
        if movie not in user.watched and movie not in scores:
            scores[movie] = (views * rating) / 100
            if len(scores) >= 20:
                break
```

---

## 🔄 Fluxo Completo de Execução

### 1. **Inicialização (uma vez)**
```
1. Carrega ratings → Cria UserProfiles
2. Carrega movies → Cria índices de gênero  
3. Calcula preferências → Bitmasks
4. Constrói LSH → MinHash + Indexação
   └─ 300k users → 128-dim signatures → 10 tables
```

### 2. **Para cada usuário a recomendar:**
```
1. LSH Query
   └─ Input: userId
   └─ Output: ~500 candidatos similares
   
2. Refina candidatos
   └─ Filtra: ≥5 filmes em comum
   └─ Ordena: por overlap
   └─ Mantém: top 500

3. Calcula Pearson
   └─ Paralelo: batches de 100
   └─ Cache: evita recálculos
   └─ Output: [(user, similarity)]

4. Collaborative Filtering
   └─ Para cada similar: agrega scores
   └─ Normaliza: divide por Σ(sim)
   └─ Baseline: + média do filme

5. Content-Based
   └─ Para cada gênero preferido
   └─ Boost: quality × popularity
   └─ Peso: 30% do score final

6. Verifica cobertura
   └─ Se < 20: adiciona populares
   └─ Ordena: top scores
   └─ Retorna: 20 recomendações
```

### 3. **Exemplo de Tempo Real**
```
User 123 request → 
LSH lookup (5ms) →
Filter candidates (10ms) →
Pearson batch (150ms) →
CF aggregation (20ms) →
CB boost (5ms) →
Sort & return (2ms)
= 192ms total
```

---

## 📈 Complexidade e Performance

### Sem LSH:
- Candidatos: O(n) onde n = 300k usuários
- Similaridade: O(n × m) onde m = filmes comuns
- **Total: ~30 segundos por usuário**

### Com LSH:
- Candidatos: O(b × t) onde b=32 bands, t=10 tables
- Similaridade: O(k × m) onde k=500 candidatos
- **Total: ~200ms por usuário (150x mais rápido!)**

### Paralelização:
- 8 threads = ~25ms por usuário
- 1000 usuários em ~25 segundos total

---

## 🎯 Por que Funciona?

1. **LSH** reduz espaço de busca em 99.8%
2. **Pearson** captura gostos reais (não apenas magnitude)
3. **Híbrido** cobre cold-start e nichos
4. **Paralelo** aproveita hardware moderno
5. **Cache** evita recálculos caros

O resultado é um sistema que gera recomendações personalizadas de alta qualidade em menos de 200ms por usuário!