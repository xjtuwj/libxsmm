/******************************************************************************
** Copyright (c) 2016-2018, Intel Corporation                                **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
******************************************************************************/
/* Hans Pabst (Intel Corp.)
******************************************************************************/
#ifndef LIBXSMM_GEMM_DIFF_C
#define LIBXSMM_GEMM_DIFF_C

#include "libxsmm_gemm_diff.h"
#include "libxsmm_main.h"

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <string.h>
#include <stdio.h>
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

/* Enable/disable specific code paths */
#if defined(LIBXSMM_INTRINSICS_KNC) && !defined(LIBXSMM_GEMM_DIFF_KNC)
/*#define LIBXSMM_GEMM_DIFF_KNC*/
#endif
#if defined(LIBXSMM_INTRINSICS_SSE3) && !defined(LIBXSMM_GEMM_DIFF_SSE3)
# define LIBXSMM_GEMM_DIFF_SSE3
#endif
#if defined(LIBXSMM_INTRINSICS_AVX) && !defined(LIBXSMM_GEMM_DIFF_AVX)
# define LIBXSMM_GEMM_DIFF_AVX
#endif
#if defined(LIBXSMM_INTRINSICS_AVX2) && !defined(LIBXSMM_GEMM_DIFF_AVX2)
# define LIBXSMM_GEMM_DIFF_AVX2
#endif
#if defined(LIBXSMM_INTRINSICS_AVX512) && !defined(LIBXSMM_GEMM_DIFF_AVX512)
# define LIBXSMM_GEMM_DIFF_AVX512
#endif


LIBXSMM_APIVAR(libxsmm_gemm_diff_function internal_gemm_diff_fn);
LIBXSMM_APIVAR(libxsmm_gemm_diffn_function internal_gemm_diffn_fn);


LIBXSMM_GEMM_DIFF_API_DEFINITION void libxsmm_gemm_diff_init(int target_arch)
{
  internal_gemm_diff_fn = libxsmm_gemm_diff_sw;
  internal_gemm_diffn_fn = libxsmm_gemm_diffn_sw;
#if defined(LIBXSMM_GEMM_DIFF_KNC)
  LIBXSMM_UNUSED(target_arch);
  internal_gemm_diffn_fn = libxsmm_gemm_diffn_imci;
  internal_gemm_diff_fn = libxsmm_gemm_diff_imci;
#else
  if (LIBXSMM_X86_AVX512 <= target_arch) {
    internal_gemm_diffn_fn = libxsmm_gemm_diffn_avx512;
    internal_gemm_diff_fn = libxsmm_gemm_diff_avx2;
  }
  else if (LIBXSMM_X86_AVX2 <= target_arch) {
    internal_gemm_diffn_fn = libxsmm_gemm_diffn_avx2;
    internal_gemm_diff_fn = libxsmm_gemm_diff_avx2;
  }
  else if (LIBXSMM_X86_AVX <= target_arch) {
    internal_gemm_diffn_fn = libxsmm_gemm_diffn_avx;
    internal_gemm_diff_fn = libxsmm_gemm_diff_avx;
  }
  else if (LIBXSMM_X86_SSE3 <= target_arch) {
    internal_gemm_diff_fn = libxsmm_gemm_diff_sse;
  }
#endif
  assert(0 != internal_gemm_diff_fn);
  assert(0 != internal_gemm_diffn_fn);
}


LIBXSMM_GEMM_DIFF_API_DEFINITION void libxsmm_gemm_diff_finalize(void)
{
}


LIBXSMM_GEMM_DIFF_API_DEFINITION unsigned int libxsmm_gemm_diff(const libxsmm_gemm_descriptor* reference, const libxsmm_gemm_descriptor* desc)
{
  /* attempt to rely on static code path avoids to rely on capability of inlining pointer-based function call */
#if defined(LIBXSMM_GEMM_DIFF_SW) && (0 != LIBXSMM_GEMM_DIFF_SW)
  return libxsmm_gemm_diff_sw(reference, desc);
#elif defined(LIBXSMM_GEMM_DIFF_KNC)
  return libxsmm_gemm_diff_imci(reference, desc);
#elif (LIBXSMM_STATIC_TARGET_ARCH == LIBXSMM_MAX_STATIC_TARGET_ARCH) /* eventually no no need for an indirect call */
# if (LIBXSMM_X86_AVX2 <= LIBXSMM_STATIC_TARGET_ARCH)
  return libxsmm_gemm_diff_avx2(reference, desc);
# elif (LIBXSMM_X86_AVX <= LIBXSMM_STATIC_TARGET_ARCH)
  return libxsmm_gemm_diff_avx(reference, desc);
# elif (LIBXSMM_X86_SSE3 <= LIBXSMM_STATIC_TARGET_ARCH)
  return libxsmm_gemm_diff_sse(reference, desc);
# else /* pointer based function call */
  assert(0 != internal_gemm_diff_fn);
  return internal_gemm_diff_fn(reference, desc);
# endif
#else /* pointer based function call */
  assert(0 != internal_gemm_diff_fn);
  return internal_gemm_diff_fn(reference, desc);
#endif
}


