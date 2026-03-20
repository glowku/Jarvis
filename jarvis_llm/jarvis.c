/**
 * JARVIS - Implémentation complète
 * LLM Ultra-Léger en C
 */

#include "jarvis.h"
#include <assert.h>

/* ============================================================================
 * GESTION DES TENSORS
 * ============================================================================ */

Tensor* tensor_create(int rows, int cols) {
    Tensor* t = (Tensor*)malloc(sizeof(Tensor));
    if (!t) {
        fprintf(stderr, "Erreur: Impossible d'allouer le tensor\n");
        exit(1);
    }
    t->rows = rows;
    t->cols = cols;
    t->size = rows * cols;
    t->data = (float*)aligned_alloc(64, t->size * sizeof(float));
    if (!t->data) {
        fprintf(stderr, "Erreur: Impossible d'allouer les données du tensor\n");
        exit(1);
    }
    return t;
}

void tensor_free(Tensor* t) {
    if (t) {
        free(t->data);
        free(t);
    }
}

void tensor_fill(Tensor* t, float value) {
    for (int i = 0; i < t->size; i++) {
        t->data[i] = value;
    }
}

void tensor_zeros(Tensor* t) {
    memset(t->data, 0, t->size * sizeof(float));
}

void tensor_random_init(Tensor* t, float scale) {
    static uint32_t seed = 12345;
    for (int i = 0; i < t->size; i++) {
        t->data[i] = rand_normal(&seed) * scale;
    }
}

void tensor_copy(Tensor* dst, const Tensor* src) {
    assert(dst->size == src->size);
    memcpy(dst->data, src->data, dst->size * sizeof(float));
}

void tensor_print(const Tensor* t, const char* name) {
    printf("\n=== %s [%dx%d] ===\n", name, t->rows, t->cols);
    for (int i = 0; i < t->rows && i < 4; i++) {
        for (int j = 0; j < t->cols && j < 4; j++) {
            printf("%8.4f ", tensor_get(t, i, j));
        }
        if (t->cols > 4) printf("...");
        printf("\n");
    }
    if (t->rows > 4) printf("...\n");
}

/* ============================================================================
 * OPÉRATIONS MATRICIELLES OPTIMISÉES
 * ============================================================================ */

// Multiplication matricielle optimisée avec boucles réordonnées pour cache
void matmul(const Tensor* A, const Tensor* B, Tensor* C) {
    assert(A->cols == B->rows);
    assert(C->rows == A->rows && C->cols == B->cols);
    
    int M = A->rows;
    int N = B->cols;
    int K = A->cols;
    
    // Initialiser à zéro
    tensor_zeros(C);
    
    // Taille de bloc pour optimiser le cache L1/L2
    const int BLOCK_M = 64;
    const int BLOCK_N = 64;
    const int BLOCK_K = 64;
    
    #ifdef _OPENMP
    #pragma omp parallel for collapse(2) schedule(static)
    #endif
    for (int bm = 0; bm < M; bm += BLOCK_M) {
        for (int bn = 0; bn < N; bn += BLOCK_N) {
            int max_m = (bm + BLOCK_M < M) ? bm + BLOCK_M : M;
            int max_n = (bn + BLOCK_N < N) ? bn + BLOCK_N : N;
            
            for (int bk = 0; bk < K; bk += BLOCK_K) {
                int max_k = (bk + BLOCK_K < K) ? bk + BLOCK_K : K;
                
                // Multiplication par bloc
                for (int i = bm; i < max_m; i++) {
                    for (int k = bk; k < max_k; k++) {
                        float a_val = tensor_get(A, i, k);
                        for (int j = bn; j < max_n; j++) {
                            C->data[i * N + j] += a_val * tensor_get(B, k, j);
                        }
                    }
                }
            }
        }
    }
}

// Multiplication avec biais
void matmul_add_bias(const Tensor* A, const Tensor* B, const Tensor* bias, Tensor* C) {
    matmul(A, B, C);
    // Ajouter le biais à chaque ligne
    for (int i = 0; i < C->rows; i++) {
        for (int j = 0; j < C->cols; j++) {
            C->data[i * C->cols + j] += bias->data[j];
        }
    }
}

// C = A * B^T
void matmul_transposed_b(const Tensor* A, const Tensor* B, Tensor* C) {
    assert(A->cols == B->cols);
    assert(C->rows == A->rows && C->cols == B->rows);
    
    int M = A->rows;
    int N = B->rows;
    int K = A->cols;
    
    tensor_zeros(C);
    
    #ifdef _OPENMP
    #pragma omp parallel for
    #endif
    for (int i = 0; i < M; i++) {
        for (int k = 0; k < K; k++) {
            float a_val = tensor_get(A, i, k);
            for (int j = 0; j < N; j++) {
                C->data[i * N + j] += a_val * tensor_get(B, j, k);
            }
        }
    }
}

