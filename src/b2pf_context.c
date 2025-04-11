/*************************************************
*       Base Unicode to Presentation Forms       *
*************************************************/

/* This file contains functions for context manipulation in the B2PF library.

                 Copyright (c) 2025 Philip Hazel

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


//#define DEBUGRULES
//#define DEBUGTREE

#ifdef DEBUGTREE
/*************************************************
*          Debugging print tree function         *
*************************************************/

static void
print_tree(tree_node *t, int level)
{
int i;

if (t == NULL) return;
print_tree(t->right, level + 1);

for (i = 0; i < level; i++) fprintf(stderr, "  ");

switch (t->type)
  {
  case CT_COMB:
  fprintf(stderr, "%04lx-%04lx C ", t->first, t->last);
  break;

  case CT_LIG:
  fprintf(stderr, "%04lx L %04x", t->first, t->pforms[0]);
  break;

  case CT_AFT:
  fprintf(stderr, "%04lx A %04x", t->first, t->pforms[0]);
  break;

  case CT_MISC:
  fprintf(stderr, "%04lx-%04lx M ", t->first, t->last);
  break;

  case CT_PRES:
  fprintf(stderr, "%04lx-%04lx P ", t->first, t->last);
  for (i = 0; i < 4; i++)
    if (t->pforms[i] == 0) fprintf(stderr, " -");
      else fprintf(stderr, " %04x", t->pforms[i]);
  break;

  default:
  fprintf(stderr, "Unknown node type in tree");
  break;
  }

fprintf(stderr, "\n");
print_tree(t->left, level + 1);
}
#endif


#ifdef DEBUGRULES
/*************************************************
*         Debugging print rules function         *
*************************************************/

static void
print_rules(coded_rule *r)
{
for (; r != NULL; r = r->next)
  {
  uint32_t c;
  uint32_t *code = r->code;

  fprintf(stderr, "R %d/%d [", r->prelen, r->replen);

  /*  Print each option; currently there are none */

  fprintf(stderr, "] ");

  for (c = *code++; c != R_REND; c = *code++)
    {
    int i, j;
    uschar buffer[8];
    uschar *b;

    switch (c)
      {
      case R_WSTART: fprintf(stderr, "^ "); break;
      case R_WEND: fprintf(stderr, "$ "); break;
      case R_BECOMES: fprintf(stderr, "-> "); break;
      case R_BRA: fprintf(stderr, "( "); break;
      case R_KET: fprintf(stderr, ") "); break;
      case R_ANY: fprintf(stderr, ". "); break;
      case R_FINAL: fprintf(stderr, "\\f "); break;
      case R_INITIAL: fprintf(stderr, "\\i "); break;
      case R_MEDIAL: fprintf(stderr, "\\m "); break;
      case R_JOINNEXT: fprintf(stderr, "\\n "); break;
      case R_JOINPREV: fprintf(stderr, "\\p "); break;
      case R_ISOLATED: fprintf(stderr, "\\s "); break;
      case R_NOTJOINNEXT: fprintf(stderr, "\\N "); break;
      case R_NOTJOINPREV: fprintf(stderr, "\\P "); break;

      default:
      if (c >= 0x80000000u) fprintf(stderr, "<%X> ", c); else
        {
        for (i = 0; i < PRIV(utf8_table1_size); i++)
          if ((int)c <= PRIV(utf8_table1)[i]) break;
        b = buffer + i;
        for (j = i; j > 0; j--)
         {
         *b-- = 0x80 | (c & 0x3f);
         c >>= 6;
         }
        *b = PRIV(utf8_table2)[i] | c;
        fprintf(stderr, "%.*s ", i+1, buffer);
        }
      break;
      }
    }

  fprintf(stderr, "\n");
  }
}

#endif



/*************************************************
*         Check ligature tree in a context       *
*************************************************/