LIBXSMM_GEMM_DIFF_API_DEFINITION unsigned int libxsmm_gemm_diff_sw(const libxsmm_gemm_descriptor* reference, const libxsmm_gemm_descriptor* desc)
{
#if defined(LIBXSMM_GEMM_DIFF_SW) && (2 == (LIBXSMM_GEMM_DIFF_SW))
  return 0 != memcmp(reference, desc, LIBXSMM_GEMM_DESCRIPTOR_SIZE);
#else
  typedef unsigned int LIBXSMM_MAY_ALIAS uia_type;
  const uia_type *const ia = (const uia_type*)reference, *const ib = (const uia_type*)desc;
  const unsigned int end = (LIBXSMM_GEMM_DESCRIPTOR_SIZE >> 2/*LOG2(sizeof(int))*/);
  unsigned int result, i;
  assert(0 == LIBXSMM_MOD2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, sizeof(unsigned int)));
  assert(0 != reference && 0 != desc);
  result = ia[0] ^ ib[0];
  for (i = 1; i < end; ++i) {
    result |= (ia[i] ^ ib[i]);
  }
  return result;
#endif
}


LIBXSMM_GEMM_DIFF_API_DEFINITION LIBXSMM_INTRINSICS(LIBXSMM_X86_SSE3)
unsigned int libxsmm_gemm_diff_sse(const libxsmm_gemm_descriptor* reference, const libxsmm_gemm_descriptor* desc)
{
  assert(0 != reference && 0 != desc);
#if defined(LIBXSMM_GEMM_DIFF_SSE3)
  assert(0 == LIBXSMM_MOD2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, sizeof(unsigned int)));
# if (16 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  {
    const __m128i a128 = LIBXSMM_INTRINSICS_LDDQU_SI128((const __m128i*)reference);
    const __m128i b128 = LIBXSMM_INTRINSICS_LDDQU_SI128((const __m128i*)desc);
    const __m128i c128 = _mm_cmpeq_epi8(a128, b128);
    return 0xFFFF != _mm_movemask_epi8(c128);
  }
# else
  return libxsmm_gemm_diff_sw(reference, desc);
# endif
#else
  { static int error_once = 0;
    if (0 != libxsmm_verbosity /* library code is expected to be mute */
     && 1 == LIBXSMM_ATOMIC_ADD_FETCH(&error_once, 1, LIBXSMM_ATOMIC_RELAXED))
    {
      fprintf(stderr, "LIBXSMM WARNING: unable to enter SSE code path!\n");
    }
  }
  return libxsmm_gemm_diff_sw(reference, desc);
#endif
}


LIBXSMM_GEMM_DIFF_API_DEFINITION LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX)
unsigned int libxsmm_gemm_diff_avx(const libxsmm_gemm_descriptor* reference, const libxsmm_gemm_descriptor* desc)
{
  assert(0 != reference && 0 != desc);
#if defined(LIBXSMM_GEMM_DIFF_AVX)
  assert(0 == LIBXSMM_MOD2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, sizeof(unsigned int)));
# if (28 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  {
    const int yes = 0x80000000, no = 0x0;
    const __m256i m256 = _mm256_set_epi32(no, yes, yes, yes, yes, yes, yes, yes);
#   if defined(LIBXSMM_GEMM_DIFF_MASK_A) || !defined(LIBXSMM_GEMM_DIFF_ZERO_PADDED)
    const __m256i a256 = _mm256_castps_si256(_mm256_maskload_ps((const float*)reference, m256));
#   else
    /*const __m256i a256 = _mm256_lddqu_si256((const __m256i*)reference);*/
    const __m256i a256 = _mm256_loadu_si256((const __m256i*)reference);
