// Auto-generated file. Do not edit!
//   Template: src/qs8-igemm/4x8-aarch64-neon-mlal-lane-ld64.S.in
//   Generator: tools/xngen
//
// Copyright 2021 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.


#include <xnnpack/assembly.h>

# void xnn_qs8_igemm_minmax_rndnu_ukernel_4x8__asm_aarch64_neon_mlal_lane_ld64(
#     size_t mr,                 x0
#     size_t nc,                 x1
#     size_t kc,                 x2 / x0
#     size_t ks,                 x3 / x9
#     const int8_t** restrict a, x4
#     const int8_t* restrict w,  x5
#     int8_t* restrict c,        x6
#     size_t cm_stride,          x7
#     size_t cn_stride,                  [sp] -> x10
#     size_t a_offset,                   [sp + 8] -> x8
#     const int8_t* zero,                [sp + 16] -> x12
#     const xnn_qs8_conv_minmax_params params [sp + 24] -> (x11)

# d8-d15, x19-x30 need to be preserved if used. x18 is reserved by the OS.

// Register usage
// A0  x13  v0
// A1  x14  v1
// A2  x15  v2
// A3  x20  v3
// B    x5  v5
// C0   x6 v24 v28
// C1  x16 v25 v29
// C2  x17 v26 v30
// C3   x7 v27 v31
# unused  v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23

BEGIN_FUNCTION xnn_qs8_igemm_minmax_rndnu_ukernel_4x8__asm_aarch64_neon_mlal_lane_ld64

        # Clamp C pointers
        CMP         x0, 2                   // if mr < 2
        LDP         x10, x8, [sp]           // Load cn_stride, a_offset
        ADD         x16, x6, x7             // c1 = c0 + cm_stride
        CSEL        x16, x6,  x16, LO       //   c1 = c0

        ADD         x17, x16, x7            // c2 = c1 + cm_stride
        LDP         x12, x11, [sp, 16]      // Load zero, params pointer
                                            // if mr <= 2
        CSEL        x17, x16, x17, LS       //   c2 = c1

        CMP         x0, 4                   // if mr < 4
        STR         x20, [sp, -16]!         // Save x20 on stack
        ADD         x7,  x17, x7            // c3 = c2 + cm_stride
        CSEL        x7,  x17, x7, LO        //   c3 = c2



        .p2align    3
0:
        # Load initial bias from w into accumulators
        LDP         q24, q28, [x5], 32
        MOV         v25.16b, v24.16b
        MOV         v26.16b, v24.16b
        MOV         v27.16b, v24.16b
        MOV         v29.16b, v28.16b
        MOV         v30.16b, v28.16b
        MOV         v31.16b, v28.16b
        MOV         x9, x3                  // p = ks

        .p2align    3
1:
        # Load next 4 A pointers
        LDP         x13, x14, [x4], 16
        LDP         x15, x20, [x4], 16

        CMP         x13, x12                // if a0 == zero
        ADD         x13, x13, x8            // a0 += a_offset
        CSEL        x13, x12, x13, EQ       //   a0 = zero, else += a0 + a_offset
        CMP         x14, x12                // if a1 == zero
        ADD         x14, x14, x8            // a1 += a_offset
        CSEL        x14, x12, x14, EQ       //   a1 = zero, else += a1 + a_offset
        CMP         x15, x12                // if a2 == zero
        ADD         x15, x15, x8            // a2 += a_offset
        CSEL        x15, x12, x15, EQ       //   a2 = zero, else += a2 + a_offset
        CMP         x20, x12                // if a3 == zero
        ADD         x20, x20, x8            // a3 += a_offset
        CSEL        x20, x12, x20, EQ       //   a3 = zero, else += a3 + a_offset

        # Is there at least 8 bytes for main loop?
        SUBS        x0, x2, 8               // k = kc - 8
        B.LO        4f

        # Main loop - 8 bytes of A
        .p2align    3
