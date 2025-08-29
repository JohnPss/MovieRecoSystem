#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


// Namespace para agrupar todas as configurações do sistema de recomendação.
namespace Config
{
   // --- Parâmetros de Recomendação ---
   const int TOP_K = 5;                // Número de filmes a serem recomendados para cada usuário.
   const int MAX_SIMILAR_USERS = 500;  // Número máximo de usuários similares a serem considerados no cálculo.
   const int MIN_COMMON_ITEMS = 1;     // Número mínimo de itens avaliados em comum para que dois usuários sejam considerados similares.
   const float MIN_RATING = 3.5f;      // Nota mínima (rating) para que uma avaliação seja considerada positiva.
   const float MIN_SIMILARITY = 0.01f; // Limiar mínimo de similaridade para que um usuário seja considerado no cálculo.
   const int MAX_CANDIDATES = 1000;    // Número máximo de filmes candidatos a serem considerados antes do ranqueamento final.

   // --- Parâmetros para o Locality-Sensitive Hashing (LSH) ---
   const int NUM_HASH_FUNCTIONS = 96;        // Número total de funções de hash a serem utilizadas no MinHashing.
   const int NUM_BANDS = 24;                 // Número de bandas para o LSH. Aumentar este valor aumenta a chance de encontrar candidatos similares.
   const int ROWS_PER_BAND = 4;              // Número de linhas (hashes) por banda. Calculado como `NUM_HASH_FUNCTIONS / NUM_BANDS`.
   const int NUM_TABLES = 8;                 // Número de tabelas de hash. Um balanço entre performance e a qualidade (recall) dos resultados.
   const uint32_t LARGE_PRIME = 4294967291u; // Um número primo grande usado nos cálculos das funções de hash.

   // --- Parâmetros de Desempenho e Concorrência ---
   const int NUM_THREADS = std::thread::hardware_concurrency() - 2; // Número de threads para processamento paralelo. Deixa 2 núcleos livres para o sistema.
   const int BATCH_SIZE = 100;                                      // Tamanho do lote de usuários a ser processado por cada thread.

   // --- Pesos para o Sistema Híbrido ---
   const float CF_WEIGHT = 1.0f;         // Peso para o score do Filtro Colaborativo (Collaborative Filtering).
   const float CB_WEIGHT = 1.0f;         // Peso para o score do Filtro Baseado em Conteúdo (Content-Based).
   const float POPULARITY_WEIGHT = 1.5f; // Peso para o score de popularidade, usado para desempate e fallback.

   // --- Configurações de Fallback ---
   // Estratégias para quando o algoritmo principal não encontra candidatos suficientes.
   const int MIN_CANDIDATES_FOR_CF = 50;        // Limiar mínimo de candidatos para aplicar o filtro colaborativo; abaixo disso, aciona o fallback.
   const int EMERGENCY_FALLBACK_THRESHOLD = 10; // Limiar crítico de candidatos que aciona o modo de emergência (baseado em popularidade).
   const float POPULARITY_BOOST_WEIGHT = 1.5f;  // Fator de reforço aplicado à popularidade dos itens durante o fallback.

   // --- Caminhos dos Arquivos ---
   inline static const std::string USERS_FILE = "datasets/explore.dat"; // Arquivo com os usuários para os quais as recomendações serão geradas.
   inline static const std::string MOVIES_FILE = "ml-25m/movies.csv";   // Arquivo com os metadados dos filmes.
   inline static const std::string RATINGS_FILE = "datasets/input.dat"; // Arquivo com o histórico de avaliações dos usuários.
   inline static const std::string OUTPUT_FILE = "outcome/output.dat";  // Arquivo de saída para salvar as recomendações geradas.
}

#endif 
