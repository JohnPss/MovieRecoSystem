# 🎬 Sistema de Recomendação MovieLens

## 📋 Índice
1. [Visão Geral](#visão-geral)
2. [Fase 1: Carregamento e Pré-processamento](#fase-1-carregamento-e-pré-processamento)
3. [Fase 2: Geração de Recomendações](#fase-2-geração-de-recomendações)
4. [Análise de Complexidade e Performance](#análise-de-complexidade-e-performance)
5. [Validação e Qualidade](#validação-e-qualidade)
6. [Exemplos Práticos Completos](#exemplos-práticos-completos)

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

- Base de Dados MovieLens 25M: Os arquivos brutos da base de dados, especialmente o `ratings.csv`, devem estar localizados no diretório `ml-25m/`. Caso ainda não os tenha, faça o download manual da base de dados MovieLens 25M através do repositório Kaggle, disponível em: [MovieLens 25M Dataset](https://grouplens.org/datasets/movielens/25m/).

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
- Local de Download: A base de dados pode ser baixada manualmente através do repositório Kaggle, disponível em:[MovieLens 25M Dataset](https://grouplens.org/datasets/movielens/25m/)   
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

###Formato dos Arquivos de Entrada e Saída

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

    Estrutura: Cada linha representa um `usuario_id` seguido pelos `item_ids` recomendados. ****************************************************
    Exemplo:
    ```
    123 54 76 145
    ```