2:
        LD1         {v0.8b}, [x13], 8
        LDR         d5, [x5], 8
        LD1         {v1.8b}, [x14], 8
        LD1         {v2.8b}, [x15], 8
        LD1         {v3.8b}, [x20], 8
        SUBS        x0, x0, 8
        SXTL    v0.8h, v0.8b
        SXTL        v5.8h, v5.8b
        SXTL    v1.8h, v1.8b
        SXTL    v2.8h, v2.8b
        SXTL    v3.8h, v3.8b
        SMLAL       v24.4s, v5.4h, v0.h[0]
        SMLAL2      v28.4s, v5.8h, v0.h[0]
        SMLAL       v25.4s, v5.4h, v1.h[0]
        SMLAL2      v29.4s, v5.8h, v1.h[0]
        SMLAL       v26.4s, v5.4h, v2.h[0]
        SMLAL2      v30.4s, v5.8h, v2.h[0]
        SMLAL       v27.4s, v5.4h, v3.h[0]
        SMLAL2      v31.4s, v5.8h, v3.h[0]

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[1]
        SMLAL2      v28.4s, v5.8h, v0.h[1]
        SMLAL       v25.4s, v5.4h, v1.h[1]
        SMLAL2      v29.4s, v5.8h, v1.h[1]
        SMLAL       v26.4s, v5.4h, v2.h[1]
        SMLAL2      v30.4s, v5.8h, v2.h[1]
        SMLAL       v27.4s, v5.4h, v3.h[1]
        SMLAL2      v31.4s, v5.8h, v3.h[1]

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[2]
        SMLAL2      v28.4s, v5.8h, v0.h[2]
        SMLAL       v25.4s, v5.4h, v1.h[2]
        SMLAL2      v29.4s, v5.8h, v1.h[2]
        SMLAL       v26.4s, v5.4h, v2.h[2]
        SMLAL2      v30.4s, v5.8h, v2.h[2]
        SMLAL       v27.4s, v5.4h, v3.h[2]
        SMLAL2      v31.4s, v5.8h, v3.h[2]

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[3]
        SMLAL2      v28.4s, v5.8h, v0.h[3]
        SMLAL       v25.4s, v5.4h, v1.h[3]
        SMLAL2      v29.4s, v5.8h, v1.h[3]
        SMLAL       v26.4s, v5.4h, v2.h[3]
        SMLAL2      v30.4s, v5.8h, v2.h[3]
        SMLAL       v27.4s, v5.4h, v3.h[3]
        SMLAL2      v31.4s, v5.8h, v3.h[3]

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[4]
        SMLAL2      v28.4s, v5.8h, v0.h[4]
        SMLAL       v25.4s, v5.4h, v1.h[4]
        SMLAL2      v29.4s, v5.8h, v1.h[4]
        SMLAL       v26.4s, v5.4h, v2.h[4]
        SMLAL2      v30.4s, v5.8h, v2.h[4]
        SMLAL       v27.4s, v5.4h, v3.h[4]
        SMLAL2      v31.4s, v5.8h, v3.h[4]

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[5]
        SMLAL2      v28.4s, v5.8h, v0.h[5]
        SMLAL       v25.4s, v5.4h, v1.h[5]
        SMLAL2      v29.4s, v5.8h, v1.h[5]
        SMLAL       v26.4s, v5.4h, v2.h[5]
        SMLAL2      v30.4s, v5.8h, v2.h[5]
        SMLAL       v27.4s, v5.4h, v3.h[5]
        SMLAL2      v31.4s, v5.8h, v3.h[5]

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[6]
        SMLAL2      v28.4s, v5.8h, v0.h[6]
        SMLAL       v25.4s, v5.4h, v1.h[6]
        SMLAL2      v29.4s, v5.8h, v1.h[6]
        SMLAL       v26.4s, v5.4h, v2.h[6]
        SMLAL2      v30.4s, v5.8h, v2.h[6]
        SMLAL       v27.4s, v5.4h, v3.h[6]
        SMLAL2      v31.4s, v5.8h, v3.h[6]

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[7]
        SMLAL2      v28.4s, v5.8h, v0.h[7]
        SMLAL       v25.4s, v5.4h, v1.h[7]
        SMLAL2      v29.4s, v5.8h, v1.h[7]
        SMLAL       v26.4s, v5.4h, v2.h[7]
        SMLAL2      v30.4s, v5.8h, v2.h[7]
        SMLAL       v27.4s, v5.4h, v3.h[7]
        SMLAL2      v31.4s, v5.8h, v3.h[7]
        B.HS        2b

        AND         x0, x2, 7               // kc remainder 0 to 7
        # Is there a remainder?- 1 to 7 bytes of A
        CBNZ        x0, 4f

