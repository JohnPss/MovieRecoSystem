# 🎬 Sistema de Recomendação MovieLens

## 📋 Índice


---

## 🎯 Visão Geral do Projeto

Este projeto consiste no desenvolvimento de um Sistema de Recomendação de Filmes baseado em perfis de usuários e características de itens. O sistema utiliza a base de dados MovieLens 25M para identificar padrões e sugerir filmes aos usuários, visando aprimorar a experiência de descoberta de conteúdo.

## Objetivo

O principal objetivo deste trabalho é implementar um sistema de recomendação eficiente e preciso, capaz de agrupar elementos similares (filmes e usuários). A partir dessas similaridades, o sistema deverá gerar sugestões personalizadas de filmes para os usuários. Buscamos otimizar a performance computacional, a qualidade das recomendações e a organização do código.

## Funcionalidades

- **Download e Pré-processamento da Base de Dados**  
 Automação ou orientação para a obtenção e tratamento inicial dos dados da MovieLens 25M.

- **Filtragem de Usuários e Filmes**  
 Implementação de critérios para considerar apenas usuários com um mínimo de avaliações e filmes com um mínimo de avaliações recebidas.

- **Remoção de Dados Inconsistentes** 
Lógica para lidar com registros duplicados ou inválidos.

- **Geração de Arquivo de Entrada Padronizado (input.dat)**
 Formato específico usuario_id item_id1:nota1 item_id2:nota2 ....

- **Cálculo de Similaridade**
 Aplicação de métricas de distância (Euclidiana, Cosseno, Jaccard) para determinar a afinidade entre usuários.

- **Seleção dos K Usuários Mais Similares**
 Identificação dos vizinhos mais próximos.

- **Geração de Recomendações Personalizadas**
 Sugestão de filmes não avaliados pelo usuário-alvo, baseadas nas avaliações dos usuários similares.

- **Geração de Arquivo de Saída Padronizado (output.dat)**
 Formato usuario_id item_id1 item_id2 ....

- **Compilação e Execução via Makefile**
 Facilidade na gestão do processo de build e run.

## Tecnologias Utilizadas

- Linguagem de Programação: C++
- Compilador: GCC 13 ou superior
- Sistema Operacional: Linux Ubuntu 24.04 LTS
- Biblioteca Padrão: Uso exclusivo da biblioteca padrão da linguagem C++.

## Estrutura de Diretórios

A seguir, a estrutura do diretório do projeto:
```
.
├── Makefile
├── README.md
├── datasets/
│   ├── input.dat
│   └── explore.dat
├──outcome/
|   └──output.dat
├── ml-25m/
│   └── arquivos MovieLens (como ratings.csv, movies.csv, etc.)
└── src/
    ├── Config.hpp                
    ├── DataLoader.cpp
    ├── DataLoader.hpp
    ├── DataStructures.hpp        
    ├── FastRecommendation.cpp    
    ├── FastRecommendation.hpp
    ├── LSHIndex.cpp              
    ├── LSHIndex.hpp
    ├── Main.cpp                 
    ├── preProcessament.cpp       
    ├── preProcessament.hpp       
    ├── Recommendation.cpp        
    ├── Recommendation.hpp
    ├── SimilarityCalculator.cpp  
    └── SimilarityCalculator.hpp

```
- `.`: Diretório raiz do projeto.

- `Makefile`: Script para compilação e execução do projeto.

- `README.md`: Este arquivo de documentação.

- `datasets/`: Diretório para armazenar os dados de entrada processados para o sistema.

    - `input.dat`: Base de dados pré-processada no formato `usuario_id item_id1:nota1 ...`, utilizada como fonte de dados para o algoritmo de recomendação.

    - `explore.dat`: Lista de `usuario_ids` para os quais as recomendações personalizadas serão geradas.

- `outcome/`:Diretório para armazenar os resultados.

    - `output.dat`: Arquivo de saída contendo as K recomendações para cada usuário listado em `explore.dat`, no formato `usuario_id item_id1 item_id2 ...`.

- `ml-25m/`: Diretório que contém os arquivos brutos originais da base de dados MovieLens 25M, baixados diretamente do Kaggle.

    - `ratings.csv`: O arquivo principal com as avaliações dos usuários.

    - `movies.csv, tags.csv, etc.`: Outros arquivos que podem ser explorados para dados adicionais durante o pré-processamento.

