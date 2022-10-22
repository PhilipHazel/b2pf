/*************************************************
*       Base Unicode to Presentation Forms       *
*************************************************/

/* This file contains a test program for the B2PF library.

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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "b2pf.h"

typedef int BOOL;
#ifndef FALSE
#define FALSE   0
#define TRUE    1
#endif

/* Both libreadline and libedit are optionally supported. In at least one
system readline/readline.h is installed as editline/readline.h, so the
configuration code now looks for that first, falling back to
readline/readline.h. */

#if defined(SUPPORT_LIBREADLINE) || defined(SUPPORT_LIBEDIT)
#if defined(SUPPORT_LIBREADLINE)
#include <readline/readline.h>
#include <readline/history.h>
#else
#if defined(HAVE_EDITLINE_READLINE_H)
#include <editline/readline.h>
#else
#include <readline/readline.h>
#endif
#endif
#endif

/* Put the test for interactive input into a macro so that it can be changed if
required for different environments. */

#define INTERACTIVE(f) isatty(fileno(f))

/* We'll be handling bytes as unsigned chars, so make a short definition. */

typedef unsigned char uschar;

/* Execution modes; they must be the number of bytes per code unit. */

#define UNSET_MODE   0
#define B2PF8_MODE   1
#define B2PF16_MODE  2
#define B2PF32_MODE  4

/* Miscellaneous parameters */

#define INBUFFER_SIZE    256
#define PUT_BUFFER_SIZE  (4 * INBUFFER_SIZE)
#define GET_BUFFER_SIZE  (2 * PUT_BUFFER_SIZE)



/* ---------------------- System-specific definitions ---------------------- */

/* A number of things vary for Windows builds. I copied these from PCRE2. */

#if defined(_WIN32) || defined(WIN32)
#include <io.h>             /* For _setmode() */
#include <fcntl.h>          /* For _O_BINARY */
#define INPUT_MODE          "r"
#define OUTPUT_MODE         "wb"
#define BINARY_INPUT_MODE   "rb"
#define BINARY_OUTPUT_MODE  "wb"

#ifndef isatty
#define isatty _isatty         /* This is what Windows calls them, I'm told, */
#endif                         /* though in some environments they seem to   */
                               /* be already defined, hence the #ifndefs.    */
#ifndef fileno
#define fileno _fileno
#endif

#ifdef __BORLANDC__
#define _setmode(handle, mode) setmode(handle, mode)
#endif

/* Not Windows */

#else
#define INPUT_MODE          "rb"
#define OUTPUT_MODE         "wb"
#define BINARY_INPUT_MODE   "rb"
#define BINARY_OUTPUT_MODE  "wb"
#endif

/* These macros are the standard way of turning unquoted text into C strings.
They allow macros like B2PF2_MAJOR to be defined without quotes, which is
convenient for user programs that want to test their values. */

#define STRING(a)  # a
#define XSTRING(s) STRING(s)

/* 32-bit integer values in the input are read by strtoul() or strtol(). The
check needed for overflow depends on whether long ints are in fact longer than
ints. They are defined not to be shorter. */

#if ULONG_MAX > UINT32_MAX
#define U32OVERFLOW(x) (x > UINT32_MAX)
#else
#define U32OVERFLOW(x) (x == UINT32_MAX)
#endif

#if LONG_MAX > INT32_MAX
#define S32OVERFLOW(x) (x > INT32_MAX || x < INT32_MIN)
#else
#define S32OVERFLOW(x) (x == INT32_MAX || x == INT32_MIN)
#endif


/* ----------------------- Tables ---------------------------------- */

/* These are the breakpoints for different numbers of bytes in a UTF-8
character. */

static const int utf8_table1[] =
  { 0x7f, 0x7ff, 0xffff, 0x1fffff, 0x3ffffff, 0x7fffffff};

static const int utf8_table1_size = sizeof(utf8_table1) / sizeof(int);

