#include <iostream>
#include <chrono>
#include <exception>
#include <string>
#include <vector>
#include <iomanip> // Para formatar a saída do tempo

#include "FastRecommendationSystem.hpp"
#include "Config.hpp"
#include "preProcessamento.hpp"

using namespace std;
using namespace chrono;

// Definição da variável global de configuração
bool Config::USE_LSH = true;

// Função para imprimir ajuda
void printHelp()
{
    cout << "\nOpções de linha de comando:" << endl;
    cout << "  --no-lsh       : Força o uso do método de busca por força bruta." << endl;
    cout << "  --benchmark    : Executa ambos os métodos (LSH e força bruta) para comparação." << endl;
    cout << "  --help         : Mostra esta mensagem de ajuda." << endl;
}

int main(int argc, char *argv[])
{
    // --- Início da cronometragem total ---
    auto totalStart = high_resolution_clock::now();
    auto etapaStart = totalStart; // Inicializa o cronômetro de etapas

    // --- Variáveis para armazenar os tempos de cada etapa (em milissegundos) ---
    milliseconds duracao_pre_processamento(0);
    milliseconds duracao_carregamento_dados(0);
    milliseconds duracao_recomendacao_lsh(0);
    milliseconds duracao_recomendacao_bf(0);
    milliseconds duracao_recomendacao_normal(0);

    // --- Processamento de Argumentos de Linha de Comando ---
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

    try
    {
        // --- ETAPA 1: Pré-processamento do arquivo de avaliações ---
        cout << "\nIniciando pré-processamento..." << endl;
        etapaStart = high_resolution_clock::now();

        int result = process_ratings_file(); // Função de pré-processamento
        if (result != 0)
        {
            cerr << "Falha no pré-processamento. Encerrando." << endl;
            return 1;
        }

        duracao_pre_processamento = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);
        cout << "Pré-processamento concluído." << endl;
        // -----------------------------------------------------------

        FastRecommendationSystem system;

        // --- ETAPA 2: Carregamento de dados e construção de estruturas ---
        cout << "\nIniciando carregamento de dados..." << endl;
        Config::USE_LSH = useLSHDefault;
        if (runBenchmark)
            Config::USE_LSH = true; // No modo benchmark, sempre construímos o índice LSH

        etapaStart = high_resolution_clock::now();
        system.loadData();
        duracao_carregamento_dados = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);
        cout << "Carregamento de dados concluído." << endl;
        // --------------------------------------------------------------------

        // --- ETAPA 3: Processamento das Recomendações ---
        if (runBenchmark)
        {
            cout << "\n--- INICIANDO MODO BENCHMARK ---" << endl;

            // 1. Executa com LSH
            cout << "\n[Benchmark] Executando com LSH..." << endl;
            Config::USE_LSH = true;
            etapaStart = high_resolution_clock::now();
            system.processRecommendations(Config::USERS_FILE);
            duracao_recomendacao_lsh = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);
            cout << "[Benchmark] Execução com LSH concluída." << endl;

            // 2. Executa com Força Bruta
            cout << "\n[Benchmark] Executando com Força Bruta (sem LSH)..." << endl;
            Config::USE_LSH = false;
            etapaStart = high_resolution_clock::now();
            system.processRecommendations(Config::USERS_FILE);
            duracao_recomendacao_bf = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);
            cout << "[Benchmark] Execução com Força Bruta concluída." << endl;

            cout << "\n--- BENCHMARK CONCLUÍDO ---" << endl;
        }
        else
        {
            // Execução normal
            string mode = useLSHDefault ? "LSH (Otimizado)" : "Força Bruta";
            cout << "\nModo de Operação: " << mode << endl;
            cout << "Iniciando processamento de recomendações..." << endl;
            Config::USE_LSH = useLSHDefault;

            etapaStart = high_resolution_clock::now();
            system.processRecommendations(Config::USERS_FILE);
            duracao_recomendacao_normal = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);
            cout << "Processamento de recomendações concluído." << endl;
        }
        // ----------------------------------------------------
    }
    catch (const exception &e)
    {
        cerr << "Erro fatal: " << e.what() << endl;
        return 1;
    }

    // --- Fim da cronometragem total ---
    auto totalEnd = high_resolution_clock::now();
    auto totalDuration = duration_cast<milliseconds>(totalEnd - totalStart);

    // --- RELATÓRIO DE DESEMPENHO ---
    cout << "\n=============================================" << endl;
    cout << "        RELATORIO DE DESEMPENHO" << endl;
    cout << "=============================================" << endl;
    cout << fixed << setprecision(3); // Configura a saída para 3 casas decimais

    cout << "1. Pre-processamento      : " << duracao_pre_processamento.count() / 1000.0f << " segundos" << endl;
    cout << "2. Carregamento de Dados  : " << duracao_carregamento_dados.count() / 1000.0f << " segundos" << endl;

    if (runBenchmark)
    {
        cout << "3. Recomendações (LSH)    : " << duracao_recomendacao_lsh.count() / 1000.0f << " segundos" << endl;
        cout << "4. Recomendações (Forca B): " << duracao_recomendacao_bf.count() / 1000.0f << " segundos" << endl;
    }
    else
    {
        cout << "3. Recomendações          : " << duracao_recomendacao_normal.count() / 1000.0f << " segundos" << endl;
    }

    cout << "---------------------------------------------" << endl;
    cout << "TEMPO TOTAL DE EXECUCAO   : " << totalDuration.count() / 1000.0f << " segundos" << endl;
    cout << "=============================================" << endl;

    return 0;
}