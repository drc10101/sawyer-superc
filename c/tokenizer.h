/* tokenizer.h -- Byte-level BPE tokenizer for Sawyer SuperC
 *
 * Based on Colibri by Vincenzo / JustVugg
 * GPT-2-style with Unicode-property regex, 320k merges.
 * Pure C, no dependencies.
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    /* Token vocabulary */
    char **token_text;       /* [vocab_size] token strings */
    int *token_ids;          /* sorted for binary search */
    int vocab_size;

    /* BPE merges (320k) */
    char **merge_left;      /* [n_merges] */
    char **merge_right;     /* [n_merges] */
    int n_merges;

    /* Special tokens */
    int bos_id;
    int eos_id;
    int pad_id;

    /* Regex pattern for pre-tokenization (Unicode properties) */
    char *regex_pattern;
} Tokenizer;

/* Load tokenizer from model directory (tokenizer.json or merges.txt) */
Tokenizer *tokenizer_load(const char *model_path);
void tokenizer_destroy(Tokenizer *tok);

/* Encode text -> token ids. Returns number of tokens written. */
int tokenizer_encode(Tokenizer *tok, const char *text,
                     int *token_ids, int max_tokens);

/* Decode token ids -> text. Returns bytes written (excluding null). */
int tokenizer_decode(Tokenizer *tok, const int *token_ids, int n_tokens,
                     char *output, size_t output_len);

#endif /* TOKENIZER_H */