- `src/`: Contém todos os arquivos de código-fonte modularizados (implementações .cpp e cabeçalhos .hpp).

    - `Config.hpp`: Define constantes globais e parâmetros de configuração do sistema (ex: `TOP_N_RECOMMENDATIONS`).

    - `DataLoader.cpp/.hpp`: Responsável pelo carregamento dos dados de `input.dat` para estruturas de dados em memória e pelo gerenciamento desses dados.

    - `DataStructures.hpp`: Contém as definições das estruturas de dados customizadas utilizadas no projeto (ex: structs para usuários, itens, nós de listas ou árvores, etc.).

    - `FastRecommendation.cpp/.hpp`: Este módulo abriga uma abordagem otimizada ou um método específico de recomendação desenvolvido para acelerar o processo.

    - `LSHIndex.cpp/.hpp`: Contém a implementação de Locality Sensitive Hashing (LSH), uma técnica de indexação para otimização da busca por vizinhos similares em grandes conjuntos de dados. Confirme se LSH foi realmente utilizado.

    - `Main.cpp`: O ponto de entrada principal do programa, que orquestra as chamadas para as diferentes fases do sistema (pré-processamento, carregamento, recomendação, gravação de saída).

    - `preProcessament.cpp/.hpp`: Contém as funções responsáveis pelo processamento inicial da base de dados bruta (`ml-25m/ratings.csv e outros`), incluindo filtragem de usuários/filmes, remoção de duplicatas e geração do `input.dat`.

    - `Recommendation.cpp/.hpp`: Abriga a lógica central do algoritmo de recomendação, gerenciando a busca por vizinhos, a agregação de recomendações e a priorização dos filmes a serem sugeridos.

    - `SimilarityCalculator.cpp/.hpp`: Implementa as métricas de similaridade (Euclidiana, Cosseno, Jaccard) utilizadas para quantificar a afinidade entre usuários.

## Configuração do Ambiente

Para garantir a correta compilação e execução do projeto, certifique-se de que o ambiente de desenvolvimento esteja configurado com as seguintes especificações:

- Sistema Operacional: Linux Ubuntu 24.04 LTS.
 
- Compilador: GCC versão 13 ou superior (g++ para C++). Certifique que esta com a versão atualizada do compilador:

    ``` bash
    g++ --version
    ```
Caso precise instalar ou atualizar o compilador e as ferramentas de build essenciais no Ubuntu, utilize os seguintes comandos:
        
    ``` bash
    sudo apt update
    sudo apt install build-essential g++
    ```
- Biblioteca Padrão: O projeto utiliza exclusivamente a biblioteca padrão da linguagem C++. Não há dependências de bibliotecas de terceiros externas.

## Como Compilar e Executar

Este projeto utiliza um `Makefile` para simplificar e padronizar os processos de compilação e execução. É fundamental seguir os comandos especificados para garantir a reprodutibilidade do ambiente.

### Pré-requisitos

Antes de compilar e executar o sistema, certifique-se de que os seguintes pré-requisitos estejam atendidos:

