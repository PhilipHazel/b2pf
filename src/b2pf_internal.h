/*************************************************
*       Base Unicode to Presentation Forms       *
*************************************************/

/* This is an internal header file for the B2PF library.

                 Copyright (c) 2020 Philip Hazel

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * The names of any contributors to this project may not be used to
      endorse or promote products derived from this software without
      specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

#ifndef B2PF_INTERNAL_H_IDEMPOTENT_GUARD
#define B2PF_INTERNAL_H_IDEMPOTENT_GUARD


/* Standard C headers */

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Some overall parameters. */

#define WORDMAX          100  /* Longest word that can be processed */
#define STACK_BUFFSIZE  1000  /* Size of stack buffers (uints) */

/* Macros to make boolean values more obvious. The #ifndef is to pacify
compiler warnings in environments where these macros are defined elsewhere.
Unfortunately, there is no way to do the same for the typedef. */

typedef int BOOL;
#ifndef FALSE
#define FALSE   0
#define TRUE    1
#endif

/* We use a lot of unsigned characters. Make a short type, and use macros to
cast for character-handling functions. */

typedef unsigned char uschar;

#define Uisspace(c)      isspace((char)c)
#define Uisxdigit(c)     isxdigit((char)c)
#define Ustrtoul(a,b,c)  strtoul((char *)(a),(char **)(b), c)

/* The usual "stringify" macros */

#define STRING(a)  # a
#define XSTRING(s) STRING(s)

/* -ftrivial-auto-var-init support supports initializing all local variables
to avoid some classes of bug, but this can cause an unacceptable slowdown
for large on-stack arrays in hot functions. This macro lets us annotate
such arrays. */

#ifdef HAVE_ATTRIBUTE_UNINITIALIZED
#define B2PF_KEEP_UNINITIALIZED __attribute__((uninitialized))
#else
#define B2PF_KEEP_UNINITIALIZED
#endif

/* External (in the C sense) functions and tables that are private to the
libraries are always referenced using the PRIV macro. This makes it clear in
the code that a non-static object is being referenced. */

#ifndef PRIV
#define PRIV(name) _b2pf_##name
#endif

/* When compiling a DLL for Windows, the exported symbols have to be declared
using some MS magic. I found some useful information on this web page:
http://msdn2.microsoft.com/en-us/library/y4h7bcy6(VS.80).aspx. According to the
information there, using __declspec(dllexport) without "extern" we have a
definition; with "extern" we have a declaration. The settings here override the
setting in b2pf.h (which is included below); it defines only B2PF_EXP_DECL,
which is all that is needed for applications (they just import the symbols). We
use:

  B2PF_EXP_DECL    for declarations
  B2PF_EXP_DEFN    for definitions

The reason for wrapping this in #ifndef B2PF_EXP_DECL is so that b2pftest,
which is an application, but needs to import this file in order to "peek" at
internals, can #include b2pf.h first to get an application's-eye view.

In principle, people compiling for non-Windows, non-Unix-like (i.e. uncommon,
special-purpose environments) might want to stick other stuff in front of
exported symbols. That's why, in the non-Windows case, we set B2PF_EXP_DEFN
only if it is not already set. */

#ifndef B2PF_EXP_DECL
#  ifdef _WIN32
#    ifndef B2PF_STATIC
#      define B2PF_EXP_DECL       extern __declspec(dllexport)
#      define B2PF_EXP_DEFN       __declspec(dllexport)
#    else
#      define B2PF_EXP_DECL       extern
#      define B2PF_EXP_DEFN
#    endif
#  else
#    ifdef __cplusplus
#      define B2PF_EXP_DECL       extern "C"
#    else
#      define B2PF_EXP_DECL       extern
#    endif
#    ifndef B2PF_EXP_DEFN
#      define B2PF_EXP_DEFN       B2PF_EXP_DECL
#    endif
#  endif
#endif


/* Include the public B2PF header. This must follow the setting of
B2PF_EXP_DECL above. */

#include "b2pf.h"

/* This is an unsigned int value that no UTF character can ever have, as
Unicode doesn't go beyond 0x0010ffff. */

#define NOTACHAR 0xffffffff

/* This is the largest valid UTF/Unicode code point. */

#define MAX_UTF_CODE_POINT 0x10ffff

/* Types for the nodes in the trees */

enum { CT_COMB, CT_MISC, CT_PRES, CT_LIG, CT_AFT };

/* Format types */

enum { F_ISOLATED, F_INITIAL, F_MEDIAL, F_FINAL };

/* Internal error codes for context checks */

enum { CHECK_ERROR0, CHECK_ERROR1, CHECK_ERROR2, CHECK_ERROR3, CHECK_ERROR4 };


/* ---------------- Special values for rules -------------- */

