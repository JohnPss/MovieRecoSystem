# Makefile para Sistema de Recomendação MovieLens

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
OPTFLAGS = -O3 -march=native -flto -funroll-loops -ffast-math
DEBUGFLAGS = -g -O0 -DDEBUG
TARGET = recommender
TARGET_DEBUG = recommender_debug
SOURCE = src/Main.cpp
SRCDIR = src
BINDIR = bin

# Detecta número de cores para compilação paralela
NPROCS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

.PHONY: all release debug clean profile test directories

# Cria diretórios necessários
directories:
	@mkdir -p $(BINDIR)

# Compilação padrão (release)
all: release

# Versão otimizada para produção
release: CXXFLAGS += $(OPTFLAGS)
release: directories $(BINDIR)/$(TARGET)

# Versão para debugging
debug: CXXFLAGS += $(DEBUGFLAGS)
debug: directories $(BINDIR)/$(TARGET_DEBUG)

# Versão com profiling
profile: CXXFLAGS += $(OPTFLAGS) -pg
profile: directories $(BINDIR)/$(TARGET)_profile

# Compilação do executável principal
$(BINDIR)/$(TARGET): $(SOURCE)
	@echo "Compilando versão release..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Compilação concluída: $@"

# Compilação da versão debug
$(BINDIR)/$(TARGET_DEBUG): $(SOURCE)
	@echo "Compilando versão debug..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Compilação concluída: $@"

# Compilação da versão profile
$(BINDIR)/$(TARGET)_profile: $(SOURCE)
	@echo "Compilando versão profile..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Compilação concluída: $@"

# Executa o programa
run: release
	./$(BINDIR)/$(TARGET)

# Executa com valgrind para detectar memory leaks
memcheck: debug
	valgrind --leak-check=full --show-leak-kinds=all ./$(BINDIR)/$(TARGET_DEBUG)

# Benchmark de performance
benchmark: release
	@echo "Executando benchmark..."
	time ./$(BINDIR)/$(TARGET)

# Verifica se os arquivos de entrada existem
check-data:
	@test -f dataset/input.dat || (echo "Erro: dataset/input.dat não encontrado" && exit 1)
	@test -f ml-25m/movies.csv || (echo "Erro: ml-25m/movies.csv não encontrado" && exit 1)
	@test -f datasets/explore.csv || (echo "Erro: datasets/explore.csv não encontrado" && exit 1)
	@echo "Todos os arquivos de dados encontrados!"

# Instalação (copia para /usr/local/bin)
install: release
	@echo "Instalando em /usr/local/bin..."
	sudo cp $(BINDIR)/$(TARGET) /usr/local/bin/
	@echo "Instalação concluída!"

# Desinstalação
uninstall:
	@echo "Removendo de /usr/local/bin..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Desinstalação concluída!"

# Limpa arquivos compilados
clean:
	rm -rf $(BINDIR)
	rm -f *.o
	rm -f gmon.out
	@echo "Limpeza concluída!"

# Ajuda
help:
	@echo "Comandos disponíveis:"
	@echo "  make [release]  - Compila versão otimizada (padrão)"
	@echo "  make debug      - Compila versão para debugging"
	@echo "  make profile    - Compila versão com profiling"
	@echo "  make run        - Compila e executa"
	@echo "  make memcheck   - Executa com valgrind"
	@echo "  make benchmark  - Executa medindo tempo"
	@echo "  make check-data - Verifica se arquivos de dados existem"
	@echo "  make install    - Instala no sistema"
	@echo "  make clean      - Remove arquivos compilados"
	@echo "  make help       - Mostra esta ajuda"