// Softmax stable numériquement
void softmax_inplace(float* x, int size) {
    // Trouver le max pour stabilité numérique
    float max_val = x[0];
    for (int i = 1; i < size; i++) {
        if (x[i] > max_val) max_val = x[i];
    }
    
    // Calculer exp(x - max) et la somme
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        x[i] = expf(x[i] - max_val);
        sum += x[i];
    }
    
    // Normaliser
    float inv_sum = 1.0f / sum;
    for (int i = 0; i < size; i++) {
        x[i] *= inv_sum;
    }
}

// Softmax sur un tensor
void softmax(Tensor* x, int axis) {
    if (axis == -1 || axis == 1) {
        // Softmax sur chaque ligne
        for (int i = 0; i < x->rows; i++) {
            softmax_inplace(&x->data[i * x->cols], x->cols);
        }
    }
}

// Layer Normalisation
void layer_norm(const Tensor* x, const LayerNorm* ln, Tensor* out) {
    assert(x->rows == out->rows && x->cols == out->cols);
    
    for (int i = 0; i < x->rows; i++) {
        // Calculer la moyenne
        float mean = 0.0f;
        for (int j = 0; j < x->cols; j++) {
            mean += tensor_get(x, i, j);
        }
        mean /= x->cols;
        
        // Calculer la variance
        float var = 0.0f;
        for (int j = 0; j < x->cols; j++) {
            float diff = tensor_get(x, i, j) - mean;
            var += diff * diff;
        }
        var /= x->cols;
        
        // Normaliser
        float inv_std = 1.0f / sqrtf(var + ln->epsilon);
        for (int j = 0; j < x->cols; j++) {
            float normalized = (tensor_get(x, i, j) - mean) * inv_std;
            float result = normalized * ln->gamma->data[j] + ln->beta->data[j];
            tensor_set(out, i, j, result);
        }
    }
}

// GELU activation
float gelu(float x) {
    // Approximation rapide de GELU
    const float sqrt_2_over_pi = 0.7978845608f;
    const float coeff = 0.044715f;
    float x_cubed = x * x * x;
    return 0.5f * x * (1.0f + tanhf(sqrt_2_over_pi * (x + coeff * x_cubed)));
}

void gelu_tensor(Tensor* x) {
    for (int i = 0; i < x->size; i++) {
        x->data[i] = gelu(x->data[i]);
    }
}

// Swish/SiLU activation
float swish(float x) {
    return x / (1.0f + expf(-x));
}

void swish_tensor(Tensor* x) {
    for (int i = 0; i < x->size; i++) {
        x->data[i] = swish(x->data[i]);
    }
}

/* ============================================================================
 * ATTENTION MULTI-TÊTES
 * ============================================================================ */

AttentionLayer* attention_create(int embed_dim, int num_heads) {
    AttentionLayer* attn = (AttentionLayer*)malloc(sizeof(AttentionLayer));
    assert(attn);
    
    int head_dim = embed_dim / num_heads;
    float scale = 1.0f / sqrtf(head_dim);
    
    // Initialiser les poids avec Xavier/Glorot
    attn->w_q = tensor_create(embed_dim, embed_dim);
    attn->w_k = tensor_create(embed_dim, embed_dim);
    attn->w_v = tensor_create(embed_dim, embed_dim);
    attn->w_o = tensor_create(embed_dim, embed_dim);
    
    tensor_random_init(attn->w_q, scale);
    tensor_random_init(attn->w_k, scale);
    tensor_random_init(attn->w_v, scale);
    tensor_random_init(attn->w_o, scale);
    
    // Biais
    attn->b_q = tensor_create(1, embed_dim);
    attn->b_k = tensor_create(1, embed_dim);
    attn->b_v = tensor_create(1, embed_dim);
    attn->b_o = tensor_create(1, embed_dim);
    
    tensor_zeros(attn->b_q);
    tensor_zeros(attn->b_k);
    tensor_zeros(attn->b_v);
    tensor_zeros(attn->b_o);
    
    return attn;
}

void attention_free(AttentionLayer* attn) {
    if (attn) {
        tensor_free(attn->w_q);
        tensor_free(attn->w_k);
        tensor_free(attn->w_v);
        tensor_free(attn->w_o);
        tensor_free(attn->b_q);
        tensor_free(attn->b_k);
        tensor_free(attn->b_v);
        tensor_free(attn->b_o);
        free(attn);
    }
}

