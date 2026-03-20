/**
 * JARVIS - Just A Rather Very Intelligent System
 * LLM Ultra-Léger en C - Optimisé pour la langue française
 * 
 * Architecture: Transformer minimaliste et performant
 * Auteur: Assistant AI
 * Licence: MIT
 */

#ifndef JARVIS_H
#define JARVIS_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

// Définir M_PI si non défini
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

/* ============================================================================
 * CONFIGURATION - Paramètres du modèle
 * ============================================================================ */

// Taille du vocabulaire (optimisé pour français + caractères spéciaux)
#define JARVIS_VOCAB_SIZE 32000

// Dimensions du modèle (ultra-léger)
#define JARVIS_EMBED_DIM 512      // Dimension des embeddings
#define JARVIS_NUM_HEADS 8        // Nombre de têtes d'attention
#define JARVIS_NUM_LAYERS 6       // Nombre de couches transformer
#define JARVIS_FF_DIM 2048        // Dimension feed-forward (4x embed)
#define JARVIS_MAX_SEQ_LEN 512    // Longueur max de séquence
#define JARVIS_HEAD_DIM 64        // Dimension par tête (EMBED_DIM / NUM_HEADS)

// Paramètres d'entraînement/génération
#define JARVIS_LEARNING_RATE 0.0001f
#define JARVIS_DROPOUT_RATE 0.1f
#define JARVIS_TEMPERATURE 0.8f
#define JARVIS_TOP_K 40
#define JARVIS_TOP_P 0.9f

/* ============================================================================
 * STRUCTURES DE DONNÉES
 * ============================================================================ */

// Tensor pour les matrices/vecteurs
typedef struct {
    float* data;
    int rows;
    int cols;
    int size;
} Tensor;

// Tokenizer BPE simplifié
typedef struct {
    char** vocab;
    int* vocab_lengths;
    int vocab_size;
    uint32_t* merges;
    int num_merges;
} Tokenizer;

// Couche d'attention multi-têtes
typedef struct {
    Tensor* w_q;      // Poids Query [embed_dim, embed_dim]
    Tensor* w_k;      // Poids Key [embed_dim, embed_dim]
    Tensor* w_v;      // Poids Value [embed_dim, embed_dim]
    Tensor* w_o;      // Poids Output [embed_dim, embed_dim]
    Tensor* b_q;      // Biais Query
    Tensor* b_k;      // Biais Key
    Tensor* b_v;      // Biais Value
    Tensor* b_o;      // Biais Output
} AttentionLayer;

// Couche Feed-Forward
typedef struct {
    Tensor* w1;       // [embed_dim, ff_dim]
    Tensor* w2;       // [ff_dim, embed_dim]
    Tensor* b1;       // [ff_dim]
    Tensor* b2;       // [embed_dim]
} FFNLayer;

// Normalisation de couche
typedef struct {
    Tensor* gamma;
    Tensor* beta;
    float epsilon;
} LayerNorm;

// Couche Transformer complète
typedef struct {
    AttentionLayer* attn;
    FFNLayer* ffn;
    LayerNorm* norm1;
    LayerNorm* norm2;
} TransformerBlock;

// Modèle Jarvis complet
typedef struct {
    // Embeddings
    Tensor* token_embedding;      // [vocab_size, embed_dim]
    Tensor* position_embedding;   // [max_seq_len, embed_dim]
    
    // Couches Transformer
    TransformerBlock** blocks;
    
    // Normalisation finale
    LayerNorm* final_norm;
    
    // Tête de prédiction
    Tensor* lm_head;              // [embed_dim, vocab_size]
    
    // Tokenizer
    Tokenizer* tokenizer;
    
    // Dimensions
    int vocab_size;
    int embed_dim;
    int num_heads;
    int num_layers;
    int ff_dim;
    int max_seq_len;
    int head_dim;
} JarvisModel;

// Cache KV pour génération efficace
typedef struct {
    Tensor** k_cache;  // [num_layers, max_seq_len, embed_dim]
    Tensor** v_cache;  // [num_layers, max_seq_len, embed_dim]
    int cache_len;
} KVCache;

/* ============================================================================
 * FONCTIONS DE GESTION DES TENSORS
 * ============================================================================ */

Tensor* tensor_create(int rows, int cols);
void tensor_free(Tensor* t);
void tensor_fill(Tensor* t, float value);
void tensor_random_init(Tensor* t, float scale);
void tensor_zeros(Tensor* t);
void tensor_copy(Tensor* dst, const Tensor* src);
void tensor_print(const Tensor* t, const char* name);

// Fonctions inline définies dans le header
static inline float tensor_get(const Tensor* t, int row, int col) {
    return t->data[row * t->cols + col];
}

static inline void tensor_set(Tensor* t, int row, int col, float value) {
    t->data[row * t->cols + col] = value;
}

/* ============================================================================
 * OPÉRATIONS MATRICIELLES OPTIMISÉES
 * ============================================================================ */