/* These are the indicator bits for the data bits to set in the
first byte of a character, indexed by the number of additional bytes. */

static const int utf8_table2[] = { 0, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc};

/* Table of the number of extra UTF-8 bytes, indexed by the first byte masked
with 0x3f. */

const uint8_t utf8_table4[] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5 };


/* -------------------------- UTF-8 macro --------------------------------- */

/* Pick up the remaining bytes of a UTF-8 character, advancing the pointer. The
input pointer should be at the second byte. */

#define GETUTF8INC(c, p) \
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


/* ----------------------- Static variables ------------------------ */

static const char *version = NULL;
static b2pf_context *context = NULL;

static char *rules_dir_list = NULL;
static char *inbuffer = NULL;
static void *put_buffer = NULL;
static void *get_buffer = NULL;

static uint32_t global_options = 0;
BOOL had_context_error = FALSE;



/*************************************************
*          Handle error from the library         *
*************************************************/

/* Get the error message as an 8-bit string.

Arguments:
  rc          the error code
  offset      error position offset when processing
              rules line number when initializing
  processing  TRUE when processing, FALSE when initializing
  outfile     the output file

Returns:      nothing
*/

static void
handle_b2pf_error(int rc, size_t offset, BOOL processing, FILE *outfile)
{
size_t outused;
fprintf(outfile, "** B2PF error %d", rc);
if (processing) fprintf(outfile, " at offset %lu", offset);
  else if (offset > 0) fprintf(outfile, " in line %lu", offset);
b2pf_get_error_message(rc, get_buffer, GET_BUFFER_SIZE, &outused, 0);
fprintf(outfile, ": %.*s\n\n", (int)outused, (char *)get_buffer);
}



/*************************************************
*                  Read a word                   *
*************************************************/

/* Leading space is ignored.

Arguments:
  p          input pointer
  w          output pointer

Returns:     new value of input pointer
*/

static char *
readword(char *p, char *w)
{
while (isspace(*p)) p++;
while (isalnum(*p) || *p == '_') *w++ = *p++;
*w = 0;
return p;
}



/*************************************************
*           Read a quoted string                 *
*************************************************/

/* Leading space is ignored.
Arguments:
  p          input pointer
  s          output pointer

Returns:     new value of input pointer
*/

static char *
readstring(char *p, char *s)
{
while (isspace(*p)) p++;
if (*p == '"')
  {
  while (*(++p) != 0 && *p != '"') *s++ = *p;
  if (*p != 0) p++;
  }

*s = 0;
return p;
}


/*************************************************
*           Handle a command line                *
*************************************************/

/* Called when the line starts with '#' not followed by a space or exclamation
mark.

Arguments:
  p         points to *
  mode      8/16/32 mode
  outfile   the output file

Returns:    TRUE on success, FALSE on failure
*/

