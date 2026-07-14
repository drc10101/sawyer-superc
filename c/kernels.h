/* kernels.h -- int8/int4 integer-dot kernels for Sawyer SuperC
 *
 * Based on Colibri by Vincenzo / JustVugg
 * https://github.com/JustVugg/colibri
 * Licensed under Apache 2.0
 *
 * AVX2 maddubs-based kernels for int8 and packed int4 matrix-vector
 * multiply. Routing between kernels is decided per shape by
 * measurement: int4 single-row stays f32 (it measured slower in int4).
 */

#ifndef KERNELS_H
#define KERNELS_H

#include <stdint.h>
#include <stddef.h>

/* int8 dot product: Q8_0 activations x Q8_0 weights
 * Uses AVX2 maddubs instruction for 2.5x measured speedup
 * over naive f32 on supported hardware.
 */
void kernel_int8_dot(
    const int8_t *activations,  /* [rows] Q8_0 */
    const int8_t *weights,      /* [out_dim * in_dim] Q8_0 */
    float *output,              /* [out_dim] f32 result */
    int in_dim,
    int out_dim,
    const float *scales          /* [out_dim] per-row scales */
);

/* int4 packed dot product: f32 input x packed int4 weights
 * Weights are packed 2-per-byte with per-row float scales.
 * Used for MoE expert forward passes.
 */
void kernel_int4_dot(
    const float *input,          /* [in_dim] f32 */
    const uint8_t *packed_weights, /* [out_dim * in_dim / 2] packed int4 */
    float *output,              /* [out_dim] f32 result */
    int in_dim,
    int out_dim,
    const float *scales          /* [out_dim] per-row scales */
);

/* MLA weight absorption for decode
 * Absorbs kv_b into query projection so per-token k/v
 * reconstruction is skipped. Validated exact against
 * transformers reference.
 */
void kernel_mla_absorb(
    const float *query,          /* [n_heads * head_dim] */
    const float *kv_b,           /* [n_kv_heads * kv_dim] */
    float *absorbed,             /* [n_heads * kv_dim] output */
    int n_heads,
    int n_kv_heads,
    int head_dim,
    int kv_dim
);

/* Detect AVX2 support at runtime */
int kernels_avx2_available(void);

/* Auto-select best kernel family for current hardware */
void kernels_init(void);

#endif /* KERNELS_H */