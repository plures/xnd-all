/*
* BSD 3-Clause License
*
* Copyright (c) 2017-2018, plures
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <cinttypes>
#include <complex>
#include <cmath>
#include "cpu_device_binary.h"


/*****************************************************************************/
/*                                   Divmod                                  */
/*****************************************************************************/

/* Python: floatobject.c */
static inline void
_divmod(double *q, double *r, double vx, double wx)
{
    double div, mod, floordiv;

    mod = fmod(vx, wx);
    /* fmod is typically exact, so vx-mod is *mathematically* an
       exact multiple of wx.  But this is fp arithmetic, and fp
       vx - mod is an approximation; the result is that div may
       not be an exact integral value after the division, although
       it will always be very close to one.
    */
    div = (vx - mod) / wx;
    if (mod) {
        /* ensure the remainder has the same sign as the denominator */
        if ((wx < 0) != (mod < 0)) {
            mod += wx;
            div -= 1.0;
        }
    }
    else {
        /* the remainder is zero, and in the presence of signed zeroes
           fmod returns different results across platforms; ensure
           it has the same sign as the denominator. */
        mod = copysign(0.0, wx);
    }
    /* snap quotient to nearest integral value */
    if (div) {
        floordiv = floor(div);
        if (div - floordiv > 0.5)
            floordiv += 1.0;
    }
    else {
        /* div is zero - get the same sign as the true quotient */
        floordiv = copysign(0.0, vx / wx); /* zero w/ sign of vx/wx */
    }

    *q = floordiv;
    *r = mod;
}

static inline void
_divmod(float *q, float *r, float vx, float wx)
{
    float div, mod, floordiv;

    mod = fmodf(vx, wx);
    /* fmod is typically exact, so vx-mod is *mathematically* an
       exact multiple of wx.  But this is fp arithmetic, and fp
       vx - mod is an approximation; the result is that div may
       not be an exact integral value after the division, although
       it will always be very close to one.
    */
    div = (vx - mod) / wx;
    if (mod) {
        /* ensure the remainder has the same sign as the denominator */
        if ((wx < 0) != (mod < 0)) {
            mod += wx;
            div -= 1.0;
        }
    }
    else {
        /* the remainder is zero, and in the presence of signed zeroes
           fmod returns different results across platforms; ensure
           it has the same sign as the denominator. */
        mod = copysignf(0.0, wx);
    }
    /* snap quotient to nearest integral value */
    if (div) {
        floordiv = floorf(div);
        if (div - floordiv > 0.5)
            floordiv += 1.0;
    }
    else {
        /* div is zero - get the same sign as the true quotient */
        floordiv = copysignf(0.0, vx / wx); /* zero w/ sign of vx/wx */
    }

    *q = floordiv;
    *r = mod;
}

#define divmod_unsigned(T) \
static inline void \
_divmod(T *q, T *r, T a, T b) \
{                             \
   if (b == 0) {              \
        *q = 0;               \
        *r = 0;               \
   }                          \
   else {                     \
       *q = a / b;            \
       *r = a % b;            \
   }                          \
}

divmod_unsigned(uint8_t)
divmod_unsigned(uint16_t)
divmod_unsigned(uint32_t)
divmod_unsigned(uint64_t)

#define divmod_signed(T, MIN) \
static inline void                                 \
_divmod(T *q, T *r, T a, T b)                      \
{                                                  \
    if (b == 0) {                                  \
        *q = 0;                                    \
        *r = 0;                                    \
    }                                              \
    else if (a == MIN && b == -1) {                \
        *q = MIN;                                  \
        *r = 0;                                    \
    }                                              \
    else {                                         \
        int64_t qq = a / b;                        \
        int64_t rr = a % b;                        \
                                                   \
        *q = rr ? (qq - ((a < 0) ^ (b < 0))) : qq; \
        *r = a - *q * b;                           \
    }                                              \
}

divmod_signed(int8_t, INT8_MIN)
divmod_signed(int16_t, INT16_MIN)
divmod_signed(int32_t, INT32_MIN)
divmod_signed(int64_t, INT64_MIN)

template <class T>
static inline T
_floor_divide(T a, T b)
{
    T q;
    T r;

    _divmod(&q, &r, a, b);

    return q;
}

template <class T>
static inline T
_remainder(T a, T b)
{
    T q;
    T r;

    _divmod(&q, &r, a, b);

    return r;
}


/*****************************************************************************/
/*                Lexicographic comparison for complex numbers               */
/*****************************************************************************/

template <class T>
static inline bool
_isnan(T a)
{
    return std::isnan(a.real()) || std::isnan(a.imag());
}

template <class T, class U>
static inline bool
lexorder_lt(T a, U b)
{
    if (_isnan(a) || _isnan(b)) {
        return false;
    }

    return a.real() < b.real() || (a.real() == b.real() && a.imag() < b.imag());
}

template <class T, class U>
static inline bool
lexorder_le(T a, U b)
{
    if (_isnan(a) || _isnan(b)) {
        return false;
    }

    return a.real() < b.real() || (a.real() == b.real() && a.imag() <= b.imag());
}

template <class T, class U>
static inline bool
lexorder_ge(T a, U b)
{
    if (_isnan(a) || _isnan(b)) {
        return false;
    }

    return a.real() > b.real() || (a.real() == b.real() && a.imag() >= b.imag());
}

template <class T, class U>
static inline bool
lexorder_gt(T a, U b)
{
    if (_isnan(a) || _isnan(b)) {
        return false;
    }

    return a.real() > b.real() || (a.real() == b.real() && a.imag() > b.imag());
}


/*****************************************************************************/
/*                         CPU device binary kernels                         */
/*****************************************************************************/

