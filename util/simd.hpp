namespace cmpe492 {

/// aligned vector types
typedef float float4_t __attribute__((vector_size(4 * sizeof(float)), aligned(4 * sizeof(float))));
typedef float float8_t __attribute__((vector_size(8 * sizeof(float)), aligned(8 * sizeof(float))));

/// unaligned (only float-aligned) vector types
typedef float float4_unalgn_t
  __attribute__((vector_size(4 * sizeof(float)), aligned(sizeof(float))));
typedef float float8_unalgn_t
  __attribute__((vector_size(8 * sizeof(float)), aligned(sizeof(float))));

// make sure everything is sized and aligned as expected
static_assert(sizeof(float4_t) == 4 * sizeof(float));
static_assert(sizeof(float8_t) == 8 * sizeof(float));
static_assert(sizeof(float4_unalgn_t) == 4 * sizeof(float));
static_assert(sizeof(float8_unalgn_t) == 8 * sizeof(float));
static_assert(alignof(float4_t) == 4 * sizeof(float));
static_assert(alignof(float8_t) == 8 * sizeof(float));
static_assert(alignof(float4_unalgn_t) == sizeof(float));
static_assert(alignof(float8_unalgn_t) == sizeof(float));

} // namespace cmpe492