
#include "Config.hpp"

#include "FastRecommendationSystem.hpp"
#include "preProcessament.hpp"

using namespace std;
using namespace chrono;

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    auto totalStart = high_resolution_clock::now();
    milliseconds duracao_pre_processamento(0), duracao_carregamento_dados(0),
        duracao_recomendacao(0);

    try
    {
        auto etapaStart = high_resolution_clock::now();
        if (process_ratings_file() != 0)
        {
            return 1;
        }
        duracao_pre_processamento = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);

        FastRecommendationSystem system;
        etapaStart = high_resolution_clock::now();
        system.loadData();
        duracao_carregamento_dados = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);

        etapaStart = high_resolution_clock::now();
        system.processRecommendations(Config::USERS_FILE);
        duracao_recomendacao = duration_cast<milliseconds>(high_resolution_clock::now() - etapaStart);
    }
    catch (const exception &e)
    {
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