// Auto-generated file. Do not edit!
//   Template: src/qs8-gemm/c4-neon-mull-dup.c.in
//   Generator: tools/xngen
//
// Copyright 2021 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>

#include <arm_neon.h>

#include <xnnpack/gemm.h>
#include <xnnpack/math.h>


void xnn_qs8_gemm_minmax_rndnu_ukernel_2x16c4__neon_mlal_ld2r(
    size_t mr,
    size_t nc,
    size_t kc,
    const int8_t* restrict a,
    size_t a_stride,
    const void* restrict w,
    int8_t* restrict c,
    size_t cm_stride,
    size_t cn_stride,
    const union xnn_qs8_conv_minmax_params params[restrict XNN_MIN_ELEMENTS(1)]) XNN_OOB_READS
{
  assert(mr != 0);
  assert(mr <= 2);
  assert(nc != 0);
  assert(kc != 0);
  assert(kc % sizeof(int8_t) == 0);
  assert(a != NULL);
  assert(w != NULL);
  assert(c != NULL);

  kc = round_up_po2(kc, 4 * sizeof(int8_t));
  const int8_t* a0 = a;
  int8_t* c0 = c;
  const int8_t* a1 = (const int8_t*) ((uintptr_t) a0 + a_stride);
  int8_t* c1 = (int8_t*) ((uintptr_t) c0 + cm_stride);
  if XNN_UNPREDICTABLE(mr != 2) {
    a1 = a0;
    c1 = c0;
  }

  do {
    int32x4_t vacc0x01 = vreinterpretq_s32_u64(vmovl_u32(vld1_u32(w))); w = (const int32_t*) w + 2;
    int32x4_t vacc0x23 = vreinterpretq_s32_u64(vmovl_u32(vld1_u32(w))); w = (const int32_t*) w + 2;
    int32x4_t vacc0x45 = vreinterpretq_s32_u64(vmovl_u32(vld1_u32(w))); w = (const int32_t*) w + 2;
    int32x4_t vacc0x67 = vreinterpretq_s32_u64(vmovl_u32(vld1_u32(w))); w = (const int32_t*) w + 2;
    int32x4_t vacc0x89 = vreinterpretq_s32_u64(vmovl_u32(vld1_u32(w))); w = (const int32_t*) w + 2;
    int32x4_t vacc0xAB = vreinterpretq_s32_u64(vmovl_u32(vld1_u32(w))); w = (const int32_t*) w + 2;
    int32x4_t vacc0xCD = vreinterpretq_s32_u64(vmovl_u32(vld1_u32(w))); w = (const int32_t*) w + 2;
    int32x4_t vacc0xEF = vreinterpretq_s32_u64(vmovl_u32(vld1_u32(w))); w = (const int32_t*) w + 2;
    int32x4_t vacc1x01 = vacc0x01;
    int32x4_t vacc1x23 = vacc0x23;
    int32x4_t vacc1x45 = vacc0x45;
    int32x4_t vacc1x67 = vacc0x67;
    int32x4_t vacc1x89 = vacc0x89;
    int32x4_t vacc1xAB = vacc0xAB;
    int32x4_t vacc1xCD = vacc0xCD;
    int32x4_t vacc1xEF = vacc0xEF;

    size_t k = kc;

    while (k >= 16 * sizeof(int8_t)) {
      const int32x2x2_t va0x0 = vld2_dup_s32((const void*)a0); a0 += 8;
      const int32x2x2_t va0x1 = vld2_dup_s32((const void*)a0); a0 += 8;
      const int32x2x2_t va1x0 = vld2_dup_s32((const void*)a1); a1 += 8;
      const int32x2x2_t va1x1 = vld2_dup_s32((const void*)a1); a1 += 8;

      const int8x8_t vb01c0x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb23c0x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb45c0x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb67c0x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb89c0x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbABc0x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbCDc0x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbEFc0x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb01c1x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb23c1x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb45c1x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb67c1x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb89c1x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbABc1x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbCDc1x0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbEFc1x0 = vld1_s8(w); w = (const int8_t*) w + 8;

      const int8x8_t va0c0x0 = vreinterpret_s8_s32(va0x0.val[0]);
      const int8x8_t va0c0x1 = vreinterpret_s8_s32(va0x1.val[0]);
      const int8x8_t va1c0x0 = vreinterpret_s8_s32(va1x0.val[0]);
      const int8x8_t va1c0x1 = vreinterpret_s8_s32(va1x1.val[0]);

      int16x8_t vprod0x01c0 = vmull_s8(vb01c0x0, va0c0x0);
      int16x8_t vprod1x01c0 = vmull_s8(vb01c0x0, va1c0x0);
      const int8x8_t vb01c0x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0x01c0 = vmlal_s8(vprod0x01c0, vb01c0x1, va0c0x1);
      vprod1x01c0 = vmlal_s8(vprod1x01c0, vb01c0x1, va1c0x1);
      vacc0x01 = vpadalq_s16(vacc0x01, vprod0x01c0);
      vacc1x01 = vpadalq_s16(vacc1x01, vprod1x01c0);
      int16x8_t vprod0x23c0 = vmull_s8(vb23c0x0, va0c0x0);
      int16x8_t vprod1x23c0 = vmull_s8(vb23c0x0, va1c0x0);
      const int8x8_t vb23c0x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0x23c0 = vmlal_s8(vprod0x23c0, vb23c0x1, va0c0x1);
      vprod1x23c0 = vmlal_s8(vprod1x23c0, vb23c0x1, va1c0x1);
      vacc0x23 = vpadalq_s16(vacc0x23, vprod0x23c0);
      vacc1x23 = vpadalq_s16(vacc1x23, vprod1x23c0);
      int16x8_t vprod0x45c0 = vmull_s8(vb45c0x0, va0c0x0);
      int16x8_t vprod1x45c0 = vmull_s8(vb45c0x0, va1c0x0);
      const int8x8_t vb45c0x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0x45c0 = vmlal_s8(vprod0x45c0, vb45c0x1, va0c0x1);
      vprod1x45c0 = vmlal_s8(vprod1x45c0, vb45c0x1, va1c0x1);
      vacc0x45 = vpadalq_s16(vacc0x45, vprod0x45c0);
      vacc1x45 = vpadalq_s16(vacc1x45, vprod1x45c0);
      int16x8_t vprod0x67c0 = vmull_s8(vb67c0x0, va0c0x0);
      int16x8_t vprod1x67c0 = vmull_s8(vb67c0x0, va1c0x0);
      const int8x8_t vb67c0x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0x67c0 = vmlal_s8(vprod0x67c0, vb67c0x1, va0c0x1);
      vprod1x67c0 = vmlal_s8(vprod1x67c0, vb67c0x1, va1c0x1);
      vacc0x67 = vpadalq_s16(vacc0x67, vprod0x67c0);
      vacc1x67 = vpadalq_s16(vacc1x67, vprod1x67c0);
      int16x8_t vprod0x89c0 = vmull_s8(vb89c0x0, va0c0x0);
      int16x8_t vprod1x89c0 = vmull_s8(vb89c0x0, va1c0x0);
      const int8x8_t vb89c0x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0x89c0 = vmlal_s8(vprod0x89c0, vb89c0x1, va0c0x1);
      vprod1x89c0 = vmlal_s8(vprod1x89c0, vb89c0x1, va1c0x1);
      vacc0x89 = vpadalq_s16(vacc0x89, vprod0x89c0);
      vacc1x89 = vpadalq_s16(vacc1x89, vprod1x89c0);
      int16x8_t vprod0xABc0 = vmull_s8(vbABc0x0, va0c0x0);
      int16x8_t vprod1xABc0 = vmull_s8(vbABc0x0, va1c0x0);
      const int8x8_t vbABc0x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0xABc0 = vmlal_s8(vprod0xABc0, vbABc0x1, va0c0x1);
      vprod1xABc0 = vmlal_s8(vprod1xABc0, vbABc0x1, va1c0x1);
      vacc0xAB = vpadalq_s16(vacc0xAB, vprod0xABc0);
      vacc1xAB = vpadalq_s16(vacc1xAB, vprod1xABc0);
      int16x8_t vprod0xCDc0 = vmull_s8(vbCDc0x0, va0c0x0);
      int16x8_t vprod1xCDc0 = vmull_s8(vbCDc0x0, va1c0x0);
      const int8x8_t vbCDc0x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0xCDc0 = vmlal_s8(vprod0xCDc0, vbCDc0x1, va0c0x1);
      vprod1xCDc0 = vmlal_s8(vprod1xCDc0, vbCDc0x1, va1c0x1);
      vacc0xCD = vpadalq_s16(vacc0xCD, vprod0xCDc0);
      vacc1xCD = vpadalq_s16(vacc1xCD, vprod1xCDc0);
      int16x8_t vprod0xEFc0 = vmull_s8(vbEFc0x0, va0c0x0);
      int16x8_t vprod1xEFc0 = vmull_s8(vbEFc0x0, va1c0x0);
      const int8x8_t vbEFc0x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0xEFc0 = vmlal_s8(vprod0xEFc0, vbEFc0x1, va0c0x1);
      vprod1xEFc0 = vmlal_s8(vprod1xEFc0, vbEFc0x1, va1c0x1);
      vacc0xEF = vpadalq_s16(vacc0xEF, vprod0xEFc0);
      vacc1xEF = vpadalq_s16(vacc1xEF, vprod1xEFc0);
      const int8x8_t va0c1x0 = vreinterpret_s8_s32(va0x0.val[1]);
      const int8x8_t va0c1x1 = vreinterpret_s8_s32(va0x1.val[1]);
      const int8x8_t va1c1x0 = vreinterpret_s8_s32(va1x0.val[1]);
      const int8x8_t va1c1x1 = vreinterpret_s8_s32(va1x1.val[1]);

      int16x8_t vprod0x01c1 = vmull_s8(vb01c1x0, va0c1x0);
      int16x8_t vprod1x01c1 = vmull_s8(vb01c1x0, va1c1x0);
      const int8x8_t vb01c1x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0x01c1 = vmlal_s8(vprod0x01c1, vb01c1x1, va0c1x1);
      vprod1x01c1 = vmlal_s8(vprod1x01c1, vb01c1x1, va1c1x1);
      vacc0x01 = vpadalq_s16(vacc0x01, vprod0x01c1);
      vacc1x01 = vpadalq_s16(vacc1x01, vprod1x01c1);
      int16x8_t vprod0x23c1 = vmull_s8(vb23c1x0, va0c1x0);
      int16x8_t vprod1x23c1 = vmull_s8(vb23c1x0, va1c1x0);
      const int8x8_t vb23c1x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0x23c1 = vmlal_s8(vprod0x23c1, vb23c1x1, va0c1x1);
      vprod1x23c1 = vmlal_s8(vprod1x23c1, vb23c1x1, va1c1x1);
      vacc0x23 = vpadalq_s16(vacc0x23, vprod0x23c1);
      vacc1x23 = vpadalq_s16(vacc1x23, vprod1x23c1);
      int16x8_t vprod0x45c1 = vmull_s8(vb45c1x0, va0c1x0);
      int16x8_t vprod1x45c1 = vmull_s8(vb45c1x0, va1c1x0);
      const int8x8_t vb45c1x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0x45c1 = vmlal_s8(vprod0x45c1, vb45c1x1, va0c1x1);
      vprod1x45c1 = vmlal_s8(vprod1x45c1, vb45c1x1, va1c1x1);
      vacc0x45 = vpadalq_s16(vacc0x45, vprod0x45c1);
      vacc1x45 = vpadalq_s16(vacc1x45, vprod1x45c1);
      int16x8_t vprod0x67c1 = vmull_s8(vb67c1x0, va0c1x0);
      int16x8_t vprod1x67c1 = vmull_s8(vb67c1x0, va1c1x0);
      const int8x8_t vb67c1x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0x67c1 = vmlal_s8(vprod0x67c1, vb67c1x1, va0c1x1);
      vprod1x67c1 = vmlal_s8(vprod1x67c1, vb67c1x1, va1c1x1);
      vacc0x67 = vpadalq_s16(vacc0x67, vprod0x67c1);
      vacc1x67 = vpadalq_s16(vacc1x67, vprod1x67c1);
      int16x8_t vprod0x89c1 = vmull_s8(vb89c1x0, va0c1x0);
      int16x8_t vprod1x89c1 = vmull_s8(vb89c1x0, va1c1x0);
      const int8x8_t vb89c1x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0x89c1 = vmlal_s8(vprod0x89c1, vb89c1x1, va0c1x1);
      vprod1x89c1 = vmlal_s8(vprod1x89c1, vb89c1x1, va1c1x1);
      vacc0x89 = vpadalq_s16(vacc0x89, vprod0x89c1);
      vacc1x89 = vpadalq_s16(vacc1x89, vprod1x89c1);
      int16x8_t vprod0xABc1 = vmull_s8(vbABc1x0, va0c1x0);
      int16x8_t vprod1xABc1 = vmull_s8(vbABc1x0, va1c1x0);
      const int8x8_t vbABc1x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0xABc1 = vmlal_s8(vprod0xABc1, vbABc1x1, va0c1x1);
      vprod1xABc1 = vmlal_s8(vprod1xABc1, vbABc1x1, va1c1x1);
      vacc0xAB = vpadalq_s16(vacc0xAB, vprod0xABc1);
      vacc1xAB = vpadalq_s16(vacc1xAB, vprod1xABc1);
      int16x8_t vprod0xCDc1 = vmull_s8(vbCDc1x0, va0c1x0);
      int16x8_t vprod1xCDc1 = vmull_s8(vbCDc1x0, va1c1x0);
      const int8x8_t vbCDc1x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0xCDc1 = vmlal_s8(vprod0xCDc1, vbCDc1x1, va0c1x1);
      vprod1xCDc1 = vmlal_s8(vprod1xCDc1, vbCDc1x1, va1c1x1);
      vacc0xCD = vpadalq_s16(vacc0xCD, vprod0xCDc1);
      vacc1xCD = vpadalq_s16(vacc1xCD, vprod1xCDc1);
      int16x8_t vprod0xEFc1 = vmull_s8(vbEFc1x0, va0c1x0);
      int16x8_t vprod1xEFc1 = vmull_s8(vbEFc1x0, va1c1x0);
      const int8x8_t vbEFc1x1 = vld1_s8(w); w = (const int8_t*) w + 8;
      vprod0xEFc1 = vmlal_s8(vprod0xEFc1, vbEFc1x1, va0c1x1);
      vprod1xEFc1 = vmlal_s8(vprod1xEFc1, vbEFc1x1, va1c1x1);
      vacc0xEF = vpadalq_s16(vacc0xEF, vprod0xEFc1);
      vacc1xEF = vpadalq_s16(vacc1xEF, vprod1xEFc1);

      k -= 16 * sizeof(int8_t);
    }

    if (k >= 8 * sizeof(int8_t)) {
      const int32x2x2_t va0 = vld2_dup_s32((const void*)a0); a0 += 8;
      const int32x2x2_t va1 = vld2_dup_s32((const void*)a1); a1 += 8;

      const int8x8_t vb01c0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb23c0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb45c0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb67c0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb89c0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbABc0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbCDc0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbEFc0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb01c1 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb23c1 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb45c1 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb67c1 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb89c1 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbABc1 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbCDc1 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbEFc1 = vld1_s8(w); w = (const int8_t*) w + 8;

      const int8x8_t va0c0 = vreinterpret_s8_s32(va0.val[0]);
      const int8x8_t va1c0 = vreinterpret_s8_s32(va1.val[0]);

      const int16x8_t vprod0x01c0 = vmull_s8(vb01c0, va0c0);
      const int16x8_t vprod1x01c0 = vmull_s8(vb01c0, va1c0);
      vacc0x01 = vpadalq_s16(vacc0x01, vprod0x01c0);
      vacc1x01 = vpadalq_s16(vacc1x01, vprod1x01c0);
      const int16x8_t vprod0x23c0 = vmull_s8(vb23c0, va0c0);
      const int16x8_t vprod1x23c0 = vmull_s8(vb23c0, va1c0);
      vacc0x23 = vpadalq_s16(vacc0x23, vprod0x23c0);
      vacc1x23 = vpadalq_s16(vacc1x23, vprod1x23c0);
      const int16x8_t vprod0x45c0 = vmull_s8(vb45c0, va0c0);
      const int16x8_t vprod1x45c0 = vmull_s8(vb45c0, va1c0);
      vacc0x45 = vpadalq_s16(vacc0x45, vprod0x45c0);
      vacc1x45 = vpadalq_s16(vacc1x45, vprod1x45c0);
      const int16x8_t vprod0x67c0 = vmull_s8(vb67c0, va0c0);
      const int16x8_t vprod1x67c0 = vmull_s8(vb67c0, va1c0);
      vacc0x67 = vpadalq_s16(vacc0x67, vprod0x67c0);
      vacc1x67 = vpadalq_s16(vacc1x67, vprod1x67c0);
      const int16x8_t vprod0x89c0 = vmull_s8(vb89c0, va0c0);
      const int16x8_t vprod1x89c0 = vmull_s8(vb89c0, va1c0);
      vacc0x89 = vpadalq_s16(vacc0x89, vprod0x89c0);
      vacc1x89 = vpadalq_s16(vacc1x89, vprod1x89c0);
      const int16x8_t vprod0xABc0 = vmull_s8(vbABc0, va0c0);
      const int16x8_t vprod1xABc0 = vmull_s8(vbABc0, va1c0);
      vacc0xAB = vpadalq_s16(vacc0xAB, vprod0xABc0);
      vacc1xAB = vpadalq_s16(vacc1xAB, vprod1xABc0);
      const int16x8_t vprod0xCDc0 = vmull_s8(vbCDc0, va0c0);
      const int16x8_t vprod1xCDc0 = vmull_s8(vbCDc0, va1c0);
      vacc0xCD = vpadalq_s16(vacc0xCD, vprod0xCDc0);
      vacc1xCD = vpadalq_s16(vacc1xCD, vprod1xCDc0);
      const int16x8_t vprod0xEFc0 = vmull_s8(vbEFc0, va0c0);
      const int16x8_t vprod1xEFc0 = vmull_s8(vbEFc0, va1c0);
      vacc0xEF = vpadalq_s16(vacc0xEF, vprod0xEFc0);
      vacc1xEF = vpadalq_s16(vacc1xEF, vprod1xEFc0);
      const int8x8_t va0c1 = vreinterpret_s8_s32(va0.val[1]);
      const int8x8_t va1c1 = vreinterpret_s8_s32(va1.val[1]);

      const int16x8_t vprod0x01c1 = vmull_s8(vb01c1, va0c1);
      const int16x8_t vprod1x01c1 = vmull_s8(vb01c1, va1c1);
      vacc0x01 = vpadalq_s16(vacc0x01, vprod0x01c1);
      vacc1x01 = vpadalq_s16(vacc1x01, vprod1x01c1);
      const int16x8_t vprod0x23c1 = vmull_s8(vb23c1, va0c1);
      const int16x8_t vprod1x23c1 = vmull_s8(vb23c1, va1c1);
      vacc0x23 = vpadalq_s16(vacc0x23, vprod0x23c1);
      vacc1x23 = vpadalq_s16(vacc1x23, vprod1x23c1);
      const int16x8_t vprod0x45c1 = vmull_s8(vb45c1, va0c1);
      const int16x8_t vprod1x45c1 = vmull_s8(vb45c1, va1c1);
      vacc0x45 = vpadalq_s16(vacc0x45, vprod0x45c1);
      vacc1x45 = vpadalq_s16(vacc1x45, vprod1x45c1);
      const int16x8_t vprod0x67c1 = vmull_s8(vb67c1, va0c1);
      const int16x8_t vprod1x67c1 = vmull_s8(vb67c1, va1c1);
      vacc0x67 = vpadalq_s16(vacc0x67, vprod0x67c1);
      vacc1x67 = vpadalq_s16(vacc1x67, vprod1x67c1);
      const int16x8_t vprod0x89c1 = vmull_s8(vb89c1, va0c1);
      const int16x8_t vprod1x89c1 = vmull_s8(vb89c1, va1c1);
      vacc0x89 = vpadalq_s16(vacc0x89, vprod0x89c1);
      vacc1x89 = vpadalq_s16(vacc1x89, vprod1x89c1);
      const int16x8_t vprod0xABc1 = vmull_s8(vbABc1, va0c1);
      const int16x8_t vprod1xABc1 = vmull_s8(vbABc1, va1c1);
      vacc0xAB = vpadalq_s16(vacc0xAB, vprod0xABc1);
      vacc1xAB = vpadalq_s16(vacc1xAB, vprod1xABc1);
      const int16x8_t vprod0xCDc1 = vmull_s8(vbCDc1, va0c1);
      const int16x8_t vprod1xCDc1 = vmull_s8(vbCDc1, va1c1);
      vacc0xCD = vpadalq_s16(vacc0xCD, vprod0xCDc1);
      vacc1xCD = vpadalq_s16(vacc1xCD, vprod1xCDc1);
      const int16x8_t vprod0xEFc1 = vmull_s8(vbEFc1, va0c1);
      const int16x8_t vprod1xEFc1 = vmull_s8(vbEFc1, va1c1);
      vacc0xEF = vpadalq_s16(vacc0xEF, vprod0xEFc1);
      vacc1xEF = vpadalq_s16(vacc1xEF, vprod1xEFc1);

      k -= 8 * sizeof(int8_t);
    }

    if XNN_UNLIKELY(k != 0) {
      const int8x8_t va0 = vld1_s8(a0); a0 = (const int8_t*) ((uintptr_t) a0 + k);
      const int8x8_t va1 = vld1_s8(a1); a1 = (const int8_t*) ((uintptr_t) a1 + k);

      const int8x8_t vb01c0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb23c0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb45c0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb67c0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vb89c0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbABc0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbCDc0 = vld1_s8(w); w = (const int8_t*) w + 8;
      const int8x8_t vbEFc0 = vld1_s8(w); w = (const int8_t*) w + 8;

      const int8x8_t va0c0 = vreinterpret_s8_s32(vdup_lane_s32(vreinterpret_s32_s8(va0), 0));
      const int16x8_t vprod0x01c0 = vmull_s8(vb01c0, va0c0);
      vacc0x01 = vpadalq_s16(vacc0x01, vprod0x01c0);
      const int16x8_t vprod0x23c0 = vmull_s8(vb23c0, va0c0);
      vacc0x23 = vpadalq_s16(vacc0x23, vprod0x23c0);
      const int16x8_t vprod0x45c0 = vmull_s8(vb45c0, va0c0);
      vacc0x45 = vpadalq_s16(vacc0x45, vprod0x45c0);
      const int16x8_t vprod0x67c0 = vmull_s8(vb67c0, va0c0);
      vacc0x67 = vpadalq_s16(vacc0x67, vprod0x67c0);
      const int16x8_t vprod0x89c0 = vmull_s8(vb89c0, va0c0);
      vacc0x89 = vpadalq_s16(vacc0x89, vprod0x89c0);
      const int16x8_t vprod0xABc0 = vmull_s8(vbABc0, va0c0);
      vacc0xAB = vpadalq_s16(vacc0xAB, vprod0xABc0);
      const int16x8_t vprod0xCDc0 = vmull_s8(vbCDc0, va0c0);
      vacc0xCD = vpadalq_s16(vacc0xCD, vprod0xCDc0);
      const int16x8_t vprod0xEFc0 = vmull_s8(vbEFc0, va0c0);
      vacc0xEF = vpadalq_s16(vacc0xEF, vprod0xEFc0);
      const int8x8_t va1c0 = vreinterpret_s8_s32(vdup_lane_s32(vreinterpret_s32_s8(va1), 0));
      const int16x8_t vprod1x01c0 = vmull_s8(vb01c0, va1c0);
      vacc1x01 = vpadalq_s16(vacc1x01, vprod1x01c0);
      const int16x8_t vprod1x23c0 = vmull_s8(vb23c0, va1c0);
      vacc1x23 = vpadalq_s16(vacc1x23, vprod1x23c0);
      const int16x8_t vprod1x45c0 = vmull_s8(vb45c0, va1c0);
      vacc1x45 = vpadalq_s16(vacc1x45, vprod1x45c0);
      const int16x8_t vprod1x67c0 = vmull_s8(vb67c0, va1c0);
      vacc1x67 = vpadalq_s16(vacc1x67, vprod1x67c0);
      const int16x8_t vprod1x89c0 = vmull_s8(vb89c0, va1c0);
      vacc1x89 = vpadalq_s16(vacc1x89, vprod1x89c0);
      const int16x8_t vprod1xABc0 = vmull_s8(vbABc0, va1c0);
      vacc1xAB = vpadalq_s16(vacc1xAB, vprod1xABc0);
      const int16x8_t vprod1xCDc0 = vmull_s8(vbCDc0, va1c0);
      vacc1xCD = vpadalq_s16(vacc1xCD, vprod1xCDc0);
      const int16x8_t vprod1xEFc0 = vmull_s8(vbEFc0, va1c0);
      vacc1xEF = vpadalq_s16(vacc1xEF, vprod1xEFc0);
    }

#if XNN_ARCH_ARM64
    int32x4_t vacc0x0123 = vpaddq_s32(vacc0x01, vacc0x23);
    int32x4_t vacc0x4567 = vpaddq_s32(vacc0x45, vacc0x67);
    int32x4_t vacc0x89AB = vpaddq_s32(vacc0x89, vacc0xAB);
    int32x4_t vacc0xCDEF = vpaddq_s32(vacc0xCD, vacc0xEF);
    int32x4_t vacc1x0123 = vpaddq_s32(vacc1x01, vacc1x23);
    int32x4_t vacc1x4567 = vpaddq_s32(vacc1x45, vacc1x67);
    int32x4_t vacc1x89AB = vpaddq_s32(vacc1x89, vacc1xAB);
    int32x4_t vacc1xCDEF = vpaddq_s32(vacc1xCD, vacc1xEF);
#else
    const int32x2_t vsum0x01 = vpadd_s32(vget_low_s32(vacc0x01), vget_high_s32(vacc0x01));
    const int32x2_t vsum0x23 = vpadd_s32(vget_low_s32(vacc0x23), vget_high_s32(vacc0x23));
    int32x4_t vacc0x0123 = vcombine_s32(vsum0x01, vsum0x23);
    const int32x2_t vsum0x45 = vpadd_s32(vget_low_s32(vacc0x45), vget_high_s32(vacc0x45));
    const int32x2_t vsum0x67 = vpadd_s32(vget_low_s32(vacc0x67), vget_high_s32(vacc0x67));
    int32x4_t vacc0x4567 = vcombine_s32(vsum0x45, vsum0x67);
    const int32x2_t vsum0x89 = vpadd_s32(vget_low_s32(vacc0x89), vget_high_s32(vacc0x89));
    const int32x2_t vsum0xAB = vpadd_s32(vget_low_s32(vacc0xAB), vget_high_s32(vacc0xAB));
    int32x4_t vacc0x89AB = vcombine_s32(vsum0x89, vsum0xAB);
    const int32x2_t vsum0xCD = vpadd_s32(vget_low_s32(vacc0xCD), vget_high_s32(vacc0xCD));
    const int32x2_t vsum0xEF = vpadd_s32(vget_low_s32(vacc0xEF), vget_high_s32(vacc0xEF));
    int32x4_t vacc0xCDEF = vcombine_s32(vsum0xCD, vsum0xEF);
    const int32x2_t vsum1x01 = vpadd_s32(vget_low_s32(vacc1x01), vget_high_s32(vacc1x01));
    const int32x2_t vsum1x23 = vpadd_s32(vget_low_s32(vacc1x23), vget_high_s32(vacc1x23));
    int32x4_t vacc1x0123 = vcombine_s32(vsum1x01, vsum1x23);
    const int32x2_t vsum1x45 = vpadd_s32(vget_low_s32(vacc1x45), vget_high_s32(vacc1x45));
    const int32x2_t vsum1x67 = vpadd_s32(vget_low_s32(vacc1x67), vget_high_s32(vacc1x67));
    int32x4_t vacc1x4567 = vcombine_s32(vsum1x45, vsum1x67);
    const int32x2_t vsum1x89 = vpadd_s32(vget_low_s32(vacc1x89), vget_high_s32(vacc1x89));
    const int32x2_t vsum1xAB = vpadd_s32(vget_low_s32(vacc1xAB), vget_high_s32(vacc1xAB));
    int32x4_t vacc1x89AB = vcombine_s32(vsum1x89, vsum1xAB);
    const int32x2_t vsum1xCD = vpadd_s32(vget_low_s32(vacc1xCD), vget_high_s32(vacc1xCD));
    const int32x2_t vsum1xEF = vpadd_s32(vget_low_s32(vacc1xEF), vget_high_s32(vacc1xEF));
    int32x4_t vacc1xCDEF = vcombine_s32(vsum1xCD, vsum1xEF);
#endif

    const int32x4_t vright_pre_shift = vld1q_dup_s32(&params->rndnu_neon.right_pre_shift);
    const int32x4_t vmultiplier = vld1q_dup_s32(&params->rndnu_neon.multiplier);
    const int32x4_t vright_post_shift = vld1q_dup_s32(&params->rndnu_neon.right_post_shift);

    vacc0x0123 = vqshlq_s32(vacc0x0123, vright_pre_shift);
    vacc0x4567 = vqshlq_s32(vacc0x4567, vright_pre_shift);
    vacc0x89AB = vqshlq_s32(vacc0x89AB, vright_pre_shift);
    vacc0xCDEF = vqshlq_s32(vacc0xCDEF, vright_pre_shift);
    vacc1x0123 = vqshlq_s32(vacc1x0123, vright_pre_shift);
    vacc1x4567 = vqshlq_s32(vacc1x4567, vright_pre_shift);
    vacc1x89AB = vqshlq_s32(vacc1x89AB, vright_pre_shift);
    vacc1xCDEF = vqshlq_s32(vacc1xCDEF, vright_pre_shift);

    vacc0x0123 = vqdmulhq_s32(vacc0x0123, vmultiplier);
    vacc0x4567 = vqdmulhq_s32(vacc0x4567, vmultiplier);
    vacc0x89AB = vqdmulhq_s32(vacc0x89AB, vmultiplier);
    vacc0xCDEF = vqdmulhq_s32(vacc0xCDEF, vmultiplier);
    vacc1x0123 = vqdmulhq_s32(vacc1x0123, vmultiplier);
    vacc1x4567 = vqdmulhq_s32(vacc1x4567, vmultiplier);
    vacc1x89AB = vqdmulhq_s32(vacc1x89AB, vmultiplier);
    vacc1xCDEF = vqdmulhq_s32(vacc1xCDEF, vmultiplier);

    vacc0x0123 = vrshlq_s32(vacc0x0123, vright_post_shift);
    vacc0x4567 = vrshlq_s32(vacc0x4567, vright_post_shift);
    vacc0x89AB = vrshlq_s32(vacc0x89AB, vright_post_shift);
    vacc0xCDEF = vrshlq_s32(vacc0xCDEF, vright_post_shift);
    vacc1x0123 = vrshlq_s32(vacc1x0123, vright_post_shift);
    vacc1x4567 = vrshlq_s32(vacc1x4567, vright_post_shift);
    vacc1x89AB = vrshlq_s32(vacc1x89AB, vright_post_shift);
    vacc1xCDEF = vrshlq_s32(vacc1xCDEF, vright_post_shift);

    const int16x8_t voutput_zero_point = vld1q_dup_s16(&params->rndnu_neon.output_zero_point);
#if XNN_ARCH_ARM64
    int16x8_t vacc0x01234567 = vqmovn_high_s32(vqmovn_s32(vacc0x0123), vacc0x4567);
    int16x8_t vacc0x89ABCDEF = vqmovn_high_s32(vqmovn_s32(vacc0x89AB), vacc0xCDEF);
    int16x8_t vacc1x01234567 = vqmovn_high_s32(vqmovn_s32(vacc1x0123), vacc1x4567);
    int16x8_t vacc1x89ABCDEF = vqmovn_high_s32(vqmovn_s32(vacc1x89AB), vacc1xCDEF);

    vacc0x01234567 = vqaddq_s16(vacc0x01234567, voutput_zero_point);
    vacc0x89ABCDEF = vqaddq_s16(vacc0x89ABCDEF, voutput_zero_point);
    vacc1x01234567 = vqaddq_s16(vacc1x01234567, voutput_zero_point);
    vacc1x89ABCDEF = vqaddq_s16(vacc1x89ABCDEF, voutput_zero_point);

    int8x16_t vout0x0123456789ABCDEF = vqmovn_high_s16(vqmovn_s16(vacc0x01234567), vacc0x89ABCDEF);
    int8x16_t vout1x0123456789ABCDEF = vqmovn_high_s16(vqmovn_s16(vacc1x01234567), vacc1x89ABCDEF);
#else
    int16x8_t vacc0x01234567 = vcombine_s16(vqmovn_s32(vacc0x0123), vqmovn_s32(vacc0x4567));
    int16x8_t vacc0x89ABCDEF = vcombine_s16(vqmovn_s32(vacc0x89AB), vqmovn_s32(vacc0xCDEF));
    int16x8_t vacc1x01234567 = vcombine_s16(vqmovn_s32(vacc1x0123), vqmovn_s32(vacc1x4567));
    int16x8_t vacc1x89ABCDEF = vcombine_s16(vqmovn_s32(vacc1x89AB), vqmovn_s32(vacc1xCDEF));

    vacc0x01234567 = vqaddq_s16(vacc0x01234567, voutput_zero_point);
    vacc0x89ABCDEF = vqaddq_s16(vacc0x89ABCDEF, voutput_zero_point);
    vacc1x01234567 = vqaddq_s16(vacc1x01234567, voutput_zero_point);
    vacc1x89ABCDEF = vqaddq_s16(vacc1x89ABCDEF, voutput_zero_point);

    int8x16_t vout0x0123456789ABCDEF = vcombine_s8(vqmovn_s16(vacc0x01234567), vqmovn_s16(vacc0x89ABCDEF));
    int8x16_t vout1x0123456789ABCDEF = vcombine_s8(vqmovn_s16(vacc1x01234567), vqmovn_s16(vacc1x89ABCDEF));
#endif

    const int8x16_t voutput_min = vld1q_dup_s8(&params->rndnu_neon.output_min);
    vout0x0123456789ABCDEF = vmaxq_s8(vout0x0123456789ABCDEF, voutput_min);
    vout1x0123456789ABCDEF = vmaxq_s8(vout1x0123456789ABCDEF, voutput_min);

    const int8x16_t voutput_max = vld1q_dup_s8(&params->rndnu_neon.output_max);
    vout0x0123456789ABCDEF = vminq_s8(vout0x0123456789ABCDEF, voutput_max);
    vout1x0123456789ABCDEF = vminq_s8(vout1x0123456789ABCDEF, voutput_max);

    if (nc >= 16) {
      vst1q_s8(c0 + 0, vout0x0123456789ABCDEF);
      vst1q_s8(c1 + 0, vout1x0123456789ABCDEF);

      c0 = (int8_t*) ((uintptr_t) c0 + cn_stride);
      c1 = (int8_t*) ((uintptr_t) c1 + cn_stride);

      a0 = (const int8_t*) ((uintptr_t) a0 - kc);
      a1 = (const int8_t*) ((uintptr_t) a1 - kc);

      nc -= 16;
    } else {
      // Final case where not all of the 16 columns fit in the destination.
      int8x16_t vout0x01234567_1x01234567 = vcombine_s8(vget_low_s8(vout0x0123456789ABCDEF), vget_low_s8(vout1x0123456789ABCDEF));
      if (nc & 8) {
        vst1_s8(c0, vget_low_s8(vout0x01234567_1x01234567)); c0 += 8;
        vst1_s8(c1, vget_high_s8(vout0x01234567_1x01234567)); c1 += 8;
        vout0x01234567_1x01234567 = vcombine_s8(vget_high_s8(vout0x0123456789ABCDEF), vget_high_s8(vout1x0123456789ABCDEF));
      }
      if (nc & 4) {
        vst1q_lane_u32((void*) c0, vreinterpretq_u32_s8(vout0x01234567_1x01234567), 0); c0 += 4;
        vst1q_lane_u32((void*) c1, vreinterpretq_u32_s8(vout0x01234567_1x01234567), 2); c1 += 4;
        vout0x01234567_1x01234567 = vextq_s8(vout0x01234567_1x01234567, vout0x01234567_1x01234567, 4);
      }
      if (nc & 2) {
        vst1q_lane_u16((void*) c0, vreinterpretq_u16_s8(vout0x01234567_1x01234567), 0); c0 += 2;
        vst1q_lane_u16((void*) c1, vreinterpretq_u16_s8(vout0x01234567_1x01234567), 4); c1 += 2;
        vout0x01234567_1x01234567 = vextq_s8(vout0x01234567_1x01234567, vout0x01234567_1x01234567, 2);
      }
      if (nc & 1) {
        vst1q_lane_s8(c0, vout0x01234567_1x01234567, 0);
        vst1q_lane_s8(c1, vout0x01234567_1x01234567, 8);
      }

      nc = 0;
    }
  } while (nc != 0);
}
