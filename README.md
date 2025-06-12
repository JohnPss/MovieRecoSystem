# 🎬 Sistema de Recomendação MovieLens - Documentação Completa do Algoritmo

## 📋 Índice
1. [Visão Geral](#visão-geral)
2. [Fase 1: Carregamento e Pré-processamento](#fase-1-carregamento-e-pré-processamento)
3. [Fase 2: Geração de Recomendações](#fase-2-geração-de-recomendações)
4. [Análise de Complexidade e Performance](#análise-de-complexidade-e-performance)
5. [Validação e Qualidade](#validação-e-qualidade)
6. [Exemplos Práticos Completos](#exemplos-práticos-completos)

---

## 🎯 Visão Geral

O sistema implementa um **algoritmo híbrido** que combina três técnicas complementares:

```
┌─────────────────────────────────────────────────────────┐
│          Sistema de Recomendação Híbrido                │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  70% Collaborative Filtering (CF)                       │
│  ├── Encontra usuários similares (Pearson)             │
│  └── Agrega suas avaliações                            │
│                                                         │
│  30% Content-Based (CB)                                 │
│  ├── Analisa gêneros preferidos                        │
│  └── Recomenda filmes similares                        │
│                                                         │
│  Fallback: Popularidade                                 │
│  └── Garante 20 recomendações                          │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 🚀 Por que essa abordagem?

- **Collaborative Filtering**: Captura padrões complexos e gostos não óbvios
- **Content-Based**: Resolve cold start e adiciona diversidade
- **Popularity**: Garante qualidade mínima e cobertura

---

## 📚 Fase 1: Carregamento e Pré-processamento

### 1.1 **Carregamento de Ratings** (`dataset/input.dat`)

```
Formato: userID movieId:rating movieId:rating ...
Exemplo: 123 1:5.0 2:4.5 3:2.0 7:4.0 9:3.5
```

#### Estruturas Criadas:

```cpp
// 1. Perfil do Usuário
UserProfile[123] = {
    ratings: [(1,5.0), (2,4.5), (3,2.0), (7,4.0), (9,3.5)]  // ORDENADO!
    avgRating: 3.8  // (5.0+4.5+2.0+4.0+3.5)/5
    preferredGenres: [calculado depois]
}

// 2. Índice Invertido (filme → usuários)
movieToUsers[1] = [(123,5.0), (456,4.0), (789,4.5), ...]
movieToUsers[2] = [(123,4.5), (321,3.0), (654,5.0), ...]

// 3. Estatísticas Globais
globalAvgRating = 3.52      // Média de TODOS os 20M ratings
movieAvgRatings[1] = 4.2    // Média do filme 1
moviePopularity[1] = 50000  // 50k pessoas viram filme 1
```

#### 🔑 Otimização Importante:
Os ratings são **ordenados por movieId** para permitir merge O(n) ao invés de busca O(n²).

### 1.2 **Carregamento de Filmes** (`ml-25m/movies.csv`)

```
Formato: movieId,title,genres
Exemplo: 1,"Toy Story (1995)",Adventure|Animation|Children
```

#### Processamento de Gêneros:

```cpp
// 1. Mapeamento gênero → ID (para bitmask)
genreToId = {
    "Adventure": 0,
    "Animation": 1,
    "Children": 2,
    "Comedy": 3,
    "Action": 4,
    "Sci-Fi": 5,
    ...
}

// 2. Filme com bitmask
Movie[1] = {
    title: "Toy Story (1995)"
    genres: ["Adventure", "Animation", "Children"]
    genreBitmask: 0b00000111  // bits 0,1,2 ligados
}

// 3. Índice gênero → filmes
genreToMovies[1] = [1, 45, 89, 234, ...]  // Todos com Animation
```

### 1.3 **Cálculo de Preferências dos Usuários**

Para cada usuário, identificamos seus gêneros favoritos:

```cpp
User 123 avaliou:
- Toy Story (Animation|Children): 5.0 ✓ (≥3.5)
- Matrix (Action|Sci-Fi): 4.5 ✓
- Transformers (Action|Sci-Fi): 2.0 ✗ (ignorado)
- WALL-E (Animation|Sci-Fi): 5.0 ✓

Cálculo de score por gênero:
Animation: (5.0-3.5) + (5.0-3.5) = 3.0
Sci-Fi: (4.5-3.5) + (5.0-3.5) = 2.5
Action: (4.5-3.5) = 1.0
Children: (5.0-3.5) = 1.5

Top 5 → preferredGenres: Animation|Sci-Fi|Children|Action (bitmask)
```

---

## 🔍 Fase 2: Geração de Recomendações

### 2.1 **Identificação de Candidatos Similares**

```cpp
// Para User 123 que viu filmes [1,2,3,7,9]

// PASSO 1: Conta filmes em comum
candidateCount = {}
Para filme 1: adiciona users [456,789,321,...] ao contador
Para filme 2: adiciona users [456,654,987,...] ao contador
...

// Resultado:
candidateCount = {
    456: 4,  // viu 4 filmes em comum
    789: 3,  // viu 3 filmes em comum
    321: 5,  // viu 5 filmes em comum
    654: 1,  // apenas 1 filme - DESCARTADO
}

// PASSO 2: Filtra (mínimo 5 filmes) e ordena (máximo 500)
candidates = [321, 456, 789, ...]  // Ordenado por filmes em comum
```

### 2.2 **Cálculo de Similaridade (Pearson)**

#### Por que Pearson ao invés de Cosseno?

```
Exemplo revelador:

User A (generoso): [5, 5, 5, 5, 5]  média=5.0
User B (crítico):  [3, 3, 3, 3, 3]  média=3.0
User C (seletivo): [1, 2, 3, 4, 5]  média=3.0

Cosseno(A,B) = 1.0  ✗ (máxima similaridade - ERRADO!)
Pearson(A,B) = undefined (sem variância - CORRETO!)

Cosseno(B,C) = 0.97 ✗ (alta similaridade - ERRADO!)
Pearson(B,C) = 0.0  ✓ (sem correlação - CORRETO!)
```

#### Cálculo Detalhado:

```cpp
User 123 vs User 321 (5 filmes em comum):

User 123: [5.0, 4.5, 5.0, 3.0, 4.0]
User 321: [4.0, 5.0, 5.0, 2.0, 3.0]

// Implementação otimizada (evita calcular médias)
n = 5
sum1 = 21.5,     sum2 = 19.0
sum1Sq = 95.25,  sum2Sq = 79.0
pSum = 83.5      // (5×4 + 4.5×5 + 5×5 + 3×2 + 4×3)

numerator = n×pSum - sum1×sum2 = 5×83.5 - 21.5×19.0 = 9.0
denominator = √[(n×sum1Sq - sum1²)(n×sum2Sq - sum2²)] = 21.82

similarity = 9.0 / 21.82 = 0.412
```

### 2.3 **Agregação de Scores (Collaborative Filtering)**

Para cada filme NÃO visto pelo usuário:

```cpp
// User 123 não viu "Inception"
// Seus similares que viram:

User 456 (sim=0.82): deu 5.0, sua média=3.5
User 789 (sim=0.75): deu 4.5, sua média=4.0  
User 321 (sim=0.68): deu 4.0, sua média=3.0

// Cálculo do score:
contribuições = {
    456: 0.82 × (5.0 - 3.5) = 1.23   // Adorou! (+1.5 acima da média)
    789: 0.75 × (4.5 - 4.0) = 0.375  // Gostou um pouco (+0.5)
    321: 0.68 × (4.0 - 3.0) = 0.68   // Gostou (+1.0)
}

score = Σ(contribuições) / Σ(similaridades) + baseline
      = (1.23 + 0.375 + 0.68) / (0.82 + 0.75 + 0.68) + 4.3
      = 2.285 / 2.25 + 4.3
      = 5.316
```

#### Processo Completo:

```python
scores = {}

Para cada usuário similar:
    Para cada filme que ele viu:
        Se User 123 NÃO viu:
            scores[filme] += sim × (rating - média_dele)

Para cada filme em scores:
    scores[filme] = scores[filme] / Σ(sim) + média_geral_filme
```

### 2.4 **Content-Based Boost**

Adiciona pontos para filmes com gêneros preferidos:

```cpp
User 123 prefere: Animation, Sci-Fi, Comedy

Para "WALL-E" (Animation|Sci-Fi):
1. Matches: 2 gêneros ✓
2. Qualidade: rating médio = 4.2/5.0 = 0.84
3. Popularidade: log(25000)/10 = 1.0 (capped)
4. Boost = 0.5×0.84 + 0.5×1.0 = 0.92
5. Score += 0.92 × 0.3 (peso CB) = 0.276

Resultado:
- Antes (só CF): WALL-E = 3.93
- Depois (CF+CB): WALL-E = 4.206
```

### 2.5 **Popularity Fallback**

Quando CF+CB geram menos de 20 recomendações:

```cpp
// Temos apenas 15 recomendações, precisamos de mais 5

filmes_populares = [
    (Shawshank, 80000×4.8 = 384000),
    (Godfather, 75000×4.7 = 352500),
    (Dark Knight, 85000×4.5 = 382500),
    ...
]

// Normaliza e adiciona os top 5 não vistos
Para cada filme popular não visto:
    score = (popularidade × rating) / 100
```

---

## ⚡ Análise de Complexidade e Performance

### Complexidade Temporal

```
Carregamento:
- Ratings: O(R) onde R = 20M ratings
- Filmes: O(M × G) onde M = 60k filmes, G = gêneros/filme

Por usuário:
- Encontrar candidatos: O(F × U_f) onde F = filmes do user, U_f = users/filme
- Calcular Pearson: O(C × I) onde C = candidatos (≤500), I = itens comuns
- CF scores: O(S × R_s) onde S = similares (≤50), R_s = ratings deles
- CB boost: O(G × M_g) onde G = gêneros preferidos, M_g = filmes/gênero
- Ordenação: O(N log N) onde N = filmes candidatos

Total por usuário: ~O(50×200 + 1000×log(1000)) ≈ O(20k)
```

### Otimizações Implementadas

1. **Estruturas Ordenadas**: Merge O(n) ao invés de nested loops O(n²)
2. **Bitmasks**: Comparação de gêneros em O(1)
3. **Cache de Similaridades**: Evita recálculos
4. **Paralelização**: Batches de 100 cálculos simultâneos
5. **Limites Inteligentes**: Máx 500 candidatos, 50 similares

---

## ✅ Validação e Qualidade

### Por que Confiar nas Recomendações?

1. **Pearson é Robusto**:
   - Remove viés individual (normalização)
   - Captura correlações verdadeiras
   - Resistente a diferentes escalas de rating

2. **Filtragem de Ruído**:
   - MIN_COMMON_ITEMS = 5 (evita coincidências)
   - MIN_SIMILARITY = 0.1 (remove correlações fracas)
   - MIN_RATING = 3.5 (apenas preferências positivas)

3. **Sistema Híbrido**:
   - CF falha → CB compensa
   - CB falha → Popularity garante
   - Sempre retorna 20 recomendações de qualidade

### Métricas Esperadas

- **RMSE**: ~0.85 (típico para MovieLens)
- **Precision@20**: ~0.30 (30% relevância)
- **Coverage**: ~80% dos filmes
- **Tempo médio**: 200-800ms por usuário

---

## 🎬 Exemplos Práticos Completos

### Exemplo 1: Usuário Típico

```
User 123 - Perfil:
- 45 filmes avaliados
- Gosta de Sci-Fi cerebral e animações de qualidade
- Média: 3.8

Processo:
1. Encontrados 127 candidatos similares
2. Top 50 similares (Pearson 0.3-0.85)
3. CF gerou scores para 89 filmes
4. CB adicionou boost para 23 filmes Sci-Fi/Animation
5. Não precisou de popularity fallback

Top 5 Recomendações:
1. Inception (5.32) - CF forte + CB boost
2. Ex Machina (4.95) - CF forte
3. Inside Out (4.87) - CF + CB Animation
4. Arrival (4.82) - CF forte
5. WALL-E (4.75) - CF médio + CB forte
```

### Exemplo 2: Usuário Novo (Cold Start)

```
User 999 - Perfil:
- Apenas 3 filmes avaliados
- Star Wars 5.0, Avatar 4.0, Titanic 2.0
- Preferências: Sci-Fi, Adventure

Processo:
1. Apenas 8 candidatos (poucos filmes)
2. 3 similares fracos (Pearson 0.15-0.25)
3. CF gerou apenas 5 recomendações
4. CB adicionou 8 filmes Sci-Fi/Adventure
5. Popularity completou com 7 filmes

Recomendações balanceadas:
- 25% CF (personalizado mas limitado)
- 40% CB (baseado em gêneros)
- 35% Popularity (qualidade garantida)
```

### Exemplo 3: Usuário "Diferentão"

```
User 777 - Perfil:
- 200 filmes avaliados
- Gosta de documentários obscuros e horror B
- Pouquíssimos usuários similares

Processo:
1. 2000+ candidatos mas poucos similares
2. Apenas 5 com Pearson > 0.3
3. CF limitado, CB encontrou nicho horror
4. Popularity necessária para completar

Mix final personalizado para gostos únicos!
```

---

## 🚀 Conclusão

Este sistema implementa o estado da arte em recomendação, combinando:
- **Matemática sólida** (Pearson correlation)
- **Engenharia eficiente** (estruturas otimizadas)
- **Robustez prática** (sistema híbrido)

O resultado é um sistema que gera recomendações personalizadas de alta qualidade em menos de 1 segundo por usuário, escalável para milhões de usuários e itens!