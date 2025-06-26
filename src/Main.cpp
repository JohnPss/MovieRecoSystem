#include <iostream>
#include <chrono>
#include <exception>
#include <string> // NOVO: para processar argumentos
#include <vector> // NOVO: para processar argumentos
#include "FastRecommendationSystem.hpp"
#include "Config.hpp"
#include "preProcessamento.hpp"


using namespace std;
using namespace chrono;

// NOVO: Definição da variável global de configuração
bool Config::USE_LSH = true;

// NOVO: Função para imprimir ajuda
void printHelp()
{
    cout << "\nOpções de linha de comando:" << endl;
    cout << "  --no-lsh       : Força o uso do método antigo de busca (força bruta)." << endl;
    cout << "  --benchmark    : Executa ambos os métodos (LSH e força bruta) para comparação de tempo." << endl;
    cout << "  --help         : Mostra esta mensagem de ajuda." << endl;
}

int main(int argc, char *argv[])
{

    using namespace std::chrono;

    auto start = high_resolution_clock::now();

    int result = process_ratings_file();

    auto end = high_resolution_clock::now();
    duration<double> elapsed = end - start;

    std::cout << "Tempo de execução: " << elapsed.count() << " segundos\n";


    // --- NOVO: Processamento de Argumentos de Linha de Comando ---
    vector<string> args(argv + 1, argv + argc);
    bool runBenchmark = false;
    bool useLSHDefault = true;

    for (const auto &arg : args)
    {
        if (arg == "--no-lsh")
        {
            useLSHDefault = false;
        }
        else if (arg == "--benchmark")
        {
            runBenchmark = true;
        }
        else if (arg == "--help")
        {
            printHelp();
            return 0;
        }
    }
    // ----------------------------------------------------------------

    cout << "=== Sistema de Recomendação Rápido MovieLens ===" << endl;
    cout << "Threads: " << Config::NUM_THREADS << endl;

    auto totalStart = high_resolution_clock::now();

    try
    {
        FastRecommendationSystem system;

        // Carrega dados (e constrói o índice LSH se não estiver desabilitado)
        Config::USE_LSH = useLSHDefault; // Garante que a construção do índice respeite o --no-lsh
        if (runBenchmark)
            Config::USE_LSH = true; // No modo benchmark, sempre construímos o índice

        system.loadData();

        // --- MODIFICADO: Lógica de Execução ---
        if (runBenchmark)
        {
            cout << "\n--- INICIANDO MODO BENCHMARK ---" << endl;

            // 1. Executa com LSH
            cout << "\n[Benchmark] Executando com LSH..." << endl;
            Config::USE_LSH = true;
            system.processRecommendations(Config::USERS_FILE);

            // 2. Executa com Força Bruta
            cout << "\n[Benchmark] Executando com Força Bruta (sem LSH)..." << endl;
            Config::USE_LSH = false;
            system.processRecommendations(Config::USERS_FILE);

            cout << "\n--- BENCHMARK CONCLUÍDO ---" << endl;
        }
        else
        {
            // Execução normal
            Config::USE_LSH = useLSHDefault;
            string mode = Config::USE_LSH ? "LSH (Otimizado)" : "Força Bruta";
            cout << "\nModo de Operação: " << mode << endl;
            system.processRecommendations(Config::USERS_FILE);
        }
        // ------------------------------------------

        auto totalEnd = high_resolution_clock::now();
        auto totalDuration = duration_cast<milliseconds>(totalEnd - totalStart);
        cout << "\n=== TEMPO TOTAL: " << totalDuration.count() / 1000.0f << " segundos ===" << endl;
    }
    catch (const exception &e)
    {
        cerr << "Erro: " << e.what() << endl;
        return 1;
    }

    return 0;
}