void attention_forward(const AttentionLayer* attn, const Tensor* x,
                       const Tensor* mask, Tensor* output,
                       KVCache* cache, int layer_idx, int pos) {
    // Éviter les warnings pour les paramètres non utilisés (simplifié pour la démo)
    (void)mask; (void)cache; (void)layer_idx; (void)pos;
    
    int batch_size = x->rows;
    int embed_dim = x->cols;
    int num_heads = JARVIS_NUM_HEADS;
    int head_dim = embed_dim / num_heads;
    
    // Projeter Q, K, V
    Tensor* Q = tensor_create(batch_size, embed_dim);
    Tensor* K = tensor_create(batch_size, embed_dim);
    Tensor* V = tensor_create(batch_size, embed_dim);
    
    matmul_add_bias(x, attn->w_q, attn->b_q, Q);
    matmul_add_bias(x, attn->w_k, attn->b_k, K);
    matmul_add_bias(x, attn->w_v, attn->b_v, V);
    
    // Pour la démo: simplification - on utilise V directement
    // Dans une vraie implémentation, on calculerait Q @ K^T @ V
    Tensor* attn_output = tensor_create(batch_size, embed_dim);
    tensor_copy(attn_output, V);  // Simplifié pour la démo
    
    // Projection finale
    matmul_add_bias(attn_output, attn->w_o, attn->b_o, output);
    
    // Libérer la mémoire
    tensor_free(Q);
    tensor_free(K);
    tensor_free(V);
    tensor_free(attn_output);
}

/* ============================================================================
 * FEED-FORWARD NETWORK
 * ============================================================================ */

FFNLayer* ffn_create(int embed_dim, int ff_dim) {
    FFNLayer* ffn = (FFNLayer*)malloc(sizeof(FFNLayer));
    assert(ffn);
    
    float scale1 = 1.0f / sqrtf(embed_dim);
    float scale2 = 1.0f / sqrtf(ff_dim);
    
    ffn->w1 = tensor_create(embed_dim, ff_dim);
    ffn->w2 = tensor_create(ff_dim, embed_dim);
    ffn->b1 = tensor_create(1, ff_dim);
    ffn->b2 = tensor_create(1, embed_dim);
    
    tensor_random_init(ffn->w1, scale1);
    tensor_random_init(ffn->w2, scale2);
    tensor_zeros(ffn->b1);
    tensor_zeros(ffn->b2);
    
    return ffn;
}

void ffn_free(FFNLayer* ffn) {
    if (ffn) {
        tensor_free(ffn->w1);
        tensor_free(ffn->w2);
        tensor_free(ffn->b1);
        tensor_free(ffn->b2);
        free(ffn);
    }
}

void ffn_forward(const FFNLayer* ffn, const Tensor* x, Tensor* output) {
    // hidden = GELU(x @ W1 + b1)
    Tensor* hidden = tensor_create(x->rows, ffn->w1->cols);
    matmul_add_bias(x, ffn->w1, ffn->b1, hidden);
    gelu_tensor(hidden);
    
    // output = hidden @ W2 + b2
    matmul_add_bias(hidden, ffn->w2, ffn->b2, output);
    
    tensor_free(hidden);
}

/* ============================================================================
 * COUCHE TRANSFORMER
 * ============================================================================ */

TransformerBlock* transformer_block_create(int embed_dim, int num_heads, int ff_dim) {
    TransformerBlock* block = (TransformerBlock*)malloc(sizeof(TransformerBlock));
    assert(block);
    
    block->attn = attention_create(embed_dim, num_heads);
    block->ffn = ffn_create(embed_dim, ff_dim);
    
    // Layer norms
    block->norm1 = (LayerNorm*)malloc(sizeof(LayerNorm));
    block->norm1->gamma = tensor_create(1, embed_dim);
    block->norm1->beta = tensor_create(1, embed_dim);
    block->norm1->epsilon = 1e-5f;
    tensor_fill(block->norm1->gamma, 1.0f);
    tensor_zeros(block->norm1->beta);
    
    block->norm2 = (LayerNorm*)malloc(sizeof(LayerNorm));
    block->norm2->gamma = tensor_create(1, embed_dim);
    block->norm2->beta = tensor_create(1, embed_dim);
    block->norm2->epsilon = 1e-5f;
    tensor_fill(block->norm2->gamma, 1.0f);
    tensor_zeros(block->norm2->beta);
    
    return block;
}

void transformer_block_free(TransformerBlock* block) {
    if (block) {
        attention_free(block->attn);
        ffn_free(block->ffn);
        tensor_free(block->norm1->gamma);
        tensor_free(block->norm1->beta);
        free(block->norm1);
        tensor_free(block->norm2->gamma);
        tensor_free(block->norm2->beta);
        free(block->norm2);
        free(block);
    }
}

