#include "jarvis.h"
#include <stdio.h>

int main() {
    printf("Creating model...\n");
    JarvisModel* model = jarvis_create();
    printf("Model created\n");
    
    printf("Init tokenizer...\n");
    tokenizer_init_basic(model->tokenizer);
    printf("Tokenizer done\n");
    
    printf("Init weights...\n");
    jarvis_init_weights(model);
    printf("Weights done\n");
    
    printf("Encoding...\n");
    int num_tokens;
    int* tokens = tokenizer_encode(model->tokenizer, "Bonjour", &num_tokens);
    printf("Encoded %d tokens\n", num_tokens);
    for (int i = 0; i < num_tokens; i++) {
        printf("  token[%d] = %d\n", i, tokens[i]);
    }
    
    printf("Creating tensors...\n");
    Tensor* logits = tensor_create(num_tokens, model->vocab_size);
    printf("Logits: %dx%d\n", logits->rows, logits->cols);
    
    // Manual forward pass step by step
    printf("Creating input tensor x...\n");
    Tensor* x = tensor_create(num_tokens, model->embed_dim);
    printf("x: %dx%d\n", x->rows, x->cols);
    
    printf("Filling embeddings...\n");
    for (int i = 0; i < num_tokens; i++) {
        int token_id = tokens[i];
        if (token_id >= model->vocab_size) token_id = 1;
        printf("  Token %d: id=%d\n", i, token_id);
        for (int j = 0; j < model->embed_dim; j++) {
            x->data[i * model->embed_dim + j] = 0.01f;  // Simplified
        }
    }
    printf("Embeddings done\n");
    
    printf("Testing transformer block 0...\n");
    Tensor* next = tensor_create(num_tokens, model->embed_dim);
    printf("next: %dx%d\n", next->rows, next->cols);
    
    printf("Calling transformer_block_forward...\n");
    transformer_block_forward(model->blocks[0], x, NULL, next, NULL, 0, 0);
    printf("Block 0 done!\n");
    
    printf("Success!\n");
    return 0;
}
