// Copyright 2023 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <cstddef>

#include <wasm_simd128.h>

#include <xnnpack/assembler.h>
#include <xnnpack/microparams.h>
#include <xnnpack/post-operation.h>
#include <xnnpack/wasm-assembler.h>
#include <xnnpack/wasmsimd-gemm-igemm-commons.h>


namespace xnnpack {
namespace {
class F32IGemmSplatGenerator : public internal::GemmIGemmCommons {
 public:
  using GemmIGemmCommons::GemmIGemmCommons;
  void generate(const char* name, size_t max_mr, size_t iters, size_t k_per_iteration, size_t full_unroll,
                size_t nc_mod_nr, const jit_gemm_params* jit_gemm_params) {
    ValTypesToInt locals_declaration = {{i32, max_mr * 2 + 5}, {v128, max_mr * 3 + 16}};
    AddFunc<12>({}, name, locals_declaration,
                [&](auto mr, auto nc, auto kc, const auto ks, auto a, auto w, auto c, auto cm_stride, auto cn_stride,
                    auto a_offset, auto zero, auto params) {
                  InitPostOps(jit_gemm_params, params);

                  LocalsArray cs = MakeLocalsArray(max_mr, i32);
                  ClampCs(cs, mr, c, cm_stride);

                  LocalsArray vacc0123 = MakeLocalsArray(max_mr, v128);
                  LocalsArray vacc4567 = MakeLocalsArray(max_mr, v128);

                  DoWhile(
                    [&] {
                      InitAccumulators(vacc0123, w, /*offset=*/0);
                      InitAccumulators(vacc4567, w, /*offset=*/sizeof(v128_t));

                      w = I32Add(w, I32Const(8 * sizeof(float)));

                      Local p = MakeLocal(ks);

                      DoWhile(
                        [&] {
                          LocalsArray as = MakeLocalsArray(max_mr, i32);
                          for (size_t i = 0; i < max_mr; i++) {
                            as[i] = I32Load(a, /*offset=*/i * sizeof(void*));
                            as[i] = Select(I32Add(as[i], a_offset), as[i], I32Ne(as[i], zero));
                          }

                          InnerLoop(as, vacc0123, vacc4567, w, kc, max_mr, k_per_iteration, iters, full_unroll);

                          a = I32Add(a, I32Const(max_mr * sizeof(void*)));
                          p = I32Sub(p, I32Const(max_mr * sizeof(void*)));
                        },
                        [&] { I32NeZ(p); });

                      ApplyPostOps(vacc0123);
                      ApplyPostOps(vacc4567);

                      IfElse([&] { I32GeU(nc, I32Const(8)); },
                             [&] {
                               for (int i = max_mr - 1; i >= 0; i--) {
                                 V128Store(cs[i], vacc0123[i]);
                                 V128Store(cs[i], vacc4567[i], /*offset=*/sizeof(v128_t));
                                 cs[i] = I32Add(cs[i], cn_stride);
                               }
                               a = I32Sub(a, ks);
                               nc = I32Sub(nc, I32Const(8));
                             },
                             [&] {
                               If([&] { I32And(nc, I32Const(4)); },
                                  [&] {
                                    for (int i = max_mr - 1; i >= 0; i--) {
                                      V128Store(cs[i], vacc0123[i]);
                                      vacc0123[i] = vacc4567[i];
                                      cs[i] = I32Add(cs[i], I32Const(sizeof(v128_t)));
                                    }
                                  });
                               If([&] { I32And(nc, I32Const(2)); },
                                  [&] {
                                    for (int i = max_mr - 1; i >= 0; i--) {
                                      V128Store64Lane(cs[i], vacc0123[i], 0);
                                      vacc0123[i] = I64x2Shuffle(vacc0123[i], vacc0123[i], {1, 1});
                                      cs[i] = I32Add(cs[i], I32Const(2 * sizeof(float)));
                                    }
                                  });
                               If([&] { I32And(nc, I32Const(1)); },
                                  [&] {
                                    for (int i = max_mr - 1; i >= 0; i--) {
                                      V128Store32Lane(cs[i], vacc0123[i], 0);
                                    }
                                  });
                               Return();
                             });
                    },
                    [&] { I32NeZ(nc); });
                });
  }
 private:
  static size_t IterationsInMainLoop(size_t k_const) { return (k_const / 4) * 4; }

