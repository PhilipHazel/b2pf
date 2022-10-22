/*************************************************
*       Base Unicode to Presentation Forms       *
*************************************************/

/* This file contains the error message function for the B2PF library.

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

#define KNOWN_OPTIONS (B2PF_UTF_16|B2PF_UTF_32)


/* In order to reduce the number of relocations needed when a shared library is
loaded dynamically, error texts are coded as one long string, and we just count
through to the one that's wanted. This isn't a performance issue because these
strings are used only when there is an error. Each substring ends with \0 to
insert a null character. This includes the final substring, so that the whole
string ends with \0\0, which can be detected when counting through. */

#define STRING(a)  # a
#define XSTRING(s) STRING(s)

static const uschar error_texts[] =
  "No error\0"
  "Context check failed: call b2pf_get_check_message() for details\0"
  "Failed to get memory\0"
  "Invalid NULL argument\0"
  "Bad option setting\0"
  /* 5 */
  "Buffer is too small\0"
  "Unknown error number\0"
  "Failed to open rules file\0"
  "No space after rule identifier\0"
  "Duplicate character or range overlap in rule\0"
  /* 10 */
  "Unknown escape sequence in rule\0"
  "Missing ] after rule options\0"
  "Misplaced ^ in rule (must be at start)\0"
  "Misplaced $ in rule (must be at end or just before ->)\0"
  "Found ) before ( in rule\0"
  /* 15 */
  "Repeated ( in rule\0"
  "Repeated ) in rule\0"
  "Missing ) in rule\0"
  "Unknown rule identifier\0"
  "Invalid character range in rule\0"
  /* 20 */
  "Extraneous character(s) at end of rule\0"
  "Word is too long (maximum " XSTRING(MAXWORD) " characters)\0"
  "Internal error 1\0"
  "Internal error 2\0"
  "Internal error 3\0"
  /* 25 */
  "No matched character for character type replacement\0"
  "Matched character does not have the requested presentation form\0"
  "Missing or invalid ligature data\0"
  "Duplicate ligature\0"
  "\\n, \\N, \\p, and \\P are invalid in replacement text\0"
  ;

/* UTF error texts are in the same format. */

static const uschar utf_error_texts[] =
  "no error\0"
  "UTF-8 error: 1 byte missing at end\0"
  "UTF-8 error: 2 bytes missing at end\0"
  "UTF-8 error: 3 bytes missing at end\0"
  "UTF-8 error: 4 bytes missing at end\0"
  /* 5 */
  "UTF-8 error: 5 bytes missing at end\0"
  "UTF-8 error: byte 2 top bits not 0x80\0"
  "UTF-8 error: byte 3 top bits not 0x80\0"
  "UTF-8 error: byte 4 top bits not 0x80\0"
  "UTF-8 error: byte 5 top bits not 0x80\0"
  /* 10 */
  "UTF-8 error: byte 6 top bits not 0x80\0"
  "UTF-8 error: 5-byte character is not allowed (RFC 3629)\0"
  "UTF-8 error: 6-byte character is not allowed (RFC 3629)\0"
  "UTF-8 error: code points greater than 0x10ffff are not defined\0"
  "UTF-8 error: code points 0xd800-0xdfff are not defined\0"
  /* 15 */
  "UTF-8 error: overlong 2-byte sequence\0"
  "UTF-8 error: overlong 3-byte sequence\0"
  "UTF-8 error: overlong 4-byte sequence\0"
  "UTF-8 error: overlong 5-byte sequence\0"
  "UTF-8 error: overlong 6-byte sequence\0"
  /* 20 */
  "UTF-8 error: isolated byte with 0x80 bit set\0"
  "UTF-8 error: illegal byte (0xfe or 0xff)\0"
  "UTF-16 error: missing low surrogate at end\0"
  "UTF-16 error: invalid low surrogate\0"
  "UTF-16 error: isolated low surrogate\0"
  /* 25 */
  "UTF-32 error: code points 0xd800-0xdfff are not defined\0"
  "UTF-32 error: code points greater than 0x10ffff are not defined\0"
  ;



/*************************************************
*          Copy message to buffer                *
*************************************************/

/* Copy to 8, 16, or 32-bit buffer as appropriate. As the error texts contain
only ASCII characters, we can just copy each code unit, whatever the width.

Arguments:
  message         the message text
  message_buffer  where to put it
  buffer_size     buffer size (code units)
  buffer_used     set how many used
  options         specify code unit width

Returns:          B2PF_SUCCESS or an error codew
*/

