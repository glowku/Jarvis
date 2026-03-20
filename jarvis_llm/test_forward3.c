#include "jarvis.h"
#include <stdio.h>

int main() {
    printf("Creating model...\n");
    JarvisModel* model = jarvis_create();
    tokenizer_init_basic(model->tokenizer);
    jarvis_init_weights(model);
    
    printf("Encoding...\n");
    int num_tokens;
    int* tokens = tokenizer_encode(model->tokenizer, "Bonjour", &num_tokens);
    printf("Tokens: %d\n", num_tokens);
    
    printf("Creating logits tensor...\n");
    Tensor* logits = tensor_create(num_tokens, model->vocab_size);
    
    printf("Calling jarvis_forward...\n");
    jarvis_forward(model, tokens, num_tokens, logits, NULL);
    printf("Forward done!\n");
    
    printf("Success!\n");
    return 0;
}