static BOOL
check_ligatures(b2pf_context *context, tree_node *t, BOOL preligs)
{
tree_node *tt1, *tt2;
uint64_t x, y;

if (t == NULL) return TRUE;
if (!check_ligatures(context, t->left, preligs) ||
    !check_ligatures(context, t->right, preligs))
  return FALSE;

x = (t->first) >> 32;
y = (t->first) & 0xffffffffu;

tt1 = PRIV(tree_search)(context->chartreebase, x);
tt2 = PRIV(tree_search)(context->chartreebase, y);

/* For "pre" ligatures, both characters must be known and either both be
combiners or both be non-combiners. */

if (preligs)
  {
  if (tt1 == NULL || tt2 == NULL)
    {
    context->check_error = (tt1 == NULL)? CHECK_ERROR1 : CHECK_ERROR2;
    goto FAIL;
    }

  if ((tt1->type == CT_COMB) != (tt2->type == CT_COMB))
    {
    context->check_error = CHECK_ERROR3;
    goto FAIL;
    }
  }

/* For "after" ligatures, unknown characters are allowed, but neither character
can be a combiner. */

else
  {
  context->prelig_error = FALSE;
  if ((tt1 != NULL && tt1->type == CT_COMB) ||
      (tt2 != NULL && tt2->type == CT_COMB))
    {
    context->check_error = CHECK_ERROR4;
    goto FAIL;
    }
  }

/* All checks succeeded */

return TRUE;

FAIL:
context->prelig_error = preligs;
context->ligs[0] = x;
context->ligs[1] = y;
return FALSE;
}



/*************************************************
*          Check context for validity            *
*************************************************/

/* This function is called at the start of b2pf_format_string() when there have
been changes to the context. It check for inconsistencies in the context.
Details are stored in the context, for retrieval by b2pf_get_check_message().

Argument:  pointer to the context
Returns:   TRUE for success, FALSE for fail
*/

BOOL
PRIV(check_context)(b2pf_context *context)
{
if (!check_ligatures(context, context->ligtreebase, TRUE) ||
    !check_ligatures(context, context->aftertreebase, FALSE))
  return FALSE;

context->checked = TRUE;
context->check_error = CHECK_ERROR0;  /* No error */
return TRUE;
}



/*************************************************
*          Default malloc/free functions         *
*************************************************/

/* Ignore the "user data" argument in each case. */

static void *default_malloc(size_t size, void *data)
{
(void)data;
return malloc(size);
}

static void default_free(void *block, void *data)
{
(void)data;
free(block);
return;
}



/*************************************************
*              Context free functions            *
*************************************************/

static void
free_tree(b2pf_context *context, tree_node *t)
{
if (t == NULL) return;
free_tree(context, t->left);
free_tree(context, t->right);
context->free(t, context->memory_data);
}


B2PF_EXP_DEFN void
b2pf_context_free(b2pf_context *context)
{
coded_rule *rule, *next;
if (context == NULL) return;
free_tree(context, context->chartreebase);
free_tree(context, context->ligtreebase);
free_tree(context, context->aftertreebase);
for (rule = context->rules; rule != NULL; rule = next)
  {
  next = rule->next;
  context->free(rule, context->memory_data);
  }
context->free(context, context->memory_data);
}



/*************************************************
*          Read character or Unicode value       *
*************************************************/

/* If the next two characters are "U+", read a hex code point. Otherwise, pick
up a Unicode character and check its validity.

Arguments:
  p           pointer in input string
  cptr        where to return the result
  errptr      where to put an error code

Returns:      new value of p or NULL on error
*/

static uschar *
readchar (uschar *p, uint32_t *cptr, int *errptr)
{
uint32_t c;
GETUTF8INC(c, p);

if (c == 'U' && *p == '+' && Uisxdigit(p[1]))
  {
  unsigned long int x = Ustrtoul(++p, &p, 16);
  if (x > MAX_UTF_CODE_POINT)
    {
    *errptr = B2PF_ERROR_UTF8_ERR13;
    return NULL;
    }
  if (x >= 0xd800 && x <= 0xdfff)
    {
    *errptr = B2PF_ERROR_UTF8_ERR14;
    return NULL;
    }
  c = (uint32_t)x;
  }

*cptr = c;
return p;
}



/*************************************************
*            Read rule options                   *
*************************************************/

/*
Arguments:
  p          points to initial '['
  optr       where to put options
  errptr     where to return an error code

Returns:     update pointer or NULL on error
*/

static uschar *
read_rule_options(uschar *p, uint32_t *optr, int *errptr)
{
p++;

while (*p != 0 && *p != ']')
  {
  while (Uisspace(*p)) p++;

/* READ OPTIONS - none defined yet */


(void)optr;

  }

if (*p != ']')
  {
  *errptr = B2PF_ERROR_MISSINGOPTKET;
  return NULL;
  }

return ++p;
}



