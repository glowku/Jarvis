/**
 * JARVIS - Interface Principale
 * LLM Ultra-Léger en C - Optimisé pour le français
 */

#include "jarvis.h"
#include <signal.h>
#include <unistd.h>

static volatile int running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
    printf("\n\nArrêt de JARVIS...\n");
}

void print_banner(void) {
    printf("\n");
    printf("    ██╗ █████╗ ██████╗ ██╗   ██╗██╗███████╗\n");
    printf("    ██║██╔══██╗██╔══██╗██║   ██║██║██╔════╝\n");
    printf("    ██║███████║██████╔╝██║   ██║██║███████╗\n");
    printf("    ██║██╔══██║██╔══██╗╚██╗ ██╔╝██║╚════██║\n");
    printf("    ██║██║  ██║██║  ██║ ╚████╔╝ ██║███████║\n");
    printf("    ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝  ╚═══╝  ╚═╝╚══════╝\n");
    printf("\n");
    printf("    Just A Rather Very Intelligent System\n");
    printf("    ═══════════════════════════════════\n");
    printf("    Version 1.0 - LLM Ultra-Léger en C\n");
    printf("    Optimisé pour la langue française\n");
    printf("\n");
}

void print_help(void) {
    printf("\nCommandes disponibles:\n");
    printf("  /help        - Afficher cette aide\n");
    printf("  /info        - Informations sur le modèle\n");
    printf("  /clear       - Effacer l'écran\n");
    printf("  /reset       - Réinitialiser la conversation\n");
    printf("  /save <fichier> - Sauvegarder les poids\n");
    printf("  /load <fichier> - Charger les poids\n");
    printf("  /temp <val>  - Définir la température (0.1-2.0)\n");
    printf("  /tokens <n>  - Nombre max de tokens à générer\n");
    printf("  /quit        - Quitter JARVIS\n");
    printf("\n");
}