#   endif
    const __m256i b256 = _mm256_castps_si256(_mm256_maskload_ps((const float*)desc, m256));
    /* avoid warning about eval. in unspecified order: r0, r1 */
    const int r0 = _mm256_testnzc_si256(a256, b256);
    const int r1 = _mm256_testnzc_si256(b256, a256);
    return r0 | r1;
  }
# else
  { /* no difference between SSE and AVX based implementation */
    return libxsmm_gemm_diff_sse(reference, desc);
  }
# endif
#else
  { static int error_once = 0;
    if (0 != libxsmm_verbosity /* library code is expected to be mute */
     && 1 == LIBXSMM_ATOMIC_ADD_FETCH(&error_once, 1, LIBXSMM_ATOMIC_RELAXED))
    {
      fprintf(stderr, "LIBXSMM WARNING: unable to enter AVX code path!\n");
    }
  }
  return libxsmm_gemm_diff_sse(reference, desc);
#endif
}


LIBXSMM_GEMM_DIFF_API_DEFINITION LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX2)
unsigned int libxsmm_gemm_diff_avx2(const libxsmm_gemm_descriptor* reference, const libxsmm_gemm_descriptor* desc)
{
  assert(0 != reference && 0 != desc);
#if defined(LIBXSMM_GEMM_DIFF_AVX2)
  assert(0 == LIBXSMM_MOD2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, sizeof(unsigned int)));
# if (28 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  {
    const int yes = 0x80000000, no = 0x0;
    const __m256i m256 = _mm256_set_epi32(no, yes, yes, yes, yes, yes, yes, yes);
#   if defined(LIBXSMM_GEMM_DIFF_MASK_A) || !defined(LIBXSMM_GEMM_DIFF_ZERO_PADDED)
    const __m256i a256 = _mm256_maskload_epi32((const int*)reference, m256);
#   else
    /*const __m256i a256 = _mm256_lddqu_si256((const __m256i*)reference);*/
    const __m256i a256 = _mm256_loadu_si256((const __m256i*)reference);
#   endif
    const __m256i b256 = _mm256_maskload_epi32((const int*)desc, m256);
    /* avoid warning about eval. in unspecified order: r0, r1 */
    const int r0 = _mm256_testnzc_si256(a256, b256);
    const int r1 = _mm256_testnzc_si256(b256, a256);
    return r0 | r1;
  }
# elif (16 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  { /* no difference between AVX and AVX2 based implementation */
    return libxsmm_gemm_diff_avx(reference, desc);
  }
# else
  return libxsmm_gemm_diff_avx(reference, desc);
# endif
#else
  { static int error_once = 0;
    if (0 != libxsmm_verbosity /* library code is expected to be mute */
     && 1 == LIBXSMM_ATOMIC_ADD_FETCH(&error_once, 1, LIBXSMM_ATOMIC_RELAXED))
    {
      fprintf(stderr, "LIBXSMM WARNING: unable to enter AVX2 code path!\n");
    }
  }
  return libxsmm_gemm_diff_avx(reference, desc);
#endif
}


LIBXSMM_GEMM_DIFF_API_DEFINITION unsigned int libxsmm_gemm_diff_imci(const libxsmm_gemm_descriptor* reference, const libxsmm_gemm_descriptor* desc)
{
  assert(0 != reference && 0 != desc);
#if defined(LIBXSMM_GEMM_DIFF_KNC) && (28 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  assert(0 ==  LIBXSMM_MOD2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, sizeof(unsigned int)));
  {
    const __mmask16 mask = (0xFFFF >> (16 - (LIBXSMM_GEMM_DESCRIPTOR_SIZE >> 2/*LOG2(sizeof(int))*/)));
    const __m512i a512 = _mm512_mask_loadunpackhi_epi32(
      _mm512_mask_loadunpacklo_epi32(_mm512_set1_epi32(0), mask, reference),
      mask, ((const char*)reference) + 32);
    const __m512i b512 = _mm512_mask_loadunpackhi_epi32(
      _mm512_mask_loadunpacklo_epi32(_mm512_set1_epi32(0), mask, desc),
      mask, ((const char*)desc) + 32);
    return _mm512_reduce_or_epi32(_mm512_xor_si512(a512, b512));
  }
#else
  return libxsmm_gemm_diff_sw(reference, desc);
#endif
}