  void InnerLoop(LocalsArray& as, LocalsArray& vacc0123, LocalsArray& vacc4567, Local& w, Local& kc, size_t max_mr,
                 size_t k_per_iteration, size_t k_const, bool full_unroll) {
    if (full_unroll) {
      InnerLoopFullUnroll(as, vacc0123, vacc4567, w, kc, max_mr, k_const);
    } else {
      InnerLoopPartialUnroll(as, vacc0123, vacc4567, w, kc, max_mr, k_per_iteration, k_const);
    }
  }

  void InnerLoopFullUnroll(LocalsArray& as, LocalsArray& vacc0123, LocalsArray& vacc4567, Local& w, Local& kc,
                           size_t max_mr, size_t k_const) {
    Local k = MakeLocal(kc);
    if (k_const >= 4) {
      InnerLoopFullUnrollNoRemainder(as, vacc0123, vacc4567, w, k, max_mr, IterationsInMainLoop(k_const));
    }
    Remainder(as, vacc0123, vacc4567, k, w, k_const, max_mr);
  }

  void InnerLoopPartialUnroll(LocalsArray& as, LocalsArray& vacc0123, LocalsArray& vacc4567, Local& w, Local& kc,
                              size_t max_mr, size_t k_per_iteration, size_t k_const) {
    Local k = MakeLocal(kc);
    if (k_const >= 4) {
      InnerLoopPartialUnrollNoRemainder(as, vacc0123, vacc4567, w, k, max_mr, k_per_iteration);
      InnerLoopFullUnrollNoRemainder(as, vacc0123, vacc4567, w, k, max_mr,
                                     IterationsInMainLoop(k_const) % k_per_iteration);
    }

    Remainder(as, vacc0123, vacc4567, k, w, k_const, max_mr);
  }

  void Remainder(LocalsArray& as, LocalsArray& vacc0123, LocalsArray& vacc4567, Local& k, Local& w, size_t k_const,
                 size_t max_mr) {
    const size_t remainder_k = k_const % 4;
    if (remainder_k & 2) {
      k = I32Const(2 * sizeof(float));
      DoWhile(
        [&] {
          RemainderIteration(as, vacc0123, vacc4567, k, w, k_const, max_mr);
          k = I32Sub(k, I32Const(sizeof(float)));
        },
        [&] { I32NeZ(k); });
    }
    if (remainder_k & 1) {
      RemainderIteration(as, vacc0123, vacc4567, k, w, k_const, max_mr);
    }
  }

  void RemainderIteration(LocalsArray& as, LocalsArray& vacc0123, LocalsArray& vacc4567, Local& k, Local& w,
                          size_t k_const, size_t max_mr) {
    auto vas = MakeLocalsArray(max_mr, v128);
    for (size_t i = 0; i < max_mr; i++) {
      vas[i] = V128Load32Splat(as[i]);
      as[i] = I32Add(as[i], I32Const(sizeof(float)));
    }

    auto vb0123 = MakeLocal(v128);
    auto vb4567 = MakeLocal(v128);
    LoadVbs(vb0123, vb4567, w, /*offset=*/0);
    w = I32Add(w, I32Const(8 * sizeof(float)));

    MulAdd(vacc0123, vas, vb0123, max_mr);
    MulAdd(vacc4567, vas, vb4567, max_mr);
  }

  void InnerLoopFullUnrollNoRemainder(LocalsArray& as, LocalsArray& vacc0123, LocalsArray& vacc4567, Local& w, Local& k,
                                      size_t max_mr, size_t k_const) {
    if (k_const / 4 == 0) return;
    for (size_t iter = 0; iter < k_const / 4; iter++) {
      auto vas = MakeVas(as, max_mr);
      LoopComputation(vas, vacc0123, vacc4567, w, max_mr, iter * 32 * sizeof(float));
    }
    w = I32Add(w, I32Const(k_const * 8 * sizeof(float)));
    k = I32Sub(k, I32Const(k_const * sizeof(float)));
  }

  void InnerLoopPartialUnrollNoRemainder(LocalsArray& as, LocalsArray& vacc0123, LocalsArray& vacc4567, Local& w,
                                         Local& k, size_t max_mr, size_t k_per_iteration) {
    While([&] { I32GeU(k, I32Const(k_per_iteration * sizeof(float))); },
          [&] {
            for (size_t iter = 0; iter < k_per_iteration / 4; iter++) {
              auto vas = MakeVas(as, max_mr);
              LoopComputation(vas, vacc0123, vacc4567, w, max_mr, iter * 32 * sizeof(float));
            }

            w = I32Add(w, I32Const(k_per_iteration * 8 * sizeof(float)));
            k = I32Sub(k, I32Const(k_per_iteration * sizeof(float)));
          });
  }

