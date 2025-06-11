# Sistema de Recomendação MovieLens - Estrutura Modular

## 📁 Estrutura de Arquivos

```
src/
├── Config.h                    # Configurações globais do sistema
├── DataStructures.h            # Estruturas de dados (Movie, UserProfile, etc)
├── DataLoader.h/cpp            # Carregamento de arquivos e pré-processamento
├── SimilarityCalculator.h/cpp  # Cálculos de similaridade (Pearson/Cosseno)
├── RecommendationEngine.h/cpp  # Motor de recomendação (CF + CB)
├── FastRecommendationSystem.h/cpp # Sistema principal que integra tudo
└── Main.cpp                    # Ponto de entrada
```

## 🔧 Descrição dos Módulos

### **Config.h**
Centraliza todas as configurações:
- Parâmetros de recomendação (TOP_K, MIN_COMMON_ITEMS)
- Configurações de performance (NUM_THREADS, BATCH_SIZE)
- Caminhos dos arquivos de entrada

### **DataStructures.h**
Define as estruturas principais:
- `Movie`: Título, gêneros, bitmask
- `UserProfile`: Ratings, média, gêneros preferidos
- `Recommendation`: Par (movieId, score)

### **DataLoader**
Responsável por:
- Carregar ratings de `datasets/input.dat`
- Carregar filmes de `ml-25m/movies.dat`
- Calcular estatísticas (médias, popularidade)
- Construir índices invertidos
- Calcular preferências de gêneros dos usuários

### **SimilarityCalculator**
Implementa:
- Correlação de Pearson (método principal)
- Similaridade de Cosseno (alternativa)
- Cache thread-safe de similaridades

### **RecommendationEngine**
Core do sistema:
- `findCandidateUsers()`: Encontra usuários similares
- `calculateSimilarities()`: Calcula Pearson em paralelo
- `collaborativeFiltering()`: Agrega ratings dos similares
- `contentBasedBoost()`: Adiciona boost por gêneros
- `popularityFallback()`: Filmes populares para cold start

### **FastRecommendationSystem**
Classe principal que:
- Gerencia todos os componentes
- Coordena o fluxo de dados
- Fornece interface simples

## 🚀 Compilação

```bash
# Compilar versão otimizada
make clean && make

# Compilar versão debug
make debug

# Executar
./bin/recommender
```

## 📊 Fluxo de Execução

1. **Main.cpp**: Cria `FastRecommendationSystem`
2. **DataLoader**: Carrega ratings e filmes
3. **Para cada usuário**:
   - RecommendationEngine encontra candidatos
   - SimilarityCalculator calcula Pearson
   - Engine gera scores (CF + CB + Popularity)
   - Sistema retorna top-K recomendações

## ⚙️ Customização

### Mudar algoritmo de similaridade:
Em `RecommendationEngine.cpp`, linha ~130:
```cpp
// Trocar de:
float sim = similarityCalc.calculatePearsonCorrelation(userId, candidateId);
// Para:
float sim = similarityCalc.calculateCosineSimilarity(userId, candidateId);
```

### Ajustar pesos do sistema híbrido:
Em `Config.h`:
```cpp
const float CF_WEIGHT = 0.7f;  // Aumentar para mais collaborative
const float CB_WEIGHT = 0.3f;  // Aumentar para mais content-based
```

### Modificar paralelização:
```cpp
const int BATCH_SIZE = 100;     // Reduzir se tiver problemas de memória
const int NUM_THREADS = 8;      // Fixar número de threads
```

## 💡 Vantagens da Modularização

1. **Manutenção**: Cada módulo tem responsabilidade única
2. **Testabilidade**: Componentes podem ser testados isoladamente
3. **Extensibilidade**: Fácil adicionar novos algoritmos
4. **Reusabilidade**: Componentes podem ser usados em outros projetos
5. **Compilação**: Apenas módulos modificados são recompilados