/*************************************************
*       Base Unicode to Presentation Forms       *
*************************************************/

/* This file contains the main processing function for the B2PF library.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "b2pf_internal.h"

#define UTF8   1        /* Number of bytes per code unit */
#define UTF16  2
#define UTF32  4

#define KNOWN_OPTIONS (B2PF_UTF_16|B2PF_UTF_32|B2PF_INPUT_BACKCHARS| \
  B2PF_INPUT_BACKCODES|B2PF_OUTPUT_BACKCHARS|B2PF_OUTPUT_BACKCODES)


/*************************************************
*             Invert a 32-bit vector             *
*************************************************/

/* Re-ordering can be by characters (that is, a base + optional combiners), or
by codes, which is by individual code units.

Arguments:
  p        points to the vector
  size     number of elements
  bychars  if TRUE, re-order characters with combiners
  context  the current context

Returns:   nothing
*/

static void
invert(uint32_t *p, size_t size, BOOL bychars, b2pf_context *context)
{
size_t i = 0;
size_t j = size - 1;

/* Inverting by code points is straightforward and we do that first. */

while (j > i)
  {
  size_t temp = p[i];
  p[i++] = p[j];
  p[j--] = temp;
  }

if (!bychars) return;

/* Inverting by character requires checking for combining characters and moving
them to after the following non-combiner. */

for (i = 0; i < size; i++)
  {
  tree_node *t = PRIV(tree_search)(context->chartreebase, p[i]);

  if (t == NULL || t->type != CT_COMB) continue;

  for(j = i + 1; j < size; j++)
    {
    t = PRIV(tree_search)(context->chartreebase, p[j]);
    if (t == NULL || t->type != CT_COMB) break;
    }

  /* If j is at the end, there is no base character. Just ignore anomalous
  case. Otherwise, pick up the base character, shift up all the combiners, and
  put the base at the start. */

  if (j < size)
    {
    size_t k;
    uint32_t c = p[j];
    for (k = j; k > i; k--) p[k] = p[k-1];
    p[i] = c;
    }

  i = j + 1;  /* Next character to consider. */
  }
}



/*************************************************
*        Main action: format a UTF string        *
*************************************************/