3:
        # ks loop
        SUBS        x9, x9, 32              // ks -= MR * sizeof(int8_t*)
        B.HI        1b

        # Apply params - preshift, scale, postshift, bias and clamp
        LD1R        {v4.4s}, [x11], 4
        SQSHL       v24.4s, v24.4s, v4.4s   // shift to upper bits
        SQSHL       v25.4s, v25.4s, v4.4s
        SQSHL       v26.4s, v26.4s, v4.4s
        SQSHL       v27.4s, v27.4s, v4.4s
        LD1R        {v5.4s}, [x11], 4
        SQSHL       v28.4s, v28.4s, v4.4s
        SQSHL       v29.4s, v29.4s, v4.4s
        SQSHL       v30.4s, v30.4s, v4.4s
        SQSHL       v31.4s, v31.4s, v4.4s
        LD1R        {v6.4s}, [x11], 4
        SQDMULH     v24.4s, v24.4s, v5.4s   // scale without rounding
        SQDMULH     v25.4s, v25.4s, v5.4s
        SQDMULH     v26.4s, v26.4s, v5.4s
        SQDMULH     v27.4s, v27.4s, v5.4s
        SQDMULH     v28.4s, v28.4s, v5.4s
        SQDMULH     v29.4s, v29.4s, v5.4s
        SQDMULH     v30.4s, v30.4s, v5.4s
        SQDMULH     v31.4s, v31.4s, v5.4s
        SRSHL       v24.4s, v24.4s, v6.4s   // signed rounding shift left
        SRSHL       v25.4s, v25.4s, v6.4s
        SRSHL       v26.4s, v26.4s, v6.4s
        SRSHL       v27.4s, v27.4s, v6.4s
        SRSHL       v28.4s, v28.4s, v6.4s
        SRSHL       v29.4s, v29.4s, v6.4s
        SRSHL       v30.4s, v30.4s, v6.4s
        SRSHL       v31.4s, v31.4s, v6.4s

        SQXTN       v24.4h, v24.4s
        SQXTN       v25.4h, v25.4s
        SQXTN       v26.4h, v26.4s
        SQXTN       v27.4h, v27.4s
        LD1R        {v6.8h}, [x11], 2       // add bias
        SQXTN2      v24.8h, v28.4s
        SQXTN2      v25.8h, v29.4s
        SQXTN2      v26.8h, v30.4s
        SQXTN2      v27.8h, v31.4s
        LD1R        {v4.8b}, [x11], 1       // clamp min value

        SQADD       v24.8h, v24.8h, v6.8h
        SQADD       v25.8h, v25.8h, v6.8h
        SQADD       v26.8h, v26.8h, v6.8h
        SQADD       v27.8h, v27.8h, v6.8h
        LD1R        {v5.8b}, [x11]          // clamp max value
        SQXTN  v0.8b, v24.8h
        SQXTN  v1.8b, v25.8h
        SQXTN  v2.8b, v26.8h
        SQXTN  v3.8b, v27.8h
        SUB         x11, x11, 15               // rewind params pointer

        SMAX    v0.8b, v0.8b, v4.8b
        SMAX    v1.8b, v1.8b, v4.8b
        SMAX    v2.8b, v2.8b, v4.8b
        SMAX    v3.8b, v3.8b, v4.8b
        SUBS        x1, x1, 8
        SMIN    v0.8b, v0.8b, v5.8b
        SMIN    v1.8b, v1.8b, v5.8b
        SMIN    v2.8b, v2.8b, v5.8b
        SMIN    v3.8b, v3.8b, v5.8b
        B.LO        5f

        # Store full 4 x 8
        ST1         {v3.8b},  [x7], x10
        ST1         {v2.8b}, [x17], x10
        ST1         {v1.8b}, [x16], x10
        ST1         {v0.8b},  [x6], x10

        SUB         x4, x4, x3              // a -= ks

        # nc loop
        B.HI        0b

        # Restore x20 from stack
        LDR         x20, [sp], 16
        RET

        # Remainder- 1 to 7 bytes of A
        .p2align    3
