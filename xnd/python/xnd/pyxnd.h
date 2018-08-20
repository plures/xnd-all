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


#ifndef PYXND_H
#define PYXND_H
#ifdef __cplusplus
extern "C" {
#endif


#include <Python.h>
#include "ndtypes.h"
#include "xnd.h"


/****************************************************************************/
/*                           MemoryBlock Object                             */
/****************************************************************************/

/* This object owns the memory that is shared by several xnd objects. It is
   never exposed to the Python level.

   The memory block is created by the primary xnd object on initialization.
   Sub-views, slices etc. share the memory block.

   PEP-3118 imports are supported.  At a later stage the mblock object will
   potentially need to communicate with Arrow or other formats in order
   to acquire and manage external memory blocks.
*/

/* Exposed here for the benefit of Numba. The API should not be regarded
   stable across versions. */

typedef struct {
    PyObject_HEAD
    PyObject *type;    /* type owner */
    xnd_master_t *xnd; /* memblock owner */
    Py_buffer *view;   /* PEP-3118 imports */
} MemoryBlockObject;


/****************************************************************************/
/*                                 xnd object                               */
/****************************************************************************/

/* Exposed here for the benefit of Numba. The API should not be regarded
   stable across versions. */

typedef struct {
    PyObject_HEAD
    MemoryBlockObject *mblock; /* owner of the primary type and memory block */
    PyObject *type;            /* owner of the current type */
    xnd_t xnd;                 /* typed view, does not own anything */
} XndObject;

#define TYPE_OWNER(v) (((XndObject *)v)->type)
#define XND(v) (&(((XndObject *)v)->xnd))
#define XND_INDEX(v) (((XndObject *)v)->xnd.index)
#define XND_TYPE(v) (((XndObject *)v)->xnd.type)
#define XND_PTR(v) (((XndObject *)v)->xnd.ptr)


/****************************************************************************/
/*                                Capsule API                               */
/****************************************************************************/

#define Xnd_CheckExact_INDEX 0
#define Xnd_CheckExact_RETURN int
#define Xnd_CheckExact_ARGS (const PyObject *)

#define Xnd_Check_INDEX 1
#define Xnd_Check_RETURN int
#define Xnd_Check_ARGS (const PyObject *)

#define CONST_XND_INDEX 2
#define CONST_XND_RETURN const xnd_t *
#define CONST_XND_ARGS (const PyObject *)

#define Xnd_EmptyFromType_INDEX 3
#define Xnd_EmptyFromType_RETURN PyObject *
#define Xnd_EmptyFromType_ARGS (PyTypeObject *, ndt_t *t)

#define Xnd_ViewMoveNdt_INDEX 4
#define Xnd_ViewMoveNdt_RETURN PyObject *
#define Xnd_ViewMoveNdt_ARGS (const PyObject *, ndt_t *t)

#define Xnd_FromXnd_INDEX 5
#define Xnd_FromXnd_RETURN PyObject *
#define Xnd_FromXnd_ARGS (PyTypeObject *, xnd_t *x)

#define Xnd_Subscript_INDEX 6
#define Xnd_Subscript_RETURN PyObject *
#define Xnd_Subscript_ARGS (const PyObject *self, const PyObject *key)

#define Xnd_FromXndMoveType_INDEX 7
#define Xnd_FromXndMoveType_RETURN PyObject *
#define Xnd_FromXndMoveType_ARGS (const PyObject *xnd, xnd_t *x)

#define Xnd_FromXndView_INDEX 8
#define Xnd_FromXndView_RETURN PyObject *
#define Xnd_FromXndView_ARGS (xnd_view_t *x)

#define Xnd_GetType_INDEX 9
#define Xnd_GetType_RETURN PyTypeObject *
#define Xnd_GetType_ARGS (void)

#define XND_MAX_API 10


#ifdef XND_MODULE
static Xnd_CheckExact_RETURN Xnd_CheckExact Xnd_CheckExact_ARGS;
static Xnd_Check_RETURN Xnd_Check Xnd_Check_ARGS;
static CONST_XND_RETURN CONST_XND CONST_XND_ARGS;
static Xnd_EmptyFromType_RETURN Xnd_EmptyFromType Xnd_EmptyFromType_ARGS;
static Xnd_ViewMoveNdt_RETURN Xnd_ViewMoveNdt Xnd_ViewMoveNdt_ARGS;
static Xnd_FromXnd_RETURN Xnd_FromXnd Xnd_FromXnd_ARGS;
static Xnd_Subscript_RETURN Xnd_Subscript Xnd_Subscript_ARGS;
static Xnd_FromXndMoveType_RETURN Xnd_FromXndMoveType Xnd_FromXndMoveType_ARGS;
static Xnd_FromXndView_RETURN Xnd_FromXndView Xnd_FromXndView_ARGS;
static Xnd_GetType_RETURN Xnd_GetType Xnd_GetType_ARGS;
#else
static void **_xnd_api;

#define Xnd_CheckExact \
    (*(Xnd_CheckExact_RETURN (*)Xnd_CheckExact_ARGS) _xnd_api[Xnd_CheckExact_INDEX])

#define Xnd_Check \
    (*(Xnd_Check_RETURN (*)Xnd_Check_ARGS) _xnd_api[Xnd_Check_INDEX])

#define CONST_XND \
    (*(CONST_XND_RETURN (*)CONST_XND_ARGS) _xnd_api[CONST_XND_INDEX])

#define Xnd_EmptyFromType \
    (*(Xnd_EmptyFromType_RETURN (*)Xnd_EmptyFromType_ARGS) _xnd_api[Xnd_EmptyFromType_INDEX])

#define Xnd_ViewMoveNdt \
    (*(Xnd_ViewMoveNdt_RETURN (*)Xnd_ViewMoveNdt_ARGS) _xnd_api[Xnd_ViewMoveNdt_INDEX])

#define Xnd_FromXnd \
    (*(Xnd_FromXnd_RETURN (*)Xnd_FromXnd_ARGS) _xnd_api[Xnd_FromXnd_INDEX])

#define Xnd_Subscript \
    (*(Xnd_Subscript_RETURN (*)Xnd_Subscript_ARGS) _xnd_api[Xnd_Subscript_INDEX])

#define Xnd_FromXndMoveType \
    (*(Xnd_FromXndMoveType_RETURN (*)Xnd_FromXndMoveType_ARGS) _xnd_api[Xnd_FromXndMoveType_INDEX])

#define Xnd_FromXndView \
    (*(Xnd_FromXndView_RETURN (*)Xnd_FromXndView_ARGS) _xnd_api[Xnd_FromXndView_INDEX])

#define Xnd_GetType \
    (*(Xnd_GetType_RETURN (*)Xnd_GetType_ARGS) _xnd_api[Xnd_GetType_INDEX])

static int
import_xnd(void)
{
    _xnd_api = (void **)PyCapsule_Import("xnd._xnd._API", 0);
    if (_xnd_api == NULL) {
        return -1;
    }

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* PYXND_H */