#define CPU_DEVICE_BINARY(name, func, t0, t1, t2, common) \
extern "C" void                                                          \
gm_cpu_device_fixed_1D_C_##name##_##t0##_##t1##_##t2(                    \
    const char *in0, const char *in1, char *out, int64_t N)              \
{                                                                        \
    const t0##_t *_in0 = (const t0##_t *)in0;                            \
    const t1##_t *_in1 = (const t1##_t *)in1;                            \
    t2##_t *_out = (t2##_t *)out;                                        \
    int64_t i;                                                           \
                                                                         \
    for (i = 0; i < N-7; i += 8) {                                       \
        _out[i] = func((common##_t)_in0[i], (common##_t)_in1[i]);        \
        _out[i+1] = func((common##_t)_in0[i+1], (common##_t)_in1[i+1]);  \
        _out[i+2] = func((common##_t)_in0[i+2], (common##_t)_in1[i+2]);  \
        _out[i+3] = func((common##_t)_in0[i+3], (common##_t)_in1[i+3]);  \
        _out[i+4] = func((common##_t)_in0[i+4], (common##_t)_in1[i+4]);  \
        _out[i+5] = func((common##_t)_in0[i+5], (common##_t)_in1[i+5]);  \
        _out[i+6] = func((common##_t)_in0[i+6], (common##_t)_in1[i+6]);  \
        _out[i+7] = func((common##_t)_in0[i+7], (common##_t)_in1[i+7]);  \
    }                                                                    \
    for (; i < N; i++) {                                                 \
        _out[i] = func((common##_t)_in0[i], (common##_t)_in1[i]);        \
    }                                                                    \
}                                                                        \
                                                                         \
extern "C" void                                                          \
gm_cpu_device_0D_##name##_##t0##_##t1##_##t2(                            \
    const char *in0, const char *in1, char *out)                         \
{                                                                        \
    const t0##_t x = *(const t0##_t *)in0;                               \
    const t1##_t y = *(const t1##_t *)in1;                               \
    *(t2##_t *)out = func((common##_t)x, (common##_t)y);                 \
}

#ifdef _MSC_VER
  #define CPU_DEVICE_BINARYC(name, func, t0, t1, t2, common)
#else
  #define CPU_DEVICE_BINARYC(name, func, t0, t1, t2, common) \
    CPU_DEVICE_BINARY(name, func, t0, t1, t2, common)
#endif

#define CPU_DEVICE_NOIMPL(name, func, t0, t1, t2, common)
#define CPU_DEVICE_NOKERN(name, func, t0, t1, t2, common)


/*****************************************************************************/
/*                                 Arithmetic                                */
/*****************************************************************************/

#define CPU_DEVICE_ALL_BINARY(name, func, hfunc) \
    CPU_DEVICE_BINARY(name, func, uint8, uint8, uint8, uint8)                      \
    CPU_DEVICE_BINARY(name, func, uint8, uint16, uint16, uint16)                   \
    CPU_DEVICE_BINARY(name, func, uint8, uint32, uint32, uint32)                   \
    CPU_DEVICE_BINARY(name, func, uint8, uint64, uint64, uint64)                   \
    CPU_DEVICE_BINARY(name, func, uint8, int8, int16, int16)                       \
    CPU_DEVICE_BINARY(name, func, uint8, int16, int16, int16)                      \
    CPU_DEVICE_BINARY(name, func, uint8, int32, int32, int32)                      \
    CPU_DEVICE_BINARY(name, func, uint8, int64, int64, int64)                      \
    CPU_DEVICE_NOIMPL(name, hfunc, uint8, float16, float16, float16)               \
    CPU_DEVICE_BINARY(name, func, uint8, float32, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, uint8, float64, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, func, uint8, complex32, complex32, complex32)          \
    CPU_DEVICE_BINARYC(name, func, uint8, complex64, complex64, complex64)         \
    CPU_DEVICE_BINARYC(name, func, uint8, complex128, complex128, complex128)      \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, uint16, uint8, uint16, uint16)                   \
    CPU_DEVICE_BINARY(name, func, uint16, uint16, uint16, uint16)                  \
    CPU_DEVICE_BINARY(name, func, uint16, uint32, uint32, uint32)                  \
    CPU_DEVICE_BINARY(name, func, uint16, uint64, uint64, uint64)                  \
    CPU_DEVICE_BINARY(name, func, uint16, int8, int32, int32)                      \
    CPU_DEVICE_BINARY(name, func, uint16, int16, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, uint16, int32, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, uint16, int64, int64, int64)                     \
    CPU_DEVICE_NOIMPL(name, func, uint16, float16, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, uint16, float32, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, uint16, float64, float64, float64)               \
    CPU_DEVICE_NOIMPL(name, func, uint16, complex32, complex64, complex64)         \
    CPU_DEVICE_BINARYC(name, func, uint16, complex64, complex64, complex64)        \
    CPU_DEVICE_BINARYC(name, func, uint16, complex128, complex128, complex128)     \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, uint32, uint8, uint32, uint32)                   \
    CPU_DEVICE_BINARY(name, func, uint32, uint16, uint32, uint32)                  \
    CPU_DEVICE_BINARY(name, func, uint32, uint32, uint32, uint32)                  \
    CPU_DEVICE_BINARY(name, func, uint32, uint64, uint64, uint64)                  \
    CPU_DEVICE_BINARY(name, func, uint32, int8, int64, int64)                      \
    CPU_DEVICE_BINARY(name, func, uint32, int16, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, uint32, int32, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, uint32, int64, int64, int64)                     \
    CPU_DEVICE_NOIMPL(name, func, uint32, float16, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, uint32, float32, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, uint32, float64, float64, float64)               \
    CPU_DEVICE_NOIMPL(name, func, uint32, complex32, complex128, complex128)       \
    CPU_DEVICE_BINARYC(name, func, uint32, complex64, complex128, complex128)      \
    CPU_DEVICE_BINARYC(name, func, uint32, complex128, complex128, complex128)     \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, uint64, uint8, uint64, uint64)                   \
    CPU_DEVICE_BINARY(name, func, uint64, uint16, uint64, uint64)                  \
    CPU_DEVICE_BINARY(name, func, uint64, uint32, uint64, uint64)                  \
    CPU_DEVICE_BINARY(name, func, uint64, uint64, uint64, uint64)                  \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, int8, uint8, int16, int16)                       \
    CPU_DEVICE_BINARY(name, func, int8, uint16, int32, int32)                      \
    CPU_DEVICE_BINARY(name, func, int8, uint32, int64, int64)                      \
    CPU_DEVICE_BINARY(name, func, int8, int8, int8, int8)                          \
    CPU_DEVICE_BINARY(name, func, int8, int16, int16, int16)                       \
    CPU_DEVICE_BINARY(name, func, int8, int32, int32, int32)                       \
    CPU_DEVICE_BINARY(name, func, int8, int64, int64, int64)                       \
    CPU_DEVICE_NOIMPL(name, hfunc, int8, float16, float16, float16)                \
    CPU_DEVICE_BINARY(name, func, int8, float32, float32, float32)                 \
    CPU_DEVICE_BINARY(name, func, int8, float64, float64, float64)                 \
    CPU_DEVICE_NOIMPL(name, func, int8, complex32, complex32, complex32)           \
    CPU_DEVICE_BINARYC(name, func, int8, complex64, complex64, complex64)          \
    CPU_DEVICE_BINARYC(name, func, int8, complex128, complex128, complex128)       \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, int16, uint8, int16, int16)                      \
    CPU_DEVICE_BINARY(name, func, int16, uint16, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, int16, uint32, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, int16, int8, int16, int16)                       \
    CPU_DEVICE_BINARY(name, func, int16, int16, int16, int16)                      \
    CPU_DEVICE_BINARY(name, func, int16, int32, int32, int32)                      \
    CPU_DEVICE_BINARY(name, func, int16, int64, int64, int64)                      \
    CPU_DEVICE_NOIMPL(name, func, int16, float16, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, int16, float32, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, int16, float64, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, func, int16, complex32, complex64, complex64)          \
    CPU_DEVICE_BINARYC(name, func, int16, complex64, complex64, complex64)         \
    CPU_DEVICE_BINARYC(name, func, int16, complex128, complex128, complex128)      \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, int32, uint8, int32, int32)                      \
    CPU_DEVICE_BINARY(name, func, int32, uint16, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, int32, uint32, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, int32, int8, int32, int32)                       \
    CPU_DEVICE_BINARY(name, func, int32, int16, int32, int32)                      \
    CPU_DEVICE_BINARY(name, func, int32, int32, int32, int32)                      \
    CPU_DEVICE_BINARY(name, func, int32, int64, int64, int64)                      \
    CPU_DEVICE_NOIMPL(name, func, int32, float16, float64, float64)                \
    CPU_DEVICE_BINARY(name, func, int32, float32, float64, float64)                \
    CPU_DEVICE_BINARY(name, func, int32, float64, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, func, int32, complex32, complex128, complex128)        \
    CPU_DEVICE_BINARYC(name, func, int32, complex64, complex128, complex128)       \
    CPU_DEVICE_BINARYC(name, func, int32, complex128, complex128, complex128)      \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, int64, uint8, int64, int64)                      \
    CPU_DEVICE_BINARY(name, func, int64, uint16, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, int64, uint32, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, int64, int8, int64, int64)                       \
    CPU_DEVICE_BINARY(name, func, int64, int16, int64, int64)                      \
    CPU_DEVICE_BINARY(name, func, int64, int32, int64, int64)                      \
    CPU_DEVICE_BINARY(name, func, int64, int64, int64, int64)                      \
                                                                                   \
    CPU_DEVICE_NOIMPL(name, hfunc, float16, uint8, float16, float16)               \
    CPU_DEVICE_NOIMPL(name, func, float16, uint16, float32, float32)               \
    CPU_DEVICE_NOIMPL(name, func, float16, uint32, float64, float64)               \
    CPU_DEVICE_NOIMPL(name, hfunc, float16, int8, float16, float16)                \
    CPU_DEVICE_NOIMPL(name, func, float16, int16, float32, float32)                \
    CPU_DEVICE_NOIMPL(name, func, float16, int32, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, hfunc, float16, float16, float16, float16)             \
    CPU_DEVICE_NOIMPL(name, func, float16, float32, float32, float32)              \
    CPU_DEVICE_NOIMPL(name, func, float16, float64, float64, float64)              \
    CPU_DEVICE_NOIMPL(name, func, float16, complex32, complex32, complex32)        \
    CPU_DEVICE_NOIMPL(name, func, float16, complex64, complex64, complex64)        \
    CPU_DEVICE_NOIMPL(name, func, float16, complex128, complex128, complex128)     \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, float32, uint8, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, float32, uint16, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, float32, uint32, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, float32, int8, float32, float32)                 \
    CPU_DEVICE_BINARY(name, func, float32, int16, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, float32, int32, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, func, float32, float16, float32, float32)              \
    CPU_DEVICE_BINARY(name, func, float32, float32, float32, float32)              \
    CPU_DEVICE_BINARY(name, func, float32, float64, float64, float64)              \
    CPU_DEVICE_NOIMPL(name, func, float32, complex32, complex64, complex64)        \
    CPU_DEVICE_BINARYC(name, func, float32, complex64, complex64, complex64)       \
    CPU_DEVICE_BINARYC(name, func, float32, complex128, complex128, complex128)    \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, float64, uint8, float64, float64)                \
    CPU_DEVICE_BINARY(name, func, float64, uint16, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, float64, uint32, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, float64, int8, float64, float64)                 \
    CPU_DEVICE_BINARY(name, func, float64, int16, float64, float64)                \
    CPU_DEVICE_BINARY(name, func, float64, int32, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, func, float64, float16, float64, float64)              \
    CPU_DEVICE_BINARY(name, func, float64, float32, float64, float64)              \
    CPU_DEVICE_BINARY(name, func, float64, float64, float64, float64)              \
    CPU_DEVICE_NOIMPL(name, func, float64, complex32, complex128, complex128)      \
    CPU_DEVICE_BINARYC(name, func, float64, complex64, complex128, complex128)     \
    CPU_DEVICE_BINARYC(name, func, float64, complex128, complex128, complex128)    \
                                                                                   \
    CPU_DEVICE_NOIMPL(name, func, complex32, uint8, complex32, complex32)          \
    CPU_DEVICE_NOIMPL(name, func, complex32, uint16, complex64, complex64)         \
    CPU_DEVICE_NOIMPL(name, func, complex32, uint32, complex128, complex128)       \
    CPU_DEVICE_NOIMPL(name, func, complex32, int8, complex32, complex32)           \
    CPU_DEVICE_NOIMPL(name, func, complex32, int16, complex64, complex64)          \
    CPU_DEVICE_NOIMPL(name, func, complex32, int32, complex128, complex128)        \
    CPU_DEVICE_NOIMPL(name, func, complex32, float16, complex32, complex32)        \
    CPU_DEVICE_NOIMPL(name, func, complex32, float32, complex64, complex64)        \
    CPU_DEVICE_NOIMPL(name, func, complex32, float64, complex128, complex128)      \
    CPU_DEVICE_NOIMPL(name, func, complex32, complex32, complex32, complex32)      \
    CPU_DEVICE_NOIMPL(name, func, complex32, complex64, complex64, complex64)      \
    CPU_DEVICE_NOIMPL(name, func, complex32, complex128, complex128, complex128)   \
                                                                                   \
    CPU_DEVICE_BINARYC(name, func, complex64, uint8, complex64, complex64)         \
    CPU_DEVICE_BINARYC(name, func, complex64, uint16, complex64, complex64)        \
    CPU_DEVICE_BINARYC(name, func, complex64, uint32, complex128, complex128)      \
    CPU_DEVICE_BINARYC(name, func, complex64, int8, complex64, complex64)          \
    CPU_DEVICE_BINARYC(name, func, complex64, int16, complex64, complex64)         \
    CPU_DEVICE_BINARYC(name, func, complex64, int32, complex128, complex128)       \
    CPU_DEVICE_NOIMPL(name, func, complex64, float16, complex64, complex64)        \
    CPU_DEVICE_BINARYC(name, func, complex64, float32, complex64, complex64)       \
    CPU_DEVICE_BINARYC(name, func, complex64, float64, complex128, complex128)     \
    CPU_DEVICE_NOIMPL(name, func, complex64, complex32, complex64, complex64)      \
    CPU_DEVICE_BINARYC(name, func, complex64, complex64, complex64, complex64)     \
    CPU_DEVICE_BINARYC(name, func, complex64, complex128, complex128, complex128)  \
                                                                                   \
    CPU_DEVICE_BINARYC(name, func, complex128, uint8, complex128, complex128)      \
    CPU_DEVICE_BINARYC(name, func, complex128, uint16, complex128, complex128)     \
    CPU_DEVICE_BINARYC(name, func, complex128, uint32, complex128, complex128)     \
    CPU_DEVICE_BINARYC(name, func, complex128, int8, complex128, complex128)       \
    CPU_DEVICE_BINARYC(name, func, complex128, int16, complex128, complex128)      \
    CPU_DEVICE_BINARYC(name, func, complex128, int32, complex128, complex128)      \
    CPU_DEVICE_NOIMPL(name, func, complex128, float16, complex128, complex128)     \
    CPU_DEVICE_BINARYC(name, func, complex128, float32, complex128, complex128)    \
    CPU_DEVICE_BINARYC(name, func, complex128, float64, complex128, complex128)    \
    CPU_DEVICE_NOIMPL(name, func, complex128, complex32, complex128, complex128)   \
    CPU_DEVICE_BINARYC(name, func, complex128, complex64, complex128, complex128)  \
    CPU_DEVICE_BINARYC(name, func, complex128, complex128, complex128, complex128) \

#define CPU_DEVICE_ALL_BINARY_NO_COMPLEX(name, func, hfunc) \
    CPU_DEVICE_BINARY(name, func, uint8, uint8, uint8, uint8)                     \
    CPU_DEVICE_BINARY(name, func, uint8, uint16, uint16, uint16)                  \
    CPU_DEVICE_BINARY(name, func, uint8, uint32, uint32, uint32)                  \
    CPU_DEVICE_BINARY(name, func, uint8, uint64, uint64, uint64)                  \
    CPU_DEVICE_BINARY(name, func, uint8, int8, int16, int16)                      \
    CPU_DEVICE_BINARY(name, func, uint8, int16, int16, int16)                     \
    CPU_DEVICE_BINARY(name, func, uint8, int32, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, uint8, int64, int64, int64)                     \
    CPU_DEVICE_NOIMPL(name, hfunc, uint8, float16, float16, float16)              \
    CPU_DEVICE_BINARY(name, func, uint8, float32, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, uint8, float64, float64, float64)               \
    CPU_DEVICE_NOKERN(name, func, uint8, complex32, complex32, complex32)         \
    CPU_DEVICE_NOKERN(name, func, uint8, complex64, complex64, complex64)         \
    CPU_DEVICE_NOKERN(name, func, uint8, complex128, complex128, complex128)      \
                                                                                  \
    CPU_DEVICE_BINARY(name, func, uint16, uint8, uint16, uint16)                  \
    CPU_DEVICE_BINARY(name, func, uint16, uint16, uint16, uint16)                 \
    CPU_DEVICE_BINARY(name, func, uint16, uint32, uint32, uint32)                 \
    CPU_DEVICE_BINARY(name, func, uint16, uint64, uint64, uint64)                 \
    CPU_DEVICE_BINARY(name, func, uint16, int8, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, uint16, int16, int32, int32)                    \
    CPU_DEVICE_BINARY(name, func, uint16, int32, int32, int32)                    \
    CPU_DEVICE_BINARY(name, func, uint16, int64, int64, int64)                    \
    CPU_DEVICE_NOIMPL(name, func, uint16, float16, float32, float32)              \
    CPU_DEVICE_BINARY(name, func, uint16, float32, float32, float32)              \
    CPU_DEVICE_BINARY(name, func, uint16, float64, float64, float64)              \
    CPU_DEVICE_NOKERN(name, func, uint16, complex32, complex64, complex64)        \
    CPU_DEVICE_NOKERN(name, func, uint16, complex64, complex64, complex64)        \
    CPU_DEVICE_NOKERN(name, func, uint16, complex128, complex128, complex128)     \
                                                                                  \
    CPU_DEVICE_BINARY(name, func, uint32, uint8, uint32, uint32)                  \
    CPU_DEVICE_BINARY(name, func, uint32, uint16, uint32, uint32)                 \
    CPU_DEVICE_BINARY(name, func, uint32, uint32, uint32, uint32)                 \
    CPU_DEVICE_BINARY(name, func, uint32, uint64, uint64, uint64)                 \
    CPU_DEVICE_BINARY(name, func, uint32, int8, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, uint32, int16, int64, int64)                    \
    CPU_DEVICE_BINARY(name, func, uint32, int32, int64, int64)                    \
    CPU_DEVICE_BINARY(name, func, uint32, int64, int64, int64)                    \
    CPU_DEVICE_NOIMPL(name, func, uint32, float16, float64, float64)              \
    CPU_DEVICE_BINARY(name, func, uint32, float32, float64, float64)              \
    CPU_DEVICE_BINARY(name, func, uint32, float64, float64, float64)              \
    CPU_DEVICE_NOKERN(name, func, uint32, complex32, complex128, complex128)      \
    CPU_DEVICE_NOKERN(name, func, uint32, complex64, complex128, complex128)      \
    CPU_DEVICE_NOKERN(name, func, uint32, complex128, complex128, complex128)     \
                                                                                  \
    CPU_DEVICE_BINARY(name, func, uint64, uint8, uint64, uint64)                  \
    CPU_DEVICE_BINARY(name, func, uint64, uint16, uint64, uint64)                 \
    CPU_DEVICE_BINARY(name, func, uint64, uint32, uint64, uint64)                 \
    CPU_DEVICE_BINARY(name, func, uint64, uint64, uint64, uint64)                 \
                                                                                  \
    CPU_DEVICE_BINARY(name, func, int8, uint8, int16, int16)                      \
    CPU_DEVICE_BINARY(name, func, int8, uint16, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, int8, uint32, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, int8, int8, int8, int8)                         \
    CPU_DEVICE_BINARY(name, func, int8, int16, int16, int16)                      \
    CPU_DEVICE_BINARY(name, func, int8, int32, int32, int32)                      \
    CPU_DEVICE_BINARY(name, func, int8, int64, int64, int64)                      \
    CPU_DEVICE_NOIMPL(name, hfunc, int8, float16, float16, float16)               \
    CPU_DEVICE_BINARY(name, func, int8, float32, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, int8, float64, float64, float64)                \
    CPU_DEVICE_NOKERN(name, func, int8, complex32, complex32, complex32)          \
    CPU_DEVICE_NOKERN(name, func, int8, complex64, complex64, complex64)          \
    CPU_DEVICE_NOKERN(name, func, int8, complex128, complex128, complex128)       \
                                                                                  \
    CPU_DEVICE_BINARY(name, func, int16, uint8, int16, int16)                     \
    CPU_DEVICE_BINARY(name, func, int16, uint16, int32, int32)                    \
    CPU_DEVICE_BINARY(name, func, int16, uint32, int64, int64)                    \
    CPU_DEVICE_BINARY(name, func, int16, int8, int16, int16)                      \
    CPU_DEVICE_BINARY(name, func, int16, int16, int16, int16)                     \
    CPU_DEVICE_BINARY(name, func, int16, int32, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, int16, int64, int64, int64)                     \
    CPU_DEVICE_NOIMPL(name, func, int16, float16, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, int16, float32, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, int16, float64, float64, float64)               \
    CPU_DEVICE_NOKERN(name, func, int16, complex32, complex64, complex64)         \
    CPU_DEVICE_NOKERN(name, func, int16, complex64, complex64, complex64)         \
    CPU_DEVICE_NOKERN(name, func, int16, complex128, complex128, complex128)      \
                                                                                  \
    CPU_DEVICE_BINARY(name, func, int32, uint8, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, int32, uint16, int32, int32)                    \
    CPU_DEVICE_BINARY(name, func, int32, uint32, int64, int64)                    \
    CPU_DEVICE_BINARY(name, func, int32, int8, int32, int32)                      \
    CPU_DEVICE_BINARY(name, func, int32, int16, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, int32, int32, int32, int32)                     \
    CPU_DEVICE_BINARY(name, func, int32, int64, int64, int64)                     \
    CPU_DEVICE_NOIMPL(name, func, int32, float16, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, int32, float32, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, int32, float64, float64, float64)               \
    CPU_DEVICE_NOKERN(name, func, int32, complex32, complex128, complex128)       \
    CPU_DEVICE_NOKERN(name, func, int32, complex64, complex128, complex128)       \
    CPU_DEVICE_NOKERN(name, func, int32, complex128, complex128, complex128)      \
                                                                                  \
    CPU_DEVICE_BINARY(name, func, int64, uint8, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, int64, uint16, int64, int64)                    \
    CPU_DEVICE_BINARY(name, func, int64, uint32, int64, int64)                    \
    CPU_DEVICE_BINARY(name, func, int64, int8, int64, int64)                      \
    CPU_DEVICE_BINARY(name, func, int64, int16, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, int64, int32, int64, int64)                     \
    CPU_DEVICE_BINARY(name, func, int64, int64, int64, int64)                     \
                                                                                  \
    CPU_DEVICE_NOIMPL(name, hfunc, float16, uint8, float16, float16)              \
    CPU_DEVICE_NOIMPL(name, func, float16, uint16, float32, float32)              \
    CPU_DEVICE_NOIMPL(name, func, float16, uint32, float64, float64)              \
    CPU_DEVICE_NOIMPL(name, hfunc, float16, int8, float16, float16)               \
    CPU_DEVICE_NOIMPL(name, func, float16, int16, float32, float32)               \
    CPU_DEVICE_NOIMPL(name, func, float16, int32, float64, float64)               \
    CPU_DEVICE_NOIMPL(name, hfunc, float16, float16, float16, float16)            \
    CPU_DEVICE_NOIMPL(name, func, float16, float32, float32, float32)             \
    CPU_DEVICE_NOIMPL(name, func, float16, float64, float64, float64)             \
    CPU_DEVICE_NOKERN(name, func, float16, complex32, complex32, complex32)       \
    CPU_DEVICE_NOKERN(name, func, float16, complex64, complex64, complex64)       \
    CPU_DEVICE_NOKERN(name, func, float16, complex128, complex128, complex128)    \
                                                                                  \
    CPU_DEVICE_BINARY(name, func, float32, uint8, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, float32, uint16, float32, float32)              \
    CPU_DEVICE_BINARY(name, func, float32, uint32, float64, float64)              \
    CPU_DEVICE_BINARY(name, func, float32, int8, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, float32, int16, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, float32, int32, float64, float64)               \
    CPU_DEVICE_NOIMPL(name, func, float32, float16, float32, float32)             \
    CPU_DEVICE_BINARY(name, func, float32, float32, float32, float32)             \
    CPU_DEVICE_BINARY(name, func, float32, float64, float64, float64)             \
    CPU_DEVICE_NOKERN(name, func, float32, complex32, complex64, complex64)       \
    CPU_DEVICE_NOKERN(name, func, float32, complex64, complex64, complex64)       \
    CPU_DEVICE_NOKERN(name, func, float32, complex128, complex128, complex128)    \
                                                                                  \
    CPU_DEVICE_BINARY(name, func, float64, uint8, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, float64, uint16, float64, float64)              \
    CPU_DEVICE_BINARY(name, func, float64, uint32, float64, float64)              \
    CPU_DEVICE_BINARY(name, func, float64, int8, float64, float64)                \
    CPU_DEVICE_BINARY(name, func, float64, int16, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, float64, int32, float64, float64)               \
    CPU_DEVICE_NOIMPL(name, func, float64, float16, float64, float64)             \
    CPU_DEVICE_BINARY(name, func, float64, float32, float64, float64)             \
    CPU_DEVICE_BINARY(name, func, float64, float64, float64, float64)             \
    CPU_DEVICE_NOKERN(name, func, float64, complex32, complex128, complex128)     \
    CPU_DEVICE_NOKERN(name, func, float64, complex64, complex128, complex128)     \
    CPU_DEVICE_NOKERN(name, func, float64, complex128, complex128, complex128)    \
                                                                                  \
    CPU_DEVICE_NOKERN(name, func, complex32, uint8, complex32, complex32)         \
    CPU_DEVICE_NOKERN(name, func, complex32, uint16, complex64, complex64)        \
    CPU_DEVICE_NOKERN(name, func, complex32, uint32, complex128, complex128)      \
    CPU_DEVICE_NOKERN(name, func, complex32, int8, complex32, complex32)          \
    CPU_DEVICE_NOKERN(name, func, complex32, int16, complex64, complex64)         \
    CPU_DEVICE_NOKERN(name, func, complex32, int32, complex128, complex128)       \
    CPU_DEVICE_NOKERN(name, func, complex32, float16, complex32, complex32)       \
    CPU_DEVICE_NOKERN(name, func, complex32, float32, complex64, complex64)       \
    CPU_DEVICE_NOKERN(name, func, complex32, float64, complex128, complex128)     \
    CPU_DEVICE_NOKERN(name, func, complex32, complex32, complex32, complex32)     \
    CPU_DEVICE_NOKERN(name, func, complex32, complex64, complex64, complex64)     \
    CPU_DEVICE_NOKERN(name, func, complex32, complex128, complex128, complex128)  \
                                                                                  \
    CPU_DEVICE_NOKERN(name, func, complex64, uint8, complex64, complex64)         \
    CPU_DEVICE_NOKERN(name, func, complex64, uint16, complex64, complex64)        \
    CPU_DEVICE_NOKERN(name, func, complex64, uint32, complex128, complex128)      \
    CPU_DEVICE_NOKERN(name, func, complex64, int8, complex64, complex64)          \
    CPU_DEVICE_NOKERN(name, func, complex64, int16, complex64, complex64)         \
    CPU_DEVICE_NOKERN(name, func, complex64, int32, complex128, complex128)       \
    CPU_DEVICE_NOKERN(name, func, complex64, float16, complex64, complex64)       \
    CPU_DEVICE_NOKERN(name, func, complex64, float32, complex64, complex64)       \
    CPU_DEVICE_NOKERN(name, func, complex64, float64, complex128, complex128)     \
    CPU_DEVICE_NOKERN(name, func, complex64, complex32, complex64, complex64)     \
    CPU_DEVICE_NOKERN(name, func, complex64, complex64, complex64, complex64)     \
    CPU_DEVICE_NOKERN(name, func, complex64, complex128, complex128, complex128)  \
                                                                                  \
    CPU_DEVICE_NOKERN(name, func, complex128, uint8, complex128, complex128)      \
    CPU_DEVICE_NOKERN(name, func, complex128, uint16, complex128, complex128)     \
    CPU_DEVICE_NOKERN(name, func, complex128, uint32, complex128, complex128)     \
    CPU_DEVICE_NOKERN(name, func, complex128, int8, complex128, complex128)       \
    CPU_DEVICE_NOKERN(name, func, complex128, int16, complex128, complex128)      \
    CPU_DEVICE_NOKERN(name, func, complex128, int32, complex128, complex128)      \
    CPU_DEVICE_NOIMPL(name, func, complex128, float16, complex128, complex128)    \
    CPU_DEVICE_NOKERN(name, func, complex128, float32, complex128, complex128)    \
    CPU_DEVICE_NOKERN(name, func, complex128, float64, complex128, complex128)    \
    CPU_DEVICE_NOKERN(name, func, complex128, complex32, complex128, complex128)  \
    CPU_DEVICE_NOKERN(name, func, complex128, complex64, complex128, complex128)  \
    CPU_DEVICE_NOKERN(name, func, complex128, complex128, complex128, complex128) \

#define CPU_DEVICE_ALL_BINARY_FLOAT_RETURN(name, func, hfunc) \
    CPU_DEVICE_NOIMPL(name, hfunc, uint8, uint8, float16, float16)                 \
    CPU_DEVICE_BINARY(name, func, uint8, uint16, float32, float32)                 \
    CPU_DEVICE_BINARY(name, func, uint8, uint32, float64, float64)                 \
    CPU_DEVICE_NOKERN(name, func, uint8, uint64, uint64, uint64)                   \
    CPU_DEVICE_NOIMPL(name, hfunc, uint8, int8, float16, float16)                  \
    CPU_DEVICE_BINARY(name, func, uint8, int16, float32, float32)                  \
    CPU_DEVICE_BINARY(name, func, uint8, int32, float64, float64)                  \
    CPU_DEVICE_NOKERN(name, func, uint8, int64, int64, int64)                      \
    CPU_DEVICE_NOIMPL(name, hfunc, uint8, float16, float16, float16)               \
    CPU_DEVICE_BINARY(name, func, uint8, float32, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, uint8, float64, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, func, uint8, complex32, complex32, complex32)          \
    CPU_DEVICE_BINARYC(name, func, uint8, complex64, complex64, complex64)         \
    CPU_DEVICE_BINARYC(name, func, uint8, complex128, complex128, complex128)      \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, uint16, uint8, float32, float32)                 \
    CPU_DEVICE_BINARY(name, func, uint16, uint16, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, uint16, uint32, float64, float64)                \
    CPU_DEVICE_NOKERN(name, func, uint16, uint64, uint64, uint64)                  \
    CPU_DEVICE_BINARY(name, func, uint16, int8, float32, float32)                  \
    CPU_DEVICE_BINARY(name, func, uint16, int16, float32, float32)                 \
    CPU_DEVICE_BINARY(name, func, uint16, int32, float64, float64)                 \
    CPU_DEVICE_NOKERN(name, func, uint16, int64, int64, int64)                     \
    CPU_DEVICE_NOIMPL(name, func, uint16, float16, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, uint16, float32, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, uint16, float64, float64, float64)               \
    CPU_DEVICE_NOIMPL(name, func, uint16, complex32, complex64, complex64)         \
    CPU_DEVICE_BINARYC(name, func, uint16, complex64, complex64, complex64)        \
    CPU_DEVICE_BINARYC(name, func, uint16, complex128, complex128, complex128)     \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, uint32, uint8, float64, float64)                 \
    CPU_DEVICE_BINARY(name, func, uint32, uint16, float64, float64)                \
    CPU_DEVICE_BINARY(name, func, uint32, uint32, float64, float64)                \
    CPU_DEVICE_NOKERN(name, func, uint32, uint64, uint64, uint64)                  \
    CPU_DEVICE_BINARY(name, func, uint32, int8, float64, float64)                  \
    CPU_DEVICE_BINARY(name, func, uint32, int16, float64, float64)                 \
    CPU_DEVICE_BINARY(name, func, uint32, int32, float64, float64)                 \
    CPU_DEVICE_NOKERN(name, func, uint32, int64, int64, int64)                     \
    CPU_DEVICE_NOIMPL(name, func, uint32, float16, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, uint32, float32, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, uint32, float64, float64, float64)               \
    CPU_DEVICE_NOIMPL(name, func, uint32, complex32, complex128, complex128)       \
    CPU_DEVICE_BINARYC(name, func, uint32, complex64, complex128, complex128)      \
    CPU_DEVICE_BINARYC(name, func, uint32, complex128, complex128, complex128)     \
                                                                                   \
    CPU_DEVICE_NOKERN(name, func, uint64, uint8, uint64, uint64)                   \
    CPU_DEVICE_NOKERN(name, func, uint64, uint16, uint64, uint64)                  \
    CPU_DEVICE_NOKERN(name, func, uint64, uint32, uint64, uint64)                  \
    CPU_DEVICE_NOKERN(name, func, uint64, uint64, uint64, uint64)                  \
                                                                                   \
    CPU_DEVICE_NOIMPL(name, hfunc, int8, uint8, float16, float16)                  \
    CPU_DEVICE_BINARY(name, func, int8, uint16, float32, float32)                  \
    CPU_DEVICE_BINARY(name, func, int8, uint32, float64, float64)                  \
    CPU_DEVICE_NOIMPL(name, hfunc, int8, int8, float16, float16)                   \
    CPU_DEVICE_BINARY(name, func, int8, int16, float32, float32)                   \
    CPU_DEVICE_BINARY(name, func, int8, int32, float64, float64)                   \
    CPU_DEVICE_NOKERN(name, func, int8, int64, int64, int64)                       \
    CPU_DEVICE_NOIMPL(name, hfunc, int8, float16, float16, float16)                \
    CPU_DEVICE_BINARY(name, func, int8, float32, float32, float32)                 \
    CPU_DEVICE_BINARY(name, func, int8, float64, float64, float64)                 \
    CPU_DEVICE_NOIMPL(name, func, int8, complex32, complex32, complex32)           \
    CPU_DEVICE_BINARYC(name, func, int8, complex64, complex64, complex64)          \
    CPU_DEVICE_BINARYC(name, func, int8, complex128, complex128, complex128)       \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, int16, uint8, float32, float32)                  \
    CPU_DEVICE_BINARY(name, func, int16, uint16, float32, float32)                 \
    CPU_DEVICE_BINARY(name, func, int16, uint32, float64, float64)                 \
    CPU_DEVICE_BINARY(name, func, int16, int8, float32, float32)                   \
    CPU_DEVICE_BINARY(name, func, int16, int16, float32, float32)                  \
    CPU_DEVICE_BINARY(name, func, int16, int32, float64, float64)                  \
    CPU_DEVICE_NOKERN(name, func, int16, int64, int64, int64)                      \
    CPU_DEVICE_NOIMPL(name, func, int16, float16, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, int16, float32, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, int16, float64, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, func, int16, complex32, complex64, complex64)          \
    CPU_DEVICE_BINARYC(name, func, int16, complex64, complex64, complex64)         \
    CPU_DEVICE_BINARYC(name, func, int16, complex128, complex128, complex128)      \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, int32, uint8, float64, float64)                  \
    CPU_DEVICE_BINARY(name, func, int32, uint16, float64, float64)                 \
    CPU_DEVICE_BINARY(name, func, int32, uint32, float64, float64)                 \
    CPU_DEVICE_BINARY(name, func, int32, int8, float64, float64)                   \
    CPU_DEVICE_BINARY(name, func, int32, int16, float64, float64)                  \
    CPU_DEVICE_BINARY(name, func, int32, int32, float64, float64)                  \
    CPU_DEVICE_NOKERN(name, func, int32, int64, int64, int64)                      \
    CPU_DEVICE_NOIMPL(name, func, int32, float16, float64, float64)                \
    CPU_DEVICE_BINARY(name, func, int32, float32, float64, float64)                \
    CPU_DEVICE_BINARY(name, func, int32, float64, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, func, int32, complex32, complex128, complex128)        \
    CPU_DEVICE_BINARYC(name, func, int32, complex64, complex128, complex128)       \
    CPU_DEVICE_BINARYC(name, func, int32, complex128, complex128, complex128)      \
                                                                                   \
    CPU_DEVICE_NOKERN(name, func, int64, uint8, int64, int64)                      \
    CPU_DEVICE_NOKERN(name, func, int64, uint16, int64, int64)                     \
    CPU_DEVICE_NOKERN(name, func, int64, uint32, int64, int64)                     \
    CPU_DEVICE_NOKERN(name, func, int64, int8, int64, int64)                       \
    CPU_DEVICE_NOKERN(name, func, int64, int16, int64, int64)                      \
    CPU_DEVICE_NOKERN(name, func, int64, int32, int64, int64)                      \
    CPU_DEVICE_NOKERN(name, func, int64, int64, int64, int64)                      \
                                                                                   \
    CPU_DEVICE_NOIMPL(name, hfunc, float16, uint8, float16, float16)               \
    CPU_DEVICE_NOIMPL(name, func, float16, uint16, float32, float32)               \
    CPU_DEVICE_NOIMPL(name, func, float16, uint32, float64, float64)               \
    CPU_DEVICE_NOIMPL(name, hfunc, float16, int8, float16, float16)                \
    CPU_DEVICE_NOIMPL(name, func, float16, int16, float32, float32)                \
    CPU_DEVICE_NOIMPL(name, func, float16, int32, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, hfunc, float16, float16, float16, float16)             \
    CPU_DEVICE_NOIMPL(name, func, float16, float32, float32, float32)              \
    CPU_DEVICE_NOIMPL(name, func, float16, float64, float64, float64)              \
    CPU_DEVICE_NOIMPL(name, func, float16, complex32, complex32, complex32)        \
    CPU_DEVICE_NOIMPL(name, func, float16, complex64, complex64, complex64)        \
    CPU_DEVICE_NOIMPL(name, func, float16, complex128, complex128, complex128)     \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, float32, uint8, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, float32, uint16, float32, float32)               \
    CPU_DEVICE_BINARY(name, func, float32, uint32, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, float32, int8, float32, float32)                 \
    CPU_DEVICE_BINARY(name, func, float32, int16, float32, float32)                \
    CPU_DEVICE_BINARY(name, func, float32, int32, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, func, float32, float16, float32, float32)              \
    CPU_DEVICE_BINARY(name, func, float32, float32, float32, float32)              \
    CPU_DEVICE_BINARY(name, func, float32, float64, float64, float64)              \
    CPU_DEVICE_NOIMPL(name, func, float32, complex32, complex64, complex64)        \
    CPU_DEVICE_BINARYC(name, func, float32, complex64, complex64, complex64)       \
    CPU_DEVICE_BINARYC(name, func, float32, complex128, complex128, complex128)    \
                                                                                   \
    CPU_DEVICE_BINARY(name, func, float64, uint8, float64, float64)                \
    CPU_DEVICE_BINARY(name, func, float64, uint16, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, float64, uint32, float64, float64)               \
    CPU_DEVICE_BINARY(name, func, float64, int8, float64, float64)                 \
    CPU_DEVICE_BINARY(name, func, float64, int16, float64, float64)                \
    CPU_DEVICE_BINARY(name, func, float64, int32, float64, float64)                \
    CPU_DEVICE_NOIMPL(name, func, float64, float16, float64, float64)              \
    CPU_DEVICE_BINARY(name, func, float64, float32, float64, float64)              \
    CPU_DEVICE_BINARY(name, func, float64, float64, float64, float64)              \
    CPU_DEVICE_NOIMPL(name, func, float64, complex32, complex128, complex128)      \
    CPU_DEVICE_BINARYC(name, func, float64, complex64, complex128, complex128)     \
    CPU_DEVICE_BINARYC(name, func, float64, complex128, complex128, complex128)    \
                                                                                   \
    CPU_DEVICE_NOIMPL(name, func, complex32, uint8, complex32, complex32)          \
    CPU_DEVICE_NOIMPL(name, func, complex32, uint16, complex64, complex64)         \
    CPU_DEVICE_NOIMPL(name, func, complex32, uint32, complex128, complex128)       \
    CPU_DEVICE_NOIMPL(name, func, complex32, int8, complex32, complex32)           \
    CPU_DEVICE_NOIMPL(name, func, complex32, int16, complex64, complex64)          \
    CPU_DEVICE_NOIMPL(name, func, complex32, int32, complex128, complex128)        \
    CPU_DEVICE_NOIMPL(name, func, complex32, float16, complex32, complex32)        \
    CPU_DEVICE_NOIMPL(name, func, complex32, float32, complex64, complex64)        \
    CPU_DEVICE_NOIMPL(name, func, complex32, float64, complex128, complex128)      \
    CPU_DEVICE_NOIMPL(name, func, complex32, complex32, complex32, complex32)      \
    CPU_DEVICE_NOIMPL(name, func, complex32, complex64, complex64, complex64)      \
    CPU_DEVICE_NOIMPL(name, func, complex32, complex128, complex128, complex128)   \
                                                                                   \
    CPU_DEVICE_BINARYC(name, func, complex64, uint8, complex64, complex64)         \
    CPU_DEVICE_BINARYC(name, func, complex64, uint16, complex64, complex64)        \
    CPU_DEVICE_BINARYC(name, func, complex64, uint32, complex128, complex128)      \
    CPU_DEVICE_BINARYC(name, func, complex64, int8, complex64, complex64)          \
    CPU_DEVICE_BINARYC(name, func, complex64, int16, complex64, complex64)         \
    CPU_DEVICE_BINARYC(name, func, complex64, int32, complex128, complex128)       \
    CPU_DEVICE_NOIMPL(name, func, complex64, float16, complex64, complex64)        \
    CPU_DEVICE_BINARYC(name, func, complex64, float32, complex64, complex64)       \
    CPU_DEVICE_BINARYC(name, func, complex64, float64, complex128, complex128)     \
    CPU_DEVICE_NOIMPL(name, func, complex64, complex32, complex64, complex64)      \
    CPU_DEVICE_BINARYC(name, func, complex64, complex64, complex64, complex64)     \
    CPU_DEVICE_BINARYC(name, func, complex64, complex128, complex128, complex128)  \
                                                                                   \
    CPU_DEVICE_BINARYC(name, func, complex128, uint8, complex128, complex128)      \
    CPU_DEVICE_BINARYC(name, func, complex128, uint16, complex128, complex128)     \
    CPU_DEVICE_BINARYC(name, func, complex128, uint32, complex128, complex128)     \
    CPU_DEVICE_BINARYC(name, func, complex128, int8, complex128, complex128)       \
    CPU_DEVICE_BINARYC(name, func, complex128, int16, complex128, complex128)      \
    CPU_DEVICE_BINARYC(name, func, complex128, int32, complex128, complex128)      \
    CPU_DEVICE_NOIMPL(name, func, complex128, float16, complex128, complex128)     \
    CPU_DEVICE_BINARYC(name, func, complex128, float32, complex128, complex128)    \
    CPU_DEVICE_BINARYC(name, func, complex128, float64, complex128, complex128)    \
    CPU_DEVICE_NOIMPL(name, func, complex128, complex32, complex128, complex128)   \
    CPU_DEVICE_BINARYC(name, func, complex128, complex64, complex128, complex128)  \
    CPU_DEVICE_BINARYC(name, func, complex128, complex128, complex128, complex128) \

#define add(x, y) x + y
CPU_DEVICE_ALL_BINARY(add, add, add)

#define subtract(x, y) x - y
CPU_DEVICE_ALL_BINARY(subtract, subtract, sub)

#define multiply(x, y) x * y
CPU_DEVICE_ALL_BINARY(multiply, multiply, multiply)

#define floor_divide(x, y) x / y
CPU_DEVICE_ALL_BINARY_NO_COMPLEX(floor_divide, _floor_divide, _floor_divide)

#define remainder(x, y) x % y
CPU_DEVICE_ALL_BINARY_NO_COMPLEX(remainder, _remainder, _remainder)

#define divide(x, y) x / y
CPU_DEVICE_ALL_BINARY_FLOAT_RETURN(divide, divide, divide)


/*****************************************************************************/
/*                                 Comparison                                */
/*****************************************************************************/

#define CPU_DEVICE_ALL_COMPARISON(name, func, hfunc, cfunc) \
    CPU_DEVICE_BINARY(name, func, uint8, uint8, bool, uint8)                  \
    CPU_DEVICE_BINARY(name, func, uint8, uint16, bool, uint16)                \
    CPU_DEVICE_BINARY(name, func, uint8, uint32, bool, uint32)                \
    CPU_DEVICE_BINARY(name, func, uint8, uint64, bool, uint64)                \
    CPU_DEVICE_BINARY(name, func, uint8, int8, bool, int16)                   \
    CPU_DEVICE_BINARY(name, func, uint8, int16, bool, int16)                  \
    CPU_DEVICE_BINARY(name, func, uint8, int32, bool, int32)                  \
    CPU_DEVICE_BINARY(name, func, uint8, int64, bool, int64)                  \
    CPU_DEVICE_NOIMPL(name, func, uint8, float16, bool, float16)              \
    CPU_DEVICE_BINARY(name, func, uint8, float32, bool, float32)              \
    CPU_DEVICE_BINARY(name, func, uint8, float64, bool, float64)              \
    CPU_DEVICE_NOIMPL(name, cfunc, uint8, complex32, bool, complex32)         \
    CPU_DEVICE_BINARYC(name, cfunc, uint8, complex64, bool, complex64)        \
    CPU_DEVICE_BINARYC(name, cfunc, uint8, complex128, bool, complex128)      \
                                                                              \
    CPU_DEVICE_BINARY(name, func, uint16, uint8, bool, uint16)                \
    CPU_DEVICE_BINARY(name, func, uint16, uint16, bool, uint16)               \
    CPU_DEVICE_BINARY(name, func, uint16, uint32, bool, uint32)               \
    CPU_DEVICE_BINARY(name, func, uint16, uint64, bool, uint64)               \
    CPU_DEVICE_BINARY(name, func, uint16, int8, bool, int32)                  \
    CPU_DEVICE_BINARY(name, func, uint16, int16, bool, int32)                 \
    CPU_DEVICE_BINARY(name, func, uint16, int32, bool, int32)                 \
    CPU_DEVICE_BINARY(name, func, uint16, int64, bool, int64)                 \
    CPU_DEVICE_NOIMPL(name, func, uint16, float16, bool, float32)             \
    CPU_DEVICE_BINARY(name, func, uint16, float32, bool, float32)             \
    CPU_DEVICE_BINARY(name, func, uint16, float64, bool, float64)             \
    CPU_DEVICE_NOIMPL(name, cfunc, uint16, complex32, bool, complex64)        \
    CPU_DEVICE_BINARYC(name, cfunc, uint16, complex64, bool, complex64)       \
    CPU_DEVICE_BINARYC(name, cfunc, uint16, complex128, bool, complex128)     \
                                                                              \
    CPU_DEVICE_BINARY(name, func, uint32, uint8, bool, uint32)                \
    CPU_DEVICE_BINARY(name, func, uint32, uint16, bool, uint32)               \
    CPU_DEVICE_BINARY(name, func, uint32, uint32, bool, uint32)               \
    CPU_DEVICE_BINARY(name, func, uint32, uint64, bool, uint64)               \
    CPU_DEVICE_BINARY(name, func, uint32, int8, bool, int64)                  \
    CPU_DEVICE_BINARY(name, func, uint32, int16, bool, int64)                 \
    CPU_DEVICE_BINARY(name, func, uint32, int32, bool, int64)                 \
    CPU_DEVICE_BINARY(name, func, uint32, int64, bool, int64)                 \
    CPU_DEVICE_NOIMPL(name, func, uint32, float16, bool, float64)             \
    CPU_DEVICE_BINARY(name, func, uint32, float32, bool, float64)             \
    CPU_DEVICE_BINARY(name, func, uint32, float64, bool, float64)             \
    CPU_DEVICE_NOIMPL(name, cfunc, uint32, complex32, bool, complex128)       \
    CPU_DEVICE_BINARYC(name, cfunc, uint32, complex64, bool, complex128)      \
    CPU_DEVICE_BINARYC(name, cfunc, uint32, complex128, bool, complex128)     \
                                                                              \
    CPU_DEVICE_BINARY(name, func, uint64, uint8, bool, uint64)                \
    CPU_DEVICE_BINARY(name, func, uint64, uint16, bool, uint64)               \
    CPU_DEVICE_BINARY(name, func, uint64, uint32, bool, uint64)               \
    CPU_DEVICE_BINARY(name, func, uint64, uint64, bool, uint64)               \
                                                                              \
    CPU_DEVICE_BINARY(name, func, int8, uint8, bool, int16)                   \
    CPU_DEVICE_BINARY(name, func, int8, uint16, bool, int32)                  \
    CPU_DEVICE_BINARY(name, func, int8, uint32, bool, int64)                  \
    CPU_DEVICE_BINARY(name, func, int8, int8, bool, int8)                     \
    CPU_DEVICE_BINARY(name, func, int8, int16, bool, int16)                   \
    CPU_DEVICE_BINARY(name, func, int8, int32, bool, int32)                   \
    CPU_DEVICE_BINARY(name, func, int8, int64, bool, int64)                   \
    CPU_DEVICE_NOIMPL(name, func, int8, float16, bool, float16)               \
    CPU_DEVICE_BINARY(name, func, int8, float32, bool, float32)               \
    CPU_DEVICE_BINARY(name, func, int8, float64, bool, float64)               \
    CPU_DEVICE_NOIMPL(name, cfunc, int8, complex32, bool, complex32)          \
    CPU_DEVICE_BINARYC(name, cfunc, int8, complex64, bool, complex64)         \
    CPU_DEVICE_BINARYC(name, cfunc, int8, complex128, bool, complex128)       \
                                                                              \
    CPU_DEVICE_BINARY(name, func, int16, uint8, bool, int16)                  \
    CPU_DEVICE_BINARY(name, func, int16, uint16, bool, int32)                 \
    CPU_DEVICE_BINARY(name, func, int16, uint32, bool, int64)                 \
    CPU_DEVICE_BINARY(name, func, int16, int8, bool, int16)                   \
    CPU_DEVICE_BINARY(name, func, int16, int16, bool, int16)                  \
    CPU_DEVICE_BINARY(name, func, int16, int32, bool, int32)                  \
    CPU_DEVICE_BINARY(name, func, int16, int64, bool, int64)                  \
    CPU_DEVICE_NOIMPL(name, func, int16, float16, bool, float32)              \
    CPU_DEVICE_BINARY(name, func, int16, float32, bool, float32)              \
    CPU_DEVICE_BINARY(name, func, int16, float64, bool, float64)              \
    CPU_DEVICE_NOIMPL(name, cfunc, int16, complex32, bool, complex64)         \
    CPU_DEVICE_BINARYC(name, cfunc, int16, complex64, bool, complex64)        \
    CPU_DEVICE_BINARYC(name, cfunc, int16, complex128, bool, complex128)      \
                                                                              \
    CPU_DEVICE_BINARY(name, func, int32, uint8, bool, int32)                  \
    CPU_DEVICE_BINARY(name, func, int32, uint16, bool, int32)                 \
    CPU_DEVICE_BINARY(name, func, int32, uint32, bool, int64)                 \
    CPU_DEVICE_BINARY(name, func, int32, int8, bool, int32)                   \
    CPU_DEVICE_BINARY(name, func, int32, int16, bool, int32)                  \
    CPU_DEVICE_BINARY(name, func, int32, int32, bool, int32)                  \
    CPU_DEVICE_BINARY(name, func, int32, int64, bool, int64)                  \
    CPU_DEVICE_NOIMPL(name, func, int32, float16, bool, float64)              \
    CPU_DEVICE_BINARY(name, func, int32, float32, bool, float64)              \
    CPU_DEVICE_BINARY(name, func, int32, float64, bool, float64)              \
    CPU_DEVICE_NOIMPL(name, cfunc, int32, complex32, bool, complex128)        \
    CPU_DEVICE_BINARYC(name, cfunc, int32, complex64, bool, complex128)       \
    CPU_DEVICE_BINARYC(name, cfunc, int32, complex128, bool, complex128)      \
                                                                              \
    CPU_DEVICE_BINARY(name, func, int64, uint8, bool, int64)                  \
    CPU_DEVICE_BINARY(name, func, int64, uint16, bool, int64)                 \
    CPU_DEVICE_BINARY(name, func, int64, uint32, bool, int64)                 \
    CPU_DEVICE_BINARY(name, func, int64, int8, bool, int64)                   \
    CPU_DEVICE_BINARY(name, func, int64, int16, bool, int64)                  \
    CPU_DEVICE_BINARY(name, func, int64, int32, bool, int64)                  \
    CPU_DEVICE_BINARY(name, func, int64, int64, bool, int64)                  \
                                                                              \
    CPU_DEVICE_NOIMPL(name, func, float16, uint8, bool, float16)              \
    CPU_DEVICE_NOIMPL(name, func, float16, uint16, bool, float32)             \
    CPU_DEVICE_NOIMPL(name, func, float16, uint32, bool, float64)             \
    CPU_DEVICE_NOIMPL(name, func, float16, int8, bool, float16)               \
    CPU_DEVICE_NOIMPL(name, func, float16, int16, bool, float32)              \
    CPU_DEVICE_NOIMPL(name, func, float16, int32, bool, float64)              \
    CPU_DEVICE_NOIMPL(name, func, float16, float16, bool, float16)            \
    CPU_DEVICE_NOIMPL(name, func, float16, float32, bool, float32)            \
    CPU_DEVICE_NOIMPL(name, func, float16, float64, bool, float64)            \
    CPU_DEVICE_NOIMPL(name, cfunc, float16, complex32, bool, complex32)       \
    CPU_DEVICE_NOIMPL(name, cfunc, float16, complex64, bool, complex64)       \
    CPU_DEVICE_NOIMPL(name, cfunc, float16, complex128, bool, complex128)     \
                                                                              \
    CPU_DEVICE_BINARY(name, func, float32, uint8, bool, float32)              \
    CPU_DEVICE_BINARY(name, func, float32, uint16, bool, float32)             \
    CPU_DEVICE_BINARY(name, func, float32, uint32, bool, float64)             \
    CPU_DEVICE_BINARY(name, func, float32, int8, bool, float32)               \
    CPU_DEVICE_BINARY(name, func, float32, int16, bool, float32)              \
    CPU_DEVICE_BINARY(name, func, float32, int32, bool, float64)              \
    CPU_DEVICE_NOIMPL(name, func, float32, float16, bool, float32)            \
    CPU_DEVICE_BINARY(name, func, float32, float32, bool, float32)            \
    CPU_DEVICE_BINARY(name, func, float32, float64, bool, float64)            \
    CPU_DEVICE_NOIMPL(name, cfunc, float32, complex32, bool, complex64)       \
    CPU_DEVICE_BINARYC(name, cfunc, float32, complex64, bool, complex64)      \
    CPU_DEVICE_BINARYC(name, cfunc, float32, complex128, bool, complex128)    \
                                                                              \
    CPU_DEVICE_BINARY(name, func, float64, uint8, bool, float64)              \
    CPU_DEVICE_BINARY(name, func, float64, uint16, bool, float64)             \
    CPU_DEVICE_BINARY(name, func, float64, uint32, bool, float64)             \
    CPU_DEVICE_BINARY(name, func, float64, int8, bool, float64)               \
    CPU_DEVICE_BINARY(name, func, float64, int16, bool, float64)              \
    CPU_DEVICE_BINARY(name, func, float64, int32, bool, float64)              \
    CPU_DEVICE_NOIMPL(name, func, float64, float16, bool, float64)            \
    CPU_DEVICE_BINARY(name, func, float64, float32, bool, float64)            \
    CPU_DEVICE_BINARY(name, func, float64, float64, bool, float64)            \
    CPU_DEVICE_NOIMPL(name, cfunc, float64, complex32, bool, complex128)      \
    CPU_DEVICE_BINARYC(name, cfunc, float64, complex64, bool, complex128)     \
    CPU_DEVICE_BINARYC(name, cfunc, float64, complex128, bool, complex128)    \
                                                                              \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, uint8, bool, complex32)         \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, uint16, bool, complex64)        \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, uint32, bool, complex128)       \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, int8, bool, complex32)          \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, int16, bool, complex64)         \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, int32, bool, complex128)        \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, float16, bool, complex32)       \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, float32, bool, complex64)       \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, float64, bool, complex128)      \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, complex32, bool, complex32)     \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, complex64, bool, complex64)     \
    CPU_DEVICE_NOIMPL(name, cfunc, complex32, complex128, bool, complex128)   \
                                                                              \
    CPU_DEVICE_BINARYC(name, cfunc, complex64, uint8, bool, complex64)        \
    CPU_DEVICE_BINARYC(name, cfunc, complex64, uint16, bool, complex64)       \
    CPU_DEVICE_BINARYC(name, cfunc, complex64, uint32, bool, complex128)      \
    CPU_DEVICE_BINARYC(name, cfunc, complex64, int8, bool, complex64)         \
    CPU_DEVICE_BINARYC(name, cfunc, complex64, int16, bool, complex64)        \
    CPU_DEVICE_BINARYC(name, cfunc, complex64, int32, bool, complex128)       \
    CPU_DEVICE_NOIMPL(name, cfunc, complex64, float16, bool, complex64)       \
    CPU_DEVICE_BINARYC(name, cfunc, complex64, float32, bool, complex64)      \
    CPU_DEVICE_BINARYC(name, cfunc, complex64, float64, bool, complex128)     \
    CPU_DEVICE_NOIMPL(name, cfunc, complex64, complex32, bool, complex64)     \
    CPU_DEVICE_BINARYC(name, cfunc, complex64, complex64, bool, complex64)    \
    CPU_DEVICE_BINARYC(name, cfunc, complex64, complex128, bool, complex128)  \
                                                                              \
    CPU_DEVICE_BINARYC(name, cfunc, complex128, uint8, bool, complex128)      \
    CPU_DEVICE_BINARYC(name, cfunc, complex128, uint16, bool, complex128)     \
    CPU_DEVICE_BINARYC(name, cfunc, complex128, uint32, bool, complex128)     \
    CPU_DEVICE_BINARYC(name, cfunc, complex128, int8, bool, complex128)       \
    CPU_DEVICE_BINARYC(name, cfunc, complex128, int16, bool, complex128)      \
    CPU_DEVICE_BINARYC(name, cfunc, complex128, int32, bool, complex128)      \
    CPU_DEVICE_NOIMPL(name, cfunc, complex128, float16, bool, complex128)     \
    CPU_DEVICE_BINARYC(name, cfunc, complex128, float32, bool, complex128)    \
    CPU_DEVICE_BINARYC(name, cfunc, complex128, float64, bool, complex128)    \
    CPU_DEVICE_NOIMPL(name, cfunc, complex128, complex32, bool, complex128)   \
    CPU_DEVICE_BINARYC(name, cfunc, complex128, complex64, bool, complex128)  \
    CPU_DEVICE_BINARYC(name, cfunc, complex128, complex128, bool, complex128) \