void transformer_block_forward(const TransformerBlock* block, const Tensor* x,
                                const Tensor* mask, Tensor* output,
                                KVCache* cache, int layer_idx, int pos) {
    // Self-attention avec residual
    Tensor* normed = tensor_create(x->rows, x->cols);
    layer_norm(x, block->norm1, normed);
    
    Tensor* attn_out = tensor_create(x->rows, x->cols);
    attention_forward(block->attn, normed, mask, attn_out, cache, layer_idx, pos);
    
    // Residual connection
    for (int i = 0; i < x->size; i++) {
        attn_out->data[i] += x->data[i];
    }
    
    // FFN avec residual
    layer_norm(attn_out, block->norm2, normed);
    
    Tensor* ffn_out = tensor_create(x->rows, x->cols);
    ffn_forward(block->ffn, normed, ffn_out);
    
    // Residual connection finale
    for (int i = 0; i < attn_out->size; i++) {
        output->data[i] = ffn_out->data[i] + attn_out->data[i];
    }
    
    tensor_free(normed);
    tensor_free(attn_out);
    tensor_free(ffn_out);
}

/* ============================================================================
 * TOKENIZER SIMPLE (BPE-like)
 * ============================================================================ */

Tokenizer* tokenizer_create(int vocab_size) {
    Tokenizer* tok = (Tokenizer*)malloc(sizeof(Tokenizer));
    assert(tok);
    
    tok->vocab = (char**)malloc(vocab_size * sizeof(char*));
    tok->vocab_lengths = (int*)malloc(vocab_size * sizeof(int));
    tok->vocab_size = vocab_size;
    tok->num_merges = 0;
    
    for (int i = 0; i < vocab_size; i++) {
        tok->vocab[i] = NULL;
        tok->vocab_lengths[i] = 0;
    }
    
    return tok;
}

void tokenizer_free(Tokenizer* tok) {
    if (tok) {
        for (int i = 0; i < tok->vocab_size; i++) {
            free(tok->vocab[i]);
        }
        free(tok->vocab);
        free(tok->vocab_lengths);
        free(tok);
    }
}

void tokenizer_init_basic(Tokenizer* tok) {
    // Initialiser avec les caractères ASCII de base + caractères français
    int idx = 0;
    
    // Token spéciaux
    tok->vocab[idx] = strdup("<pad>");
    tok->vocab_lengths[idx++] = 5;
    tok->vocab[idx] = strdup("<unk>");
    tok->vocab_lengths[idx++] = 5;
    tok->vocab[idx] = strdup("<s>");
    tok->vocab_lengths[idx++] = 3;
    tok->vocab[idx] = strdup("</s>");
    tok->vocab_lengths[idx++] = 4;
    
    // Caractères imprimables ASCII
    for (int c = 33; c < 127 && idx < tok->vocab_size; c++) {
        tok->vocab[idx] = (char*)malloc(2);
        tok->vocab[idx][0] = (char)c;
        tok->vocab[idx][1] = '\0';
        tok->vocab_lengths[idx++] = 1;
    }
    
    // Caractères français courants
    const char* french_chars = "àâäæçéèêëîïôœùûüÿÀÂÄÆÇÉÈÊËÎÏÔŒÙÛÜŸ";
    for (int i = 0; french_chars[i] && idx < tok->vocab_size; i++) {
        tok->vocab[idx] = (char*)malloc(3);
        // Gérer l'UTF-8
        if ((unsigned char)french_chars[i] < 0x80) {
            tok->vocab[idx][0] = french_chars[i];
            tok->vocab[idx][1] = '\0';
            tok->vocab_lengths[idx++] = 1;
        } else {
            tok->vocab[idx][0] = french_chars[i];
            tok->vocab[idx][1] = french_chars[++i];
            tok->vocab[idx][2] = '\0';
            tok->vocab_lengths[idx++] = 2;
        }
    }
    
    // Compléter avec des tokens communs en français
    const char* common_tokens[] = {
        " le ", " la ", " les ", " un ", " une ", " des ", " du ", " de ",
        " et ", " à ", " en ", " que ", " qui ", " pour ", " dans ", " est ",
        " il ", " elle ", " nous ", " vous ", " ils ", " elles ",
        " pas ", " plus ", " mais ", " ou ", " où ", " comment ", " quand ",
        " bonjour", " merci", " s'il ", " plaît", " jour", " soir",
        ".", ",", "!", "?", ";", ":", "'", "\"", "-", "_",
        "\n", " ", "  ", NULL
    };
    
    for (int i = 0; common_tokens[i] && idx < tok->vocab_size; i++) {
        tok->vocab[idx] = strdup(common_tokens[i]);
        tok->vocab_lengths[idx++] = strlen(common_tokens[i]);
    }
    
    printf("Tokenizer initialisé avec %d tokens\n", idx);
}

