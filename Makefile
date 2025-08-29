# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I$(SRCDIR) -I$(SRCDIR)/core -I$(SRCDIR)/data -I$(SRCDIR)/algorithms -I$(SRCDIR)/system -I$(SRCDIR)/preprocessing -I$(SRCDIR)/main
OPTFLAGS = -O3 -march=native -flto -funroll-loops -ffast-math
DEBUGFLAGS = -g -O0 -DDEBUG

# Diretórios e arquivos
SRCDIR = src
OBJDIR = build/objects
BINDIR = build
TARGET = $(BINDIR)/app

# Fontes e objetos - incluir todos os subdiretórios
SRCS = $(shell find $(SRCDIR) -name "*.cpp")
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

# Compilar tudo (modo release com otimizações)
all: CXXFLAGS += $(OPTFLAGS)
all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpar apenas arquivos gerados (binário e objetos)
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Executar o programa
run: all
	./$(TARGET)

.PHONY: all clean run
