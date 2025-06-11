#include <iostream>
#include <chrono>
#include <exception>
#include "FastRecommendationSystem.hpp"
#include "Config.hpp"

using namespace std;
using namespace chrono;

int main()
{
    cout << "=== Sistema de Recomendação Rápido MovieLens ===" << endl;
    cout << "Threads: " << Config::NUM_THREADS << endl;

    auto totalStart = high_resolution_clock::now();

    try
    {
        FastRecommendationSystem system;

        // Carrega dados
        system.loadData();

        // Processa recomendações
        system.processRecommendations(Config::USERS_FILE);

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