.PHONY: all clean

# Compilateur
CC := gcc

# Options de compilation
# -g : permet le débogage
# -Wall : active la plupart des avertissements de compilation
# -pthread : pour la programmation avec des threads
CFLAGS := -g -Wall -pthread

# Répertoires
SRC_DIR := src
BIN_DIR := bin

# Fichiers source et objets
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

# Nom de l'exécutable
EXECUTABLE := $(BIN_DIR)/program

# Règle par défaut
all: $(EXECUTABLE)

# Lien
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

# Compilation
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $^ -o $@


# Exécution du programme
#run: $(EXECUTABLE)
#	@echo "Exécution de $(EXECUTABLE)..."
#	@./$(EXECUTABLE)


clean:
	rm -f $(BIN_DIR)/*.o $(EXECUTABLE)
	@echo "Nettoyage complet!"

