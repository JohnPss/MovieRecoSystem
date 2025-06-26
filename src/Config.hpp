#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <thread>
#include <cstdint> // Necessário para uint32_t

// Configurações do sistema de recomendação
namespace Config
{
    // Parâmetros de recomendação
    const int TOP_K = 5;               // Número de recomendações a retornar
    const int MAX_SIMILAR_USERS = 500; // Máximo de usuários similares a considerar
    const int MIN_COMMON_ITEMS = 5;    // Mínimo de filmes em comum para calcular similaridade
    const float MIN_RATING = 3.5f;     // Rating mínimo para considerar positivo
    const float MIN_SIMILARITY = 0.1f; // Similaridade mínima para considerar usuário

    // --- Parâmetros LSH ---
    const int NUM_HASH_FUNCTIONS = 128; // Tamanho da Assinatura
    const int NUM_BANDS = 32;           // Número de Provas por Gincana
    const int ROWS_PER_BAND = 4;        // Rigidez de cada Prova
    const int NUM_TABLES = 10;          // Número de Gincanas
    const uint32_t LARGE_PRIME = 4294967291u;

    extern bool USE_LSH;

    // Parâmetros de performance
    const int NUM_THREADS = std::thread::hardware_concurrency();
    const int BATCH_SIZE = 100;     // Tamanho do batch para processamento paralelo
    const int MAX_CANDIDATES = 500; // Máximo de candidatos a processar

    // Pesos do sistema híbrido
    const float CF_WEIGHT = 0.7f; // Peso do collaborative filtering
    const float CB_WEIGHT = 0.3f; // Peso do content-based filtering

    // // Arquivos de entrada
    // const char *RATINGS_FILE = "datasets/input.dat";
    // const char *MOVIES_FILE = "ml-25m/movies.csv";
    // const char *USERS_FILE = "datasets/explore.dat";

    inline static const std::string USERS_FILE = "datasets/explore.dat";
    inline static const std::string MOVIES_FILE = "ml-25m/movies.csv";
    inline static const std::string RATINGS_FILE = "datasets/input.dat";
}

#endif // CONFIG_H

/*
 ==================================================================================
 GUIA DE AFINAÇÃO E EXPLICAÇÃO DOS PARÂMETROS
 ----------------------------------------------------------------------------------
 Estes valores controlam o equilíbrio entre a velocidade da busca por vizinhos,
 o uso de memória e a qualidade (precisão vs. abrangência) dos candidatos encontrados.
 Analogia: Pense nisto como uma "gincana de afinidade" para encontrar utilizadores
 com gostos semelhantes de forma rápida.
 ==================================================================================

 * @param NUM_HASH_FUNCTIONS (Número de Funções de Hash)
 * Analogia: O número de "perguntas" no "Mega-Quiz de Gosto" de cada utilizador.
 * - MAIS FUNÇÕES: Assinatura mais detalhada e precisa, mas mais lenta de construir e usa mais memória.
 * - MENOS FUNÇÕES: Assinatura mais "grosseira", mais rápida, mas pode não distinguir bem os utilizadores.

 * @param NUM_BANDS (Número de Bandas)
 * Analogia: O número de "provas" em cada gincana. (b)
 * Aumentar 'b' (e diminuir 'r') aumenta a probabilidade de encontrar vizinhos semelhantes
 * (reduz os falsos negativos).

 * @param ROWS_PER_BAND (Linhas por Banda)
 * Analogia: O número de "perguntas" em cada prova da gincana. (r)
 * A regra é: NUM_BANDS * ROWS_PER_BAND deve ser igual a NUM_HASH_FUNCTIONS.
 * - VALOR ALTO: Torna cada prova mais rigorosa (ex: 8 de 8 perguntas têm de ser iguais). Isto reduz
 * candidatos irrelevantes (falsos positivos), mas pode perder vizinhos moderadamente semelhantes.
 * - VALOR BAIXO: Torna a prova mais fácil, resultando em mais candidatos e menos risco de perder vizinhos.

 * @param NUM_TABLES (Número de Tabelas)
 * Analogia: O número de "gincanas" independentes que realizamos.
 * Funciona como uma rede de segurança para garantir que vizinhos semelhantes colidam.
 * - MAIS TABELAS: Aumenta muito a probabilidade de encontrar todos os vizinhos relevantes (recall),
 * mas aumenta linearmente o uso de memória e o tempo de indexação.
 * - MENOS TABELAS: Indexação muito mais rápida e leve, mas com maior risco de não encontrar um vizinho.

 * @param LARGE_PRIME (Número Primo Grande)
 * Um número primo grande usado nos cálculos de hash para garantir uma boa distribuição.
 * Deve ser maior que o número máximo de IDs de filmes.

*/