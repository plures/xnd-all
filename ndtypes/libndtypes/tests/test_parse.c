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


#include <stdio.h>
#include "test.h"


const char *parse_tests[] = {
  "2395344366 * Any",
  "N * Any",
  "N * M * Any",
  "10 * 2395344366 * Any",
  "10 * 10 * 2395344366 * Any",
  "L * Any",
  "10 * L * Any",
  "10 * 10 * L * Any",
  "var * Any",
  "array of Any",
  "... * Any",
  "10 * Any",
  "Any",
  "10 * ScalarKind",
  "ScalarKind",
  "10 * bool",
  "bool",
  "10 * SignedKind",
  "SignedKind",
  "10 * int8",
  "int8",
  "10 * int16",
  "int16",
  "10 * int32",
  "int32",
  "10 * int64",
  "int64",
  "10 * UnsignedKind",
  "UnsignedKind",
  "10 * uint8",
  "uint8",
  "10 * uint16",
  "uint16",
  "10 * uint32",
  "uint32",
  "10 * uint64",
  "uint64",
  "10 * FloatKind",
  "FloatKind",
  "10 * float32",
  "float32",
  "10 * float64",
  "float64",
  "10 * ComplexKind",
  "ComplexKind",
  "10 * complex64",
  "complex64",
  "10 * complex128",
  "complex128",
  "10 * float64",
  "float64",
  "10 * intptr",
  "intptr",
  "10 * uintptr",
  "uintptr",
  "10 * char",
  "char",
  "10 * char('A')",
  "char('A')",
  "10 * char('ascii')",
  "char('ascii')",
  "10 * char('us-ascii')",
  "char('us-ascii')",
  "10 * char('U8')",
  "char('U8')",
  "10 * char('utf8')",
  "char('utf8')",
  "10 * char('utf-8')",
  "char('utf-8')",
  "10 * char('U16')",
  "char('U16')",
  "10 * char('utf16')",
  "char('utf16')",
  "10 * char('utf-16')",
  "char('utf-16')",
  "10 * char('U32')",
  "char('U32')",
  "10 * char('utf32')",
  "char('utf32')",
  "10 * char('utf-32')",
  "char('utf-32')",
  "10 * char('ucs2')",
  "char('ucs2')",
  "10 * char('ucs-2')",
  "char('ucs-2')",
  "10 * char('ucs_2')",
  "char('ucs_2')",
  "10 * string",
  "string",
  "10 * FixedStringKind",
  "FixedStringKind",
  "10 * fixed_string(3641573028)",
  "fixed_string(3641573028)",
  "10 * fixed_string(4053683685, 'A')",
  "fixed_string(4053683685, 'A')",
  "10 * fixed_string(729742655, 'ascii')",
  "fixed_string(729742655, 'ascii')",
  "10 * fixed_string(3675114521, 'us-ascii')",
  "fixed_string(3675114521, 'us-ascii')",
  "10 * fixed_string(1506029184, 'U8')",
  "fixed_string(1506029184, 'U8')",
  "10 * fixed_string(3952068488, 'utf8')",
  "fixed_string(3952068488, 'utf8')",
  "10 * fixed_string(966590426, 'utf-8')",
  "fixed_string(966590426, 'utf-8')",
  "10 * fixed_string(1439340594, 'U16')",
  "fixed_string(1439340594, 'U16')",
  "10 * fixed_string(2949183030, 'utf16')",
  "fixed_string(2949183030, 'utf16')",
  "10 * fixed_string(2591745097, 'utf-16')",
  "fixed_string(2591745097, 'utf-16')",
  "10 * fixed_string(3116157301, 'U32')",
  "fixed_string(3116157301, 'U32')",
  "10 * fixed_string(375133997, 'utf32')",
  "fixed_string(375133997, 'utf32')",
  "10 * fixed_string(3253997171, 'utf-32')",
  "fixed_string(3253997171, 'utf-32')",
  "10 * fixed_string(969064578, 'ucs2')",
  "fixed_string(969064578, 'ucs2')",
  "10 * fixed_string(2403487820, 'ucs-2')",
  "fixed_string(2403487820, 'ucs-2')",
  "10 * fixed_string(2347175950, 'ucs_2')",
  "fixed_string(2347175950, 'ucs_2')",
  "10 * bytes(align=2)",
  "bytes(align=4)",
  "10 * FixedBytesKind",
  "FixedBytesKind",
  "10 * fixed_bytes(size=1904128700, align=4)",
  "fixed_bytes(size=1904128700, align=4)",
  "10 * categorical(63)",
  "categorical(63)",
  "10 * categorical(10, 63)",
  "categorical(10, 63)",
  "10 * categorical(10, 20, 63)",
  "categorical(10, 20, 63)",
  "10 * categorical(18514)",
  "categorical(18514)",
  "10 * categorical(-176354404)",
  "categorical(-176354404)",
  "10 * categorical(10, -176354404)",
  "10 * categorical(-176354404, 10)",
  "10 * categorical(500601201.0)",
  "categorical(20, 500601201.0)",
  "10 * categorical(10, -20, 147573419.42583135)",
  "categorical(10, 'jRAMoBPQ')",
  "categorical('jRAMoBPQ', 10)",
  "10 * categorical(9223372036854775807)",
  "10 * categorical(-9223372036854775808)",
  "10 * categorical(1.17550e-38)",
  "10 * categorical(2.22508e-308)",
  "categorical(3.40282e+38)",
  "10 * categorical(10, 3.40282e+38)",
  "10 * categorical(1.79769e+308)",
  "10 * categorical(10, 1.79769e+308)",
  "10 * categorical(\"\")",
  "categorical(\"\")",
  "10 * categorical(10, \"\")",
  "10 * categorical(\"\", 10)",
  "10 * ref(Any)",
  "ref(Any)",
  "10 * (...)",
  "(...)",
  "10 * (Any)",
  "(Any)",
  "10 * (int64, Any)",
  "(int64, Any)",
  "10 * (int64, int64, Any)",
  "(int64, int64, Any)",
  "10 * (Any)",
  "(Any)",
  "10 * (Any, )",
  "(Any, )",
  "10 * (Any, ...)",
  "(Any, ...)",
  "10 * {...}",
  "10 * [Float of float64]",
  "10 * [Float of float64 | Int of int8]",
  "{...}",
  "10 * {a: Any}",
  "{a: Any}",
  "10 * {x: int64, a: Any}",
  "{x: int64, a: Any}",
  "10 * {x: int64, x: int64, a: Any}",
  "{x: int64, x: int64, a: Any}",
  "10 * {a : int64}",
  "{a : int64}",
  "10 * {a : int64, }",
  "{a : int64, }",
  "10 * {a : int64, ...}",
  "{a : int64, ...}",
  "(...) -> Any",
  "() -> (...)",
  "(int64) -> void",
  "(Any) -> Any",
  "() -> (Any)",
  "(int64, Any)-> Any",
  "int64, Any -> Any",
  "() -> (int64, Any)",
  "() -> int64, Any",
  "void -> int64",
  "int64, Any -> void",
  "(int64, int64, Any) -> Any",
  "() -> (int64, int64, Any)",
  "() -> (Any)",
  "(Any,) -> Any",
  "() -> (Any,)",
  "(Any, ...) -> Any",
  "() -> (Any, ...)",
  "10 * Up",
  "Up",
  "10 * X(Any)",
  "X(Any)",

  /* BEGIN RANDOM */
  "fixed_bytes(size=1342281600, align=16)",
  "categorical(\"a\", 1619552300, 49062, 18772, -84, 'l')",
  "categorical(408786241.474, 2840608325, 44038, 1763886348, 'jOt', 24)",
  "categorical(-2049848438, 94, 3599739756, 3577056037, 318483846.634, NA)",
  "categorical(-2049848438, 94, NA, 3599739756, 3577056037, 318483846.634)",
  "1286044356 * LbR6JPFI * 2096958782 * uint8",
  "categorical(-507014936.36, -25910, 'xM3Mys0XqH', 4265882500)",
  "var * var * FixedBytesKind",
  "array of FixedBytesKind",
  "var * var * var * fixed_bytes(size=2816010912, align=16)",
  "array of fixed_bytes(size=2816010912, align=16)",
  "36 * 16 * fixed_bytes(size=912328236, align=2)",
  "var * var * float64",
  "array of float64",
  "var * var * var * ZcTmBXnKPi",
  "array of ZcTmBXnKPi",
  "categorical('omhwkoWVWw', 43, 946986991)",
  "var * float32",
  "var * var * var * uint16",
  "var * var * var * ScalarKind",
  "array of ScalarKind",
  "fixed_bytes(size=280180380, align=2)",
  "fixed_string(1901614748, 'utf-32')",
  "var * M7",
  "var * bytes(align=2)",
  "var * var * ULy8(char)",
  "array of ULy8(char)",
  "fixed_bytes(size=1882259744, align=16)",
  "categorical(-136747211.015, -58, -83)",
  "3739637665 * 1476113530 * 1701021063 * IKd * 450514382 * WXn",
  "ref(AilcKv4su1(FixedBytesKind))",
  "var * R8KFFEabJ",
  "var * QoFb",
  "var * fixed_string(1233383142, 'utf-32')",
  "array of fixed_string(1233383142, 'utf-32')",
  "fixed_bytes(size=2882797968, align=4)",
  "fixed_bytes(size=1233209904, align=8)",
  "WhRsMHHXYp(categorical(145))",
  "var * var * bytes(align=2)",
  "categorical(-49, -26791, 1780, -85, 'pX', 8094, -400096436.783)",
  "D6nfBg_(categorical(147170982, 16278, 208837895.375, \"ylR7\", 26651))",
  "categorical(-322735020.187, 3229733591, 482331467, 10416529.4912)",
  /* END RANDOM */

  /* BEGIN MANUALLY GENERATED */
  "()",
  "(...)",
  "(int64, ...)",
  "{}",
  "{...}",
  "() -> ()",
  "(...) -> ()",
  "(...) -> (...)",
  "(int64, ...) -> complex128",
  "() -> int32",
  "(int32) -> int32",
  "(int32, float64) -> int32",
  "int32, float64 -> int32",
  "(...) -> int32",
  "void -> float32",

  "10 * defined_t",
  "defined_t",

  "10 * foo_t",
  "foo_t",

  /* bfloat16 */
  "10 * bfloat16",

  /* bcomplex32 */
  "10 * bcomplex32",

  /* Half-float */
  "10 * float16",
  "10 * complex32",

  /* Constructor syntax for arrays */
  "10 * Matrix(3 * 2 * float64)",

  /* Attributes and metadata */
  "fixed(shape=10) * int32",
  "fixed(shape=10) * int32",
  "(fixed(shape=10) * int32, float32)",
  "(fixed(shape=10) * int32, float32 |pack=2|, uint8)",
  "(fixed(shape=10) * int32, float32, uint8, pack=2)",
  "{x : float32 |align=16|, y : float64}",

  "fixed(shape=10) * complex128",

  "var(offsets=[0,10]) * var(offsets=[0,1,3,6,10,15,21,28,36,45,55]) * float64",
  "var(offsets=[0,2]) * var(offsets=[0,3,7]) * var(offsets=[0,5,11,18,26,35,45,56]) * float64",

  /* Tagged unions */
  "2 * 3  * [ThisRecord of {first: (int64, complex128), second: string}]",
  "2 * 3  * [ThisRecord of {first: (int64, complex128), second: string} | ThatTuple of (bytes, string)]",
  "2 * 3  * [Int of int64 | ThisRecord of {first: (int64, complex128), second: string} | ThatTuple of (bytes, string)]",
  "array of [Int of int64 | ThisRecord of {first: (int64, complex128), second: uint8} | ThatTuple of (float64, float64)]",

  "fixed(shape=10) * complex128",

  /* Short ref notation */
  "&float64",
  "&2 * 10 * {a: (&int64, &float64)}",

  /* Fortran types */
  "!2 * 10 * float16",
  "!2 * 10 * {a: !2 * 10 * float16}",
  "!2 * 10 * {a: !2 * 10 * float16}",
  "!2 * 10 * bfloat16",
  "!2 * 10 * {a: !2 * 10 * bfloat16}",
  "!2 * 10 * {a: !2 * 10 * bfloat16}",
  "!2 * 10 * {a: !2 * 10 * (int64, string)}",

  /* Nested variable dimensions */
  "var(offsets=[0, 2]) * var(offsets=[0,4,10]) * (var(offsets=[0,2]) * var(offsets=[0,3,5]) * float32, int64)",

  "2 * N * {a: !2 * 10 * (int64, string)}",

  /* Overflow */
  "9223372036854775807 * uint8",
  "1 * 1 * 9223372036854775807 * uint8",

  /* END MANUALLY GENERATED */

   NULL
};