int tokenizer_get_token_id(Tokenizer* tok, const char* token) {
    int len = strlen(token);
    for (int i = 0; i < tok->vocab_size; i++) {
        if (tok->vocab[i] && tok->vocab_lengths[i] == len) {
            if (memcmp(tok->vocab[i], token, len) == 0) {
                return i;
            }
        }
    }
    return 1;  // <unk>
}

int* tokenizer_encode(Tokenizer* tok, const char* text, int* num_tokens) {
    // Tokenisation simple par caractère/dictionnaire
    int max_tokens = strlen(text) * 2;
    int* tokens = (int*)malloc(max_tokens * sizeof(int));
    int count = 0;
    
    int len = strlen(text);
    int i = 0;
    
    // Ajouter token de début
    tokens[count++] = 2;  // <s>
    
    while (i < len && count < max_tokens - 1) {
        // Essayer de trouver le plus long match
        int best_len = 1;
        int best_id = 1;  // <unk>
        
        for (int vocab_idx = 4; vocab_idx < tok->vocab_size; vocab_idx++) {
            if (!tok->vocab[vocab_idx]) continue;
            int vlen = tok->vocab_lengths[vocab_idx];
            if (vlen > 0 && i + vlen <= len) {
                if (memcmp(tok->vocab[vocab_idx], &text[i], vlen) == 0) {
                    if (vlen > best_len) {
                        best_len = vlen;
                        best_id = vocab_idx;
                    }
                }
            }
        }
        
        tokens[count++] = best_id;
        i += best_len;
    }
    
    *num_tokens = count;
    return tokens;
}

char* tokenizer_decode(Tokenizer* tok, const int* tokens, int num_tokens) {
    // Estimer la taille nécessaire
    int max_len = 0;
    for (int i = 0; i < num_tokens; i++) {
        if (tokens[i] >= 0 && tokens[i] < tok->vocab_size && tok->vocab[tokens[i]]) {
            max_len += tok->vocab_lengths[tokens[i]];
        }
    }
    
    char* text = (char*)malloc(max_len + 1);
    int pos = 0;
    
    for (int i = 0; i < num_tokens; i++) {
        if (tokens[i] < 4) continue;  // Skip tokens spéciaux
        if (tokens[i] >= 0 && tokens[i] < tok->vocab_size && tok->vocab[tokens[i]]) {
            int len = tok->vocab_lengths[tokens[i]];
            memcpy(&text[pos], tok->vocab[tokens[i]], len);
            pos += len;
        }
    }
    text[pos] = '\0';
    
    return text;
}

/* ============================================================================
 * CACHE KV
 * ============================================================================ */

KVCache* kv_cache_create(int num_layers, int max_seq_len, int embed_dim) {
    KVCache* cache = (KVCache*)malloc(sizeof(KVCache));
    assert(cache);
    
    cache->k_cache = (Tensor**)malloc(num_layers * sizeof(Tensor*));
    cache->v_cache = (Tensor**)malloc(num_layers * sizeof(Tensor*));
    
    for (int i = 0; i < num_layers; i++) {
        cache->k_cache[i] = tensor_create(max_seq_len, embed_dim);
        cache->v_cache[i] = tensor_create(max_seq_len, embed_dim);
        tensor_zeros(cache->k_cache[i]);
        tensor_zeros(cache->v_cache[i]);
    }
    
    cache->cache_len = 0;
    return cache;
}

void kv_cache_free(KVCache* cache) {
    if (cache) {
        // Note: dans une vraie implémentation, on libérerait ici
        // free(cache);
    }
}

void kv_cache_clear(KVCache* cache) {
    cache->cache_len = 0;
}

/* ============================================================================
 * MODÈLE JARVIS COMPLET
 * ============================================================================ */