static BOOL
handle_command(char *p, int mode, FILE *outfile)
{
int rc;
char word[128];

(void)mode;  /* Not currently used */

word[0] = 0;
if (!isspace(*(++p))) p = readword(p, word);

if (strcmp(word, "context_create") == 0)
  {
  unsigned int ln;
  uint32_t options = 0;  /* None yet defined */
  p = readstring(p, word);
  if (context != NULL) b2pf_context_free(context);
  rc = b2pf_context_create(word, rules_dir_list, options, &context,
    NULL,NULL,NULL, &ln);
  if (rc != B2PF_SUCCESS)
    {
    handle_b2pf_error(rc, (size_t)ln, FALSE, outfile);
    return FALSE;
    }
  }

else if (strcmp(word, "context_add_file") == 0)
  {
  unsigned int ln;
  uint32_t options = 0;  /* None yet defined */
  p = readstring(p, word);
  rc = b2pf_context_add_file(context, word, rules_dir_list, options, &ln);
  if (rc != B2PF_SUCCESS)
    {
    handle_b2pf_error(rc, (size_t)ln, FALSE, outfile);
    if (rc == B2PF_ERROR_NULL && context == NULL)
      {
      fprintf(outfile, "** b2pftest: Can't extend non-existent context\n");
      return FALSE;
      }
    }
  }

else if (strcmp(word, "context_add_line") == 0)
  {
  rc = b2pf_context_add_line(context, p);
  if (rc != B2PF_SUCCESS)
    {
    handle_b2pf_error(rc, 0, FALSE, outfile);
    had_context_error = TRUE;
    if (rc == B2PF_ERROR_NULL && context == NULL)
      {
      fprintf(outfile, "** b2pftest: Can't extend non-existent context\n");
      return FALSE;
      }
    }
  }

else if (strcmp(word, "input_backchars") == 0)
  {
  global_options |= B2PF_INPUT_BACKCHARS;
  }

else if (strcmp(word, "input_backcodes") == 0)
  {
  global_options |= B2PF_INPUT_BACKCODES;
  }

else if (strcmp(word, "output_backchars") == 0)
  {
  global_options |= B2PF_OUTPUT_BACKCHARS;
  }

else if (strcmp(word, "output_backcodes") == 0)
  {
  global_options |= B2PF_OUTPUT_BACKCODES;
  }

else if (strcmp(word, "reset") == 0)
  {
  global_options = 0;
  }

else
  {
  fprintf(outfile, "** b2pftest: Unknown command \"#%s\"\n", word);
  return FALSE;
  }

return TRUE;
}



/*************************************************
*           Handle a data line                   *
*************************************************/

/* Note that the input must be handled as unsigned characters because there
will be bytes greater than 127.

Arguments:
  p          pointer to UTF-8 input line
  mode       8/16/32
  outfile    the output file

Returns:     FALSE for UTF-8 error when converting to UTF-16 or UTF-32
             TRUE in all other cases
*/