LIBXSMM_GEMM_DIFF_API_DEFINITION unsigned int libxsmm_gemm_diffn(const libxsmm_gemm_descriptor* reference,
  const void* descs, unsigned int hint, unsigned int ndescs, int nbytes)
{
  /* attempt to rely on static code path avoids to rely on capability of inlining pointer-based function call */
#if defined(LIBXSMM_GEMM_DIFF_SW) && (0 != LIBXSMM_GEMM_DIFF_SW)
  return libxsmm_gemm_diffn_sw(reference, descs, hint, ndescs, nbytes);
#elif defined(LIBXSMM_GEMM_DIFF_KNC)
  return libxsmm_gemm_diffn_imci(reference, descs, hint, ndescs, nbytes);
#elif (LIBXSMM_STATIC_TARGET_ARCH == LIBXSMM_MAX_STATIC_TARGET_ARCH) /* eventually no no need for an indirect call */
# if (LIBXSMM_X86_AVX512 <= LIBXSMM_STATIC_TARGET_ARCH)
  return libxsmm_gemm_diffn_avx512(reference, descs, hint, ndescs, nbytes);
# elif (LIBXSMM_X86_AVX2 <= LIBXSMM_STATIC_TARGET_ARCH)
  return libxsmm_gemm_diffn_avx2(reference, descs, hint, ndescs, nbytes);
# elif (LIBXSMM_X86_AVX <= LIBXSMM_STATIC_TARGET_ARCH)
  return libxsmm_gemm_diffn_avx(reference, descs, hint, ndescs, nbytes);
# else /* pointer based function call */
  assert(0 != internal_gemm_diffn_fn);
  return internal_gemm_diffn_fn(reference, descs, hint, ndescs, nbytes);
# endif
#else /* pointer based function call */
  assert(0 != internal_gemm_diffn_fn);
  return internal_gemm_diffn_fn(reference, descs, hint, ndescs, nbytes);
#endif
}


LIBXSMM_GEMM_DIFF_API_DEFINITION unsigned int libxsmm_gemm_diffn_sw(const libxsmm_gemm_descriptor* reference,
  const void* descs, unsigned int hint, unsigned int ndescs, int nbytes)
{
  const char *const desc = (const char*)descs;
  const unsigned int end = hint + ndescs;
  unsigned int i;
  for (i = hint; i < end; ++i) {
    const unsigned int j = i % ndescs; /* wrap around index */
    /* negative stride runs backwards */
    if (0 == libxsmm_gemm_diff_sw(reference, (const libxsmm_gemm_descriptor*)(desc + j * nbytes))) {
      return j;
    }
  }
  return ndescs;
}


LIBXSMM_GEMM_DIFF_API_DEFINITION LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX)
unsigned int libxsmm_gemm_diffn_avx(const libxsmm_gemm_descriptor* reference,
  const void* descs, unsigned int hint, unsigned int ndescs, int nbytes)
{
#if defined(LIBXSMM_GEMM_DIFF_AVX)
  assert(/*is pot*/ndescs == LIBXSMM_UP2POT(ndescs));
# if (28 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  assert(32 == nbytes); /* padded descriptor array */
  {
    const unsigned int end = hint + ndescs;
    const char *const desc = (const char*)descs;
    const int yes = 0x80000000, no = 0x0;
    unsigned int i;
    const __m256i m256 = _mm256_set_epi32(no, yes, yes, yes, yes, yes, yes, yes);
#   if defined(LIBXSMM_GEMM_DIFF_MASK_A) || !defined(LIBXSMM_GEMM_DIFF_ZERO_PADDED)
    const __m256i a256 = _mm256_castps_si256(_mm256_maskload_ps((const float*)reference, m256));
#   else
    /*const __m256i a256 = _mm256_lddqu_si256((const __m256i*)reference);*/
    const __m256i a256 = _mm256_loadu_si256((const __m256i*)reference);
#   endif
    for (i = hint; i < end; ++i) {
      const unsigned int j = LIBXSMM_MOD2(i, ndescs); /* wrap around index */
#   if defined(LIBXSMM_GEMM_DIFF_ZERO_PADDED)
      const __m256i b256 = _mm256_loadu_si256((const __m256i*)(desc + j * nbytes));
#   else
      const __m256i b256 = _mm256_castps_si256(_mm256_maskload_ps((const float*)(desc + j * nbytes), m256));
#   endif
      if (0 == _mm256_testnzc_si256(a256, b256) && 0 == _mm256_testnzc_si256(b256, a256)) {
        return j;
      }
    }
    return ndescs;
  }
# elif (16 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  assert(16 == nbytes); /* padded descriptor array */
  { /* TODO: implement for 16 Byte descriptor */
    return libxsmm_gemm_diffn_sw(reference, descs, hint, ndescs, nbytes);
  }
# else
  return libxsmm_gemm_diffn_sw(reference, descs, hint, ndescs, nbytes);
# endif
#else
  { static int error_once = 0;
    if (0 != libxsmm_verbosity /* library code is expected to be mute */
     && 1 == LIBXSMM_ATOMIC_ADD_FETCH(&error_once, 1, LIBXSMM_ATOMIC_RELAXED))
    {
      fprintf(stderr, "LIBXSMM WARNING: unable to enter AVX code path!\n");
    }
  }
  return libxsmm_gemm_diffn_sw(reference, descs, hint, ndescs, nbytes);
#endif
}