4:
        AND         x0, x2, 7               // kc remainder 1 to 7

        LD1         {v0.8b}, [x13], x0
        LDR         d5, [x5], 8
        LD1         {v1.8b}, [x14], x0
        LD1         {v2.8b}, [x15], x0
        LD1         {v3.8b}, [x20], x0
        SXTL    v0.8h, v0.8b
        SXTL        v5.8h, v5.8b
        SXTL    v1.8h, v1.8b
        SXTL    v2.8h, v2.8b
        SXTL    v3.8h, v3.8b
        SMLAL       v24.4s, v5.4h, v0.h[0]
        SMLAL2      v28.4s, v5.8h, v0.h[0]
        SMLAL       v25.4s, v5.4h, v1.h[0]
        SMLAL2      v29.4s, v5.8h, v1.h[0]
        SMLAL       v26.4s, v5.4h, v2.h[0]
        SMLAL2      v30.4s, v5.8h, v2.h[0]
        SMLAL       v27.4s, v5.4h, v3.h[0]
        SMLAL2      v31.4s, v5.8h, v3.h[0]
        CMP         x0, 2
        B.LO        3b

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[1]
        SMLAL2      v28.4s, v5.8h, v0.h[1]
        SMLAL       v25.4s, v5.4h, v1.h[1]
        SMLAL2      v29.4s, v5.8h, v1.h[1]
        SMLAL       v26.4s, v5.4h, v2.h[1]
        SMLAL2      v30.4s, v5.8h, v2.h[1]
        SMLAL       v27.4s, v5.4h, v3.h[1]
        SMLAL2      v31.4s, v5.8h, v3.h[1]
        B.EQ        3b

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[2]
        SMLAL2      v28.4s, v5.8h, v0.h[2]
        SMLAL       v25.4s, v5.4h, v1.h[2]
        SMLAL2      v29.4s, v5.8h, v1.h[2]
        SMLAL       v26.4s, v5.4h, v2.h[2]
        SMLAL2      v30.4s, v5.8h, v2.h[2]
        SMLAL       v27.4s, v5.4h, v3.h[2]
        SMLAL2      v31.4s, v5.8h, v3.h[2]
        CMP         x0, 4
        B.LO        3b

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[3]
        SMLAL2      v28.4s, v5.8h, v0.h[3]
        SMLAL       v25.4s, v5.4h, v1.h[3]
        SMLAL2      v29.4s, v5.8h, v1.h[3]
        SMLAL       v26.4s, v5.4h, v2.h[3]
        SMLAL2      v30.4s, v5.8h, v2.h[3]
        SMLAL       v27.4s, v5.4h, v3.h[3]
        SMLAL2      v31.4s, v5.8h, v3.h[3]
        B.EQ        3b

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[4]
        SMLAL2      v28.4s, v5.8h, v0.h[4]
        SMLAL       v25.4s, v5.4h, v1.h[4]
        SMLAL2      v29.4s, v5.8h, v1.h[4]
        SMLAL       v26.4s, v5.4h, v2.h[4]
        SMLAL2      v30.4s, v5.8h, v2.h[4]
        SMLAL       v27.4s, v5.4h, v3.h[4]
        SMLAL2      v31.4s, v5.8h, v3.h[4]
        CMP         x0, 6
        B.LO        3b

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[5]
        SMLAL2      v28.4s, v5.8h, v0.h[5]
        SMLAL       v25.4s, v5.4h, v1.h[5]
        SMLAL2      v29.4s, v5.8h, v1.h[5]
        SMLAL       v26.4s, v5.4h, v2.h[5]
        SMLAL2      v30.4s, v5.8h, v2.h[5]
        SMLAL       v27.4s, v5.4h, v3.h[5]
        SMLAL2      v31.4s, v5.8h, v3.h[5]
        B.EQ        3b

        LDR         d5, [x5], 8
        SXTL        v5.8h, v5.8b
        SMLAL       v24.4s, v5.4h, v0.h[6]
        SMLAL2      v28.4s, v5.8h, v0.h[6]
        SMLAL       v25.4s, v5.4h, v1.h[6]
        SMLAL2      v29.4s, v5.8h, v1.h[6]
        SMLAL       v26.4s, v5.4h, v2.h[6]
        SMLAL2      v30.4s, v5.8h, v2.h[6]
        SMLAL       v27.4s, v5.4h, v3.h[6]
        SMLAL2      v31.4s, v5.8h, v3.h[6]
        B           3b

        # Store odd width
        .p2align    3
5:
        TBZ         x1, 2, 6f
        STR         s3, [x7], 4
        STR         s2, [x17], 4
        DUP         s3, v3.s[1]
        DUP         s2, v2.s[1]
        STR         s1, [x16], 4
        STR         s0, [x6], 4
        DUP         s1, v1.s[1]
        DUP         s0, v0.s[1]
6:
        TBZ         x1, 1, 7f
        STR         h3, [x7], 2
        STR         h2, [x17], 2
        DUP         h3, v3.h[1]
        DUP         h2, v2.h[1]
        STR         h1, [x16], 2
        STR         h0, [x6], 2
        DUP         h1, v1.h[1]
        DUP         h0, v0.h[1]
7:
        TBZ         x1, 0, 8f
        STR         b3, [x7]
        STR         b2, [x17]
        STR         b1, [x16]
        STR         b0, [x6]
8:
        # Restore x20 from stack
        LDR         x20, [sp], 16
        RET

END_FUNCTION xnn_qs8_igemm_minmax_rndnu_ukernel_4x8__asm_aarch64_neon_mlal_lane_ld64

#ifdef __ELF__
.section ".note.GNU-stack","",%progbits
#endif
