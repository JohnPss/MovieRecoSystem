CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -mtune=native -flto -funroll-loops -ffast-math -DNDEBUG
CXXFLAGS += -pthread -Wno-unused-result -fomit-frame-pointer -finline-functions
LDFLAGS = -pthread -flto

SRCDIR = src
BUILDDIR = build
TARGET = $(BUILDDIR)/preProcessamento

SOURCES = $(SRCDIR)/preProcessamento.cpp $(SRCDIR)/main.cpp
OBJECTS = $(BUILDDIR)/preProcessamento.o $(BUILDDIR)/main.o

.PHONY: all clean run

all: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJECTS) | $(BUILDDIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)
	rm -f datasets/input.dat

run: $(TARGET)
	./$(TARGET)

install-deps:
	@echo "Verificando dependências..."
	@which g++ > /dev/null || (echo "g++ não encontrado. Instale com: sudo apt install g++" && exit 1)
	@echo "Dependências OK!"

profile: CXXFLAGS += -pg
profile: LDFLAGS += -pg
profile: $(TARGET)

debug: CXXFLAGS = -std=c++17 -O0 -g -Wall -Wextra -pthread -DDEBUG
debug: LDFLAGS = -pthread
debug: $(TARGET)

info:
	@echo "Configuração de compilação:"
	@echo "CXX: $(CXX)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "Threads disponíveis: $(shell nproc)"

benchmark: $(TARGET)
	@echo "Executando benchmark..."
	time ./$(TARGET)