/*************************************************
*            Process one rules line              *
*************************************************/

/* This function is called from below, but can also be called externally to add
to a context.

Arguments:
  context      points to a context
  rline        a rules line

Returns:       B2PF_SUCCESS or an error code
*/

B2PF_EXP_DEFN int
b2pf_context_add_line(b2pf_context *context, const char *rline)
{
char ctype;
int i, rcount, ccount, errorcode, ttype;
BOOL hadbra, hadket, hadarrow;
tree_node *t, **tbase;
#ifdef DEBUGTREE
const char *tname;
#endif
uint32_t c, d, options;
uint32_t prelen, replen;
uint32_t rulebuffer[128];
uint32_t lig[3];
coded_rule *rule, **rulesptr;
uschar *p = (uschar *)rline;

if (context == NULL || rline == NULL) return B2PF_ERROR_NULL;
prelen = replen = 0;   /* Pre-assertion and replacement lengths */
hadbra = hadket = hadarrow = FALSE;

/* New rules are added to the end of the list. */

rulesptr = &(context->rules);
while (*rulesptr != NULL) rulesptr = &((*rulesptr)->next);

/* Empty and comment lines are ignored. There must be at least one space after
the first significant character. */

while (Uisspace(*p)) p++;
c = *p++;
if (c == 0 || c == '#') return B2PF_SUCCESS;
if (*p++ != ' ') return B2PF_ERROR_BADRULE;

switch (c)
  {
  /* -------- Unknown character at line start -------- */

  default: return B2PF_ERROR_UNKNOWNRULE;

  /* -------- Add miscelleous characters or combiners to the tree -------- */

  case 'C':  /* Combining characters */
  case 'M':  /* Letters with no special characteristics */
  ctype = (c == 'C')? CT_COMB : CT_MISC;

  for (;;)
    {
    while (Uisspace(*p)) p++;
    if (*p == 0 || *p == '#') break;
    p = readchar(p, &c, &errorcode);
    if (p == NULL) return errorcode;
    if (*p == '-')
      {
      if (*(++p) == 0 || Uisspace(*p)) return B2PF_ERROR_BADRANGE;
      p = readchar(p, &d, &errorcode);
      if (p == NULL || d < c) return B2PF_ERROR_BADRANGE;
      }
    else d = c;

    t = context->malloc(sizeof(tree_node), context->memory_data);
    if (t == NULL) return B2PF_ERROR_MEMORY;

    t->first = c;
    t->last = d;
    t->type = ctype;

    if (!PRIV(tree_insert)(&(context->chartreebase), t))
      {
      context->free(t, context->memory_data);
      return B2PF_ERROR_BADCHAR;
      }
    }
  break;


  /* -------- Read "after" ligature information -------- */

  case 'A':
  tbase = &(context->aftertreebase);
#ifdef DEBUGTREE
  tname = "AFTER";
#endif
  ttype = CT_AFT;
  goto READLIG;

  /* -------- Read ligature information -------- */

  case 'L':
  tbase = &(context->ligtreebase);
#ifdef DEBUGTREE
  tname = "LIGATURE";
#endif
  ttype = CT_LIG;

  READLIG:
  while (Uisspace(*p)) p++;
  for (i = 0; i < 3; i++)
    {
    if (*p == 0 || *p == '#') return B2PF_ERROR_BADLIGATURE;
    p = readchar(p, &c, &errorcode);
    if (p == NULL) return errorcode;
    lig[i] = c;
    while (Uisspace(*p)) p++;
    }
  if (*p != 0 && *p != '#') return B2PF_ERROR_EXTRACHARS;

  t = context->malloc(sizeof(tree_node), context->memory_data);
  if (t == NULL) return B2PF_ERROR_MEMORY;

  t->first = lig[0];
  t->first = t->last = (t->first << 32) | lig[1];
  t->type = ttype;
  t->pforms[0] = lig[2];
  if (!PRIV(tree_insert)(tbase, t)) return B2PF_ERROR_DUPLIGATURE;

#ifdef DEBUGTREE
  fprintf(stderr, "\n%s TREE ENTRY\n", tname);
  print_tree(t, 0);
#endif

  break;


  /* -------- Read rule-specific options -------- */

  case 'O':  /* Options */

  break;


  /* -------- Read letter with presentation forms -------- */

  case 'P':  /* Letter with presentation forms */
  while (Uisspace(*p)) p++;
  if (*p == 0) break;
  p = readchar(p, &c, &errorcode);
  if (p == NULL) return errorcode;

  t = context->malloc(sizeof(tree_node), context->memory_data);
  if (t == NULL) return B2PF_ERROR_MEMORY;
  t->first = t->last = c;
  t->type = CT_PRES;

  /* Now read 4 presentation forms */

  while (Uisspace(*p)) p++;
  for (i = 0; i < 4; i++)
    {
    if (*p == 0)
      {
      t->pforms[i] = 0;
      continue;
      }
    p = readchar(p, &c, &errorcode);
    if (p == NULL) return errorcode;
    if (c == '-') c = 0;       /* Non-existent form */
    t->pforms[i] = c;
    while (Uisspace(*p)) p++;
    }

  if (*p != 0 && *p != '#')
    {
    context->free(t, context->memory_data);
    return B2PF_ERROR_EXTRACHARS;
    }

  if (!PRIV(tree_insert)(&(context->chartreebase), t))
    return B2PF_ERROR_BADCHAR;

#ifdef NEVER

/* This seemed like a good idea when I was starting, but I'm not so sure now,
so it is cut out, but I'm leaving the code so that it could easily be
reinstated if necessary. */

  /* Clone the tree entry for any presentation forms that exist and are
  different. This is simpler than a system of multiple pointers to the same
  data, and the memory duplication is minimal. */

  kt = t;
  for (i = 0; i < 4; i++)
    {
    int j;
    c = kt->pforms[i];
    if (c == 0 || c == kt->first) continue;
    for (j = 0; j < i; j++) if (c == kt->pforms[j]) continue;
    t = context->malloc(sizeof(tree_node), context->memory_data);
    if (t == NULL) return B2PF_ERROR_MEMORY;
    memcpy(t, kt, sizeof(tree_node));
    t->first = t->last = kt->pforms[i];
    if (!PRIV(tree_insert)(&(context->chartreebase), t))
      {
      context->free(t, context->memory_data);
      return B2PF_ERROR_BADCHAR;
      }
    }
#endif

  break;


  /* -------- Read a rule -------- */

  case 'R':  /* Rule */
  while (Uisspace(*p)) p++;
  options = 0;
  rcount = 0;
  ccount = 0;

  if (*p == '[')
    {
    p = read_rule_options(p, &options, &errorcode);
    if (p == NULL) return errorcode;
    }

  /* Don't recognize U+ in line, only when preceded by \ */

  while (Uisspace(*p)) p++;
  while (*p != 0)
    {
    if (*p == 'U')
      {
      c = 'U';
      p++;
      }
    else
      {
      p = readchar(p, &c, &errorcode);
      if (p == NULL) return errorcode;
      }

    switch (c)
      {
      case '#':   /* Comment; ignore rest of line */
      *p = 0;
      break;

      case '(':   /* Start of replaceable characters */
      if (hadarrow) goto LITERAL;
      if (hadbra) return B2PF_ERROR_DOUBLEBRA;
      hadbra = TRUE;
      prelen = ccount;
      ccount = 0;
      rulebuffer[rcount++] = R_BRA;
      break;

      case ')':   /* End of replaceable characters */
      if (hadarrow) goto LITERAL;
      if (hadket) return B2PF_ERROR_DOUBLEKET;
      if (!hadbra) return B2PF_ERROR_MISSINGBRA;
      hadket = TRUE;
      replen = ccount;
      rulebuffer[rcount++] = R_KET;
      break;

      case '^':
      if (hadarrow) goto LITERAL;
      if (rcount != 0) return B2PF_ERROR_BADCIRCUMFLEX;
      rulebuffer[rcount++] = R_WSTART;
      break;

      case '$':
      if (hadarrow) goto LITERAL;
      rulebuffer[rcount++] = R_WEND;
        {
        uschar *pp = p;
        while (Uisspace(*pp)) pp++;
        if (*pp != 0 && *pp != '#' && (*pp != '-' || pp[1] != '>'))
          return B2PF_ERROR_BADDOLLAR;
        }
      break;

      case '.':
      rulebuffer[rcount++] = R_ANY;
      ccount++;
      break;

      case '\\':
      switch (*p)
        {
        case 'f':
        rulebuffer[rcount++] = R_FINAL;
        break;

        case 'i':
        rulebuffer[rcount++] = R_INITIAL;
        break;

        case 'm':
        rulebuffer[rcount++] = R_MEDIAL;
        break;

        case 'n':
        if (hadarrow) return B2PF_ERROR_BADREPLACE;
        rulebuffer[rcount++] = R_JOINNEXT;
        break;

        case 'N':
        if (hadarrow) return B2PF_ERROR_BADREPLACE;
        rulebuffer[rcount++] = R_NOTJOINNEXT;
        break;

        case 'p':
        if (hadarrow) return B2PF_ERROR_BADREPLACE;
        rulebuffer[rcount++] = R_JOINPREV;
        break;

        case 'P':
        if (hadarrow) return B2PF_ERROR_BADREPLACE;
        rulebuffer[rcount++] = R_NOTJOINPREV;
        break;

        case 's':
        rulebuffer[rcount++] = R_ISOLATED;
        break;

        case 'U':
        if (p[1] != '+') return B2PF_ERROR_BADESCAPE;
        p = readchar(p, &c, &errorcode);
        if (p == NULL) return errorcode;
        rulebuffer[rcount++] = c;
        p--;  /* Re-scan terminator */
        break;

        default:
        if (isalnum(*p)) return B2PF_ERROR_BADESCAPE;
        rulebuffer[rcount++] = *p;
        break;
        }

      p++;
      ccount++;
      break;

      case '-':
      if (hadarrow) goto LITERAL;
      if (*p == '>')
        {
        rulebuffer[rcount++] = R_BECOMES;
        p++;
        hadarrow = TRUE;
        if (!hadbra) prelen = ccount;
        break;
        }
      /* Fall through */

      default:
      LITERAL:
      rulebuffer[rcount++] = c;
      ccount++;
      break;
      }

    while (Uisspace(*p)) p++;
    }

  if (hadbra && !hadket) return B2PF_ERROR_MISSINGKET;
  if (rcount == 0) return B2PF_SUCCESS;  /* Empty rule; nothing to save */
  if (!hadbra && !hadarrow) prelen = ccount;

  rulebuffer[rcount++] = R_REND;
  rule = context->malloc(offsetof(coded_rule,code) + rcount*sizeof(uint32_t),
    context->memory_data);
  if (rule == NULL) return B2PF_ERROR_MEMORY;
  *rulesptr = rule;
  rulesptr = &(rule->next);
  rule->next = NULL;
  rule->options = options;
  rule->prelen = prelen;
  rule->replen = replen;
  memcpy(rule->code, rulebuffer, rcount*sizeof(uint32_t));

#ifdef DEBUGRULES
  fprintf(stderr, "\nADDED RULE\n");
  print_rules(rule);
  fprintf(stderr, "\n");
#endif

  break;
  }

context->checked = FALSE;
return B2PF_SUCCESS;
}



