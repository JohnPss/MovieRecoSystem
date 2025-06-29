# Makefile para Sistema de Recomendação MovieLens

# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I$(SRCDIR)
OPTFLAGS = -O3 -march=native -flto -funroll-loops -ffast-math
DEBUGFLAGS = -g -O0 -DDEBUG

# Diretórios e arquivos
TARGET = recommender
TARGET_DEBUG = recommender_debug
SRCDIR = src
BINDIR = bin
OBJDIR = obj

# Fontes e objetos
# MODIFICAÇÃO: Adicionamos LSHIndex.cpp à lista de fontes.
# A forma com 'wildcard' abaixo é ainda melhor, pois encontra todos os .cpp automaticamente.
SOURCES = $(wildcard $(SRCDIR)/*.cpp)

# Se preferir a lista manual, seria assim:
# SOURCES = $(SRCDIR)/Main.cpp \
#           $(SRCDIR)/FastRecommendationSystem.cpp \
#           $(SRCDIR)/DataLoader.cpp \
#           $(SRCDIR)/SimilarityCalculator.cpp \
#           $(SRCDIR)/RecommendationEngine.cpp \
#           $(SRCDIR)/LSHIndex.cpp

OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))
OBJECTS_DEBUG = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/debug/%.o,$(SOURCES))

# Núcleos do processador
NPROCS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

.PHONY: all release debug clean profile test directories run memcheck benchmark install uninstall help check-data

# Compilação padrão
all: release

# Cria diretórios necessários
directories:
	@mkdir -p $(BINDIR) $(OBJDIR) $(OBJDIR)/debug

# Versão otimizada para produção
release: CXXFLAGS += $(OPTFLAGS)
release: directories $(BINDIR)/$(TARGET)

# Versão para debugging
debug: CXXFLAGS += $(DEBUGFLAGS)
debug: directories $(BINDIR)/$(TARGET_DEBUG)

# Versão com profiling
profile: CXXFLAGS += $(OPTFLAGS) -pg
profile: directories $(BINDIR)/$(TARGET)_profile

# Linkagem (release)
$(BINDIR)/$(TARGET): $(OBJECTS)
	@echo "Linkando versão release..."
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Compilação concluída: $@"

# Linkagem (debug)
$(BINDIR)/$(TARGET_DEBUG): $(OBJECTS_DEBUG)
	@echo "Linkando versão debug..."
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Compilação concluída: $@"

# Linkagem (profile)
$(BINDIR)/$(TARGET)_profile: $(OBJECTS)
	@echo "Linkando versão profile..."
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Compilação concluída: $@"

# Compilação de objetos (release)
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo "Compilando $<..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compilação de objetos (debug)
$(OBJDIR)/debug/%.o: $(SRCDIR)/%.cpp
	@echo "Compilando (debug) $<..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Executa o programa (release)
run: release
	./$(BINDIR)/$(TARGET)

# Executa com valgrind para detectar memory leaks
memcheck: debug
	valgrind --leak-check=full --show-leak-kinds=all ./$(BINDIR)/$(TARGET_DEBUG)

# Benchmark de performance
benchmark: release
	@echo "Executando benchmark..."
	time ./$(BINDIR)/$(TARGET)

# Verifica se arquivos de dados existem
check-data:
	@test -f datasets/input.dat || (echo "Erro: dataset/input.dat não encontrado" && exit 1)
	@test -f ml-25m/movies.csv || (echo "Erro: ml-25m/movies.csv não encontrado" && exit 1)
	@test -f datasets/explore.dat || (echo "Erro: datasets/explore.csv não encontrado" && exit 1)
	@echo "Todos os arquivos de dados encontrados!"

# Instalação do binário
install: release
	@echo "Instalando em /usr/local/bin..."
	sudo cp $(BINDIR)/$(TARGET) /usr/local/bin/
	@echo "Instalação concluída!"

# Remove o binário instalado
uninstall:
	@echo "Removendo de /usr/local/bin..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Desinstalação concluída!"

# Limpa arquivos compilados
clean:
	rm -rf $(BINDIR) $(OBJDIR)
	rm -f gmon.out
	@echo "Limpeza concluída!"

# Checar consumo de memória com Massif
massif: release
	valgrind --tool=massif --massif-out-file=massif.out ./$(BINDIR)/$(TARGET)
	ms_print massif.out | less

rmse-test: directories
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $(BINDIR)/rmse_test \
	    $(SRCDIR)/RMSETest.cpp $(filter-out $(SRCDIR)/Main.cpp,$(SOURCES))

# Ajuda
help:
	@echo "Comandos disponíveis:"
	@echo "  make            - Compila versão release"
	@echo "  make debug      - Compila versão debug"
	@echo "  make profile    - Compila versão com profiling"
	@echo "  make run        - Executa o programa"
	@echo "  make memcheck   - Roda valgrind para memory leaks"
	@echo "  make benchmark  - Executa e mede tempo"
	@echo "  make check-data - Verifica arquivos de entrada"
	@echo "  make install    - Instala em /usr/local/bin"
	@echo "  make uninstall  - Remove de /usr/local/bin"
	@echo "  make clean      - Limpa arquivos compilados"
	@echo "  make help       - Exibe esta ajuda"