LIBXSMM_GEMM_DIFF_API_DEFINITION LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX2)
unsigned int libxsmm_gemm_diffn_avx2(const libxsmm_gemm_descriptor* reference,
  const void* descs, unsigned int hint, unsigned int ndescs, int nbytes)
{
#if defined(LIBXSMM_GEMM_DIFF_AVX2)
  assert(/*is pot*/ndescs == LIBXSMM_UP2POT(ndescs));
# if (28 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  assert(32 == nbytes); /* padded descriptor array */
  {
    const unsigned int end = hint + ndescs;
    const char *const desc = (const char*)descs;
    unsigned int i;
    const int yes = 0x80000000, no = 0x0;
    const __m256i m256 = _mm256_set_epi32(no, yes, yes, yes, yes, yes, yes, yes);
#   if defined(LIBXSMM_GEMM_DIFF_MASK_A) || !defined(LIBXSMM_GEMM_DIFF_ZERO_PADDED)
    const __m256i a256 = _mm256_maskload_epi32((const int*)reference, m256);
#   else
    /*const __m256i a256 = _mm256_lddqu_si256((const __m256i*)reference);*/
    const __m256i a256 = _mm256_loadu_si256((const __m256i*)reference);
#   endif
    for (i = hint; i < end; ++i) {
      const unsigned int j = LIBXSMM_MOD2(i, ndescs); /* wrap around index */
#   if defined(LIBXSMM_GEMM_DIFF_ZERO_PADDED)
      const __m256i b256 = _mm256_loadu_si256((const __m256i*)(desc + j * nbytes));
#   else
      const __m256i b256 = _mm256_maskload_epi32((const int*)(desc + j * nbytes), m256);
#   endif
      if (0 == _mm256_testnzc_si256(a256, b256) && 0 == _mm256_testnzc_si256(b256, a256)) {
        return j;
      }
    }
    return ndescs;
  }
# elif (16 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  assert(16 == nbytes); /* padded descriptor array */
  { /* TODO: implement for 16 Byte descriptor */
    return libxsmm_gemm_diffn_avx(reference, descs, hint, ndescs, nbytes);
  }
# else
  return libxsmm_gemm_diffn_avx(reference, descs, hint, ndescs, nbytes);
# endif
#else
  { static int error_once = 0;
    if (0 != libxsmm_verbosity /* library code is expected to be mute */
     && 1 == LIBXSMM_ATOMIC_ADD_FETCH(&error_once, 1, LIBXSMM_ATOMIC_RELAXED))
    {
      fprintf(stderr, "LIBXSMM WARNING: unable to enter AVX2 code path!\n");
    }
  }
  return libxsmm_gemm_diffn_avx(reference, descs, hint, ndescs, nbytes);
#endif
}