/*************************************************
*     Add the contents of a file to a context    *
*************************************************/

/* This makes it possible to have multiple sets of rules that can be added to a
base context.

Arguments:
  context         pointer to an existing context
  rules_name      name of rules file to merge
  rules_dir_list  colon-separated list of extra directories to search
  options         option bits (none yet defined)
  lineptr         line number set on error

Returns:          0 on success or an error code
*/

B2PF_EXP_DEFN int
b2pf_context_add_file(b2pf_context *context,
  const char *rules_name, const char *rules_dir_list,
  uint32_t options, unsigned int *lineptr)
{
int errorcode;
unsigned int line_number = 0;
FILE *f = NULL;
uschar buffer[128];

(void)options;   /* No options yet defined */

if (context == NULL || lineptr == NULL) return B2PF_ERROR_NULL;

/* If the rules file name is empty, do nothing */

if (rules_name == NULL || *rules_name == 0) return B2PF_SUCCESS;

/* Search for a rules file, starting with any additional directories. */

if (rules_dir_list != NULL)
  {
  const char *pp = rules_dir_list;
  while (*pp != 0)
    {
    int len;
    const char *ep = strchr(pp, ':');
    if (ep == NULL) ep = strchr(pp, 0);

    /* Using sprintf() for the whole string causes a compiler warning if
    -Wformat-overflow is set. */

    len = ep - pp;
    (void)memcpy(buffer, pp, len);
    sprintf((char *)buffer + len, "/%s", rules_name);
    f = fopen((char *)buffer, "r");
    if (f != NULL || *ep == 0) break;
    pp = ep + 1;
    }
  }

/* Try the default directory if not yet found. */

if (f == NULL)
  {
  sprintf((char *)buffer, "%s/%s", XSTRING(DATADIR) "/b2pf/rules/", rules_name);
  f = fopen((char *)buffer, "r");
  if (f == NULL)
    {
    errorcode = B2PF_ERROR_FILE;
    goto ERROR;
    }
  }

/* Process the lines of the file */

while (fgets((char *)buffer, sizeof(buffer), f) != NULL)
  {
  line_number++;
  errorcode = b2pf_context_add_line(context, (char *)buffer);
  if (errorcode != B2PF_SUCCESS) goto ERROR;
  }

/* All done - rule file successfully read. */

#ifdef DEBUGTREE
fprintf(stderr, "\nCHARACTER TREE\n");
print_tree(context->chartreebase, 0);
fprintf(stderr, "\nLIGATURE TREE\n");
print_tree(context->ligtreebase, 0);
fprintf(stderr, "\nAFTER TREE\n");
print_tree(context->aftertreebase, 0);
#ifndef DEBUGRULES
fprintf(stderr, "\n");
#endif
#endif

#ifdef DEBUGRULES
fprintf(stderr, "\nRULES\n");
print_rules(context->rules);
fprintf(stderr, "\n");
#endif

fclose(f);
return B2PF_SUCCESS;

/* Error exit */

ERROR:
if (f != NULL) fclose(f);
*lineptr = line_number;
return errorcode;
}