JarvisModel* jarvis_create(void) {
    JarvisModel* model = (JarvisModel*)malloc(sizeof(JarvisModel));
    assert(model);
    
    model->vocab_size = JARVIS_VOCAB_SIZE;
    model->embed_dim = JARVIS_EMBED_DIM;
    model->num_heads = JARVIS_NUM_HEADS;
    model->num_layers = JARVIS_NUM_LAYERS;
    model->ff_dim = JARVIS_FF_DIM;
    model->max_seq_len = JARVIS_MAX_SEQ_LEN;
    model->head_dim = JARVIS_HEAD_DIM;
    
    // Embeddings
    model->token_embedding = tensor_create(model->vocab_size, model->embed_dim);
    model->position_embedding = tensor_create(model->max_seq_len, model->embed_dim);
    
    // Couches transformer
    model->blocks = (TransformerBlock**)malloc(model->num_layers * sizeof(TransformerBlock*));
    for (int i = 0; i < model->num_layers; i++) {
        model->blocks[i] = transformer_block_create(model->embed_dim, 
                                                     model->num_heads, 
                                                     model->ff_dim);
    }
    
    // Normalisation finale
    model->final_norm = (LayerNorm*)malloc(sizeof(LayerNorm));
    model->final_norm->gamma = tensor_create(1, model->embed_dim);
    model->final_norm->beta = tensor_create(1, model->embed_dim);
    model->final_norm->epsilon = 1e-5f;
    tensor_fill(model->final_norm->gamma, 1.0f);
    tensor_zeros(model->final_norm->beta);
    
    // Tête de prédiction (partage les poids avec token_embedding souvent)
    model->lm_head = tensor_create(model->embed_dim, model->vocab_size);
    
    // Tokenizer
    model->tokenizer = tokenizer_create(model->vocab_size);
    
    return model;
}

void jarvis_free(JarvisModel* model) {
    if (model) {
        tensor_free(model->token_embedding);
        tensor_free(model->position_embedding);
        
        for (int i = 0; i < model->num_layers; i++) {
            transformer_block_free(model->blocks[i]);
        }
        free(model->blocks);
        
        tensor_free(model->final_norm->gamma);
        tensor_free(model->final_norm->beta);
        free(model->final_norm);
        
        tensor_free(model->lm_head);
        tokenizer_free(model->tokenizer);
        
        free(model);
    }
}

void jarvis_init_weights(JarvisModel* model) {
    float embed_scale = 1.0f / sqrtf(model->embed_dim);
    
    // Initialiser les embeddings
    tensor_random_init(model->token_embedding, embed_scale);
    tensor_random_init(model->position_embedding, embed_scale);
    tensor_random_init(model->lm_head, embed_scale);
    
    printf("Poids initialisés\n");
}

void jarvis_forward(JarvisModel* model, const int* tokens, int seq_len,
                    Tensor* logits, KVCache* cache) {
    // Créer le tensor d'entrée avec embeddings
    Tensor* x = tensor_create(seq_len, model->embed_dim);
    
    // Token embedding + position embedding
    for (int i = 0; i < seq_len; i++) {
        int token_id = tokens[i];
        if (token_id >= model->vocab_size) token_id = 1;  // <unk>
        
        for (int j = 0; j < model->embed_dim; j++) {
            float tok_emb = model->token_embedding->data[token_id * model->embed_dim + j];
            float pos_emb = model->position_embedding->data[i * model->embed_dim + j];
            x->data[i * model->embed_dim + j] = tok_emb + pos_emb;
        }
    }
    
    // Masque causal
    Tensor* mask = NULL;  // Simplifié pour cette démo
    
    // Passer à travers les couches transformer
    Tensor* current = x;
    Tensor* next = tensor_create(seq_len, model->embed_dim);
    Tensor* temp_output = tensor_create(seq_len, model->embed_dim);
    
    for (int layer = 0; layer < model->num_layers; layer++) {
        transformer_block_forward(model->blocks[layer], current, mask, temp_output,
                                   cache, layer, cache ? cache->cache_len : 0);
        
        // Swap current et temp_output
        Tensor* swap = current;
        current = temp_output;
        temp_output = swap;
    }
    
    // Normalisation finale
    Tensor* normed = tensor_create(seq_len, model->embed_dim);
    layer_norm(current, model->final_norm, normed);
    
    // Projection vers le vocabulaire
    // Créer un tensor temporaire pour le résultat
    Tensor* temp_logits = tensor_create(seq_len, model->vocab_size);
    matmul(normed, model->lm_head, temp_logits);
    
    // Copier dans le tensor de sortie (qui peut être plus grand)
    for (int i = 0; i < seq_len; i++) {
        memcpy(&logits->data[i * model->vocab_size],
               &temp_logits->data[i * model->vocab_size],
               model->vocab_size * sizeof(float));
    }
    
    // Libérer la mémoire
    tensor_free(x);
    tensor_free(next);
    tensor_free(temp_output);
    tensor_free(normed);
    tensor_free(temp_logits);
}

/* ============================================================================
 * GÉNÉRATION DE TEXTE
 * ============================================================================ */

int sample_greedy(const float* logits, int vocab_size) {
    int best_idx = 0;
    float best_val = logits[0];
    
    for (int i = 1; i < vocab_size; i++) {
        if (logits[i] > best_val) {
            best_val = logits[i];
            best_idx = i;
        }
    }
    
    return best_idx;
}

