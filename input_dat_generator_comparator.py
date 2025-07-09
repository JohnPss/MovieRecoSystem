import pandas as pd
import os
from collections import defaultdict

def preprocess_ratings(ratings_file):
    """
    Pré-processa o arquivo ratings.csv seguindo os critérios do PDF:
    - Usuários com pelo menos 50 avaliações
    - Filmes avaliados por pelo menos 50 usuários
    - Remove duplicados e inconsistentes
    """
    print("Lendo ratings.csv...")
    df = pd.read_csv(ratings_file)
    
    print(f"Dados originais: {len(df)} registros")
    
    # Remove duplicados
    df = df.drop_duplicates(subset=['userId', 'movieId'])
    print(f"Após remover duplicados: {len(df)} registros")
    
    # Remove registros inconsistentes (ratings fora do range esperado)
    df = df[(df['rating'] >= 0.5) & (df['rating'] <= 5.0)]
    print(f"Após remover inconsistentes: {len(df)} registros")
    
    # Conta avaliações por usuário
    user_counts = df['userId'].value_counts()
    valid_users = user_counts[user_counts >= 50].index
    print(f"Usuários com >= 50 avaliações: {len(valid_users)}")
    
    # Filtra por usuários válidos
    df = df[df['userId'].isin(valid_users)]
    print(f"Após filtrar usuários: {len(df)} registros")
    
    # Conta avaliações por filme
    movie_counts = df['movieId'].value_counts()
    valid_movies = movie_counts[movie_counts >= 50].index
    print(f"Filmes com >= 50 avaliações: {len(valid_movies)}")
    
    # Filtra por filmes válidos
    df = df[df['movieId'].isin(valid_movies)]
    print(f"Após filtrar filmes: {len(df)} registros")
    
    return df

def generate_input_dat(df, output_file):
    """
    Gera o arquivo input.dat no formato especificado:
    usuario_id item_id1:nota1 item_id2:nota2 item_id3:nota3 ...
    """
    print("Gerando input.dat...")
    
    # Agrupa por usuário - formato para arquivo
    user_ratings_file = defaultdict(list)
    # Formato para comparação
    user_ratings_dict = defaultdict(dict)
    
    for _, row in df.iterrows():
        user_id = int(row['userId'])
        movie_id = int(row['movieId'])
        rating = float(row['rating'])
        
        user_ratings_file[user_id].append(f"{movie_id}:{rating}")
        user_ratings_dict[user_id][movie_id] = rating
    
    # Escreve arquivo
    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    with open(output_file, 'w', encoding='utf-8') as f:
        for user_id in sorted(user_ratings_file.keys()):
            ratings_str = ' '.join(user_ratings_file[user_id])
            f.write(f"{user_id} {ratings_str}\n")
    
    print(f"Arquivo gerado: {output_file}")
    print(f"Total de usuários: {len(user_ratings_dict)}")
    
    return dict(user_ratings_dict)