static int
copy_to_buffer(const uschar *message, void *message_buffer, size_t buffer_size,
  size_t *buffer_used, uint32_t options)
{
size_t used;
int yield = B2PF_SUCCESS;

if ((options & ~KNOWN_OPTIONS) != 0 ||
    (options & (B2PF_UTF_16|B2PF_UTF_32)) == (B2PF_UTF_16|B2PF_UTF_32))
  return B2PF_ERROR_BADOPTIONS;

if (buffer_size == 0) return B2PF_ERROR_OVERFLOW;

if ((options & (B2PF_UTF_16|B2PF_UTF_32)) == 0)
  {
  uint8_t *p8 = (uint8_t *)message_buffer;
  for (used = 0; *message != 0; used++)
    {
    if (used >= buffer_size - 1)
      {
      yield = B2PF_ERROR_OVERFLOW;
      break;
      }
    p8[used] = *message++;
    }
  p8[used] = 0;
  }

else if ((options & B2PF_UTF_16) != 0)
  {
  uint16_t *p16 = (uint16_t *)message_buffer;
  for (used = 0; *message != 0; used++)
    {
    if (used >= buffer_size - 1)
      {
      yield = B2PF_ERROR_OVERFLOW;
      break;
      }
    p16[used] = *message++;
    }
  p16[used] = 0;
  }

else
  {
  uint32_t *p32 = (uint32_t *)message_buffer;
  for (used = 0; *message != 0; used++)
    {
    if (used >= buffer_size - 1)
      {
      yield = B2PF_ERROR_OVERFLOW;
      break;
      }
    p32[used] = *message++;
    }
  p32[used] = 0;
  }

*buffer_used = used;
return yield;
}



/*************************************************
*              Get error message                 *
*************************************************/

/* This gets the fixed message that is associated with each error number.

Arguments:
  errorcode       the error code
  message_buffer  where to put it
  buffer_size     buffer size (code units)
  buffer_used     set how many used
  options         specify code unit width

Returns:          B2PF_SUCCESS or an error code
*/

B2PF_EXP_DEFN int
b2pf_get_error_message(int errorcode, void *message_buffer, size_t buffer_size,
  size_t *buffer_used, uint32_t options)
{
int n;
const uschar *message;

if (errorcode > 0)
  {
  message = error_texts;
  n = errorcode;
  }
else
  {
  message = utf_error_texts;
  n = -errorcode;
  }

for (; n > 0; n--)
  {
  while (*message++ != 0) {};
  if (*message == 0) return B2PF_ERROR_BADERROR;
  }

return copy_to_buffer(message, message_buffer, buffer_size, buffer_used,
  options);
}



/*************************************************
*        Get details of context check            *
*************************************************/

/* This function constructs a message that gives details of a context checking
failure.

Arguments:
  context         context that has an error
  message_buffer  where to put it
  buffer_size     buffer size (code units)
  buffer_used     set how many used
  options         specify code unit width

Returns:          B2PF_SUCCESS or an error code
*/


B2PF_EXP_DEFN int
b2pf_get_check_message(b2pf_context *context, void *message_buffer,
  size_t buffer_size, size_t *buffer_used, uint32_t options)
{
uschar buff[256];

switch(context->check_error)
  {
  case CHECK_ERROR0:
  strcpy((char *)buff, "No context check error found");
  break;

  case CHECK_ERROR1:
  sprintf((char *)buff, "The first character in the U+%04X/U+%04X ligature is unknown",
    context->ligs[0], context->ligs[1]);
  break;

  case CHECK_ERROR2:
  sprintf((char *)buff, "The second character in the U+%04X/U+%04X ligature is unknown",
    context->ligs[0], context->ligs[1]);
  break;

  case CHECK_ERROR3:
  sprintf((char *)buff, "The U+%04X/U+%04X ligature has characters of different combining types",
    context->ligs[0], context->ligs[1]);
  break;

  case CHECK_ERROR4:
  sprintf((char *)buff, "The U+%04X/U+%04X \"after\" ligature contains a combining character",
    context->ligs[0], context->ligs[1]);
  break;

  default:
  sprintf((char *)buff, "Internal error: unknown context check code");
  break;
  }

return copy_to_buffer(buff, message_buffer, buffer_size, buffer_used,
  options);
}

/* End of b2pf_error.c */