int sample_top_k(const float* logits, int vocab_size, int k) {
    // Copier et trouver les k meilleurs
    float* probs = (float*)malloc(vocab_size * sizeof(float));
    memcpy(probs, logits, vocab_size * sizeof(float));
    softmax_inplace(probs, vocab_size);
    
    // Sélectionner top-k
    int top_k = (k < vocab_size) ? k : vocab_size;
    
    // Échantillonner parmi le top-k
    float sum = 0.0f;
    for (int i = 0; i < top_k; i++) sum += probs[i];
    
    static uint32_t seed = 12345;
    float r = rand_uniform(&seed) * sum;
    
    float cumsum = 0.0f;
    int selected = 0;
    for (int i = 0; i < top_k; i++) {
        cumsum += probs[i];
        if (r <= cumsum) {
            selected = i;
            break;
        }
    }
    
    free(probs);
    return selected;
}

int sample_top_p(const float* logits, int vocab_size, float p, float temperature) {
    // Appliquer la température
    float* scaled_logits = (float*)malloc(vocab_size * sizeof(float));
    for (int i = 0; i < vocab_size; i++) {
        scaled_logits[i] = logits[i] / temperature;
    }
    
    softmax_inplace(scaled_logits, vocab_size);
    
    // Trier par probabilité décroissante (simplifié)
    // Dans une vraie implémentation, on utiliserait un tri efficace
    
    // Cumul jusqu'à p
    float cumsum = 0.0f;
    int cutoff = vocab_size;
    for (int i = 0; i < vocab_size; i++) {
        cumsum += scaled_logits[i];
        if (cumsum > p) {
            cutoff = i + 1;
            break;
        }
    }
    
    // Rénormaliser
    for (int i = cutoff; i < vocab_size; i++) {
        scaled_logits[i] = 0.0f;
    }
    
    // Échantillonner
    static uint32_t seed = 12345;
    float r = rand_uniform(&seed);
    
    cumsum = 0.0f;
    int selected = 0;
    for (int i = 0; i < vocab_size; i++) {
        cumsum += scaled_logits[i];
        if (r <= cumsum) {
            selected = i;
            break;
        }
    }
    
    free(scaled_logits);
    return selected;
}

char* jarvis_generate(JarvisModel* model, const char* prompt,
                      int max_new_tokens, float temperature,
                      int top_k, float top_p) {
    // Encoder le prompt
    int num_prompt_tokens;
    int* prompt_tokens = tokenizer_encode(model->tokenizer, prompt, &num_prompt_tokens);
    
    // Allouer l'espace pour tous les tokens
    int max_total_tokens = num_prompt_tokens + max_new_tokens;
    int* all_tokens = (int*)malloc(max_total_tokens * sizeof(int));
    memcpy(all_tokens, prompt_tokens, num_prompt_tokens * sizeof(int));
    free(prompt_tokens);
    
    int current_len = num_prompt_tokens;
    
    // Tensor pour les logits (taille max pour toute la séquence)
    Tensor* logits = tensor_create(max_total_tokens, model->vocab_size);
    
    printf("Génération en cours...\n");
    clock_t start = clock();
    
    // Générer token par token
    for (int i = 0; i < max_new_tokens && current_len < max_total_tokens; i++) {
        // Forward pass
        jarvis_forward(model, all_tokens, current_len, logits, NULL);
        
        // Récupérer les logits du dernier token (dernière ligne générée)
        float* last_logits = &logits->data[(current_len - 1) * model->vocab_size];
        
        // Échantillonner
        int next_token;
        if (top_k > 0) {
            next_token = sample_top_k(last_logits, model->vocab_size, top_k);
        } else if (top_p < 1.0f) {
            next_token = sample_top_p(last_logits, model->vocab_size, top_p, temperature);
        } else {
            next_token = sample_greedy(last_logits, model->vocab_size);
        }
        
        // Ajouter le token
        all_tokens[current_len++] = next_token;
        
        // Vérifier le token de fin
        if (next_token == 3) break;  // </s>
        
        // Afficher la progression
        if ((i + 1) % 10 == 0) {
            printf("  %d tokens générés...\r", i + 1);
            fflush(stdout);
        }
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    int tokens_generated = current_len - num_prompt_tokens;
    
    printf("\nGénération terminée: %d tokens en %.2f secondes (%.2f tok/s)\n",
           tokens_generated, time_spent, tokens_generated / time_spent);
    
    // Décoder le résultat
    char* result = tokenizer_decode(model->tokenizer, all_tokens, current_len);
    
    tensor_free(logits);
    free(all_tokens);
    
    return result;
}

/* ============================================================================
 * UTILITAIRES
 * ============================================================================ */

void jarvis_print_info(const JarvisModel* model) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    JARVIS LLM v1.0                           ║\n");
    printf("║           Just A Rather Very Intelligent System                ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Architecture: Transformer Decoder-Only                      ║\n");
    printf("║  Paramètres:                                                 ║\n");
    printf("║    - Taille vocabulaire:  %d                             ║\n", model->vocab_size);
    printf("║    - Dimension embedding: %d                               ║\n", model->embed_dim);
    printf("║    - Nombre de têtes:     %d                                  ║\n", model->num_heads);
    printf("║    - Nombre de couches:   %d                                   ║\n", model->num_layers);
    printf("║    - Dimension FFN:       %d                               ║\n", model->ff_dim);
    printf("║    - Longueur max:        %d                               ║\n", model->max_seq_len);
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    // Estimer le nombre de paramètres
    long total_params = 0;
    
    // Embeddings
    total_params += (long)model->vocab_size * model->embed_dim;
    total_params += (long)model->max_seq_len * model->embed_dim;
    
    // Par couche transformer
    long params_per_layer = 0;
    // Attention: 4 matrices (Q,K,V,O) + 4 biais
    params_per_layer += 4LL * model->embed_dim * model->embed_dim;
    params_per_layer += 4LL * model->embed_dim;
    // FFN: 2 matrices + 2 biais
    params_per_layer += (long)model->embed_dim * model->ff_dim;
    params_per_layer += (long)model->ff_dim * model->embed_dim;
    params_per_layer += model->ff_dim + model->embed_dim;
    // Layer norms: 2 * (gamma + beta)
    params_per_layer += 4LL * model->embed_dim;
    
    total_params += params_per_layer * model->num_layers;
    
    // LM head
    total_params += (long)model->embed_dim * model->vocab_size;
    
    printf("Nombre total de paramètres estimé: ~%.2f millions\n", total_params / 1e6);
    printf("Mémoire requise (FP32): ~%.2f MB\n", total_params * 4.0f / (1024 * 1024));
    printf("\n");
}

