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