#define less(x, y) x < y
CPU_DEVICE_ALL_COMPARISON(less, less, less, lexorder_lt)

#define less_equal(x, y) x <= y
CPU_DEVICE_ALL_COMPARISON(less_equal, less_equal, less_equal, lexorder_le)

#define greater_equal(x, y) x >= y
CPU_DEVICE_ALL_COMPARISON(greater_equal, greater_equal, greater_equal, lexorder_ge)

#define greater(x, y) x > y
CPU_DEVICE_ALL_COMPARISON(greater, greater, greater, lexorder_gt)


/*****************************************************************************/
/*                              Two return values                            */
/*****************************************************************************/

#define CPU_DEVICE_BINARY_MV(name, func, t0, t1, t2, t3) \
extern "C" void                                                          \
gm_cpu_device_fixed_1D_C_##name##_##t0##_##t1##_##t2##_##t3(             \
    const char *in0, const char *in1, char *out0, char *out1, int64_t N) \
{                                                                        \
    const t0##_t *_in0 = (const t0##_t *)in0;                            \
    const t1##_t *_in1 = (const t1##_t *)in1;                            \
    t2##_t *_out0 = (t2##_t *)out0;                                      \
    t3##_t *_out1 = (t3##_t *)out1;                                      \
    int64_t i;                                                           \
                                                                         \
    for (i = 0; i < N; i++) {                                            \
        func(&_out0[i], &_out1[i], _in0[i], _in1[i]);                    \
    }                                                                    \
}                                                                        \
                                                                         \