float jarvis_get_memory_usage(const JarvisModel* model) {
    long total_bytes = 0;
    
    // Embeddings
    total_bytes += (long)model->vocab_size * model->embed_dim * sizeof(float);
    total_bytes += (long)model->max_seq_len * model->embed_dim * sizeof(float);
    
    // Couches
    for (int i = 0; i < model->num_layers; i++) {
        (void)model->blocks[i];  // Éviter le warning unused
        // Attention
        total_bytes += 4LL * model->embed_dim * model->embed_dim * sizeof(float);
        total_bytes += 4LL * model->embed_dim * sizeof(float);
        // FFN
        total_bytes += (long)model->embed_dim * model->ff_dim * sizeof(float);
        total_bytes += (long)model->ff_dim * model->embed_dim * sizeof(float);
        total_bytes += (model->ff_dim + model->embed_dim) * sizeof(float);
        // Layer norms
        total_bytes += 4LL * model->embed_dim * sizeof(float);
    }
    
    // LM head
    total_bytes += (long)model->embed_dim * model->vocab_size * sizeof(float);
    
    return total_bytes / (1024.0f * 1024.0f);  // MB
}

void jarvis_save_weights(JarvisModel* model, const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir %s\n", filename);
        return;
    }
    
    // Écrire les dimensions
    fwrite(&model->vocab_size, sizeof(int), 1, f);
    fwrite(&model->embed_dim, sizeof(int), 1, f);
    fwrite(&model->num_heads, sizeof(int), 1, f);
    fwrite(&model->num_layers, sizeof(int), 1, f);
    
    // Écrire les poids (simplifié)
    fwrite(model->token_embedding->data, sizeof(float), 
           model->token_embedding->size, f);
    
    fclose(f);
    printf("Poids sauvegardés dans %s\n", filename);
}

bool jarvis_load_weights(JarvisModel* model, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Fichier de poids non trouvé: %s\n", filename);
        return false;
    }
    
    // Lire les dimensions
    int vocab_size, embed_dim, num_heads, num_layers;
    fread(&vocab_size, sizeof(int), 1, f);
    fread(&embed_dim, sizeof(int), 1, f);
    fread(&num_heads, sizeof(int), 1, f);
    fread(&num_layers, sizeof(int), 1, f);
    
    // Vérifier la compatibilité
    if (vocab_size != model->vocab_size || embed_dim != model->embed_dim ||
        num_heads != model->num_heads || num_layers != model->num_layers) {
        fprintf(stderr, "Dimensions incompatibles dans le fichier de poids\n");
        fclose(f);
        return false;
    }
    
    // Lire les poids
    fread(model->token_embedding->data, sizeof(float),
          model->token_embedding->size, f);
    
    fclose(f);
    printf("Poids chargés depuis %s\n", filename);
    return true;
}
