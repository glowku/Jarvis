# JARVIS - Just A Rather Very Intelligent System

**JARVIS** est un LLM (Large Language Model) ultra-léger écrit en C pur, optimisé pour la langue française.

## Caractéristiques

- **Ultra-léger** : Conçu pour fonctionner sur des machines avec peu de mémoire
- **Rapide** : Optimisations agressives, support OpenMP
- **Français natif** : Tokenizer optimisé pour la langue française
- **Architecture moderne** : Transformer avec attention multi-têtes
- **Pure C** : Aucune dépendance externe (hors math.h)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     JARVIS LLM v1.0                         │
├─────────────────────────────────────────────────────────────┤
│  Paramètres:                                                │
│    • Vocabulaire:      32,000 tokens                        │
│    • Embedding dim:    512                                  │
│    • Têtes d'attention: 8                                   │
│    • Couches:          6                                    │
│    • FFN dim:          2,048                                │
│    • Longueur max:     512 tokens                           │
├─────────────────────────────────────────────────────────────┤
│  ~100 millions de paramètres (~400 MB en FP32)              │
└─────────────────────────────────────────────────────────────┘
```

## Compilation

### Prérequis

- GCC ou Clang (C11)
- Make
- OpenMP (optionnel mais recommandé)

### Compilation standard

```bash
make
```

### Modes de compilation

```bash
make release    # Optimisations maximales pour production
make debug      # Avec symboles de débogage
make profile    # Pour profilage des performances
```

## Utilisation

### Mode interactif (par défaut)

```bash
./jarvis
```

### Mode benchmark

```bash
./jarvis --benchmark
```

### Mode démonstration

```bash
./jarvis --demo
```

### Commandes interactives

| Commande | Description |
|----------|-------------|
| `/help` | Afficher l'aide |
| `/info` | Informations sur le modèle |
| `/clear` | Effacer l'écran |
| `/reset` | Réinitialiser la conversation |
| `/temp <val>` | Définir la température (0.1-2.0) |
| `/tokens <n>` | Nombre max de tokens à générer |
| `/save <fichier>` | Sauvegarder les poids |
| `/load <fichier>` | Charger les poids |
| `/quit` | Quitter JARVIS |

## Structure du projet

```
jarvis_llm/
├── jarvis.h          # Header avec structures et déclarations
├── jarvis.c          # Implémentation complète du modèle
├── main.c            # Interface utilisateur
├── Makefile          # Système de build
└── README.md         # Documentation
```

## Optimisations

### Matérielles
- Vectorisation SIMD automatique
- Alignement mémoire (64 bytes)
- Cache blocking pour les matrices

### Algorithmiques
- Multiplication matricielle optimisée
- Softmax numériquement stable
- GELU avec approximation rapide

### Parallélisme
- OpenMP pour les boucles parallèles
- Support multi-cœur automatique

## Architecture détaillée

```
Input Tokens
     │
     ▼
┌─────────────────┐
│ Token Embedding │
│  + Positional   │
└────────┬────────┘
         │
    ┌────┴────┐
    │         │
    ▼         ▼
┌─────────┐ ┌─────────┐
│ Layer 1 │ │ Layer N │
│ ┌─────┐ │ │ ┌─────┐ │
│ │Attn │ │ │ │Attn │ │
│ │Norm │ │ │ │Norm │ │
│ ├─────┤ │ │ ├─────┤ │
│ │ FFN │ │ │ │ FFN │ │
│ │Norm │ │ │ │Norm │ │
│ └─────┘ │ │ └─────┘ │
└────┬────┘ └────┬────┘
     │           │
     └─────┬─────┘
           ▼
    ┌─────────────┐
    │ Final Norm  │
    └──────┬──────┘
           ▼
    ┌─────────────┐
    │  LM Head    │
    └──────┬──────┘
           ▼
    Output Logits
```

## Fonctionnalités

### Tokenizer
- BPE (Byte Pair Encoding) simplifié
- Support UTF-8 pour caractères français
- Vocabulaire optimisé pour le français

### Attention
- Multi-head attention
- Masque causal pour génération autoregressive
- Scaling par 1/√d_k

### Génération
- Échantillonnage greedy
- Top-k sampling
- Nucleus sampling (top-p)
- Température configurable

## Performances estimées

| Configuration | Tokens/sec |
|--------------|------------|
| CPU 4 cœurs  | ~50-100    |
| CPU 8 cœurs  | ~100-200   |
| CPU 16 cœurs | ~200-400   |

*Dépend du CPU, de la mémoire cache et des optimisations du compilateur*

## Licence

MIT License - Libre d'utilisation et de modification.

## Auteur

Créé avec passion pour l'IA légère et accessible.

---

**Note** : Ce modèle est initialisé avec des poids aléatoires. Pour des résultats cohérents, il faudrait l'entraîner sur un corpus de texte français ou charger des poids pré-entraînés.