LIBXSMM_GEMM_DIFF_API_DEFINITION LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512)
unsigned int libxsmm_gemm_diffn_avx512(const libxsmm_gemm_descriptor* reference,
  const void* descs, unsigned int hint, unsigned int ndescs, int nbytes)
{
#if defined(LIBXSMM_GEMM_DIFF_AVX512) && !defined(LIBXSMM_INTRINSICS_AVX512_NOREDUCTIONS)
  assert(/*is pot*/ndescs == LIBXSMM_UP2POT(ndescs));
# if (28 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  assert(32 == nbytes); /* padded descriptor array */
  {
    const unsigned int hint_even = (hint & 0xFFFFFFFE), end = hint_even + ndescs;
    const char *const desc = (const char*)descs;
    unsigned int i;
    const __mmask16 mask_lo = 0x7F, mask_hi = 0x7F00;
    const int yes = 0x80000000, no = 0x0;
    const __m256i m256 = _mm256_set_epi32(no, yes, yes, yes, yes, yes, yes, yes);
#   if defined(LIBXSMM_GEMM_DIFF_MASK_A)
    const __m256i a256 = _mm256_maskload_epi32((const int*)reference, m256);
#   else
    /* SKX: consider _mm256_maskz_expandloadu_epi32 */
    const __m256i a256 = _mm256_loadu_si256((const __m256i*)reference);
#   endif
    const __m512i a512 = _mm512_broadcast_i64x4(a256);
    for (i = hint_even; i < end; i += 2) {
      const unsigned int j = LIBXSMM_MOD2(i, ndescs); /* wrap around index */
      const __m512i b512 = _mm512_loadu_si512(desc + j * nbytes);
      const __m512i c512 = _mm512_xor_si512(a512, b512);
      if (0 == _mm512_mask_reduce_or_epi32(mask_lo, c512)) {
        return j;
      }
      if (0 == _mm512_mask_reduce_or_epi32(mask_hi, c512)) {
        return j + 1;
      }
    }
    return ndescs;
  }
# elif (16 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  assert(16 == nbytes); /* padded descriptor array */
  { /* TODO: implement for 16 Byte descriptor */
    return libxsmm_gemm_diffn_avx2(reference, descs, hint, ndescs, nbytes);
  }
# else
  return libxsmm_gemm_diffn_avx2(reference, descs, hint, ndescs, nbytes);
# endif
#else
  { static int error_once = 0;
    if (0 != libxsmm_verbosity /* library code is expected to be mute */
     && 1 == LIBXSMM_ATOMIC_ADD_FETCH(&error_once, 1, LIBXSMM_ATOMIC_RELAXED))
    {
      fprintf(stderr, "LIBXSMM WARNING: unable to enter AVX-512 code path!\n");
    }
  }
  return libxsmm_gemm_diffn_avx2(reference, descs, hint, ndescs, nbytes);
#endif
}


LIBXSMM_GEMM_DIFF_API_DEFINITION unsigned int libxsmm_gemm_diffn_imci(const libxsmm_gemm_descriptor* reference,
  const void* descs, unsigned int hint, unsigned int ndescs, int nbytes)
{
#if defined(LIBXSMM_GEMM_DIFF_KNC) && (28 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  assert(/*is pot*/ndescs == LIBXSMM_UP2POT(ndescs));
  assert(32 == nbytes); /* padded descriptor array */
  {
    const unsigned int hint_even = (hint & 0xFFFFFFFE), end = hint_even + ndescs;
    const char *const desc = (const char*)descs;
    unsigned int i;
    const __mmask16 mask_lo = 0x7F, mask_hi = 0x7F00;
    const __m512i h512 = _mm512_mask_loadunpackhi_epi32(
      _mm512_mask_loadunpacklo_epi32(_mm512_set1_epi32(0), mask_lo, reference),
      mask_lo, ((const char*)reference) + 32);
    const __m512i a512 = _mm512_permute4f128_epi32(h512, 0x44);
    for (i = hint_even; i < end; i += 2) {
      const unsigned int j = LIBXSMM_MOD2(i, ndescs); /* wrap around index */
      const char *const d = desc + j * nbytes;
      const __m512i b512 = _mm512_loadunpackhi_epi32(_mm512_loadunpacklo_epi32(_mm512_set1_epi32(0), d), d + 32);
      const __m512i c512 = _mm512_xor_si512(a512, b512);
      if (0 == _mm512_mask_reduce_or_epi32(mask_lo, c512)) {
        return j;
      }
      if (0 == _mm512_mask_reduce_or_epi32(mask_hi, c512)) {
        return j + 1;
      }
    }
    return ndescs;
  }
#elif defined(LIBXSMM_GEMM_DIFF_KNC) && (16 == LIBXSMM_GEMM_DESCRIPTOR_SIZE)
  assert(/*is pot*/ndescs == LIBXSMM_UP2POT(ndescs));
  assert(16 == nbytes); /* padded descriptor array */
  { /* TODO: implement for 16 Byte descriptor */
    return libxsmm_gemm_diffn_sw(reference, descs, hint, ndescs, nbytes);
  }
#else
  return libxsmm_gemm_diffn_sw(reference, descs, hint, ndescs, nbytes);
#endif
}

#endif /*LIBXSMM_GEMM_DIFF_C*/