/* These, when tested, do not consume a character. */
#define R_REND            0x80000000u  /* End of rule */
#define R_WSTART          0x80000001u  /* Start of word */
#define R_WEND            0x80000002u  /* End of word */
#define R_BRA             0x80000003u  /* Start of replaceable chars */
#define R_KET             0x80000004u  /* End of replaceable chars */
#define R_BECOMES         0x80000005u  /* -> */
/* These always consume a character and must follow the above. */
#define R_ANY             0x80000006u  /* Any character */
#define R_FINAL           0x80000007u  /* Character with final form */
#define R_INITIAL         0x80000008u  /* Character with initial form */
#define R_MEDIAL          0x80000009u  /* Character with medial form */
#define R_JOINNEXT        0x8000000Au  /* Character that can join to next */
#define R_JOINPREV        0x8000000Bu  /* Character that can join to previous */
#define R_ISOLATED        0x8000000Cu  /* Character with isolated form */
#define R_NOTJOINNEXT     0x8000000Du  /* Character that can't join to next */
#define R_NOTJOINPREV     0x8000000Eu  /* Character that can't join to previous */


/* ---------------- UTF macros ---------------- */

/* Tests whether a UTF code point is complete or needs more. */

#define HASUTF8EXTRALEN(c) ((c) >= 0xc0)
#define HASUTF16EXTRALEN(c) (((c) & 0xfc00u) == 0xd800u)
#define HASUTF32EXTRALEN(c) (0)

/* Macro to load the additional bytes of a UTF-8 character, advancing the
pointer. Called when the previous byte is >= 0xc0. */

#define GETUTF8REMINC(c, p) \
    { \
    if ((c & 0x20u) == 0) \
      c = ((c & 0x1fu) << 6) | (*p++ & 0x3fu); \
    else if ((c & 0x10u) == 0) \
      { \
      c = ((c & 0x0fu) << 12) | ((*p & 0x3fu) << 6) | (p[1] & 0x3fu); \
      p += 2; \
      } \
    else if ((c & 0x08u) == 0) \
      { \
      c = ((c & 0x07u) << 18) | ((*p & 0x3fu) << 12) | \
          ((p[1] & 0x3fu) << 6) | (p[2] & 0x3fu); \
      p += 3; \
      } \
    else if ((c & 0x04u) == 0) \
      { \
      c = ((c & 0x03u) << 24) | ((*p & 0x3fu) << 18) | \
          ((p[1] & 0x3fu) << 12) | ((p[2] & 0x3fu) << 6) | \
          (p[3] & 0x3fu); \
      p += 4; \
      } \
    else \
      { \
      c = ((c & 0x01u) << 30) | ((*p & 0x3fu) << 24) | \
          ((p[1] & 0x3fu) << 18) | ((p[2] & 0x3fu) << 12) | \
          ((p[3] & 0x3fu) << 6) | (p[4] & 0x3fu); \
      p += 5; \
      } \
    }

/* Get the next UTF-8 character, advancing the pointer. */

#define GETUTF8INC(c, p) \
  c = *p++; \
  if (c >= 0xc0u) GETUTF8REMINC(c, p);


/* ----------------------- INTERNAL STRUCTURES --------------------------- */

/* Structure for each node in the character and ligature trees. For character
trees, the keys are single 32-bit characters, but a single node can be used for
a range. For ligatures first and last are always the same combined pair of
characters. */

typedef struct tree_node
  {
  struct tree_node *left;    /* pointer to left child */
  struct tree_node *right;   /* pointer to right child */
  uint64_t first;            /* first key in range */
  uint64_t last;             /* last key in range */
  uint32_t pforms[4];        /* presentation forms or ligature info */
  uschar balance;            /* balancing factor */
  uschar type;               /* type of character */
  }
tree_node;

/* Structure for each rule. The final element is specified with a large value,
but the memory obtained is just exactly what is needed for the rule. This
avoids certain warnings about overflow which can happen if, for example, the
vector is specified with [1].  */

typedef struct coded_rule
  {
  struct coded_rule *next;
  uint32_t options;
  uint32_t prelen;     /* Pre-assertion length */
  uint32_t replen;     /* Number of chars to replace */
  uint32_t code[32767];
  }
coded_rule;


/* ----------------------- HIDDEN STRUCTURES ----------------------------- */

typedef struct b2pf_real_context {
  void *(*malloc)(size_t, void *);
  void  (*free)(void *, void *);
  void *memory_data;
  tree_node *chartreebase;
  tree_node *ligtreebase;
  tree_node *aftertreebase;
  coded_rule *rules;
  uint32_t ligs[2];
  uint32_t check_error;
  BOOL checked;
  BOOL prelig_error;
} b2pf_real_context;


/* ------------------ PRIVATE TABLES AND FUNCTIONS -----------------------*/

extern const int      PRIV(utf8_table1)[];
extern const int      PRIV(utf8_table1_size);
extern const int      PRIV(utf8_table2)[];
extern const int      PRIV(utf8_table3)[];
extern const uint8_t  PRIV(utf8_table4)[];

extern BOOL _b2pf_check_context(b2pf_context *);
extern int  _b2pf_format_string(uint32_t *, size_t, uint32_t *, size_t,
  size_t *, b2pf_context *, size_t *);
extern int  _b2pf_valid_utf8(uint8_t *, size_t, size_t *);
extern int  _b2pf_valid_utf16(uint16_t *, size_t, size_t *);
extern int  _b2pf_valid_utf32(uint32_t *, size_t, size_t *);

extern BOOL       _b2pf_tree_insert(tree_node **, tree_node *);
extern tree_node *_b2pf_tree_search(tree_node *, uint64_t);


#endif  /* B2PF_INTERNAL_H_IDEMPOTENT_GUARD */

/* End of p2pf_internal.h */
