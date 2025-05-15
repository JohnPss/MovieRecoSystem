#ifndef SIMILARIDADE_HPP
#define SIMILARIDADE_HPP

#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>

using namespace std;

// Distância Euclidiana
double distanciaEuclidiana(const vector<double> &a, const vector<double> &b)
{
    double soma = 0.0;
    for (size_t i = 0; i < a.size(); i++)
        soma += pow(a[i] - b[i], 2);
    return sqrt(soma);
}

// Similaridade do Cosseno
double similaridadeCosseno(const vector<double> &a, const vector<double> &b)
{
    double dot = 0.0, normA = 0.0, normB = 0.0;
    for (size_t i = 0; i < a.size(); i++)
    {
        dot += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    return dot / (sqrt(normA) * sqrt(normB));
}

// Similaridade de Jaccard (binário)
double similaridadeJaccard(const vector<int> &a, const vector<int> &b)
{
    int intersec = 0, uniao = 0;
    for (size_t i = 0; i < a.size(); i++)
    {
        if (a[i] == 1 || b[i] == 1)
            uniao++;
        if (a[i] == 1 && b[i] == 1)
            intersec++;
    }
    return (double)intersec / uniao;
}

// Distância de Manhattan
double distanciaManhattan(const vector<double> &a, const vector<double> &b)
{
    double soma = 0.0;
    for (size_t i = 0; i < a.size(); i++)
        soma += fabs(a[i] - b[i]);
    return soma;
}

// Distância de Chebyshev
double distanciaChebyshev(const vector<double> &a, const vector<double> &b)
{
    double max_diff = 0.0;
    for (size_t i = 0; i < a.size(); i++)
        max_diff = max(max_diff, fabs(a[i] - b[i]));
    return max_diff;
}

// Distância de Hamming
int distanciaHamming(const vector<int> &a, const vector<int> &b)
{
    int dist = 0;
    for (size_t i = 0; i < a.size(); i++)
        if (a[i] != b[i])
            dist++;
    return dist;
}

// Similaridade de Dice (binário)
double similaridadeDice(const vector<int> &a, const vector<int> &b)
{
    int intersec = 0, total = 0;
    for (size_t i = 0; i < a.size(); i++)
    {
        if (a[i] == 1 && b[i] == 1)
            intersec++;
        if (a[i] == 1 || b[i] == 1)
            total++;
    }
    return (2.0 * intersec) / (a.size() + b.size());
}

// Correlação de Pearson
double correlacaoPearson(const vector<double> &a, const vector<double> &b)
{
    double mediaA = accumulate(a.begin(), a.end(), 0.0) / a.size();
    double mediaB = accumulate(b.begin(), b.end(), 0.0) / b.size();
    double num = 0.0, denA = 0.0, denB = 0.0;
    for (size_t i = 0; i < a.size(); i++)
    {
        num += (a[i] - mediaA) * (b[i] - mediaB);
        denA += pow(a[i] - mediaA, 2);
        denB += pow(b[i] - mediaB, 2);
    }
    return num / (sqrt(denA) * sqrt(denB));
}

#endif
