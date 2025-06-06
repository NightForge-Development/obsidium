// Auto-generated file. Do not edit!
//   Template: src/f32-gemm/wasmsimd-s4.c.in
//   Generator: tools/xngen
//
// Copyright 2020 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>

#include <wasm_simd128.h>

#include <xnnpack/gemm.h>


void xnn_f32_qc8w_gemm_minmax_ukernel_6x8s4__wasmsimd_arm(
    size_t mr,
    size_t nc,
    size_t kc,
    const float* restrict a,
    size_t a_stride,
    const void* restrict w,
    float* restrict c,
    size_t cm_stride,
    size_t cn_stride,
    const union xnn_f32_minmax_params params[restrict XNN_MIN_ELEMENTS(1)])
{
  assert(mr != 0);
  assert(mr <= 6);
  assert(nc != 0);
  assert(kc != 0);
  assert(kc % sizeof(float) == 0);
  assert(a != NULL);
  assert(w != NULL);
  assert(c != NULL);

  const float* a0 = a;
  float* c0 = c;
  const float* a1 = (const float*) ((uintptr_t) a0 + a_stride);
  float* c1 = (float*) ((uintptr_t) c0 + cm_stride);
  if XNN_UNPREDICTABLE(mr < 2) {
    a1 = a0;
    c1 = c0;
  }
  const float* a2 = (const float*) ((uintptr_t) a1 + a_stride);
  float* c2 = (float*) ((uintptr_t) c1 + cm_stride);
  if XNN_UNPREDICTABLE(mr <= 2) {
    a2 = a1;
    c2 = c1;
  }
  const float* a3 = (const float*) ((uintptr_t) a2 + a_stride);
  float* c3 = (float*) ((uintptr_t) c2 + cm_stride);
  if XNN_UNPREDICTABLE(mr < 4) {
    a3 = a2;
    c3 = c2;
  }
  const float* a4 = (const float*) ((uintptr_t) a3 + a_stride);
  float* c4 = (float*) ((uintptr_t) c3 + cm_stride);
  if XNN_UNPREDICTABLE(mr <= 4) {
    a4 = a3;
    c4 = c3;
  }
  const float* a5 = (const float*) ((uintptr_t) a4 + a_stride);
  float* c5 = (float*) ((uintptr_t) c4 + cm_stride);
  if XNN_UNPREDICTABLE(mr != 6) {
    a5 = a4;
    c5 = c4;
  }

  const v128_t vmin = wasm_v128_load64_splat(params->wasmsimd.min);
  const v128_t vmax = wasm_v128_load64_splat(params->wasmsimd.max);
  do {
    v128_t vacc0x0123 = wasm_v128_load((const float*) w + 0);
    v128_t vacc0x4567 = wasm_v128_load((const float*) w + 4);
    v128_t vacc1x0123 = vacc0x0123;
    v128_t vacc1x4567 = vacc0x4567;
    v128_t vacc2x0123 = vacc0x0123;
    v128_t vacc2x4567 = vacc0x4567;
    v128_t vacc3x0123 = vacc0x0123;
    v128_t vacc3x4567 = vacc0x4567;
    v128_t vacc4x0123 = vacc0x0123;
    v128_t vacc4x4567 = vacc0x4567;
    v128_t vacc5x0123 = vacc0x0123;
    v128_t vacc5x4567 = vacc0x4567;
    w = (const float*) w + 8;

    size_t k = kc;
    while (k >= 4 * sizeof(float)) {
      v128_t va0 = wasm_v128_load(a0);
      a0 += 4;
      v128_t va1 = wasm_v128_load(a1);
      a1 += 4;
      v128_t va2 = wasm_v128_load(a2);
      a2 += 4;
      v128_t va3 = wasm_v128_load(a3);
      a3 += 4;
      v128_t va4 = wasm_v128_load(a4);
      a4 += 4;
      v128_t va5 = wasm_v128_load(a5);
      a5 += 4;


      const v128_t vb01234567c0 = wasm_i16x8_load8x8((const int8_t*) w + 0);
      const v128_t vbi0123c0 = wasm_i32x4_extend_low_i16x8(vb01234567c0);
      const v128_t vbi4567c0 = wasm_i32x4_extend_high_i16x8(vb01234567c0);
      const v128_t vb0123c0 = wasm_f32x4_convert_i32x4(vbi0123c0);
      const v128_t vb4567c0 = wasm_f32x4_convert_i32x4(vbi4567c0);

      vacc0x0123 = wasm_f32x4_add(wasm_f32x4_mul(va0, vb0123c0), vacc0x0123);
      vacc1x0123 = wasm_f32x4_add(wasm_f32x4_mul(va1, vb0123c0), vacc1x0123);
      vacc2x0123 = wasm_f32x4_add(wasm_f32x4_mul(va2, vb0123c0), vacc2x0123);
      vacc3x0123 = wasm_f32x4_add(wasm_f32x4_mul(va3, vb0123c0), vacc3x0123);
      vacc4x0123 = wasm_f32x4_add(wasm_f32x4_mul(va4, vb0123c0), vacc4x0123);
      vacc5x0123 = wasm_f32x4_add(wasm_f32x4_mul(va5, vb0123c0), vacc5x0123);
      vacc0x4567 = wasm_f32x4_add(wasm_f32x4_mul(va0, vb4567c0), vacc0x4567);
      vacc1x4567 = wasm_f32x4_add(wasm_f32x4_mul(va1, vb4567c0), vacc1x4567);
      vacc2x4567 = wasm_f32x4_add(wasm_f32x4_mul(va2, vb4567c0), vacc2x4567);
      vacc3x4567 = wasm_f32x4_add(wasm_f32x4_mul(va3, vb4567c0), vacc3x4567);
      vacc4x4567 = wasm_f32x4_add(wasm_f32x4_mul(va4, vb4567c0), vacc4x4567);
      vacc5x4567 = wasm_f32x4_add(wasm_f32x4_mul(va5, vb4567c0), vacc5x4567);

      va0 = wasm_v32x4_shuffle(va0, va0, 1, 2, 3, 0);
      va1 = wasm_v32x4_shuffle(va1, va1, 1, 2, 3, 0);
      va2 = wasm_v32x4_shuffle(va2, va2, 1, 2, 3, 0);
      va3 = wasm_v32x4_shuffle(va3, va3, 1, 2, 3, 0);
      va4 = wasm_v32x4_shuffle(va4, va4, 1, 2, 3, 0);
      va5 = wasm_v32x4_shuffle(va5, va5, 1, 2, 3, 0);

      const v128_t vb01234567c1 = wasm_i16x8_load8x8((const int8_t*) w + 8);
      const v128_t vbi0123c1 = wasm_i32x4_extend_low_i16x8(vb01234567c1);
      const v128_t vbi4567c1 = wasm_i32x4_extend_high_i16x8(vb01234567c1);
      const v128_t vb0123c1 = wasm_f32x4_convert_i32x4(vbi0123c1);
      const v128_t vb4567c1 = wasm_f32x4_convert_i32x4(vbi4567c1);

      vacc0x0123 = wasm_f32x4_add(wasm_f32x4_mul(va0, vb0123c1), vacc0x0123);
      vacc1x0123 = wasm_f32x4_add(wasm_f32x4_mul(va1, vb0123c1), vacc1x0123);
      vacc2x0123 = wasm_f32x4_add(wasm_f32x4_mul(va2, vb0123c1), vacc2x0123);
      vacc3x0123 = wasm_f32x4_add(wasm_f32x4_mul(va3, vb0123c1), vacc3x0123);
      vacc4x0123 = wasm_f32x4_add(wasm_f32x4_mul(va4, vb0123c1), vacc4x0123);
      vacc5x0123 = wasm_f32x4_add(wasm_f32x4_mul(va5, vb0123c1), vacc5x0123);
      vacc0x4567 = wasm_f32x4_add(wasm_f32x4_mul(va0, vb4567c1), vacc0x4567);
      vacc1x4567 = wasm_f32x4_add(wasm_f32x4_mul(va1, vb4567c1), vacc1x4567);
      vacc2x4567 = wasm_f32x4_add(wasm_f32x4_mul(va2, vb4567c1), vacc2x4567);
      vacc3x4567 = wasm_f32x4_add(wasm_f32x4_mul(va3, vb4567c1), vacc3x4567);
      vacc4x4567 = wasm_f32x4_add(wasm_f32x4_mul(va4, vb4567c1), vacc4x4567);
      vacc5x4567 = wasm_f32x4_add(wasm_f32x4_mul(va5, vb4567c1), vacc5x4567);

      va0 = wasm_v32x4_shuffle(va0, va0, 1, 2, 3, 0);
      va1 = wasm_v32x4_shuffle(va1, va1, 1, 2, 3, 0);
      va2 = wasm_v32x4_shuffle(va2, va2, 1, 2, 3, 0);
      va3 = wasm_v32x4_shuffle(va3, va3, 1, 2, 3, 0);
      va4 = wasm_v32x4_shuffle(va4, va4, 1, 2, 3, 0);
      va5 = wasm_v32x4_shuffle(va5, va5, 1, 2, 3, 0);

      const v128_t vb01234567c2 = wasm_i16x8_load8x8((const int8_t*) w + 16);
      const v128_t vbi0123c2 = wasm_i32x4_extend_low_i16x8(vb01234567c2);
      const v128_t vbi4567c2 = wasm_i32x4_extend_high_i16x8(vb01234567c2);
      const v128_t vb0123c2 = wasm_f32x4_convert_i32x4(vbi0123c2);
      const v128_t vb4567c2 = wasm_f32x4_convert_i32x4(vbi4567c2);

      vacc0x0123 = wasm_f32x4_add(wasm_f32x4_mul(va0, vb0123c2), vacc0x0123);
      vacc1x0123 = wasm_f32x4_add(wasm_f32x4_mul(va1, vb0123c2), vacc1x0123);
      vacc2x0123 = wasm_f32x4_add(wasm_f32x4_mul(va2, vb0123c2), vacc2x0123);
      vacc3x0123 = wasm_f32x4_add(wasm_f32x4_mul(va3, vb0123c2), vacc3x0123);
      vacc4x0123 = wasm_f32x4_add(wasm_f32x4_mul(va4, vb0123c2), vacc4x0123);
      vacc5x0123 = wasm_f32x4_add(wasm_f32x4_mul(va5, vb0123c2), vacc5x0123);
      vacc0x4567 = wasm_f32x4_add(wasm_f32x4_mul(va0, vb4567c2), vacc0x4567);
      vacc1x4567 = wasm_f32x4_add(wasm_f32x4_mul(va1, vb4567c2), vacc1x4567);
      vacc2x4567 = wasm_f32x4_add(wasm_f32x4_mul(va2, vb4567c2), vacc2x4567);
      vacc3x4567 = wasm_f32x4_add(wasm_f32x4_mul(va3, vb4567c2), vacc3x4567);
      vacc4x4567 = wasm_f32x4_add(wasm_f32x4_mul(va4, vb4567c2), vacc4x4567);
      vacc5x4567 = wasm_f32x4_add(wasm_f32x4_mul(va5, vb4567c2), vacc5x4567);

      va0 = wasm_v32x4_shuffle(va0, va0, 1, 2, 3, 0);
      va1 = wasm_v32x4_shuffle(va1, va1, 1, 2, 3, 0);
      va2 = wasm_v32x4_shuffle(va2, va2, 1, 2, 3, 0);
      va3 = wasm_v32x4_shuffle(va3, va3, 1, 2, 3, 0);
      va4 = wasm_v32x4_shuffle(va4, va4, 1, 2, 3, 0);
      va5 = wasm_v32x4_shuffle(va5, va5, 1, 2, 3, 0);

      const v128_t vb01234567c3 = wasm_i16x8_load8x8((const int8_t*) w + 24);
      const v128_t vbi0123c3 = wasm_i32x4_extend_low_i16x8(vb01234567c3);
      const v128_t vbi4567c3 = wasm_i32x4_extend_high_i16x8(vb01234567c3);
      const v128_t vb0123c3 = wasm_f32x4_convert_i32x4(vbi0123c3);
      const v128_t vb4567c3 = wasm_f32x4_convert_i32x4(vbi4567c3);

      vacc0x0123 = wasm_f32x4_add(wasm_f32x4_mul(va0, vb0123c3), vacc0x0123);
      vacc1x0123 = wasm_f32x4_add(wasm_f32x4_mul(va1, vb0123c3), vacc1x0123);
      vacc2x0123 = wasm_f32x4_add(wasm_f32x4_mul(va2, vb0123c3), vacc2x0123);
      vacc3x0123 = wasm_f32x4_add(wasm_f32x4_mul(va3, vb0123c3), vacc3x0123);
      vacc4x0123 = wasm_f32x4_add(wasm_f32x4_mul(va4, vb0123c3), vacc4x0123);
      vacc5x0123 = wasm_f32x4_add(wasm_f32x4_mul(va5, vb0123c3), vacc5x0123);
      vacc0x4567 = wasm_f32x4_add(wasm_f32x4_mul(va0, vb4567c3), vacc0x4567);
      vacc1x4567 = wasm_f32x4_add(wasm_f32x4_mul(va1, vb4567c3), vacc1x4567);
      vacc2x4567 = wasm_f32x4_add(wasm_f32x4_mul(va2, vb4567c3), vacc2x4567);
      vacc3x4567 = wasm_f32x4_add(wasm_f32x4_mul(va3, vb4567c3), vacc3x4567);
      vacc4x4567 = wasm_f32x4_add(wasm_f32x4_mul(va4, vb4567c3), vacc4x4567);
      vacc5x4567 = wasm_f32x4_add(wasm_f32x4_mul(va5, vb4567c3), vacc5x4567);

      w = (const int8_t*) w + 32;
      k -= 4 * sizeof(float);
    }
    if XNN_UNLIKELY(k != 0) {
      v128_t va0 = wasm_v128_load(a0);
      a0 = (const float*) ((uintptr_t) a0 + k);
      v128_t va1 = wasm_v128_load(a1);
      a1 = (const float*) ((uintptr_t) a1 + k);
      v128_t va2 = wasm_v128_load(a2);
      a2 = (const float*) ((uintptr_t) a2 + k);
      v128_t va3 = wasm_v128_load(a3);
      a3 = (const float*) ((uintptr_t) a3 + k);
      v128_t va4 = wasm_v128_load(a4);
      a4 = (const float*) ((uintptr_t) a4 + k);
      v128_t va5 = wasm_v128_load(a5);
      a5 = (const float*) ((uintptr_t) a5 + k);

      const v128_t vzero = wasm_f32x4_const_splat(0.0f);

      const v128_t vb01234567c0 = wasm_i16x8_load8x8((const int8_t*) w + 0);
      const v128_t vbi0123c0 = wasm_i32x4_extend_low_i16x8(vb01234567c0);
      const v128_t vbi4567c0 = wasm_i32x4_extend_high_i16x8(vb01234567c0);
      const v128_t vb0123c0 = wasm_f32x4_convert_i32x4(vbi0123c0);
      const v128_t vb4567c0 = wasm_f32x4_convert_i32x4(vbi4567c0);

      vacc0x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va0, wasm_f32x4_eq(vb0123c0, vzero)), vb0123c0), vacc0x0123);
      vacc1x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va1, wasm_f32x4_eq(vb0123c0, vzero)), vb0123c0), vacc1x0123);
      vacc2x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va2, wasm_f32x4_eq(vb0123c0, vzero)), vb0123c0), vacc2x0123);
      vacc3x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va3, wasm_f32x4_eq(vb0123c0, vzero)), vb0123c0), vacc3x0123);
      vacc4x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va4, wasm_f32x4_eq(vb0123c0, vzero)), vb0123c0), vacc4x0123);
      vacc5x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va5, wasm_f32x4_eq(vb0123c0, vzero)), vb0123c0), vacc5x0123);
      vacc0x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va0, wasm_f32x4_eq(vb4567c0, vzero)), vb4567c0), vacc0x4567);
      vacc1x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va1, wasm_f32x4_eq(vb4567c0, vzero)), vb4567c0), vacc1x4567);
      vacc2x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va2, wasm_f32x4_eq(vb4567c0, vzero)), vb4567c0), vacc2x4567);
      vacc3x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va3, wasm_f32x4_eq(vb4567c0, vzero)), vb4567c0), vacc3x4567);
      vacc4x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va4, wasm_f32x4_eq(vb4567c0, vzero)), vb4567c0), vacc4x4567);
      vacc5x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va5, wasm_f32x4_eq(vb4567c0, vzero)), vb4567c0), vacc5x4567);

      va0 = wasm_v32x4_shuffle(va0, va0, 1, 2, 3, 0);
      va1 = wasm_v32x4_shuffle(va1, va1, 1, 2, 3, 0);
      va2 = wasm_v32x4_shuffle(va2, va2, 1, 2, 3, 0);
      va3 = wasm_v32x4_shuffle(va3, va3, 1, 2, 3, 0);
      va4 = wasm_v32x4_shuffle(va4, va4, 1, 2, 3, 0);
      va5 = wasm_v32x4_shuffle(va5, va5, 1, 2, 3, 0);

      const v128_t vb01234567c1 = wasm_i16x8_load8x8((const int8_t*) w + 8);
      const v128_t vbi0123c1 = wasm_i32x4_extend_low_i16x8(vb01234567c1);
      const v128_t vbi4567c1 = wasm_i32x4_extend_high_i16x8(vb01234567c1);
      const v128_t vb0123c1 = wasm_f32x4_convert_i32x4(vbi0123c1);
      const v128_t vb4567c1 = wasm_f32x4_convert_i32x4(vbi4567c1);

      vacc0x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va0, wasm_f32x4_eq(vb0123c1, vzero)), vb0123c1), vacc0x0123);
      vacc1x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va1, wasm_f32x4_eq(vb0123c1, vzero)), vb0123c1), vacc1x0123);
      vacc2x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va2, wasm_f32x4_eq(vb0123c1, vzero)), vb0123c1), vacc2x0123);
      vacc3x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va3, wasm_f32x4_eq(vb0123c1, vzero)), vb0123c1), vacc3x0123);
      vacc4x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va4, wasm_f32x4_eq(vb0123c1, vzero)), vb0123c1), vacc4x0123);
      vacc5x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va5, wasm_f32x4_eq(vb0123c1, vzero)), vb0123c1), vacc5x0123);
      vacc0x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va0, wasm_f32x4_eq(vb4567c1, vzero)), vb4567c1), vacc0x4567);
      vacc1x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va1, wasm_f32x4_eq(vb4567c1, vzero)), vb4567c1), vacc1x4567);
      vacc2x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va2, wasm_f32x4_eq(vb4567c1, vzero)), vb4567c1), vacc2x4567);
      vacc3x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va3, wasm_f32x4_eq(vb4567c1, vzero)), vb4567c1), vacc3x4567);
      vacc4x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va4, wasm_f32x4_eq(vb4567c1, vzero)), vb4567c1), vacc4x4567);
      vacc5x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va5, wasm_f32x4_eq(vb4567c1, vzero)), vb4567c1), vacc5x4567);

      va0 = wasm_v32x4_shuffle(va0, va0, 1, 2, 3, 0);
      va1 = wasm_v32x4_shuffle(va1, va1, 1, 2, 3, 0);
      va2 = wasm_v32x4_shuffle(va2, va2, 1, 2, 3, 0);
      va3 = wasm_v32x4_shuffle(va3, va3, 1, 2, 3, 0);
      va4 = wasm_v32x4_shuffle(va4, va4, 1, 2, 3, 0);
      va5 = wasm_v32x4_shuffle(va5, va5, 1, 2, 3, 0);

      const v128_t vb01234567c2 = wasm_i16x8_load8x8((const int8_t*) w + 16);
      const v128_t vbi0123c2 = wasm_i32x4_extend_low_i16x8(vb01234567c2);
      const v128_t vbi4567c2 = wasm_i32x4_extend_high_i16x8(vb01234567c2);
      const v128_t vb0123c2 = wasm_f32x4_convert_i32x4(vbi0123c2);
      const v128_t vb4567c2 = wasm_f32x4_convert_i32x4(vbi4567c2);

      vacc0x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va0, wasm_f32x4_eq(vb0123c2, vzero)), vb0123c2), vacc0x0123);
      vacc1x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va1, wasm_f32x4_eq(vb0123c2, vzero)), vb0123c2), vacc1x0123);
      vacc2x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va2, wasm_f32x4_eq(vb0123c2, vzero)), vb0123c2), vacc2x0123);
      vacc3x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va3, wasm_f32x4_eq(vb0123c2, vzero)), vb0123c2), vacc3x0123);
      vacc4x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va4, wasm_f32x4_eq(vb0123c2, vzero)), vb0123c2), vacc4x0123);
      vacc5x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va5, wasm_f32x4_eq(vb0123c2, vzero)), vb0123c2), vacc5x0123);
      vacc0x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va0, wasm_f32x4_eq(vb4567c2, vzero)), vb4567c2), vacc0x4567);
      vacc1x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va1, wasm_f32x4_eq(vb4567c2, vzero)), vb4567c2), vacc1x4567);
      vacc2x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va2, wasm_f32x4_eq(vb4567c2, vzero)), vb4567c2), vacc2x4567);
      vacc3x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va3, wasm_f32x4_eq(vb4567c2, vzero)), vb4567c2), vacc3x4567);
      vacc4x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va4, wasm_f32x4_eq(vb4567c2, vzero)), vb4567c2), vacc4x4567);
      vacc5x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va5, wasm_f32x4_eq(vb4567c2, vzero)), vb4567c2), vacc5x4567);

      va0 = wasm_v32x4_shuffle(va0, va0, 1, 2, 3, 0);
      va1 = wasm_v32x4_shuffle(va1, va1, 1, 2, 3, 0);
      va2 = wasm_v32x4_shuffle(va2, va2, 1, 2, 3, 0);
      va3 = wasm_v32x4_shuffle(va3, va3, 1, 2, 3, 0);
      va4 = wasm_v32x4_shuffle(va4, va4, 1, 2, 3, 0);
      va5 = wasm_v32x4_shuffle(va5, va5, 1, 2, 3, 0);

      const v128_t vb01234567c3 = wasm_i16x8_load8x8((const int8_t*) w + 24);
      const v128_t vbi0123c3 = wasm_i32x4_extend_low_i16x8(vb01234567c3);
      const v128_t vbi4567c3 = wasm_i32x4_extend_high_i16x8(vb01234567c3);
      const v128_t vb0123c3 = wasm_f32x4_convert_i32x4(vbi0123c3);
      const v128_t vb4567c3 = wasm_f32x4_convert_i32x4(vbi4567c3);

      vacc0x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va0, wasm_f32x4_eq(vb0123c3, vzero)), vb0123c3), vacc0x0123);
      vacc1x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va1, wasm_f32x4_eq(vb0123c3, vzero)), vb0123c3), vacc1x0123);
      vacc2x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va2, wasm_f32x4_eq(vb0123c3, vzero)), vb0123c3), vacc2x0123);
      vacc3x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va3, wasm_f32x4_eq(vb0123c3, vzero)), vb0123c3), vacc3x0123);
      vacc4x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va4, wasm_f32x4_eq(vb0123c3, vzero)), vb0123c3), vacc4x0123);
      vacc5x0123 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va5, wasm_f32x4_eq(vb0123c3, vzero)), vb0123c3), vacc5x0123);
      vacc0x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va0, wasm_f32x4_eq(vb4567c3, vzero)), vb4567c3), vacc0x4567);
      vacc1x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va1, wasm_f32x4_eq(vb4567c3, vzero)), vb4567c3), vacc1x4567);
      vacc2x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va2, wasm_f32x4_eq(vb4567c3, vzero)), vb4567c3), vacc2x4567);
      vacc3x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va3, wasm_f32x4_eq(vb4567c3, vzero)), vb4567c3), vacc3x4567);
      vacc4x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va4, wasm_f32x4_eq(vb4567c3, vzero)), vb4567c3), vacc4x4567);
      vacc5x4567 = wasm_f32x4_add(wasm_f32x4_mul(wasm_v128_andnot(va5, wasm_f32x4_eq(vb4567c3, vzero)), vb4567c3), vacc5x4567);

      w = (const int8_t*) w + 32;
    }

    const v128_t vscale0123 = wasm_v128_load((const float*) w + 0);
    vacc0x0123 = wasm_f32x4_mul(vacc0x0123, vscale0123);
    vacc1x0123 = wasm_f32x4_mul(vacc1x0123, vscale0123);
    vacc2x0123 = wasm_f32x4_mul(vacc2x0123, vscale0123);
    vacc3x0123 = wasm_f32x4_mul(vacc3x0123, vscale0123);
    vacc4x0123 = wasm_f32x4_mul(vacc4x0123, vscale0123);
    vacc5x0123 = wasm_f32x4_mul(vacc5x0123, vscale0123);
    const v128_t vscale4567 = wasm_v128_load((const float*) w + 4);
    vacc0x4567 = wasm_f32x4_mul(vacc0x4567, vscale4567);
    vacc1x4567 = wasm_f32x4_mul(vacc1x4567, vscale4567);
    vacc2x4567 = wasm_f32x4_mul(vacc2x4567, vscale4567);
    vacc3x4567 = wasm_f32x4_mul(vacc3x4567, vscale4567);
    vacc4x4567 = wasm_f32x4_mul(vacc4x4567, vscale4567);
    vacc5x4567 = wasm_f32x4_mul(vacc5x4567, vscale4567);
    w = (const float*) w + 8;
    vacc0x0123 = wasm_f32x4_max(vmin, vacc0x0123);
    vacc1x0123 = wasm_f32x4_max(vmin, vacc1x0123);
    vacc2x0123 = wasm_f32x4_max(vmin, vacc2x0123);
    vacc3x0123 = wasm_f32x4_max(vmin, vacc3x0123);
    vacc4x0123 = wasm_f32x4_max(vmin, vacc4x0123);
    vacc5x0123 = wasm_f32x4_max(vmin, vacc5x0123);
    vacc0x4567 = wasm_f32x4_max(vmin, vacc0x4567);
    vacc1x4567 = wasm_f32x4_max(vmin, vacc1x4567);
    vacc2x4567 = wasm_f32x4_max(vmin, vacc2x4567);
    vacc3x4567 = wasm_f32x4_max(vmin, vacc3x4567);
    vacc4x4567 = wasm_f32x4_max(vmin, vacc4x4567);
    vacc5x4567 = wasm_f32x4_max(vmin, vacc5x4567);

    vacc0x0123 = wasm_f32x4_min(vmax, vacc0x0123);
    vacc1x0123 = wasm_f32x4_min(vmax, vacc1x0123);
    vacc2x0123 = wasm_f32x4_min(vmax, vacc2x0123);
    vacc3x0123 = wasm_f32x4_min(vmax, vacc3x0123);
    vacc4x0123 = wasm_f32x4_min(vmax, vacc4x0123);
    vacc5x0123 = wasm_f32x4_min(vmax, vacc5x0123);
    vacc0x4567 = wasm_f32x4_min(vmax, vacc0x4567);
    vacc1x4567 = wasm_f32x4_min(vmax, vacc1x4567);
    vacc2x4567 = wasm_f32x4_min(vmax, vacc2x4567);
    vacc3x4567 = wasm_f32x4_min(vmax, vacc3x4567);
    vacc4x4567 = wasm_f32x4_min(vmax, vacc4x4567);
    vacc5x4567 = wasm_f32x4_min(vmax, vacc5x4567);

    if XNN_LIKELY(nc >= 8) {
      wasm_v128_store(c5, vacc5x0123);
      wasm_v128_store(c5 + 4, vacc5x4567);
      c5 = (float*) ((uintptr_t) c5 + cn_stride);
      wasm_v128_store(c4, vacc4x0123);
      wasm_v128_store(c4 + 4, vacc4x4567);
      c4 = (float*) ((uintptr_t) c4 + cn_stride);
      wasm_v128_store(c3, vacc3x0123);
      wasm_v128_store(c3 + 4, vacc3x4567);
      c3 = (float*) ((uintptr_t) c3 + cn_stride);
      wasm_v128_store(c2, vacc2x0123);
      wasm_v128_store(c2 + 4, vacc2x4567);
      c2 = (float*) ((uintptr_t) c2 + cn_stride);
      wasm_v128_store(c1, vacc1x0123);
      wasm_v128_store(c1 + 4, vacc1x4567);
      c1 = (float*) ((uintptr_t) c1 + cn_stride);
      wasm_v128_store(c0, vacc0x0123);
      wasm_v128_store(c0 + 4, vacc0x4567);
      c0 = (float*) ((uintptr_t) c0 + cn_stride);

      a5 = (const float*) ((uintptr_t) a5 - kc);
      a4 = (const float*) ((uintptr_t) a4 - kc);
      a3 = (const float*) ((uintptr_t) a3 - kc);
      a2 = (const float*) ((uintptr_t) a2 - kc);
      a1 = (const float*) ((uintptr_t) a1 - kc);
      a0 = (const float*) ((uintptr_t) a0 - kc);

      nc -= 8;
    } else {
      if (nc & 4) {
        wasm_v128_store(c5, vacc5x0123);
        wasm_v128_store(c4, vacc4x0123);
        wasm_v128_store(c3, vacc3x0123);
        wasm_v128_store(c2, vacc2x0123);
        wasm_v128_store(c1, vacc1x0123);
        wasm_v128_store(c0, vacc0x0123);

        vacc5x0123 = vacc5x4567;
        vacc4x0123 = vacc4x4567;
        vacc3x0123 = vacc3x4567;
        vacc2x0123 = vacc2x4567;
        vacc1x0123 = vacc1x4567;
        vacc0x0123 = vacc0x4567;

        c5 += 4;
        c4 += 4;
        c3 += 4;
        c2 += 4;
        c1 += 4;
        c0 += 4;
      }
      if (nc & 2) {
        wasm_v128_store64_lane(c5, vacc5x0123, 0);
        wasm_v128_store64_lane(c4, vacc4x0123, 0);
        wasm_v128_store64_lane(c3, vacc3x0123, 0);
        wasm_v128_store64_lane(c2, vacc2x0123, 0);
        wasm_v128_store64_lane(c1, vacc1x0123, 0);
        wasm_v128_store64_lane(c0, vacc0x0123, 0);

        vacc5x0123 = wasm_v64x2_shuffle(vacc5x0123, vacc5x0123, 1, 1);
        vacc4x0123 = wasm_v64x2_shuffle(vacc4x0123, vacc4x0123, 1, 1);
        vacc3x0123 = wasm_v64x2_shuffle(vacc3x0123, vacc3x0123, 1, 1);
        vacc2x0123 = wasm_v64x2_shuffle(vacc2x0123, vacc2x0123, 1, 1);
        vacc1x0123 = wasm_v64x2_shuffle(vacc1x0123, vacc1x0123, 1, 1);
        vacc0x0123 = wasm_v64x2_shuffle(vacc0x0123, vacc0x0123, 1, 1);

        c5 += 2;
        c4 += 2;
        c3 += 2;
        c2 += 2;
        c1 += 2;
        c0 += 2;
      }
      if (nc & 1) {
        wasm_v128_store32_lane(c5, vacc5x0123, 0);
        wasm_v128_store32_lane(c4, vacc4x0123, 0);
        wasm_v128_store32_lane(c3, vacc3x0123, 0);
        wasm_v128_store32_lane(c2, vacc2x0123, 0);
        wasm_v128_store32_lane(c1, vacc1x0123, 0);
        wasm_v128_store32_lane(c0, vacc0x0123, 0);
      }

      nc = 0;
    }
  } while (nc != 0);
}