static BOOL
handle_data(uschar *p, int mode, FILE *outfile)
{
size_t insize, outused, error_offset;
size_t psize = strlen((char *)p);
uschar *pstart = p;
int rc;
int charcount = 0;
uint32_t options = global_options;
uint16_t *p16 = (uint16_t *)put_buffer;
uint32_t *p32 = (uint32_t *)put_buffer;

/* Convert the UTF-8 input into the appropriate width. In 8-bit mode, just
copy verbatim. This allows for the testing of invalid UTF-8. In the other
modes we don't allow invalid UTF-8. */

if (mode == B2PF8_MODE)
  {
  insize = psize;
  (void)memcpy(put_buffer, inbuffer, insize);
  }

else
  {
  insize = 0;
  options |= (mode == B2PF16_MODE)? B2PF_UTF_16 : B2PF_UTF_32;

  while (*p != 0)
    {
    uint32_t c = *p++;
    charcount++;
    psize--;

    /* Pick up the UTF-8 code point. Only un-decodable errors are picked up
    here. Overlong coding and other tests are left to the library. */

    if (c >= 128)
      {
      size_t ab;
      if (c < 0xc0)
        {
        fprintf(outfile, "** b2pftest: UTF-8 error: isolated 10xx xxxx byte\n");
        goto BADUTF;
        }
      if (c >= 0xfe)
        {
        fprintf(outfile, "** b2pftest: UTF-8 error: invalid 0xfe or 0xff byte\n");
        goto BADUTF;
        }
      ab = utf8_table4[c & 0x3f];   /* Number of additional bytes (1-5) */
      if (psize < ab)
        {
        fprintf(outfile, "** b2pftest: UTF-8 error: missing bytes at end of string\n");
        goto BADUTF;
        }
      psize -= ab;  /* Length remaining, check bytes for 0x80 */
      for (; ab > 0; ab--)
        {
        if ((p[ab-1] & 0xc0) != 0x80)
          {
          fprintf(outfile, "** b2pftest: UTF-8 error: invalid byte (top bits not 0x80)\n");
          goto BADUTF;
          }
        }
      GETUTF8INC(c, p);
      }

    /* Store the character in the appropriate width */

    if (mode == B2PF16_MODE)
      {
      if (c > 0xffff)
        {
        c -= 0x10000;
        p16[insize++] = 0xd800 | (c >> 10);
        p16[insize++] = 0xdc00 | (c & 0x3ff);
        }
      else p16[insize++] = c;
      }
    else  /* UTF-32 */
      {
      p32[insize++] = c;
      }
    }
  }

/* Do the business. */

rc = b2pf_format_string(context, put_buffer, insize, get_buffer,
  GET_BUFFER_SIZE/mode, &outused, options, &error_offset);

/* Handle a context check error, but then carry on to output the processed
string. */

if (rc == B2PF_ERROR_CONTEXTCHECK)
  {
  size_t used;
  char buff[INBUFFER_SIZE];
  fprintf(outfile, "** B2PF context check failed (error %d):\n", rc);
  b2pf_get_check_message(context, buff, INBUFFER_SIZE, &used, 0);
  fprintf(outfile, "** %.*s\n\n", (int)used, buff);
  }

/* Handle any other error from the library, but still return TRUE, as that is
not an error in b2pftest. */

else if (rc != B2PF_SUCCESS)
  {
  handle_b2pf_error(rc, error_offset, TRUE, outfile);
  return TRUE;
  }

/* Output the processed string. */

fprintf(outfile, "  ");

if (mode == B2PF8_MODE)
  {
  fprintf(outfile, "%.*s", (int)outused, (char *)get_buffer);
  }

else
  {
  p16 = (uint16_t *)get_buffer;
  p32 = (uint32_t *)get_buffer;

  while (outused > 0)
    {
    int i, j;
    uint32_t c;
    uschar buffer[8];
    uschar *b;

    outused--;
    if (mode == B2PF16_MODE)
      {
      c = *p16++;
      if ((c & 0xfc00u) == 0xd800u)
        {
        c = (((c & 0x3ffu) << 10) | (*p16++ & 0x3ffu)) + 0x10000u;
        outused--;
        }
      }
    else c = *p32++;

    /* Convert to UTF-8 and output */

    for (i = 0; i < utf8_table1_size; i++)
      if ((int)c <= utf8_table1[i]) break;
    b = buffer + i;
    for (j = i; j > 0; j--)
     {
     *b-- = 0x80 | (c & 0x3f);
     c >>= 6;
     }
    *b = utf8_table2[i] | c;
    fprintf(outfile, "%.*s", i+1, buffer);
    }
  }

fprintf(outfile, "\n");
return TRUE;

/* An error was detected while loading UTF-8 to convert to UTF-16/32. */

BADUTF:
fprintf(outfile, "** at character %d offset %lu\n", charcount, p - pstart);
return FALSE;
}



/*************************************************
*               Print B2PF version               *
*************************************************/

/* The hackery below is to cope with the case when B2PF_PRERELEASE is set to an
empty string (which it is for real releases). If the second alternative is used
in this case, it does not leave a space before the date. On the other hand, if
all four macros are put into a single XSTRING when B2PF_PRERELEASE is not
empty, an unwanted space is inserted. There are problems using an "obvious"
approach like this:

   XSTRING(B2PF_MAJOR) "." XSTRING(B2PF_MINOR)
   XSTRING(B2PF_PRERELEASE) " " XSTRING(B2PF_DATE)

because, when B2PF_PRERELEASE is empty, this leads to an attempted expansion
of STRING(). The C standard states: "If (before argument substitution) any
argument consists of no preprocessing tokens, the behavior is undefined." It
turns out the gcc treats this case as a single empty string - which is what
we really want - but Visual C grumbles about the lack of an argument for the
macro. Unfortunately, both are within their rights. As there seems to be no
way to test for a macro's value being empty at compile time, we have to
resort to a runtime test. */

