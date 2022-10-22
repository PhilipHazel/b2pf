/*************************************************
*       Base Unicode to Presentation Forms       *
*************************************************/

/* This file contains the binary balanced tree management functions for the
B2PF library.

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


#define tree_lbal      1         /* left subtree is longer */
#define tree_rbal      2         /* right subtree is longer */
#define tree_bmask     3         /* mask for flipping bits */


/*************************************************
*         Insert a new node into a tree          *
*************************************************/

/* Each tree node may represent a range of characters, from first to last,
inclusive.

Arguments:
  treebase      pointer to the root of the tree
  node          the node to insert, with data fields filled in

Returns:        TRUE if the node is successfully inserted,
                FALSE otherwise (duplicate or overlapping node found)
*/

BOOL
PRIV(tree_insert)(tree_node **treebase, tree_node *node)
{
tree_node *p = *treebase;
tree_node **q, *r, *s, **t;
int a;

/* Initialize the tree fields of the node */

node->left = NULL;
node->right = NULL;
node->balance = 0;

/* Deal with an empty tree */

if (p == NULL)
  {
  *treebase = node;
  return TRUE;
  }

/* The tree is not empty. While finding the insertion point, q points to the
pointer to p, and t points to the pointer to the potential re-balancing point.
*/

q = treebase;
t = q;

/* Loop to search tree for place to insert new node */

for (;;)
  {
  if ((node->first >= p->first && node->first <= p->last) ||
      (node->last >= p->first && node->last <= p->last))
    return FALSE;   /* Duplication or overlap */

  /* Deal with climbing down the tree, exiting from the loop when we reach a
  leaf. */

  q = (node->first > p->last)? &(p->right) : &(p->left);
  p = *q;
  if (p == NULL) break;

  /* Save the address of the pointer to the last node en route which has a
  non-zero balance factor. */

  if (p->balance != 0) t = q;
  }

/* When the above loop completes, q points to the pointer to NULL; that is the
place at which the new node must be inserted. */

*q = node;

/* Set up s as the potential re-balancing point, and r as the next node after
it along the route*/

s = *t;
r = (node->first > s->last)? s->right : s->left;

/* Adjust balance factors along the route from s to node. */

p = r;
while (p != node)
  if (node->last < p->first)
    {
    p->balance = tree_lbal;
    p = p->left;
    }
  else
    {
    p->balance = tree_rbal;
    p = p->right;
    }

/* Now the World-Famous Balancing Act */

a = (node->last < s->first)? tree_lbal : tree_rbal;

if (s->balance == 0)       /* The tree has grown higher */
  s->balance = a;
else if (s->balance != a)  /* The tree has become more balanced */
  s->balance = 0;

else                       /* The tree has got out of balance */
  {
  if (r->balance == a)     /* Perform a single rotation */
    {
    p = r;
    if (a == tree_rbal)
      {
      s->right = r->left;
      r->left = s;
      }
    else
      {
      s->left = r->right;
      r->right = s;
      }
    s->balance = 0;
    r->balance = 0;
    }

  else                          /* Perform a double rotation */
    {
    if (a == tree_rbal)
      {
      p = r->left;
      r->left = p->right;
      p->right = r;
      s->right = p->left;
      p->left = s;
      }
    else
      {
      p = r->right;
      r->right = p->left;
      p->left = r;
      s->left = p->right;
      p->right = s;
      }

    s->balance = (p->balance == a)? (a^tree_bmask) : 0;
    r->balance = (p->balance == (a^tree_bmask))? a : 0;
    p->balance = 0;
    }

  /* Finishing touch */

  *t = p;
  }

return TRUE;     /* Successful insertion */
}


/*************************************************
*          Search tree for node by               *
*************************************************/

/*
Arguments:
  p          the root node of the tree
  u          code point being sought

Returns:     pointer to the found node, or NULL
*/

tree_node *
PRIV(tree_search)(tree_node *p, uint64_t u)
{
while (p != NULL)
  {
  if (u >= p->first && u <= p->last) return p;
  p = (u < p->first)? p->left : p->right;
  }
return NULL;
}

/* End of b2pf_tree.c */