- Base de Dados MovieLens 25M: Os arquivos brutos da base de dados, especialmente o `ratings.csv`, devem estar localizados no diretório `ml-25m/`. Caso ainda não os tenha, faça o download manual da base de dados MovieLens 25M através do repositório Kaggle, disponível em: [MovieLens 25M Dataset](https://www.kaggle.com/datasets/garymk/movielens-25m-dataset).

- Arquivo de Usuários para Exploração (`explore.dat`): Este arquivo deve ser criado manualmente por você e colocado no diretório `datasets`. Ele contém a lista de `usuario_ids` para os quais o sistema irá gerar recomendações personalizadas.

- Localização: `datasets/explore.dat`
- Estrutura: Cada linha deve conter um único `usuario_id`,Exemplo:
```
123
456
789
```
Geração de `input.dat`: O arquivo `input.dat` (a base de dados pré-processada) será gerado automaticamente pelo programa durante a sua execução inicial, a partir dos dados brutos em `ml-25m/`. Não é necessário criá-lo manualmente antes de executar o `make run`.    
        
### Compilação
Para compilar o projeto, navegue até o diretório raiz do projeto no seu terminal e execute os seguintes comandos:
```
make clean
make 
```
- `make clean`: Este comando remove todos os arquivos de objeto (`.o`), dependências (`.d`) e o executável principal gerado em compilações anteriores, garantindo um processo de build limpo e sem resíduos.

- `make`: Este comando compila todo o código-fonte C++ (`.cpp` e `.hpp`) presente no diretório `src/` e gera o executável principal do projeto.

### Execução
Após a compilação bem-sucedida, o sistema pode ser executado. O comando de execução disparará o processo de pré-processamento (se `input.dat` não existir ou estiver desatualizado), o processo de recomendação para os usuários listados em `explore.dat` e salvará os resultados em 
`output.dat`.
``` Bash
make run
```
Este comando fará com que o programa:

- Verifique/Gere `datasets/input.dat`: Se o `input.dat` não existir ou precisar ser atualizado, o sistema realizará o pré-processamento dos dados brutos de `ml-25m/` e gerará este arquivo.

- Carregue os dados de `datasets/input.dat` para a memória.

- Leia os `usuario_ids` do `datasets/explore.dat`.

- Para cada `usuario_id` em `explore.dat`, execute o algoritmo de recomendação.

- Grave as recomendações geradas no arquivo `outcome/output.dat`    .

## Processamento e Estrutura dos Dados
Esta seção detalha como os dados são obtidos, pré-processados e os formatos esperados para os arquivos de entrada e saída do sistema.

### Download da Base de Dados MovieLens 25M
A base de dados fundamental para este projeto é a MovieLens 25M. É essencial que o arquivo `ratings.csv` seja utilizado, e outros arquivos como `movies.csv` podem ser explorados para enriquecer o processo de pré-processamento.  
- Local de Download: A base de dados pode ser baixada manualmente através do repositório Kaggle, disponível em:[MovieLens 25M Dataset](https://www.kaggle.com/datasets/garymk/movielens-25m-dataset)   
- Localização no Projeto: Após o download e descompactação, os arquivos originais (ex: `ratings.csv`, `movies.csv`) devem ser colocados no diretório `ml-25m/` do projeto.
### Pré-processamento
O módulo de pré-processamento do sistema (`preProcessament.cpp/.hpp`) é responsável por transformar os dados brutos da MovieLens 25M em um formato otimizado para o algoritmo de recomendação. Este processo é executado automaticamente pelo programa. Os critérios de filtragem são:
- Filtragem de Usuários: Serão considerados apenas usuários que realizaram pelo menos 50 avaliações distintas. Isso ajuda a focar em usuários com um histórico de preferências mais robusto.

- Filtragem de Filmes: Serão utilizados apenas filmes que foram avaliados por pelo menos 50 usuários. Este critério garante que filmes com pouquíssimas avaliações, que poderiam distorcer os resultados, sejam desconsiderados.

- Remoção de Duplicados e Inconsistências: Registros duplicados ou inconsistentes no `ratings.csv` são identificados e removidos para garantir a integridade dos dados.

- Geração de `input.dat`: O resultado do pré-processamento é salvo no arquivo `datasets/input.dat` no seguinte formato:
```
usuario_id item_id1:nota1 item_id2:nota2 item_id3:nota3 ...
```
Exemplo:
```
123 12:4.0 54:3.5 76:5.0 145:2.0
```
Cada linha representa um `usuario_id` seguido de suas respectivas avaliações, onde `item_id` é o identificador do filme e `nota` é a avaliação do usuário para aquele filme.

### Formato dos Arquivos de Entrada e Saída

Os arquivos utilizados pelo sistema seguem padrões específicos para garantir compatibilidade e processamento correto:

- `datasets/input.dat`:

    Propósito: Base de dados principal e pré-processada para o sistema de recomendação.

    Localização: `datasets/`

    Formato: Texto puro (UTF-8).

    Estrutura: `usuario_id item_id1:nota1 item_id2:nota2 ...`.

- `datasets/explore.dat`:

    Propósito: Lista de `usuario_ids` para os quais o sistema deverá gerar recomendações personalizadas.

    Localização: `datasets/`

    Formato: Texto puro (UTF-8).

    Estrutura: Cada linha contém um único `usuario_id`.
    Exemplo:
    ```
    123
    456
    789
    ```
- `outcome/output.dat`:

    Propósito: Contém as K recomendações geradas para cada usuário listado em explore.dat.

    Localização: `outcome/`

    Formato: Texto puro (UTF-8).

    Estrutura: Cada linha representa um `usuario_id` seguido pelos `item_ids` recomendados.
    Exemplo:
    ```
    123 54 76 145
    ```

## Algoritmo de Recomendação

O sistema implementa uma abordagem avançada de Filtragem Colaborativa Baseada em Usuários (`User-Based Collaborative Filtering`), otimizada para eficiência e precisão na geração de recomendações.

### Métricas de Similaridade

Após testes com diferentes métricas como Jaccard e Pearson, a Similaridade do Cosseno foi a métrica escolhida para calcular a afinidade entre usuários.
Motivo da Escolha (Similaridade do Cosseno):
A Similaridade do Cosseno é ideal para sistemas de recomendação por diversas razões:
- Foco na Direção: Concentra-se na direção dos vetores de avaliação dos usuários, e não na magnitude. Isso significa que usuários com padrões de avaliação semelhantes (gostam/não gostam dos mesmos filmes), mas que avaliam em escalas diferentes (um sempre dá notas mais altas que o outro), ainda assim serão considerados similares.
- Robustez a Tendências de Avaliação: É menos sensível a usuários que avaliam consistentemente com notas mais altas ou mais baixas, focando no relacionamento relativo entre as avaliações.
- Eficácia com Dados Esparsos: Funciona bem mesmo com dados esparsos, onde a maioria dos filmes não foi avaliada pela maioria dos usuários, pois considera apenas os itens avaliados em comum.
- Boa Performance Computacional: Apresenta um equilíbrio favorável entre acurácia e custo computacional.

### Lógica de Recomendação Detalhada
Para cada `usuario_id` presente no arquivo `explore.dat`, o procedimento de recomendação segue as etapas:
#### Pré-cálculo e Otimização com LSH
- Assinaturas MinHash: Perfis de usuários são convertidos em "impressões digitais" compactas (assinaturas MinHash).
- Indexação em `bands`: Essas assinaturas são organizadas em `buckets` através de múltiplas `bands`, permitindo uma busca rápida por usuários potencialmente similares.
- Busca de Candidatos (`findCandidateUsersLSH`): O LSH é utilizado para encontrar eficientemente um conjunto inicial de usuários que são candidatos a serem vizinhos similares, reduzindo drasticamente o espaço de busca.
- Fallback Inteligente: Caso o número de candidatos encontrados via LSH seja insuficiente, o sistema relaxa automaticamente os critérios de busca para garantir a obtenção de um número adequado de vizinhos.
- Configuração Otimizada LSH: A otimização do LSH é configurada em `src/Config.hpp` com `96 funções hash`, distribuídas em `24 bands` com `4 rows por band`, utilizando `8 tabelas` para equilibrar a precisão e o recall.
#### Cálculo Preciso da Similaridade
- A Similaridade do Cosseno é então calculada entre o usuário-alvo e os usuários candidatos identificados pelo LSH.

- São considerados apenas usuários com no mínimo `MIN_COMMON_ITEMS` (definido em `Config.hpp`) filmes avaliados em comum.

- É aplicada uma filtragem para considerar apenas usuários com similaridade acima de `MIN_SIMILARITY` (definido em `Config.hpp`).
#### Seleção dos `MAX_SIMILAR_USERS` Vizinhos Mais Similares:
- Dentre os usuários que atenderam aos critérios de similaridade, os `MAX_SIMILAR_USERS` (constante em `Config.hpp`) com as maiores pontuações de Similaridade do Cosseno são selecionados como vizinhos.
#### Geração e Ponderação das Recomendações de Filmes:
Para cada filme não avaliado pelo usuário-alvo, o sistema calcula uma pontuação de recomendação. A implementação utiliza uma média ponderada sofisticada para prever a avaliação do usuário-alvo para esse filme. A lógica central é:
```
scores[movieId] += similarity * (rating - simUserAvg);
```
- Média Ponderada: Cada vizinho contribui para a pontuação do filme proporcionalmente à sua Similaridade do Cosseno com o usuário-alvo.

- Normalização Personalizada: A expressão (`rating - simUserAvg`) normaliza a avaliação do vizinho (`rating`) subtraindo a média das avaliações do próprio vizinho (`simUserAvg`). Isso compensa as tendências de avaliação individuais (usuários que dão notas consistentemente altas ou baixas), garantindo que a recomendação reflita o desvio do vizinho em relação à sua própria média, e não apenas a nota absoluta.

- Sobreposição de Interesse: Filmes avaliados por um maior número de vizinhos similares naturalmente acumulam mais pontos, refletindo um maior "consenso" entre os usuários com gostos semelhantes.

- Boost de Popularidade : Se o sistema usa dados de `movies.csv` para popularidade, filmes mais populares podem receber um pequeno peso extra para evitar recomendações muito obscuras.

- Content-Based Boost : Se a lógica considera os gêneros preferidos do usuário (inferidos de suas avaliações), filmes desses gêneros podem receber um "boost" adicional. (Se você não implementou os boosts de popularidade/gênero, me avise para remover).
#### Seleção dos Top-N Filmes para Recomendação:
- Os filmes são classificados com base nas suas pontuações calculadas.

- Os `TOP_K` filmes com as maiores pontuações são selecionados para serem recomendados ao usuário-alvo.

### Configurações Chave (src/Config.hpp)
```
namespace Config
{
    // Parâmetros de recomendação principal
    const int TOP_K = 20;               // Número de recomendações a retornar para cada usuário
    const int MAX_SIMILAR_USERS = 500;  // Máximo de usuários similares a considerar para o CF
    const int MIN_COMMON_ITEMS = 1;     // Mínimo de filmes em comum entre dois usuários para cálculo de similaridade
    const float MIN_RATING = 3.5f;      // Rating mínimo para um filme ser considerado uma avaliação "positiva"
    const float MIN_SIMILARITY = 0.01f; // Similaridade mínima aceita para um usuário ser considerado vizinho
    const int MAX_CANDIDATES = 1000;    // Máximo de candidatos a vizinhos a serem buscados via LSH

    // Parâmetros LSH (Locality Sensitive Hashing) otimizados
    const int NUM_HASH_FUNCTIONS = 96;  // Total de funções hash utilizadas para gerar assinaturas MinHash
    const int NUM_BANDS = 24;           // Número de bandas para agrupar as assinaturas LSH
    const int ROWS_PER_BAND = 4;        // Número de linhas (hashes) por banda
    const int NUM_TABLES = 8;           // Número de tabelas hash para aumentar a probabilidade de colisão (recall)
    const uint32_t LARGE_PRIME = 4294967291u; // Número primo grande usado nas funções hash

    // Pesos para o sistema de recomendação híbrido
    const float CF_WEIGHT = 1.0f;         // Peso da componente de Filtragem Colaborativa
    const float CB_WEIGHT = 1.0f;         // Peso da componente de Filtragem Baseada em Conteúdo
    const float POPULARITY_WEIGHT = 3.0f; // Peso da componente de Popularidade dos filmes
    const float POPULARITY_BOOST_WEIGHT = 1.5f; // Peso adicional para o boost de popularidade em cenários específicos

    // Configurações de Fallback para candidatos de CF
    const int MIN_CANDIDATES_FOR_CF = 50;        // Limite mínimo de candidatos LSH para prosseguir normalmente com CF
    const int EMERGENCY_FALLBACK_THRESHOLD = 10; // Limite abaixo do qual o sistema entra em modo de fallback de emergência

    // Parâmetros de performance e paralelização
    const int NUM_THREADS = std::thread::hardware_concurrency() - 2; // Número de threads para processamento paralelo
    const int BATCH_SIZE = 100; // Tamanho do batch para processamento paralelo

    // Caminhos dos arquivos de dados
    inline static const std::string USERS_FILE = "datasets/explore.dat";        // Arquivo de usuários para recomendar
    inline static const std::string MOVIES_FILE = "ml-25m/movies.csv";          // Arquivo de metadados de filmes (para CB/popularidade)
    inline static const std::string RATINGS_FILE = "datasets/input.dat";        // Arquivo de avaliações pré-processadas
    inline static const std::string OUTPUT_FILE = "outcome/output.dat";         // Arquivo de saída com as recomendações
    inline static const std::string DEBUG_OUTPUT_FILE = "outcome/debug_recommendations.txt"; // Arquivo para logs de depuração
}
```


## 