static void
print_version(FILE *f)
{
if (version == NULL)
  version = (XSTRING(Z B2PF_PRERELEASE)[1] == 0)?
    XSTRING(B2PF_MAJOR.B2PF_MINOR B2PF_DATE) :
    XSTRING(B2PF_MAJOR.B2PF_MINOR) XSTRING(B2PF_PRERELEASE B2PF_DATE);
fprintf(f, "B2PF version %s\n", version);
}



/*************************************************
*             Usage function                     *
*************************************************/

static void
usage(void)
{
printf("Usage:     b2pftest [options] [<input file> [<output file>]]\n\n");
printf("Input and output default to stdin and stdout.\n");
#if defined(SUPPORT_LIBREADLINE) || defined(SUPPORT_LIBEDIT)
printf("If input is a terminal, readline() is used to read from it.\n");
#else
printf("This version of b2pftest is not linked with readline().\n");
#endif
printf("\nOptions:\n");
printf("  -8            run tests using 8-bit strings (default)\n");
printf("  -16           run tests using 16-bit strings\n");
printf("  -32           run tests using 32-bit strings\n");
printf("  -F <list>     specify colon-separated rules directories\n");
printf("  -help         show usage information\n");
printf("  -q            quiet: do not output b2pf version number at start\n");
printf("  -version      show b2pf version and exit\n");
}



/*************************************************
*                Main Program                    *
*************************************************/

