/*************************************************
*       Base Unicode to Presentation Forms       *
*************************************************/

/* This file contains the function that does the actual work of the B2PF
library.

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


/* This is a fake character tree node that is used for ligatures that are not
listed in the character tree. */

static tree_node default_miscchar = {
  NULL, NULL,      /* left, right */
  0, 0,            /* first, last (should never be consulted) */
  { 0, 0, 0, 0 },  /* no presentation forms */
  0,               /* balance factor (never consulted) */
  CT_MISC          /* type */
  };



/*************************************************
*      Convert a word to presentation form       *
*************************************************/

/* This is what the whole library is about.

Arguments:
  word           points to start of word
  count          number of characters in the word (at least 1)
  treecache      cache of tree pointers for each character
  outbuffer      output buffer
  outsize        size of output buffer
  outusedptr     pointer to output used value
  context        the current context
  error_offset   where to return an offset on error (initialized to 0)

Returns:         B2PF_SUCCESS or an error code
*/

static int
format_word(uint32_t *word, size_t count, tree_node **treecache,
  uint32_t *outbuffer, size_t outsize, size_t *outusedptr,
  b2pf_context *context, size_t *error_offset)
{
tree_node *t;
size_t i;
size_t outused = *outusedptr;

/* Scan character by character and apply the rules. */

for (i = 0; i < count; i++)
  {
  coded_rule *r;
  uint32_t *rp;

  *error_offset = i;  /* In case any errors occur */

  for (r = context->rules; r != NULL; r = r->next)
    {
    size_t j, k, kket;
    size_t wildcount = 0;
    size_t wildmatch[WORDMAX];

    /* k is the scanning index. Start at the current character and go back by
    the number of chars in the pre-assertion. We have to include combiners with
    each character, so cannot just do a substraction. */

    k = i;
    kket = count;   /* Haven't hit ket yet */

    for (j = 0; j < r->prelen; j++)
      {
      for (;;)
        {
        if (k == 0) goto NEXTRULE;  /* Too few preceding characters */
        if ((treecache[--k])->type != CT_COMB) break;
        }
      }

    /* Check the rule */

    for (rp = r->code; *rp != R_REND && *rp != R_BECOMES; rp++)
      {
      /* Rule items >= R_ANY must have a character to match. */

      if (*rp >= R_ANY || (*rp & 0x80000000u) == 0)
        {
        if (k >= count) goto NEXTRULE;  /* No character available */
        t = treecache[k];
        }

      switch(*rp)
        {
        case R_WSTART:
        if (i > 0) goto NEXTRULE;
        break;

        case R_WEND:
        if (k != count) goto NEXTRULE;
        break;

        case R_BRA:
        if (k != i) return B2PF_ERROR_INTERNAL2;
        break;

        case R_KET:
        kket = k;
        break;

        case R_NOTJOINNEXT:
        if (t->type == CT_PRES &&
             (t->pforms[F_INITIAL] != 0 || t->pforms[F_MEDIAL] != 0))
          goto NEXTRULE;
        break;

        case R_NOTJOINPREV:
        if (t->type == CT_PRES &&
             (t->pforms[F_FINAL] != 0 || t->pforms[F_MEDIAL] != 0))
          goto NEXTRULE;
        break;

        case R_ANY:
        break;

        case R_FINAL:
        if (t->type != CT_PRES || t->pforms[F_FINAL] == 0) goto NEXTRULE;
        break;

        case R_INITIAL:
        if (t->type != CT_PRES || t->pforms[F_INITIAL] == 0) goto NEXTRULE;
        break;

        case R_MEDIAL:
        if (t->type != CT_PRES || t->pforms[F_MEDIAL] == 0) goto NEXTRULE;
        break;

        case R_JOINNEXT:
        if (t->type != CT_PRES ||
          (t->pforms[F_INITIAL] == 0 && t->pforms[F_MEDIAL] == 0))
            goto NEXTRULE;
        break;

        case R_JOINPREV:
        if (t->type != CT_PRES ||
          (t->pforms[F_MEDIAL] == 0 && t->pforms[F_FINAL] == 0))
            goto NEXTRULE;
        break;

        case R_ISOLATED:
        if (t->type != CT_PRES || t->pforms[F_ISOLATED] == 0) goto NEXTRULE;
        break;

        default:
        if ((*rp & 0x80000000u) != 0) return B2PF_ERROR_INTERNAL1;
        if (word[k] != *rp) goto NEXTRULE;
        break;
        }

      /* If we accepted a character, advance past it and any following
      combiners. Remember the positions of any non-literals. */

      if ((*rp & 0x80000000u) == 0)
        {
        while (++k < count && (treecache[k])->type == CT_COMB) {}
        }

      else if (*rp >= R_ANY)
        {
        if (k >= i && k <= kket) wildmatch[wildcount++] = k;
        while (++k < count && (treecache[k])->type == CT_COMB) {}
        }
      }    /* End of rule matching scan */

    /* A rule has successfully matched. Handle a replacement. */

    if (*rp == R_BECOMES) rp++;  /* Point to replacement */
    k = 0;

    for (; *rp != R_REND; rp++)
      {
      if ((*rp & 0x80000000u) != 0)
        {
        if (k >= wildcount) return B2PF_ERROR_NOTYPE;
        j = wildmatch[k++];

        if (*rp == R_ANY)
          {
          for (;;)
            {
            if (outused > outsize) return B2PF_ERROR_OVERFLOW;
            outbuffer[outused++] = word[j++];
            if (j >= count || (treecache[j])->type != CT_COMB) break;
            }
          }

        else
          {
          uint32_t form;

          t = treecache[j];
          if (t->type != CT_PRES) return B2PF_ERROR_NOPFORM;

          switch(*rp)
            {
            default: return B2PF_ERROR_INTERNAL3;
            case R_ISOLATED: form = F_ISOLATED; break;
            case R_FINAL: form = F_FINAL; break;
            case R_INITIAL: form = F_INITIAL; break;
            case R_MEDIAL: form = F_MEDIAL; break;
            case R_JOINNEXT: form = (i == 0)? F_INITIAL : F_MEDIAL; break;

            case R_JOINPREV:
            for (k = kket; k < count; k++)
              if ((treecache[k])->type != CT_COMB) break;
            form = (k >= count)? F_FINAL : F_MEDIAL;
            break;
            }

          if (t->pforms[form] == 0) return B2PF_ERROR_NOPFORM;

          if (outused > outsize) return B2PF_ERROR_OVERFLOW;
          outbuffer[outused++] = t->pforms[form];

          j++;
          for (;;)
            {
            if (j >= count || (treecache[j])->type != CT_COMB) break;
            if (outused > outsize) return B2PF_ERROR_OVERFLOW;
            outbuffer[outused++] = word[j++];
            }
          }
        }

      else
        {
        if (outused > outsize) return B2PF_ERROR_OVERFLOW;
        outbuffer[outused++] = *rp;
        }
      }

    i = kket - 1;   /* one before where to resume scanning */

    break;  /* Skip following rules */

    NEXTRULE: continue;
    }

  /* If no rule matched, copy this character and any following combiners. */

  if (r == NULL)
    {
    size_t j;
    if (outused > outsize) return B2PF_ERROR_OVERFLOW;
    outbuffer[outused++] = word[i];

    for (j = i + 1; j < count; j++)
      {
      if ((treecache[j])->type != CT_COMB) break;
      if (outused > outsize) return B2PF_ERROR_OVERFLOW;
      outbuffer[outused++] = word[++i];
      }
    }
  }   /* End of loop scanning the word's characters. */


*outusedptr = outused;
return B2PF_SUCCESS;
}



