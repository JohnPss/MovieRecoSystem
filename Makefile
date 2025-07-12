# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I$(SRCDIR)
OPTFLAGS = -O3 -march=native -flto -funroll-loops -ffast-math
DEBUGFLAGS = -g -O0 -DDEBUG

# Diretórios e arquivos
SRCDIR = src
OBJDIR = build/objects
BINDIR = build
TARGET = $(BINDIR)/app

# Fontes e objetos
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

# Compilar tudo (modo release com otimizações)
all: CXXFLAGS += $(OPTFLAGS)
all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpar apenas arquivos gerados (binário e objetos)
clean:
	rm -f $(OBJDIR)/*.o $(TARGET)

# Executar o programa
run: all
	./$(TARGET)

.PHONY: all clean run