int
main(int argc, char **argv)
{
FILE *infile = stdin;
FILE *outfile = stdout;
const char *prompt = "> ";
BOOL quiet = FALSE;
BOOL notdone = TRUE;
int test_mode = UNSET_MODE;
int yield = 0;
int op = 1;

/* The following  _setmode() stuff is some Windows magic that tells its runtime
library to translate CRLF into a single LF character. At least, that's what
I've been told: never having used Windows I take this all on trust. Originally
it set 0x8000, but then I was advised that _O_BINARY was better. */

#if defined(_WIN32) || defined(WIN32)
_setmode( _fileno( stdout ), _O_BINARY );
#endif

/* Scan command line options. */

while (argc > 1 && argv[op][0] == '-' && argv[op][1] != 0)
  {
  int arg_mode = UNSET_MODE;
  char *arg = argv[op];

  /* Select operating mode. */

  if (strcmp(arg, "-8") == 0) arg_mode = B2PF8_MODE;
  else if (strcmp(arg, "-16") == 0) arg_mode = B2PF16_MODE;
  else if (strcmp(arg, "-32") == 0) arg_mode = B2PF32_MODE;

  /* Set quiet (no version verification) */

  else if (strcmp(arg, "-q") == 0) quiet = TRUE;

  /* Set rules directory list */

  else if (strcmp(arg, "-F") == 0)
    {
    rules_dir_list = argv[++op];
    argc--;
    }

  /* Give help */

  else if (strcmp(arg, "-help") == 0 ||
           strcmp(arg, "--help") == 0)
    {
    usage();
    goto EXIT;
    }

  /* Show version */

  else if (strcmp(arg, "-version") == 0 ||
           strcmp(arg, "--version") == 0)
    {
    print_version(stdout);
    goto EXIT;
    }

  /* Unrecognized option */

  else
    {
    fprintf(stderr, "** b2pftest: Unknown or malformed option '%s'\n", arg);
    usage();
    yield = 1;
    goto EXIT;
    }
  op++;
  argc--;

  /* Check only one mode is specified */

  if (arg_mode != UNSET_MODE)
    {
    if (test_mode != UNSET_MODE && test_mode != arg_mode)
      {
      fprintf(stderr, "** b2pftest: Only one mode can be set\n");
      yield = 1;
      goto EXIT;
      }
    else test_mode = arg_mode;
    }
  }

if (test_mode == UNSET_MODE) test_mode = B2PF8_MODE;

/* Sort out the input and output files, defaulting to stdin/stdout (as set
above). */

if (argc > 1 && strcmp(argv[op], "-") != 0)
  {
  infile = fopen(argv[op], INPUT_MODE);
  if (infile == NULL)
    {
    fprintf(stderr, "** b2pftest: Failed to open '%s': %s\n", argv[op], strerror(errno));
    yield = 1;
    goto EXIT;
    }
  }

#if defined(SUPPORT_LIBREADLINE) || defined(SUPPORT_LIBEDIT)
if (INTERACTIVE(infile)) using_history();
#endif

if (argc > 2)
  {
  outfile = fopen(argv[op+1], OUTPUT_MODE);
  if (outfile == NULL)
    {
    fprintf(stderr, "** b2pftest: Failed to open '%s': %s\n", argv[op+1], strerror(errno));
    yield = 1;
    goto EXIT;
    }
  }

/* Get buffers from malloc() so that valgrind will check their misuse when
debugging. */

inbuffer = (char *)malloc(INBUFFER_SIZE);
put_buffer = malloc(PUT_BUFFER_SIZE);
get_buffer = malloc (GET_BUFFER_SIZE);

/* Output a heading line unless quiet, then process input lines. */

if (!quiet) print_version(outfile);

while (notdone)
  {
  char *p;

  /* If libreadline or libedit support is required, use readline() to read a
  line if the input is a terminal. Note that readline() removes the trailing
  newline, so we must put it back again, to be compatible with fgets(). */

#if defined(SUPPORT_LIBREADLINE) || defined(SUPPORT_LIBEDIT)
  if (INTERACTIVE(infile))
    {
    size_t len;
    char *s = readline(prompt);
    if (s == NULL) break;
    len = strlen(s);
    if (len > 0) add_history(s);
    if (len > INBUFFER_SIZE - 1) len = INBUFFER_SIZE - 1;
    memcpy(inbuffer, s, len);
    inbuffer[len] = '\n';
    inbuffer[++len] = 0;
    free(s);
    }
  else
#endif

  /* Read the next line by normal means, prompting if the file is a tty. */

    {
    if (INTERACTIVE(infile)) printf("%s", prompt);
    if (fgets(inbuffer, INBUFFER_SIZE, infile) == NULL) break;
    }

  /* Remove leading spaces */

  p = inbuffer;
  while (*p != 0 && isspace(*p)) p++;

  /* Command or command line */

  if (*p == 0 || *p == '#')
    {
    if (!INTERACTIVE(infile)) fprintf(outfile, "%s", inbuffer);
    fflush(outfile);
    if (*p == 0 || p[1] == ' ' || p[1] == '!') continue;
    notdone = handle_command(p, test_mode, outfile);
    }

  /* Data line; give up if previous context error. Note that a FALSE return is
  given by handle_data only for a UTF-8 error in the input. Errors from the
  library still return TRUE. */

  else
    {
    size_t s = strlen(inbuffer);
    if (!INTERACTIVE(infile)) fprintf(outfile, "> %s", inbuffer);
    fflush(outfile);
    if (had_context_error)
      {
      fprintf(outfile, "** b2pftest: Giving up because of previous context error(s).\n");
      yield = 1;
      goto EXIT;
      }
    while (s > 0 && inbuffer[s-1] == '\n') s--;
    inbuffer[s] = 0;
    notdone = handle_data((uschar *)p, test_mode, outfile);
    }
  }

EXIT:

#if defined(SUPPORT_LIBREADLINE) || defined(SUPPORT_LIBEDIT)
if (infile != NULL && INTERACTIVE(infile)) clear_history();
#endif

if (infile != NULL && infile != stdin) fclose(infile);
if (outfile != NULL && outfile != stdout) fclose(outfile);

if (inbuffer != NULL) free(inbuffer);
if (put_buffer != NULL) free(put_buffer);
if (get_buffer != NULL) free(get_buffer);

if (context != NULL) b2pf_context_free(context);

return yield;
}

/* End of b2pf.c */