/*************************************************
*      Convert a string to presentation form     *
*************************************************/

/* The string is scanned for words, that is, sequences that start with a known
letter and contain only letters and combiners. Ligatures are processed while
extracting a word. Then each word is processed according to the rules. All
other characters are copied verbatim.

Arguments:
  inbuffer      input, in 32-bit characters
  insize        number of input characters
  outbuffer     where to put the output
  outsize       size of output buffer
  outusedptr    where to put the amount used
  context       the current context
  error_offset  where to return an offset after an error

Returns:        B2PF_SUCCESS or an error code
*/

int
PRIV(format_string)(uint32_t *inbuffer, size_t insize, uint32_t *outbuffer,
  size_t outsize, size_t *outusedptr, b2pf_context *context,
  size_t *error_offset)
{
int yield = B2PF_SUCCESS;
uint32_t *p = inbuffer;
uint32_t *pend = inbuffer + insize;
size_t outused = 0;
size_t save_outused;

if (inbuffer == NULL || outbuffer == NULL || outusedptr == NULL ||
    context == NULL || error_offset == NULL) return B2PF_ERROR_NULL;

if (!context->checked && !PRIV(check_context)(context))
  yield = B2PF_ERROR_CONTEXTCHECK;

/* Scan the string searching for the starts of words, copying any non-word
characters and combiners, which cannot start a word. Then read each word and
cause it to be processed. */

while (p < pend)
  {
  int rc;
  size_t wordcount = 0;
  size_t word_error_offset = 0;
  uschar previous_type;
  uint32_t previous;
  uint32_t word[WORDMAX];
  tree_node *treecache[WORDMAX];
  tree_node *t = PRIV(tree_search)(context->chartreebase, *p);

  /* Not a start of word character */

  if (t == NULL || t->type == CT_COMB)
    {
    if (outused >= outsize)
      {
      *error_offset = p - inbuffer;
      return B2PF_ERROR_OVERFLOW;
      }
    outbuffer[outused++] = *p++;
    continue;
    }

  /* We are now at the start of a word. Save the first character and its tree
  pointer. */

  previous = word[wordcount] = *p;
  previous_type = t->type;
  treecache[wordcount++] = t;
  *error_offset = p - inbuffer;  /* In case of error while formatting */

  /* Read the rest of the word. Any character in the characters tree is
  allowed. Check with the previous character for a possible ligature. */

  for (++p; p < pend; p++)
    {
    tree_node *tt = NULL;   /* No ligature */
    uint32_t *pp = p;       /* Pointer to second ligature character */
    uint32_t c = *p;        /* Second ligature character */

    t = PRIV(tree_search)(context->chartreebase, c);
    if (t == NULL) break;  /* Unknown character ends word */

    /* Ligatures can be formed by combining characters as well as by
    non-combining characters, but not between one of each. However, if a
    non-combiner is followed by a combiner, look past one or more combiners to
    see if the next non-combiner can ligature with the first character. */

    if (previous_type != CT_COMB || t->type == CT_COMB) /* Ligature possible */
      {
      uint64_t lkey = previous;
      lkey = (lkey << 32) | c;

      if (previous_type != CT_COMB && t->type == CT_COMB)
        {
        for (pp = p+1; pp < pend; pp++)
          {
          uint32_t cc = *pp;
          tt = PRIV(tree_search)(context->chartreebase, cc);
          if (tt == NULL) goto ENDLIGCHECK;  /* Not a ligature */
          if (tt->type != CT_COMB)           /* Found possible 2nd char */
            {
            lkey = (lkey & 0xffffffff00000000u) | cc;
            break;
            }
          }
        }

      /* Check for a ligature */

      tt = PRIV(tree_search)(context->ligtreebase, lkey);
      }

    ENDLIGCHECK:

    /* If we found a ligature, replace the previous character. If the ligature
    has not been defined in the characters tree, treat it as a miscellaneous
    character by pointing to a fake node. */

    if (tt != NULL)
      {
      previous = word[wordcount-1] = tt->pforms[0];
      t = PRIV(tree_search)(context->chartreebase, previous);
      if (t == NULL) t = &default_miscchar;
      treecache[wordcount-1] = t;
      previous_type = t->type;

      /* If pp > p it means we skipped some combiners between two non-combiners
      that formed a ligature. Move all the intermediate characters up by one so
      as to remove the second character from the input. */

      while (pp > p)
        {
        pp[0] = pp[-1];
        pp--;
        }
      }

    /* Not a ligature; add the current character to the word. */

    else
      {
      if (wordcount >= WORDMAX)
        {
        *error_offset = p - inbuffer;
        return B2PF_ERROR_OVERLONGWORD;
        }
      previous = word[wordcount] = c;
      previous_type = t->type;
      treecache[wordcount++] = t;
      }
    }  /* End of loop to get the next word */

  /* p is now pointing after the word. */

  save_outused = outused;  /* Start of word */
  rc = format_word(word, wordcount, treecache, outbuffer, outsize, &outused,
    context, &word_error_offset);

  if (rc != B2PF_SUCCESS)
    {
    *error_offset += word_error_offset;
    return rc;
    }

  /* If there is an "after" tree, scan the output word for possible ligatures
  to apply at this point. */

  if (context->aftertreebase != NULL)
    {
    size_t x = save_outused;
    while (x < outused - 1)
      {
      size_t y;
      uint64_t lkey;
      tree_node *tt;

      /* Only non-combiner ligatures are recognized here. */

      t = PRIV(tree_search)(context->chartreebase, outbuffer[x]);
      if (t != NULL && t->type == CT_COMB)
        {
        x++;
        continue;
        }

      for (y = x + 1; y < outused; y++)
        {
        t = PRIV(tree_search)(context->chartreebase, outbuffer[y]);
        if (t == NULL || t->type != CT_COMB) break;
        }
      if (y >= outused) break;  /* No following non-combiner; end of word */

      lkey = outbuffer[x];
      lkey = (lkey << 32) | outbuffer[y];
      tt = PRIV(tree_search)(context->aftertreebase, lkey);

      /* If no ligature found, advance to next character. If found,
      replace the current character, slide up the rest of the word, and
      rescan from the current character. */

      if (tt == NULL)
        {
        x = y;
        continue;
        }
      outbuffer[x] = tt->pforms[0];
      memmove(outbuffer + y, outbuffer + (y+1),
        (outused - (y+1))*sizeof(uint32_t));
      outused--;
      }
    }
  }

*outusedptr = outused;
return yield;   /* Either B2PF_SUCCESS or B2PF_ERROR_CONTEXTCHECK */
}

/* End of b2pf_format.c */
