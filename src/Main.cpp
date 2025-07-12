#include <iostream>
#include <chrono>
#include <exception>
#include <string>
#include <vector>
#include <iomanip>

#include "FastRecommendationSystem.hpp"
#include "Config.hpp"
#include "preProcessament.hpp"

using namespace std;
using namespace chrono;

int main(int argc, char *argv[])
{
    // Argumentos não são mais necessários, mas mantemos a assinatura padrão do main.
    (void)argc;
    (void)argv;

    auto totalStart = high_resolution_clock::now();
    milliseconds duracao_pre_processamento(0), duracao_carregamento_dados(0),
        duracao_recomendacao(0);

    cout << "=== Sistema de Recomendação Rápido MovieLens ===" << endl;
    cout << "Modo de Operação: LSH (Otimizado)" << endl;
    cout << "Threads: " << Config::NUM_THREADS << endl;

    try
    {
        // --- ETAPA 1: Pré-processamento do arquivo de avaliações ---
        cout << "\nIniciando pré-processamento..." << endl;
        auto etapaStart = high_resolution_clock::now();
        if (process_ratings_file() != 0)
        {
            cerr << "Falha no pré-processamento. Encerrando." << endl;
            return 1;
        }
        duracao_pre_processamento = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);
        cout << "Pré-processamento concluído." << endl;

        // --- ETAPA 2: Carregamento de dados e construção de estruturas ---
        FastRecommendationSystem system;
        cout << "\nIniciando carregamento de dados e construção do índice LSH..." << endl;
        etapaStart = high_resolution_clock::now();
        system.loadData();
        duracao_carregamento_dados = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);
        cout << "Carregamento de dados concluído." << endl;

        // --- ETAPA 3: Processamento das Recomendações ---
        cout << "\nIniciando processamento de recomendações..." << endl;
        etapaStart = high_resolution_clock::now();
        system.processRecommendations(Config::USERS_FILE);
        duracao_recomendacao = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);
        cout << "Processamento de recomendações concluído." << endl;
    }
    catch (const exception &e)
    {
        cerr << "Erro fatal: " << e.what() << endl;
        return 1;
    }

    auto totalEnd = high_resolution_clock::now();
    auto totalDuration = duration_cast<milliseconds>(totalEnd - totalStart);

    // --- RELATÓRIO DE DESEMPENHO ---
    cout << "\n=============================================" << endl;
    cout << "        RELATORIO DE DESEMPENHO" << endl;
    cout << "=============================================" << endl;
    cout << fixed << setprecision(3);
    cout << "1. Pre-processamento      : " << duracao_pre_processamento.count() / 1000.0f << " segundos" << endl;
    cout << "2. Carregamento de Dados  : " << duracao_carregamento_dados.count() / 1000.0f << " segundos" << endl;
    cout << "3. Recomendações          : " << duracao_recomendacao.count() / 1000.0f << " segundos" << endl;
    cout << "---------------------------------------------" << endl;
    cout << "TEMPO TOTAL DE EXECUCAO   : " << totalDuration.count() / 1000.0f << " segundos" << endl;
    cout << "=============================================" << endl;

    return 0;
}