void interactive_mode(JarvisModel* model) {
    char input[4096];
    char* response;
    float temperature = JARVIS_TEMPERATURE;
    int max_tokens = 256;
    
    printf("Mode interactif activé. Tapez /help pour les commandes.\n\n");
    
    while (running) {
        printf("\033[1;36mVous:\033[0m ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        // Enlever le newline
        input[strcspn(input, "\n")] = '\0';
        
        // Commandes spéciales
        if (input[0] == '/') {
            if (strcmp(input, "/quit") == 0 || strcmp(input, "/q") == 0) {
                break;
            } else if (strcmp(input, "/help") == 0) {
                print_help();
                continue;
            } else if (strcmp(input, "/info") == 0) {
                jarvis_print_info(model);
                printf("Mémoire utilisée: %.2f MB\n", jarvis_get_memory_usage(model));
                continue;
            } else if (strcmp(input, "/clear") == 0) {
                printf("\033[2J\033[H");
                print_banner();
                continue;
            } else if (strcmp(input, "/reset") == 0) {
                printf("Conversation réinitialisée.\n");
                continue;
            } else if (strncmp(input, "/temp ", 6) == 0) {
                temperature = atof(input + 6);
                if (temperature < 0.1f) temperature = 0.1f;
                if (temperature > 2.0f) temperature = 2.0f;
                printf("Température définie à %.2f\n", temperature);
                continue;
            } else if (strncmp(input, "/tokens ", 8) == 0) {
                max_tokens = atoi(input + 8);
                if (max_tokens < 1) max_tokens = 1;
                if (max_tokens > 1024) max_tokens = 1024;
                printf("Max tokens défini à %d\n", max_tokens);
                continue;
            } else if (strncmp(input, "/save ", 6) == 0) {
                jarvis_save_weights(model, input + 6);
                continue;
            } else if (strncmp(input, "/load ", 6) == 0) {
                jarvis_load_weights(model, input + 6);
                continue;
            }
        }
        
        // Ignorer les entrées vides
        if (strlen(input) == 0) continue;
        
        // Générer la réponse
        printf("\033[1;33mJARVIS:\033[0m ");
        fflush(stdout);
        
        // Construire le prompt avec un format conversationnel
        char prompt[8192];
        snprintf(prompt, sizeof(prompt), 
                 "Question: %s\nRéponse:", input);
        
        response = jarvis_generate(model, prompt, max_tokens, temperature, 
                                   JARVIS_TOP_K, JARVIS_TOP_P);
        
        printf("%s\n", response);
        free(response);
    }
}

void benchmark_mode(JarvisModel* model) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    MODE BENCHMARK                            ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    const char* test_prompts[] = {
        "Bonjour, comment allez-vous ?",
        "Expliquez-moi la théorie de la relativité.",
        "Quelle est la capitale de la France ?",
        "Racontez-moi une histoire courte.",
        "Quels sont les bienfaits du sport ?",
        NULL
    };
    
    int num_tokens = 128;
    float total_time = 0.0f;
    
    for (int i = 0; test_prompts[i]; i++) {
        printf("Test %d: \"%s\"\n", i + 1, test_prompts[i]);
        printf("-");
        for (int j = 0; j < 60; j++) printf("-");
        printf("\n");
        
        clock_t start = clock();
        char* response = jarvis_generate(model, test_prompts[i], num_tokens, 
                                          0.8f, 40, 0.9f);
        clock_t end = clock();
        
        double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
        total_time += time_spent;
        
        printf("Réponse: %s\n", response);
        printf("Temps: %.3f secondes\n", time_spent);
        printf("\n");
        
        free(response);
    }
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    RÉSULTATS DU BENCHMARK                    ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Temps total:     %8.3f secondes                         ║\n", total_time);
    printf("║  Tests effectués: %8d                                   ║\n", 5);
    printf("║  Temps moyen:     %8.3f secondes                         ║\n", total_time / 5);
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

void demo_mode(JarvisModel* model) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    MODE DÉMONSTRATION                        ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    const char* demos[] = {
        "Bonjour ! Je suis JARVIS, votre assistant IA.",
        "La France est un pays d'Europe occidentale.",
        "L'intelligence artificielle révolutionne notre monde.",
        "Le français est une langue riche et expressive.",
        NULL
    };
    
    for (int i = 0; demos[i]; i++) {
        printf("\033[1;35m[Démo %d]\033[0m Prompt: \"%s\"\n", i + 1, demos[i]);
        printf("\033[1;33mJARVIS:\033[0m ");
        
        char prompt[1024];
        snprintf(prompt, sizeof(prompt), "%s Continuez:", demos[i]);
        
        char* response = jarvis_generate(model, prompt, 64, 0.8f, 40, 0.9f);
        printf("%s\n\n", response);
        
        free(response);
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    
    print_banner();
    
    // Analyser les arguments
    int mode = 0;  // 0 = interactif, 1 = benchmark, 2 = demo
    const char* weights_file = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--benchmark") == 0 || strcmp(argv[i], "-b") == 0) {
            mode = 1;
        } else if (strcmp(argv[i], "--demo") == 0 || strcmp(argv[i], "-d") == 0) {
            mode = 2;
        } else if (strcmp(argv[i], "--weights") == 0 || strcmp(argv[i], "-w") == 0) {
            if (i + 1 < argc) {
                weights_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  -h, --help       Afficher cette aide\n");
            printf("  -b, --benchmark  Mode benchmark\n");
            printf("  -d, --demo       Mode démonstration\n");
            printf("  -w, --weights    Charger des poids pré-entraînés\n");
            printf("\n");
            return 0;
        }
    }
    
    printf("Initialisation de JARVIS...\n");
    clock_t init_start = clock();
    
    // Créer le modèle
    JarvisModel* model = jarvis_create();
    
    // Initialiser le tokenizer
    tokenizer_init_basic(model->tokenizer);
    
    // Initialiser ou charger les poids
    if (weights_file) {
        if (!jarvis_load_weights(model, weights_file)) {
            printf("Initialisation des poids aléatoires...\n");
            jarvis_init_weights(model);
        }
    } else {
        jarvis_init_weights(model);
    }
    
    clock_t init_end = clock();
    double init_time = (double)(init_end - init_start) / CLOCKS_PER_SEC;
    
    printf("Initialisation terminée en %.3f secondes\n", init_time);
    jarvis_print_info(model);
    
    // Exécuter le mode sélectionné
    switch (mode) {
        case 1:
            benchmark_mode(model);
            break;
        case 2:
            demo_mode(model);
            break;
        default:
            interactive_mode(model);
            break;
    }
    
    // Nettoyage
    printf("Libération de la mémoire...\n");
    jarvis_free(model);
    
    printf("Au revoir !\n");
    return 0;
}
