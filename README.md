# 🎬 Sistema de Recomendação MovieLens

## 📋 Índice


---


## 📝Introdução

Este trabalho foi desenvolvido como parte da disciplina de Algoritmos e Estruturas de Dados I (AESD1), sob a orientação do professor [Michel Pires Silva](https://github.com/mpiress). Nosso objetivo principal é o desenvolvimento e otimização de um sistema de recomendação capaz de sugerir agrupamentos de elementos similares, utilizando perfis de usuários e características dos itens.

Inspirando-nos em conceitos de algoritmos de classificação e similares, este projeto aprofunda-se em técnicas para melhorar a eficácia e a eficiência computacional de sistemas de recomendação. A ideia é um modelo de "treino e teste" tradicional para classificação de bases de dados categorizadas, aplicando uma abordagem de filtragem colaborativa e baseada em conteúdo, priorizando critérios de buscas de vizinhos próximos.

Para este estudo, utilizamos a vasta base de dados:
**[MovieLens 25M Dataset](https://www.kaggle.com/datasets/garymk/movielens-25m-dataset)**. 

Este dataset é amplamente reconhecido na área de sistemas de recomendação e contém um grande volume de avaliações de filmes por usuários, além de metadados sobre os filmes. O arquivo principal utilizado é o `ratings.csv`, mas outras informações complementares foram exploradas para enriquecer as recomendações.

O pré-processamento dos dados seguiu critérios específicos para garantir a qualidade da entrada:

- Foram considerados apenas usuários que realizaram pelo menos 50 avaliações distintas.
- Apenas filmes avaliados por no mínimo 50 usuários foram incluídos.
- Registros duplicados ou inconsistentes foram removidos.

O arquivo de entrada (`input.dat`) foi gerado no formato:
`usuário_id item_id1:nota1 item_id2:nota2 ...`

Nosso sistema de recomendação integra uma combinação de abordagens para gerar sugestões personalizadas:

- **Filtragem Colaborativa (CF):** Baseia-se na similaridade entre usuários.
- **Recomendação Baseada em Conteúdo (CB):** Aproveita as características dos filmes e as preferências dos usuários.
- **Popularidade:** Um componente de popularidade foi integrado para impulsionar itens bem avaliados e populares.

Para otimizar a busca por usuários similares, implementamos a técnica de **Locality Sensitive Hashing (LSH)**. A integração do LSH, juntamente com a otimização dos parâmetros e implementação de estratégias de fallback, visam garantir a robustez e a qualidade das recomendações, mesmo em cenários de escassez de dados.

O objetivo final é gerar as K melhores recomendações para cada `usuario_id` presente no arquivo `explore.dat`, armazenando os resultados no arquivo `output.dat`.

O desempenho do algoritmo será avaliado por critérios como tempo de execução, qualidade das recomendações e consumo de memória. Com o desenvolvimento do programa, buscamos meios de otimizar o processo, aprimorando tanto a acurácia das recomendações quanto a eficiência na classificação da base de dados.

## 🎯Objetivos

### Objetivo Geral

O objetivo geral deste trabalho é desenvolver e otimizar um sistema de recomendação robusto e eficiente, utilizando a base de dados MovieLens 25M, focado em entregar recomendações de alta qualidade em tempo hábil. Busca-se aprimorar a precisão das recomendações e a eficiência computacional, com ênfase na fase de geração de recomendações, visando reduzir o tempo de execução e aumentar a relevância das sugestões para os usuários.

### Objetivos Específicos

- **Pré-processar a base de dados MovieLens 25M:**  
  Desenvolver um módulo de pré-processamento para filtrar usuários com menos de 50 avaliações e filmes avaliados por menos de 50 usuários, gerando um arquivo de entrada padronizado (`input.dat`).

- **Implementar um sistema de recomendação híbrido:**  
  Integrar abordagens de filtragem colaborativa (baseada em similaridade de usuários usando Co-seno), recomendação baseada em conteúdo (utilizando preferências de gênero do usuário) e um componente de popularidade para calcular *scores* de recomendação para filmes não assistidos.

- **Otimizar a busca por usuários similares com LSH:**  
  Implementar e configurar a técnica de Locality Sensitive Hashing (LSH) para acelerar a identificação de usuários similares, ajustando parâmetros como número de funções hash, bandas e tabelas para equilibrar recall e precisão, e explorar técnicas de multi-probe LSH para garantir a diversidade de candidatos.

- **Aprimorar a resiliência do sistema com estratégias de fallback:**  
  Desenvolver e integrar mecanismos de *fallback* para garantir a geração de um número adequado de recomendações mesmo em cenários de poucos vizinhos similares, como preenchimento com filmes populares ou candidatos de similaridade mais baixa.

- **Avaliar o desempenho e a qualidade das recomendações:**  
  Medir o tempo médio de execução por usuário, o consumo de memória e a qualidade das recomendações geradas, buscando atender aos critérios de eficiência computacional e relevância definidos.

## 🔬 Modelagem da Aplicação

Partindo dos objetivos de desenvolver um sistema de recomendação eficiente para a base de dados MovieLens, nossa modelagem buscou equilibrar a qualidade das recomendações com o desempenho computacional. Para isso, adotamos uma arquitetura modular em C++, aproveitando as características da linguagem para processar grandes volumes de dados e otimizar operações críticas. A escolha do C++ foi motivada por seu desempenho, controle de baixo nível e a riqueza de sua Standard Template Library (STL), que oferece estruturas de dados otimizadas e flexíveis.

Nosso sistema de recomendação opera em três etapas principais, conforme visível no `main.cpp`:
1. **Pré-processamento dos Dados:**  
   Nesta etapa, o arquivo `ratings.csv` original do MovieLens 25M é lido, filtrado e convertido para um formato otimizado (`input.dat`). Este passo é crucial para garantir que apenas dados relevantes (usuários e filmes com número mínimo de avaliações) sejam processados e para formatar os dados de maneira eficiente para as etapas subsequentes.

2. **Carregamento de Dados e Construção do Índice LSH:**  
   O sistema lê os dados pré-processados, carrega informações de filmes e, mais importante, constrói o índice de **Localidade Sensível a Hash (LSH)**. Este índice é a base para a rápida identificação de usuários similares.

3. **Processamento das Recomendações:**  
    A etapa final onde as recomendações são geradas para um conjunto de usuários especificados no arquivo `explore.dat`. Este processo envolve a busca por usuários similares, o cálculo de scores de recomendação utilizando abordagens híbridas (colaborativa, baseada em conteúdo e popularidade) e a aplicação de estratégias de fallback.
   
Essa estrutura garante uma separação nítida das responsabilidades, possibilitando otimizações específicas para cada etapa.

### 📊 Estruturas de Dados

A escolha e a implementação eficientes das estruturas de dados são cruciais para o desempenho de um sistema de recomendação que lida com grandes volumes de usuários e itens. Compreendendo as necessidades de armazenamento, acesso rápido e manipulação de dados, utilizamos diversas estruturas da STL do C++:

- `std::unordered_map`: Amplamente utilizada por sua capacidade de fornecer acesso quase constante *O(1)* a elementos. É empregada para:

   - Mapear IDs de usuários para seus `UserProfile` ( `users` ).

   - Mapear IDs de filmes para suas `Movie` informações ( `movies` ).

   - Armazenar a contagem de filmes e suas somas de avaliações (`movieSums`, `movieCounts`, `movieAvgRatings`, `moviePopularity`).

   - Implementar as tabelas do LSH ( `tables` ), onde cada bucket é um vetor de IDs de usuários.

   - Manter um cache de similaridades já calculadas entre pares de usuários ( `cache` em `SimilarityCalculator`).

   - Associar IDs de gêneros a seus nomes ( `genreToId` ).

   - Agrupar usuários por filme ( `movieToUsers` ) para a fase de pré-processamento e busca de candidatos.

- `std::vector`: Utilizada para armazenar coleções dinâmicas de elementos. Sua principal aplicação inclui:

   - Armazenar as avaliações de um usuário ( `UserProfile::ratings` ) como pares (`movieId`, `rating`), que são mantidos ordenados por `movieId` para otimizar operações de interseção.

   - Representar as assinaturas MinHash ( `MinHashSignature::signature` ) para cada usuário.

   - Listar IDs de usuários dentro dos buckets de LSH.

   - Armazenar a lista de filmes para cada gênero (`genreToMovies`).

   - Gerenciar listas de candidatos e usuários similares.
     
- `std::pair`: Essencial para agrupar dois valores relacionados, como (`movieId`, `rating`) em `UserProfile::ratings` ou (`candidateId`, `commonCount`) para candidatos a usuários similares. A ordenação dos pares (`movieId`, `rating`) nos vetores de avaliações (`user.ratings`) é fundamental para otimizar a cálculo de similaridade do cosseno, permitindo um "merge-join" eficiente dos itens em comum entre dois usuários.

- `std::unordered_set`: Empregada onde a presença única de elementos e a busca rápida são prioritárias, como para `watchedMovies` e para `valid_movie`s e `allMovies` durante o pré-processamento e construção do LSH, respectivamente.



### 🏋️‍♂️ Otimizações Propostas

As otimizações implementadas visaram tanto a eficiência do pré-processamento quanto a aceleração das fases de carregamento e recomendação, que são críticas para o desempenho em tempo real. As principais áreas de otimização e as abordagens utilizadas são:

#### Pré-processamento e Carregamento de Dados

1.  **Leitura Otimizada de Arquivos (Memory Mapping e `std::from_chars`)**:
    * **Memory Mapping (`mmap`)**: O arquivo `ratings.csv` é mapeado diretamente para a memória. Isso evita as latências de E/S tradicionais, permitindo que o sistema operacional lide com o carregamento de blocos de dados de forma mais eficiente.
    * **`std::from_chars` (C++17)**: Para o parsing de números (IDs de usuário, filme e ratings) a partir do buffer mapeado, `std::from_chars` é utilizado em vez de `std::stoi` ou `std::stof`. Esta função é significativamente mais rápida porque não aloca memória dinamicamente nem lança exceções, resultando em um parsing de string para número extremamente eficiente.

2.  **Pré-processamento Paralelo:**
    * A fase de contagem de avaliações por usuário e filme (`process_chunk`) é paralelizada utilizando múltiplas threads. Cada thread processa um chunk do arquivo mapeado, gerando contagens locais.
    * A filtragem de usuários (mínimo de 50 avaliações válidas ) e filmes (mínimo de 50 avaliações ) é aplicada, e a escrita do `input.dat` é feita em paralelo para arquivos temporários (`filter_and_write_chunk`).
    * Um processo final de concatenação (`concatenate_temp_files`) une os arquivos temporários no `input.dat` final. Isso distribui a carga de E/S e processamento, acelerando o pré-processamento geral.

3.  **Alinhamento de Dados (`alignas`)**: A estrutura `ThreadData` em `DataLoader.cpp` é marcada com `alignas(64)`. Isso garante que os dados de cada thread sejam alinhados em linhas de cache separadas, mitigando o "falso compartilhamento" (false sharing) e melhorando o desempenho em sistemas multi-core.

#### Otimizações no LSH e Busca de Similares

1.  **Pré-computação de Hashes para Filmes (LSH `buildSignatures`)**:
    * Os hashes de todos os filmes únicos são pré-computados uma única vez e armazenados em um `unordered_map` (`precomputedHashes`). Isso elimina cálculos redundantes durante a construção das assinaturas MinHash para cada usuário.
    * A construção das assinaturas MinHash é paralelizada, com threads computando as assinaturas para diferentes subconjuntos de usuários.

2.  **LSH Menos Rigoroso para Buckets Maiores (Ajuste de Parâmetros)**:
    * O algoritmo LSH foi configurado com parâmetros menos rigorosos para aumentar a probabilidade de usuários similares colidirem no mesmo bucket. Isso resulta em buckets maiores, contendo mais candidatos potenciais:
        * `NUM_BANDS = 24`.
        * `ROWS_PER_BAND = 4`.
        * `NUM_HASH_FUNCTIONS = 96`.
    * O `hashBand` usa um módulo de hash menor (`% 20000`) para limitar o espaço de hash, promovendo a criação de buckets mais densos.
    * A fase de indexação no LSH (`indexSignatures`) foi ajustada para usar um número menor de bandas por tabela (`BANDS_PER_TABLE = 3`) para criar buckets mais robustos.

3.  **Multi-Probe LSH (Fallback na Busca de Candidatos)**:
    * No `findSimilarCandidates`, se o número de candidatos iniciais for baixo, o sistema tenta buscar em buckets "vizinhos" aplicando pequenas perturbações aos hashes das bandas (`multi-probe LSH`). Isso aumenta as chances de encontrar mais vizinhos quando o match inicial não é suficiente.

4.  **Candidatos de Similaridade Média (Fallback de `findCandidateUsersLSH`)**:
    * A função `findCandidateUsersLSH` foi modificada para, após obter uma lista inicial de candidatos via LSH, filtrar e priorizar aqueles com alta contagem de itens em comum. Se a lista de "alta qualidade" for muito pequena (`< 20` candidatos), o sistema preenche a lista com os melhores candidatos da lista *completa* de achados pelo LSH, garantindo um número mínimo de candidatos para a Filtragem Colaborativa.

#### Otimizações na Geração de Recomendações

1.  **Cache de Similaridade (`SimilarityCalculator`)**:
    * Para evitar o recálculo redundante da similaridade entre pares de usuários, um `std::unordered_map` (`cache`) é utilizado em `SimilarityCalculator` para armazenar os resultados de similaridades já computadas. Antes de calcular a similaridade do cosseno, o cache é consultado, resultando em um acesso $O(1)$ na maioria dos casos.

2.  **Cálculo de Similaridade do Cosseno Otimizado**:
    * A similaridade do cosseno (`calculateCosineSimilarity`) é utilizada, sendo eficaz para dados esparsos.
    * A interseção de filmes avaliados em comum entre dois usuários é feita de forma eficiente, iterando sobre os vetores de avaliações (`user.ratings`) que são **mantidos ordenados por `movieId`**. Isso permite uma abordagem de "merge-join" linear $O(N_1 + N_2)$ para encontrar itens em comum.

3.  **Paralelização das Recomendações por Usuário (`processRecommendations`)**:
    * A fase de geração de recomendações para cada usuário no `explore.dat` é paralelizada. Múltiplas threads processam lotes de usuários simultaneamente (`BATCH_SIZE`), onde cada thread chama `recommendForUser` para um subconjunto de IDs de usuários.
    * Um `std::mutex` (`fileMutex`) é usado para proteger a escrita nos arquivos de saída (`output.dat` e `debug_recommendations.txt`) para evitar condições de corrida.

4.  **Ajustes de Parâmetros de Recomendação (Configurações Otimizadas)**:
    * `MIN_COMMON_ITEMS = 1`: Reduzido para aceitar usuários com apenas 1 filme em comum, aumentando o pool de candidatos similares.
    * `MIN_SIMILARITY = 0.01f`: Muito mais permissivo, aceitando similaridades muito baixas para aumentar a chance de encontrar vizinhos.
    * `MAX_CANDIDATES = 1000`: Aumentado para fornecer mais material para o algoritmo trabalhar.
    * Pesos Otimizados para o Sistema Híbrido: `CF_WEIGHT` e `CB_WEIGHT` ajustados, com a introdução de um `POPULARITY_WEIGHT = 3.0f` para impulsionar a relevância da popularidade.
    * `POPULARITY_BOOST_WEIGHT = 1.5f`: Novo peso para impulsionar a popularidade em cenários de fallback.

---

