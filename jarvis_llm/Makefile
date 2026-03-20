# ═════════════════════════════════════════════════════════════════════════════
# JARVIS LLM - Makefile
# LLM Ultra-Léger en C - Optimisé pour la langue française
# ═════════════════════════════════════════════════════════════════════════════

# Compilateur et options
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O3 -march=native -ffast-math
CFLAGS += -fomit-frame-pointer -funroll-loops
CFLAGS += -I.

# Options de débogage (décommenter pour le débogage)
# CFLAGS = -std=c11 -Wall -Wextra -g -O0 -I.

# Bibliothèques
LDFLAGS = -lm

# Support OpenMP (optionnel mais recommandé)
OPENMP = -fopenmp

# Détection automatique de OpenMP
HAVE_OPENMP := $(shell echo 'int main(){return 0;}' | $(CC) -fopenmp -x c - -o /dev/null 2>/dev/null && echo yes || echo no)

ifeq ($(HAVE_OPENMP),yes)
    CFLAGS += $(OPENMP)
    LDFLAGS += $(OPENMP)
    $(info Compilation avec support OpenMP activé)
else
    $(info Compilation sans support OpenMP)
endif

# Fichiers source
SRCS = jarvis.c main.c
OBJS = $(SRCS:.c=.o)

# Nom de l'exécutable
TARGET = jarvis

# ═════════════════════════════════════════════════════════════════════════════
# RÈGLES PRINCIPALES
# ═════════════════════════════════════════════════════════════════════════════

.PHONY: all clean install test benchmark demo help release debug profile

# Règle par défaut
all: $(TARGET)

# Compilation de l'exécutable
$(TARGET): $(OBJS)
	@echo "🔗 Liaison: $@"
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "✅ Compilation terminée: $@"
	@echo ""
	@echo "Exécutez './$(TARGET)' pour démarrer JARVIS"
	@echo "Exécutez './$(TARGET) --help' pour voir les options"

# Compilation des fichiers objets
%.o: %.c jarvis.h
	@echo "🔨 Compilation: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# ═════════════════════════════════════════════════════════════════════════════
# MODES DE COMPILATION SPÉCIAUX
# ═════════════════════════════════════════════════════════════════════════════

# Mode release (optimisations maximales)
release: CFLAGS = -std=c11 -Wall -O3 -march=native -ffast-math -fomit-frame-pointer -funroll-loops -flto -DNDEBUG -I.
release: LDFLAGS += -flto -s
release: clean $(TARGET)
	@echo "✅ Build release terminé"

# Mode debug (avec symboles de débogage)
debug: CFLAGS = -std=c11 -Wall -Wextra -g -O0 -DDEBUG -fsanitize=address -I.
debug: LDFLAGS += -fsanitize=address
debug: clean $(TARGET)
	@echo "✅ Build debug terminé"

# Mode profilage
debug: CFLAGS = -std=c11 -Wall -O2 -g -pg -I.
debug: LDFLAGS += -pg
profile: clean $(TARGET)
	@echo "✅ Build profilage terminé"

# ═════════════════════════════════════════════════════════════════════════════
# COMMANDES DE TEST ET DÉMONSTRATION
# ═════════════════════════════════════════════════════════════════════════════

# Exécuter les tests
test: $(TARGET)
	@echo "🧪 Exécution des tests..."
	./$(TARGET) --demo

# Mode benchmark
benchmark: $(TARGET)
	@echo "📊 Exécution du benchmark..."
	./$(TARGET) --benchmark

# Mode démo
demo: $(TARGET)
	@echo "🎭 Exécution de la démo..."
	./$(TARGET) --demo

# Lancer JARVIS en mode interactif
run: $(TARGET)
	@echo "🚀 Démarrage de JARVIS..."
	./$(TARGET)

# ═════════════════════════════════════════════════════════════════════════════
# UTILITAIRES
# ═════════════════════════════════════════════════════════════════════════════

# Nettoyage
clean:
	@echo "🧹 Nettoyage..."
	rm -f $(OBJS) $(TARGET) *.o gmon.out
	rm -f *.bin *.weights
	@echo "✅ Nettoyage terminé"

# Installation (copie dans /usr/local/bin)
install: $(TARGET)
	@echo "📦 Installation de JARVIS..."
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "✅ JARVIS installé dans /usr/local/bin/"

# Désinstallation
uninstall:
	@echo "🗑️  Désinstallation de JARVIS..."
	rm -f /usr/local/bin/$(TARGET)
	@echo "✅ JARVIS désinstallé"

# Afficher l'aide
help:
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════════╗"
	@echo "║                    JARVIS LLM - Makefile                         ║"
	@echo "╠══════════════════════════════════════════════════════════════════╣"
	@echo "║                                                                  ║"
	@echo "║  Commandes disponibles:                                          ║"
	@echo "║                                                                  ║"
	@echo "║  make              - Compiler JARVIS (mode par défaut)           ║"
	@echo "║  make release      - Compilation optimisée pour production       ║"
	@echo "║  make debug        - Compilation avec symboles de débogage       ║"
	@echo "║  make profile      - Compilation pour profilage                  ║"
	@echo "║                                                                  ║"
	@echo "║  make run          - Compiler et lancer JARVIS                   ║"
	@echo "║  make test         - Exécuter les tests                          ║"
	@echo "║  make benchmark    - Exécuter le benchmark de performance        ║"
	@echo "║  make demo         - Exécuter la démonstration                   ║"
	@echo "║                                                                  ║"
	@echo "║  make install      - Installer JARVIS dans /usr/local/bin        ║"
	@echo "║  make uninstall    - Désinstaller JARVIS                         ║"
	@echo "║  make clean        - Nettoyer les fichiers compilés              ║"
	@echo "║                                                                  ║"
	@echo "╚══════════════════════════════════════════════════════════════════╝"
	@echo ""

# ═════════════════════════════════════════════════════════════════════════════
# INFORMATIONS DE COMPILATION
# ═════════════════════════════════════════════════════════════════════════════

info:
	@echo ""
	@echo "Informations de compilation:"
	@echo "  Compilateur: $(CC)"
	@echo "  CFLAGS:      $(CFLAGS)"
	@echo "  LDFLAGS:     $(LDFLAGS)"
	@echo "  Sources:     $(SRCS)"
	@echo "  Objet:       $(TARGET)"
	@echo ""