  void LoopComputation(const LocalsArray& vas, LocalsArray& vacc0123, LocalsArray& vacc4567, const Local& w,
                       size_t max_mr, uint32_t w_offset) {
    auto vb0123 = MakeLocal(v128);
    auto vb4567 = MakeLocal(v128);

    for (int c = 0; c < 4; c++) {
      LocalsArray vas_shuffled = ShuffleVas(vas, c);
      LoadVbs(vb0123, vb4567, w, w_offset, c);
      MulAdd(vacc0123, vas_shuffled, vb0123, max_mr);
      MulAdd(vacc4567, vas_shuffled, vb4567, max_mr);
    }
  }

  LocalsArray ShuffleVas(const LocalsArray& vas, size_t c) {
    auto vas_shuffled = MakeLocalsArray(vas.size(), v128);
    const auto c_narrow = static_cast<uint8_t>(c);
    for (size_t i = 0; i < vas.size(); i++) {
      vas_shuffled[i] = I32x4Shuffle(vas[i], vas[i], {c_narrow, c_narrow, c_narrow, c_narrow});
    }
    return vas_shuffled;
  }

  LocalsArray MakeVas(LocalsArray& as, size_t max_mr) {
    auto vas = MakeLocalsArray(max_mr, v128);
    for (size_t i = 0; i < max_mr; i++) {
      vas[i] = V128Load(as[i]);
      as[i] = I32Add(as[i], I32Const(4 * sizeof(float)));
    }
    return vas;
  }

};

constexpr auto generate = internal::generate_gemm_or_igemm<F32IGemmSplatGenerator>;
}  // namespace
}  // namespace xnnpack

extern "C" {
xnn_status_t xnn_generate_f32_igemm_ukernel_6x8__wasmsimd32_x86_splat_x1(xnn_code_buffer* b, size_t max_mr,
                                                                     size_t nc_mod_nr, size_t kc, size_t ks,
                                                                     const void* params) {
  static const char* kFunctionName = "xnn_generate_f32_igemm_ukernel_6x8__wasmsimd_x86_splat_x1";
  assert(max_mr <= 6);
  return xnnpack::generate(b, kFunctionName, max_mr, kc, 4, /*full_unroll=*/false, nc_mod_nr, params);
}

xnn_status_t xnn_generate_f32_igemm_ukernel_6x8__wasmsimd32_x86_splat_x2(xnn_code_buffer* b, size_t max_mr,
                                                                     size_t nc_mod_nr, size_t kc, size_t ks,
                                                                     const void* params) {
  static const char* kFunctionName = "xnn_generate_f32_igemm_ukernel_6x8__wasmsimd_x86_splat_x2";
  assert(max_mr <= 6);
  return xnnpack::generate(b, kFunctionName, max_mr, kc, /*k_per_iteration=*/8, /*full_unroll=*/false, nc_mod_nr,
                           params);
}

xnn_status_t xnn_generate_f32_igemm_ukernel_6x8__wasmsimd32_x86_splat_x4(xnn_code_buffer* b, size_t max_mr,
                                                                     size_t nc_mod_nr, size_t kc, size_t ks,
                                                                     const void* params) {
  static const char* kFunctionName = "xnn_generate_f32_igemm_ukernel_6x8__wasmsimd_x86_splat_x4";
  assert(max_mr <= 6);
  return xnnpack::generate(b, kFunctionName, max_mr, kc, /*k_per_iteration=*/16, /*full_unroll=*/false, nc_mod_nr,
                           params);
}

xnn_status_t xnn_generate_f32_igemm_ukernel_6x8__wasmsimd32_x86_splat_xinf(xnn_code_buffer* b, size_t max_mr,
                                                                       size_t nc_mod_nr, size_t kc, size_t ks,
                                                                       const void* params) {
  static const char* kFunctionName = "xnn_generate_f32_igemm_ukernel_6x8__wasmsimd_x86_splat_xinf";
  assert(max_mr <= 6);
  return xnnpack::generate(b, kFunctionName, max_mr, kc, /*k_per_iteration=*/kc / sizeof(float), /*full_unroll=*/true,
                           nc_mod_nr, params);
}
}