// Multiplication matricielle: C = A * B + C (optimisée)
void matmul(const Tensor* A, const Tensor* B, Tensor* C);
void matmul_add_bias(const Tensor* A, const Tensor* B, const Tensor* bias, Tensor* C);

// C = A * B^T
void matmul_transposed_b(const Tensor* A, const Tensor* B, Tensor* C);

// C = A^T * B
void matmul_transposed_a(const Tensor* A, const Tensor* B, Tensor* C);

// Softmax sur la dernière dimension
void softmax(Tensor* x, int axis);
void softmax_inplace(float* x, int size);

// Layer Normalization
void layer_norm(const Tensor* x, const LayerNorm* ln, Tensor* out);

// GELU activation
float gelu(float x);
void gelu_tensor(Tensor* x);

// SwiGLU activation (moderne et efficace)
float swish(float x);
void swish_tensor(Tensor* x);

/* ============================================================================
 * ATTENTION MULTI-TÊTES
 * ============================================================================ */

AttentionLayer* attention_create(int embed_dim, int num_heads);
void attention_free(AttentionLayer* attn);
void attention_forward(const AttentionLayer* attn, const Tensor* x, 
                       const Tensor* mask, Tensor* output, 
                       KVCache* cache, int layer_idx, int pos);

/* ============================================================================
 * FEED-FORWARD NETWORK
 * ============================================================================ */

FFNLayer* ffn_create(int embed_dim, int ff_dim);
void ffn_free(FFNLayer* ffn);
void ffn_forward(const FFNLayer* ffn, const Tensor* x, Tensor* output);

/* ============================================================================
 * COUCHE TRANSFORMER
 * ============================================================================ */

TransformerBlock* transformer_block_create(int embed_dim, int num_heads, int ff_dim);
void transformer_block_free(TransformerBlock* block);
void transformer_block_forward(const TransformerBlock* block, const Tensor* x,
                                const Tensor* mask, Tensor* output,
                                KVCache* cache, int layer_idx, int pos);

/* ============================================================================
 * TOKENIZER
 * ============================================================================ */

Tokenizer* tokenizer_create(int vocab_size);
void tokenizer_free(Tokenizer* tok);
void tokenizer_init_basic(Tokenizer* tok);
int* tokenizer_encode(Tokenizer* tok, const char* text, int* num_tokens);
char* tokenizer_decode(Tokenizer* tok, const int* tokens, int num_tokens);
int tokenizer_get_token_id(Tokenizer* tok, const char* token);

/* ============================================================================
 * MODÈLE JARVIS COMPLET
 * ============================================================================ */

JarvisModel* jarvis_create(void);
void jarvis_free(JarvisModel* model);
void jarvis_init_weights(JarvisModel* model);
void jarvis_forward(JarvisModel* model, const int* tokens, int seq_len,
                    Tensor* logits, KVCache* cache);

/* ============================================================================
 * CACHE KV POUR GÉNÉRATION
 * ============================================================================ */

KVCache* kv_cache_create(int num_layers, int max_seq_len, int embed_dim);
void kv_cache_free(KVCache* cache);
void kv_cache_clear(KVCache* cache);

/* ============================================================================
 * GÉNÉRATION DE TEXTE
 * ============================================================================ */

// Masque causal pour l'attention
Tensor* create_causal_mask(int seq_len);

// Échantillonnage
int sample_greedy(const float* logits, int vocab_size);
int sample_top_k(const float* logits, int vocab_size, int k);
int sample_top_p(const float* logits, int vocab_size, float p, float temperature);

// Génération complète
char* jarvis_generate(JarvisModel* model, const char* prompt,
                      int max_new_tokens, float temperature,
                      int top_k, float top_p);

/* ============================================================================
 * UTILITAIRES
 * ============================================================================ */

void jarvis_print_info(const JarvisModel* model);
float jarvis_get_memory_usage(const JarvisModel* model);
void jarvis_save_weights(JarvisModel* model, const char* filename);
bool jarvis_load_weights(JarvisModel* model, const char* filename);

// Fonctions d'activation rapides
static inline float fast_exp(float x) {
    // Approximation rapide de exp
    union { float f; int i; } u;
    u.i = (int)(12102203.0f * x + 1064866805.0f);
    return u.f;
}

static inline float fast_tanh(float x) {
    // Approximation rapide de tanh
    float x2 = x * x;
    float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
    float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
    return a / b;
}

// Générateur de nombres aléatoires rapide (xorshift)
static inline uint32_t fast_rand(uint32_t* seed) {
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;
    return *seed;
}

static inline float rand_uniform(uint32_t* seed) {
    return (float)fast_rand(seed) / 4294967296.0f;
}

static inline float rand_normal(uint32_t* seed) {
    // Box-Muller transform
    float u1 = rand_uniform(seed);
    float u2 = rand_uniform(seed);
    if (u1 < 1e-7f) u1 = 1e-7f;
    return sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
}

#endif // JARVIS_H