def parse_input_dat(file_path):
    """
    Lê um arquivo input.dat e retorna um dicionário com os dados
    """
    user_data = {}
    
    if not os.path.exists(file_path):
        print(f"Arquivo não encontrado: {file_path}")
        return user_data
    
    with open(file_path, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
                
            try:
                parts = line.split(' ')
                user_id = int(parts[0])
                
                ratings = {}
                for rating_str in parts[1:]:
                    if ':' in rating_str:
                        movie_id, rating = rating_str.split(':')
                        ratings[int(movie_id)] = float(rating)
                
                user_data[user_id] = ratings
            except Exception as e:
                print(f"Erro na linha {line_num}: {e}")
                continue
    
    return user_data

def compare_input_files(generated_data, existing_data):
    """
    Compara dois arquivos input.dat e calcula a porcentagem de igualdade
    """
    print("\n=== COMPARAÇÃO DOS ARQUIVOS ===")
    
    # Usuários
    generated_users = set(generated_data.keys())
    existing_users = set(existing_data.keys())
    
    print(f"Usuários no arquivo gerado: {len(generated_users)}")
    print(f"Usuários no arquivo existente: {len(existing_users)}")
    
    common_users = generated_users & existing_users
    print(f"Usuários em comum: {len(common_users)}")
    
    if len(generated_users) > 0:
        user_similarity = len(common_users) / len(generated_users) * 100
        print(f"Similaridade de usuários: {user_similarity:.2f}%")
    else:
        user_similarity = 0
    
    # Filmes (considerando todos os filmes de todos os usuários)
    generated_movies = set()
    existing_movies = set()
    
    for user_ratings in generated_data.values():
        generated_movies.update(user_ratings.keys())
    
    for user_ratings in existing_data.values():
        existing_movies.update(user_ratings.keys())
    
    print(f"Filmes no arquivo gerado: {len(generated_movies)}")
    print(f"Filmes no arquivo existente: {len(existing_movies)}")
    
    common_movies = generated_movies & existing_movies
    print(f"Filmes em comum: {len(common_movies)}")
    
    if len(generated_movies) > 0:
        movie_similarity = len(common_movies) / len(generated_movies) * 100
        print(f"Similaridade de filmes: {movie_similarity:.2f}%")
    else:
        movie_similarity = 0
    
    # Comparação detalhada por usuário
    exact_matches = 0
    partial_matches = 0
    total_ratings_generated = 0
    total_ratings_matching = 0
    
    for user_id in common_users:
        gen_ratings = generated_data[user_id]
        exist_ratings = existing_data[user_id]
        
        total_ratings_generated += len(gen_ratings)
        
        # Compara as avaliações do usuário
        matching_ratings = 0
        for movie_id, rating in gen_ratings.items():
            if movie_id in exist_ratings and abs(exist_ratings[movie_id] - rating) < 0.001:
                matching_ratings += 1
        
        total_ratings_matching += matching_ratings
        
        if matching_ratings == len(gen_ratings) and len(gen_ratings) == len(exist_ratings):
            exact_matches += 1
        elif matching_ratings > 0:
            partial_matches += 1
    
    print(f"\nUsuários com correspondência exata: {exact_matches}")
    print(f"Usuários com correspondência parcial: {partial_matches}")
    
    if total_ratings_generated > 0:
        rating_accuracy = total_ratings_matching / total_ratings_generated * 100
        print(f"Precisão das avaliações: {rating_accuracy:.2f}%")
    else:
        rating_accuracy = 0
    
    # Cálculo da similaridade geral
    overall_similarity = (user_similarity + movie_similarity + rating_accuracy) / 3
    print(f"\n=== SIMILARIDADE GERAL: {overall_similarity:.2f}% ===")
    
    return {
        'user_similarity': user_similarity,
        'movie_similarity': movie_similarity,
        'rating_accuracy': rating_accuracy,
        'overall_similarity': overall_similarity
    }

def main():
    # Caminhos dos arquivos
    ratings_file = "ratings.csv"
    generated_file = "datasets/input.dat"
    existing_file = "meu/input.dat"
    
    try:
        # 1. Gerar input.dat a partir do ratings.csv
        print("=== GERANDO INPUT.DAT ===")
        df_processed = preprocess_ratings(ratings_file)
        generated_data = generate_input_dat(df_processed, generated_file)
        
        # 2. Ler o input.dat existente
        print(f"\n=== LENDO ARQUIVO EXISTENTE ===")
        existing_data = parse_input_dat(existing_file)
        print(f"Usuários no arquivo existente: {len(existing_data)}")
        
        # 3. Comparar os arquivos
        results = compare_input_files(generated_data, existing_data)
        
        print(f"\n=== RESUMO FINAL ===")
        print(f"Similaridade de usuários: {results['user_similarity']:.2f}%")
        print(f"Similaridade de filmes: {results['movie_similarity']:.2f}%")
        print(f"Precisão das avaliações: {results['rating_accuracy']:.2f}%")
        print(f"SIMILARIDADE GERAL: {results['overall_similarity']:.2f}%")
        
    except FileNotFoundError as e:
        print(f"Erro: Arquivo não encontrado - {e}")
        print("Certifique-se de que os arquivos estão nos caminhos corretos:")
        print("- ratings.csv (na pasta atual)")
        print("- meu/input.dat (seu arquivo gerado)")
    except Exception as e:
        print(f"Erro inesperado: {e}")

if __name__ == "__main__":
    main()
