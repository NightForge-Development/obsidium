// Auto-generated file. Do not edit!
//   Template: src/qs8-vadd/wasmsimd.c.in
//   Generator: tools/xngen
//
// Copyright 2020 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>

#include <wasm_simd128.h>

#include <xnnpack/vadd.h>


void xnn_qs8_vadd_minmax_ukernel__wasmsimd_u8(
    size_t batch,
    const int8_t* input_a,
    const int8_t* input_b,
    int8_t* output,
    const union xnn_qs8_add_minmax_params params[restrict XNN_MIN_ELEMENTS(1)]) XNN_OOB_READS
{
  assert(batch != 0);
  assert(batch % sizeof(int8_t) == 0);
  assert(input_a != NULL);
  assert(input_b != NULL);
  assert(output != NULL);

  const v128_t vbias = wasm_v128_load64_splat(params->wasmsimd.bias);
  const v128_t va_multiplier = wasm_v128_load64_splat(params->wasmsimd.a_multiplier);
  const v128_t vb_multiplier = wasm_v128_load64_splat(params->wasmsimd.b_multiplier);
  const uint32_t vshift = params->wasmsimd.shift;
  const v128_t voutput_zero_point = wasm_v128_load64_splat(params->wasmsimd.output_zero_point);
  const v128_t voutput_min = wasm_v128_load64_splat(params->wasmsimd.output_min);
  const v128_t voutput_max = wasm_v128_load64_splat(params->wasmsimd.output_max);

  for (; batch >= 8 * sizeof(int8_t); batch -= 8 * sizeof(int8_t)) {
    const v128_t va01234567 = wasm_i16x8_load8x8(input_a);
    const v128_t vb01234567 = wasm_i16x8_load8x8(input_b);
    input_a += 8;
    input_b += 8;

    v128_t vacc0123 = wasm_i32x4_add(vbias, wasm_i32x4_mul(wasm_i32x4_extend_low_i16x8(va01234567), va_multiplier));
    v128_t vacc4567 = wasm_i32x4_add(vbias, wasm_i32x4_mul(wasm_i32x4_extend_high_i16x8(va01234567), va_multiplier));

    vacc0123 = wasm_i32x4_add(vacc0123, wasm_i32x4_mul(wasm_i32x4_extend_low_i16x8(vb01234567), vb_multiplier));
    vacc4567 = wasm_i32x4_add(vacc4567, wasm_i32x4_mul(wasm_i32x4_extend_high_i16x8(vb01234567), vb_multiplier));

    vacc0123 = wasm_i32x4_shr(vacc0123, vshift);
    vacc4567 = wasm_i32x4_shr(vacc4567, vshift);

    v128_t vout01234567 = wasm_i16x8_add_sat(wasm_i16x8_narrow_i32x4(vacc0123, vacc4567), voutput_zero_point);

    v128_t vout0123456701234567 = wasm_i8x16_narrow_i16x8(vout01234567, vout01234567);

    vout0123456701234567 = wasm_i8x16_max(vout0123456701234567, voutput_min);

    vout0123456701234567 = wasm_i8x16_min(vout0123456701234567, voutput_max);

    wasm_v128_store64_lane(output, vout0123456701234567, 0);
    output += 8;
  }
  if XNN_UNLIKELY(batch != 0) {
    {
      const v128_t va01234567 = wasm_i16x8_load8x8(input_a);
      const v128_t vb01234567 = wasm_i16x8_load8x8(input_b);

      v128_t vacc0123 = wasm_i32x4_add(vbias, wasm_i32x4_mul(wasm_i32x4_extend_low_i16x8(va01234567), va_multiplier));
      v128_t vacc4567 = wasm_i32x4_add(vbias, wasm_i32x4_mul(wasm_i32x4_extend_high_i16x8(va01234567), va_multiplier));

      vacc0123 = wasm_i32x4_add(vacc0123, wasm_i32x4_mul(wasm_i32x4_extend_low_i16x8(vb01234567), vb_multiplier));
      vacc4567 = wasm_i32x4_add(vacc4567, wasm_i32x4_mul(wasm_i32x4_extend_high_i16x8(vb01234567), vb_multiplier));

      vacc0123 = wasm_i32x4_shr(vacc0123, vshift);
      vacc4567 = wasm_i32x4_shr(vacc4567, vshift);

      v128_t vout01234567 = wasm_i16x8_add_sat(wasm_i16x8_narrow_i32x4(vacc0123, vacc4567), voutput_zero_point);

      v128_t vout0123456701234567 = wasm_i8x16_narrow_i16x8(vout01234567, vout01234567);
      vout0123456701234567 = wasm_i8x16_max(vout0123456701234567, voutput_min);
      vout0123456701234567 = wasm_i8x16_min(vout0123456701234567, voutput_max);

      if (batch & (4 * sizeof(int8_t))) {
        wasm_v128_store32_lane(output, vout0123456701234567, 0);
        vout0123456701234567 = wasm_u64x2_shr(vout0123456701234567, 32);
        output += 4;
      }
      if (batch & (2 * sizeof(int8_t))) {
        wasm_v128_store16_lane(output, vout0123456701234567, 0);
        vout0123456701234567 = wasm_u32x4_shr(vout0123456701234567, 16);
        output += 2;
      }
      if (batch & (1 * sizeof(int8_t))) {
        wasm_v128_store8_lane(output, vout0123456701234567, 0);
      }
    }
  }
}
