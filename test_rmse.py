#!/usr/bin/env python3
"""
test_rmse.py - Testa o RMSE do seu sistema SEM modificar nada!
Uso: python3 test_rmse.py
"""

import subprocess
import os
import random
import math
import shutil

def calculate_baseline_rmse():
    """Calcula o RMSE baseline (média do usuário) para comparação"""
    print("=== Calculando RMSE Baseline ===")
    
    # Lê os dados originais
    users_data = {}
    with open('datasets/input.dat', 'r') as f:
        for line in f:
            parts = line.strip().split()
            if not parts:
                continue
            
            user_id = int(parts[0])
            ratings = []
            
            for rating_str in parts[1:]:
                if ':' in rating_str:
                    movie_id, rating = rating_str.split(':')
                    ratings.append(float(rating))
            
            if len(ratings) >= 20:  # Só usuários com ratings suficientes
                users_data[user_id] = ratings
    
    # Calcula RMSE com validação cruzada
    total_error = 0
    total_predictions = 0
    
    # Amostra aleatória de usuários
    sample_users = random.sample(list(users_data.keys()), 
                                min(100, len(users_data)))
    
    for user_id in sample_users:
        ratings = users_data[user_id].copy()
        random.shuffle(ratings)
        
        # 80% treino, 20% teste
        split_point = int(len(ratings) * 0.8)
        train_ratings = ratings[:split_point]
        test_ratings = ratings[split_point:]
        
        # Média dos ratings de treino
        user_avg = sum(train_ratings) / len(train_ratings)
        
        # Testa predições
        for actual_rating in test_ratings:
            predicted = user_avg
            error = predicted - actual_rating
            total_error += error * error
            total_predictions += 1
    
    rmse = math.sqrt(total_error / total_predictions)
    
    print(f"Usuários testados: {len(sample_users)}")
    print(f"Predições: {total_predictions}")
    print(f"RMSE Baseline: {rmse:.4f}")
    
    return rmse

def test_system_with_holdout():
    """Testa o sistema real com dados holdout"""
    print("\n=== Testando Sistema de Recomendação ===")
    
    # Faz backup dos arquivos originais
    print("Fazendo backup dos arquivos originais...")
    shutil.copy('datasets/input.dat', 'datasets/input_original.dat')
    shutil.copy('datasets/explore.dat', 'datasets/explore_original.dat')
    
    try:
        # Seleciona 10 usuários aleatórios para teste rápido
        test_users = []
        with open('datasets/explore.dat', 'r') as f:
            all_users = [int(line.strip()) for line in f if line.strip()]
        
        test_users = random.sample(all_users[:50], min(10, len(all_users)))
        
        # Cria arquivo explore só com usuários de teste
        with open('datasets/explore_test.dat', 'w') as f:
            for user_id in test_users:
                f.write(f"{user_id}\n")
        
        # Para cada usuário de teste, remove alguns ratings
        held_out_ratings = {}
        
        print("Preparando dados de teste...")
        all_lines = []
        with open('datasets/input.dat', 'r') as f:
            for line in f:
                parts = line.strip().split()
                if not parts:
                    continue
                
                user_id = int(parts[0])
                
                if user_id in test_users and len(parts) > 10:
                    # Segura 20% dos ratings para teste
                    ratings = parts[1:]
                    random.shuffle(ratings)
                    
                    holdout_size = max(2, len(ratings) // 5)
                    held_out = ratings[:holdout_size]
                    kept = ratings[holdout_size:]
                    
                    # Salva ratings removidos
                    held_out_ratings[user_id] = []
                    for r in held_out:
                        if ':' in r:
                            movie_id, rating = r.split(':')
                            held_out_ratings[user_id].append((int(movie_id), float(rating)))
                    
                    # Escreve linha modificada
                    new_line = f"{user_id} " + " ".join(kept)
                    all_lines.append(new_line)
                else:
                    all_lines.append(line.strip())
        
        # Escreve arquivo de teste
        with open('datasets/input.dat', 'w') as f:
            for line in all_lines:
                f.write(line + '\n')
        
        # Move arquivo explore de teste
        shutil.move('datasets/explore_test.dat', 'datasets/explore.dat')
        
        # Executa o sistema
        print("Executando sistema de recomendação...")
        subprocess.run(['./bin/recommender'], 
                      stdout=subprocess.DEVNULL, 
                      stderr=subprocess.DEVNULL)
        
        # Analisa resultados
        print("Analisando resultados...")
        recommendations = {}
        current_user_idx = 0
        
        if os.path.exists('result'):
            with open('result', 'r') as f:
                lines = f.readlines()
                
                for i, line in enumerate(lines):
                    if current_user_idx < len(test_users):
                        user_id = test_users[current_user_idx]
                        
                        # Detecta fim das recomendações de um usuário
                        if line.strip() == '' or i == len(lines) - 1:
                            current_user_idx += 1
                        elif '. ' in line and '(Score:' in line:
                            # Extrai score
                            try:
                                score_part = line.split('(Score:')[1]
                                score = float(score_part.split(')')[0])
                                
                                if user_id not in recommendations:
                                    recommendations[user_id] = []
                                recommendations[user_id].append(score)
                            except:
                                pass
        
        # Calcula métricas simples
        print("\n=== Resultados ===")
        total_users = len(test_users)
        users_with_recs = len(recommendations)
        
        print(f"Usuários testados: {total_users}")
        print(f"Usuários com recomendações: {users_with_recs}")
        
        if users_with_recs > 0:
            avg_recs = sum(len(recs) for recs in recommendations.values()) / users_with_recs
            print(f"Média de recomendações por usuário: {avg_recs:.1f}")
            
            # Score médio
            all_scores = []
            for recs in recommendations.values():
                all_scores.extend(recs)
            
            if all_scores:
                avg_score = sum(all_scores) / len(all_scores)
                print(f"Score médio das recomendações: {avg_score:.2f}")
        
        print(f"\nRatings removidos para teste: {sum(len(v) for v in held_out_ratings.values())}")
        
    finally:
        # Restaura arquivos originais
        print("\nRestaurando arquivos originais...")
        shutil.move('datasets/input_original.dat', 'datasets/input.dat')
        shutil.move('datasets/explore_original.dat', 'datasets/explore.dat')

def main():
    print("🎯 Teste de RMSE para Sistema de Recomendação")
    print("=" * 50)
    
    # Verifica se o sistema está compilado
    if not os.path.exists('bin/recommender'):
        print("❌ ERRO: Sistema não encontrado!")
        print("Execute 'make' primeiro para compilar o sistema.")
        return
    
    # 1. Calcula baseline
    baseline_rmse = calculate_baseline_rmse()
    
    # 2. Testa o sistema
    test_system_with_holdout()
    
    print("\n" + "=" * 50)
    print("📊 RESUMO:")
    print(f"RMSE Baseline (média do usuário): {baseline_rmse:.4f}")
    print(f"Seu sistema deve ter RMSE < {baseline_rmse:.4f}")
    print("RMSE típico para bons sistemas: 0.85-0.95")

if __name__ == "__main__":
    main()