/*************************************************
*      Add a callback function to a context      *
*************************************************/

/* Arguments:
  context    the context
  callback   the callback function
  data       a data value for the callback

Returns:     0 on success or an error code
*/

B2PF_EXP_DEFN int
b2pf_context_set_callback(b2pf_context *context, uint32_t options,
  int(*callback)(uint32_t, void *), void *data)
{
if (context == NULL) return B2PF_ERROR_NULL;
if ((options & ~B2PF_CALLBACK_OPTIONS) != 0) return B2PF_ERROR_BADOPTIONS;
context->callback = callback;
context->callback_data = data;
context->options &= ~B2PF_CALLBACK_OPTIONS;
context->options |= options;
return 0;
}



/*************************************************
*         Create and initialize a context        *
*************************************************/

/* This is typically the first call to the B2PF library.

Arguments:
  rules_name      the name of the rules file
  rules_dir_list  colon-separated list of extra directories to search
  options         option bits
  contptr         where to put the context pointer if successful
  private_malloc  pointer to private malloc() or NULL
  private_free    pointer to private free() or NULL
  memory_data     data for private malloc() and free()
  lineptr         line number set on error

Returns:          0 on success or an error code
*/

B2PF_EXP_DEFN int
b2pf_context_create(const char *rules_name, const char *rules_dir_list,
  uint32_t options, b2pf_context **contptr,
  void *(*private_malloc)(size_t, void *),
  void (*private_free)(void *, void *), void *memory_data,
  unsigned int *lineptr)
{
int rc;
b2pf_context *context;

if (private_malloc == NULL) private_malloc = default_malloc;
if (private_free == NULL) private_free = default_free;

context = private_malloc(sizeof(b2pf_real_context), memory_data);
if (context == NULL) return B2PF_ERROR_MEMORY;

context->malloc = private_malloc;
context->free = private_free;
context->callback = NULL;
context->memory_data = memory_data;
context->callback_data = NULL;
context->chartreebase = NULL;
context->ligtreebase = NULL;
context->aftertreebase = NULL;
context->rules = NULL;
context->options = options;
context->checked = FALSE;
context->check_error = CHECK_ERROR0;  /* No error */

rc = b2pf_context_add_file(context, rules_name, rules_dir_list, options,
  lineptr);
if (rc != B2PF_SUCCESS) b2pf_context_free(context); else *contptr = context;
return rc;
}

/* End of b2pf_context.c */
