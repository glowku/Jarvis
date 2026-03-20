#include "jarvis.h"
#include <stdio.h>

int main() {
    printf("Test 1: Creating model...\n");
    JarvisModel* model = jarvis_create();
    printf("Model created!\n");
    
    printf("Test 2: Initializing tokenizer...\n");
    tokenizer_init_basic(model->tokenizer);
    printf("Tokenizer initialized!\n");
    
    printf("Test 3: Initializing weights...\n");
    jarvis_init_weights(model);
    printf("Weights initialized!\n");
    
    printf("Test 4: Print info...\n");
    jarvis_print_info(model);
    
    printf("Test 5: Encoding text...\n");
    int num_tokens;
    int* tokens = tokenizer_encode(model->tokenizer, "Bonjour", &num_tokens);
    printf("Encoded %d tokens\n", num_tokens);
    
    printf("Test 6: Decoding tokens...\n");
    char* decoded = tokenizer_decode(model->tokenizer, tokens, num_tokens);
    printf("Decoded: %s\n", decoded);
    
    printf("All tests passed!\n");
    return 0;
}