extern "C" void                                                          \
gm_cpu_device_0D_##name##_##t0##_##t1##_##t2##_##t3(                     \
    const char *in0, const char *in1, char *out0, char *out1)            \
{                                                                        \
    const t0##_t x = *(const t0##_t *)in0;                               \
    const t1##_t y = *(const t1##_t *)in1;                               \
    t2##_t *a = (t2##_t *)out0;                                          \
    t3##_t *b = (t3##_t *)out1;                                          \
                                                                         \
    func(a, b, x, y);                                                    \
}

#define CPU_DEVICE_ALL_BINARY_MV(name, func) \
    CPU_DEVICE_BINARY_MV(name, func, uint8, uint8, uint8, uint8)         \
    CPU_DEVICE_BINARY_MV(name, func, uint16, uint16, uint16, uint16)     \
    CPU_DEVICE_BINARY_MV(name, func, uint32, uint32, uint32, uint32)     \
    CPU_DEVICE_BINARY_MV(name, func, uint64, uint64, uint64, uint64)     \
    CPU_DEVICE_BINARY_MV(name, func, int8, int8, int8, int8)             \
    CPU_DEVICE_BINARY_MV(name, func, int16, int16, int16, int16)         \
    CPU_DEVICE_BINARY_MV(name, func, int32, int32, int32, int32)         \
    CPU_DEVICE_BINARY_MV(name, func, int64, int64, int64, int64)         \
    CPU_DEVICE_BINARY_MV(name, func, float32, float32, float32, float32) \
    CPU_DEVICE_BINARY_MV(name, func, float64, float64, float64, float64)

CPU_DEVICE_ALL_BINARY_MV(divmod, _divmod)