B2PF_EXP_DEFN int
b2pf_format_string(b2pf_context *context, void *input_string, size_t input_size,
  void *output_string, size_t output_size, size_t *output_used,
  uint32_t options, size_t *error_offset)
{
int yield = B2PF_SUCCESS;
int mode;
size_t insize, outsize, outused;
uint32_t *inbuffer = NULL;
uint32_t *outbuffer = NULL;
uint32_t stack_inbuffer[STACK_BUFFSIZE] B2PF_KEEP_UNINITIALIZED;
uint32_t stack_outbuffer[STACK_BUFFSIZE]B2PF_KEEP_UNINITIALIZED;

/* Plausibility checks */

if (context == NULL || input_string == NULL || output_string == NULL ||
    output_used == NULL || error_offset == NULL) return B2PF_ERROR_NULL;

if ((options & ~KNOWN_OPTIONS) != 0 ||
    (options & (B2PF_UTF_16|B2PF_UTF_32)) == (B2PF_UTF_16|B2PF_UTF_32) ||
    (options & (B2PF_INPUT_BACKCODES|B2PF_INPUT_BACKCHARS)) ==
               (B2PF_INPUT_BACKCODES|B2PF_INPUT_BACKCHARS) ||
    (options & (B2PF_OUTPUT_BACKCODES|B2PF_OUTPUT_BACKCHARS)) ==
               (B2PF_OUTPUT_BACKCODES|B2PF_OUTPUT_BACKCHARS))
  return B2PF_ERROR_BADOPTIONS;

/* Set up */

*error_offset = 0;
mode = ((options & B2PF_UTF_32) != 0)? UTF32 :
       ((options & B2PF_UTF_16) != 0)? UTF16 : UTF8;

/* For each code unit width, validate the UTF input/ */

if (mode == UTF8)
  yield = PRIV(valid_utf8)((uint8_t *)input_string, input_size,
    error_offset);
else if (mode == UTF16)
  yield = PRIV(valid_utf16)((uint16_t *)input_string, input_size,
    error_offset);
else   /* UTF-32 */
  yield = PRIV(valid_utf32)((uint32_t *)input_string, input_size,
    error_offset);
if (yield != B2PF_SUCCESS) goto EXIT;

/* Sort out the buffers. For UTF-8 and UTF-16, input must be converted to
UTF-32 in a local buffer. We also copy UTF-32 input so that we can shuffle the
characters if necessary when handling ligatures. If there are more input code
units than available elements in the on-stack input buffer, get memory. This
means we are safe, even when every input code unit codes for one character.
It's overkill for other cases, but does no harm (other than using more than
minimal resources). In practice, STACK_BUFFSIZE should be large enough to cater
for the majority of cases. */

if (input_size > STACK_BUFFSIZE)
  {
  inbuffer = context->malloc(input_size * sizeof(uint32_t),
    context->memory_data);
  if (inbuffer == NULL)
    {
    yield = B2PF_ERROR_MEMORY;
    goto EXIT;
    }
  }
else
  {
  inbuffer = stack_inbuffer;
  }

/* A 32-bit local output buffer is also required, except for UTF-32, when the
argument buffer can be used. For UTF-8 and UTF-16, if we got memory for an
input buffer, also get an output buffer of the same size. There's no need for
UTF-32; a small output buffer will be discovered during processing instead of
afterwards, but that's OK. */

if (mode == UTF32)
  {
  outbuffer = (uint32_t *)output_string;
  outsize = output_size;
  }
else if (inbuffer != stack_inbuffer)
  {
  outbuffer = context->malloc(input_size * sizeof(uint32_t),
    context->memory_data);
  if (outbuffer == NULL)
    {
    yield = B2PF_ERROR_MEMORY;
    goto EXIT;
    }
  outsize = input_size;
  }
else
  {
  outbuffer = stack_outbuffer;
  outsize = STACK_BUFFSIZE;
  }

/* Decode the input string into 32-bit code points. */

insize = 0;

if (mode == UTF8)
  {
  uint8_t *p8 = (uint8_t *)input_string;
  uint8_t *p8end = p8 + input_size;
  while (p8 < p8end)
    {
    uint32_t c;
    GETUTF8INC(c, p8);
    inbuffer[insize++] = c;
    }
  }

else if (mode == UTF16)
  {
  size_t i;
  uint16_t *p16 = (uint16_t *)input_string;

  for (i = 0; i < input_size; i++)
    {
    uint32_t c = *p16++;
    if ((c & 0xfc00u) == 0xd800u)
      {
      c = (((c & 0x3ffu) << 10) | (*p16++ & 0x3ffu)) + 0x10000u;
      i++;
      }
    inbuffer[insize++] = c;
    }
  }

else  /* UTF-32 */
  {
  memcpy(inbuffer, input_string, sizeof(uint32_t)*input_size);
  insize = input_size;
  }

/* If the input is backwards, invert it, then process it, and invert the output
if necessary. */

if ((options & (B2PF_INPUT_BACKCHARS|B2PF_INPUT_BACKCODES)) != 0)
  invert(inbuffer, insize, (options & B2PF_INPUT_BACKCHARS) != 0, context);

yield = PRIV(format_string)(inbuffer, insize, outbuffer, outsize, &outused,
  context, error_offset);
if (yield != B2PF_SUCCESS && yield != B2PF_ERROR_CONTEXTCHECK) goto EXIT;

if ((options & (B2PF_OUTPUT_BACKCHARS|B2PF_OUTPUT_BACKCODES)) != 0)
  invert(outbuffer, outused, (options & B2PF_OUTPUT_BACKCHARS) != 0, context);

/* Copy the output back to UTF-32, UTF-16 or UTF-8. In the UTF-32 case, the
copy is needed only if we used an internal buffer. */

if (mode == UTF32)
  {
  if (outbuffer != output_string)
    {
    if (outused > output_size) goto OVERFLOW;
    memcpy(output_string, outbuffer, sizeof(uint32_t)*outused);
    }
  *output_used = outused;
  }

else if (mode == UTF16)
  {
  size_t i;
  size_t used = 0;
  uint16_t *p16 = (uint16_t *)output_string;
  for (i = 0; i < outused; i++)
    {
    uint32_t c = outbuffer[i];
    if (c <= 0xffff)
      {
      if (used++ >= output_size) goto OVERFLOW;
      *p16++ = (uint16_t)c;
      }
    else
      {
      if (used >= output_size - 1) goto OVERFLOW;
      used += 2;
      c -= 0x10000;
      *p16++ = 0xd800 | (c >> 10);
      *p16++ = 0xdc00 | (c & 0x3ff);
      }
    }
  *output_used = used;
  }

else  /* UTF-8 */
  {
  size_t i;
  size_t used = 0;
  uint8_t *p8 = (uint8_t *)output_string;

  for (i = 0; i < outused; i++)
    {
    int j, k;
    uint8_t *pt;
    uint32_t c = outbuffer[i];
    for (j = 0; j < PRIV(utf8_table1_size); j++)
      if ((int)c <= PRIV(utf8_table1)[j]) break;
    if (used >= output_size - j) goto OVERFLOW;
    used += j + 1;
    pt = p8 += j;
    for (k = j; k > 0; k--)
     {
     *pt-- = 0x80 | (c & 0x3f);
     c >>= 6;
     }
    *pt = PRIV(utf8_table2)[j] | c;
    p8++;
    }
  *output_used = used;
  }

/* All done. */

EXIT:
if (inbuffer != NULL && inbuffer != stack_inbuffer)
  context->free(inbuffer, context->memory_data);

if (outbuffer != NULL && outbuffer != stack_outbuffer &&
    outbuffer != output_string)
  context->free(outbuffer, context->memory_data);

return yield;

/* Common overflow code */

OVERFLOW:
yield = B2PF_ERROR_OVERFLOW;
goto EXIT;
}

/* End of b2pf.c */
