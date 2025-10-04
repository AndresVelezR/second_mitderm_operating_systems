# Makefile para proyecto de procesamiento de imágenes modular
# QUÉ: Automatiza compilación incremental de proyecto C modular
# CÓMO: Usa reglas de patrón, variables y dependencias automáticas
# POR QUÉ: Mejora productividad y evita recompilación innecesaria

# ============================================================================
# VARIABLES DE CONFIGURACIÓN
# ============================================================================

CC = gcc
CFLAGS = -O2 -Wall -Wextra -pthread -Iinclude -Istb
LDFLAGS = -lm -pthread

# Directorios
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
STB_DIR = stb
RESULTS_DIR = results

# Ejecutable
TARGET = img_processor

# Archivos fuente (todos los .c en src/)
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Archivos objeto (uno por cada .c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Headers (para dependencias)
HEADERS = $(wildcard $(INC_DIR)/*.h)

# ============================================================================
# REGLAS PRINCIPALES
# ============================================================================

# Target por defecto: compilar todo
all: $(TARGET)

# Crear ejecutable desde objetos
$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "✓ Compilación exitosa: ./$(TARGET)"

# Regla de patrón: compilar .c a .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Crear directorios si no existen
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(RESULTS_DIR):
	mkdir -p $(RESULTS_DIR)

# ============================================================================
# UTILIDADES
#============================================================================

# Limpiar archivos generados
clean:
	@echo "Cleaning build files..."
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "✓ Clean complete"

# Limpiar todo incluyendo resultados
clean-all: clean
	@echo "Cleaning results..."
	rm -rf $(RESULTS_DIR)
	@echo "✓ Full clean complete"

# Rebuild completo
rebuild: clean all

# Ejecutar con imagen de prueba
run: all | $(RESULTS_DIR)
	@echo "Running image processor..."
	./$(TARGET)

# Mostrar ayuda
help:
	@echo "Makefile para procesamiento de imágenes modular"
	@echo ""
	@echo "Targets disponibles:"
	@echo "  make           - Compilar proyecto (target por defecto)"
	@echo "  make all       - Compilar proyecto"
	@echo "  make clean     - Eliminar archivos de compilación"
	@echo "  make clean-all - Eliminar archivos de compilación y resultados"
	@echo "  make rebuild   - Limpiar y recompilar todo"
	@echo "  make run       - Compilar y ejecutar"
	@echo "  make help      - Mostrar esta ayuda"
	@echo ""
	@echo "Uso del programa:"
	@echo "  ./$(TARGET) [ruta_imagen.png]"

# Evitar conflictos con archivos llamados 'clean', 'all', etc.
.PHONY: all clean clean-all rebuild run help
