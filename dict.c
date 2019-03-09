/*
 * Copyright (c) 2000 Satoru Takabayashi <satoru@namazu.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dict.h"

typedef struct _DictNode {
    int symbol;
    struct _DictNode *child;
    struct _DictNode *next;
    char *key;
    char *value;
} DictNode;

typedef DictTraverseFunc DictNodeTraverseFunc;

struct _Dict {
    DictNode *root;
};

static DictNode*	dictnode_new		(void);
static void		dictnode_destroy	(DictNode *node);
static DictNode*	dictnode_find_node	(DictNode *node, int symbol);
static void		dictnode_define		(DictNode *node, 
						 const char *key, 
						 const char *value);
static void 		dictnode_add		(DictNode *node, 
						 const char *key, 
						 const char *value);
static void 		_dictnode_add		(DictNode *node, 
						 const char *key, 
						 const char *value,
						 const char *suffix);
static DictNode*	dictnode_get		(DictNode *node,
						 const char *key);
static int 		dictnode_empty_p	(DictNode *node);
static void		dictnode_traverse	(DictNode *node,
						 DictNodeTraverseFunc
						 traverse_func,
						 void *data);

static DictNode *
dictnode_new (void)
{
    DictNode *node = malloc(sizeof(DictNode));
    if (node == NULL)
	abort();
    node->symbol = -1;
    node->child  = NULL;
    node->next   = NULL;
    node->key    = NULL;
    node->value  = NULL;
    return node;
}

static void
dictnode_destroy (DictNode *node)
{
    if (node->child)
	dictnode_destroy(node->child);
    if (node->next)
	dictnode_destroy(node->next);
    if (node->value)
	free(node->value);
    if (node->key)
	free(node->key);
    free(node);
}

static DictNode *
dictnode_find_node (DictNode *node, int symbol)
{
    if (node->symbol == symbol) {
	return node;
    } else if (node->next) {
	return dictnode_find_node(node->next, symbol);
    } else {
	return NULL;
    }
}

static void
dictnode_define (DictNode *node, const char *key, const char *value)
{
    if (node->key)
	free(node->key);
    if (node->value)
	free(node->value);

    node->key   = strdup(key);
    node->value = strdup(value);
}

static void
dictnode_add (DictNode *node, const char *key, const char *value)
{
    _dictnode_add(node, key, value, key);
}

static void
_dictnode_add (DictNode *node, 
	       const char *key, 
	       const char *value,
	       const char *suffix)
{
    assert(suffix  != NULL);
    if (dictnode_empty_p(node)) {
	node->symbol = suffix[0];
    }

    if (node->symbol == suffix[0]) {
	if (suffix[1] == '\0') {
	    dictnode_define(node, key, value);
	    return;
	} 
	if (node->child == NULL)
	    node->child = dictnode_new();
	_dictnode_add(node->child, key, value, suffix + 1);
    } else {
	if (node->next == NULL)
	    node->next = dictnode_new();
	_dictnode_add(node->next, key, value, suffix);
    }
}


static DictNode *
dictnode_get (DictNode *node, const char *key)
{
    assert(key != NULL);
    if (node == NULL)
	return NULL;
    node = dictnode_find_node(node, key[0]);
    if (node == NULL){
	return NULL;
    } else if (key[1] == '\0') {
	return node;
    } else {
	return dictnode_get(node->child, key + 1);
    }
}

static int
dictnode_empty_p (DictNode *node)
{
    return node->child == NULL && node->next == NULL && node->symbol == -1;
}

static void
dictnode_traverse (DictNode *node, 
		   DictTraverseFunc traverse_func,
		   void *data)
{
    if (node-> key && node->value)
	traverse_func(node->key, node->value, data);

    if (node->child)
	dictnode_traverse(node->child, traverse_func, data);
    if (node->next)
	dictnode_traverse(node->next,  traverse_func, data);
}

Dict *
dict_new (void)
{
    Dict *dict = malloc(sizeof(Dict));
    if (dict == NULL)
	abort();
    dict->root = dictnode_new();
    return dict;
}

void
dict_destroy (Dict *dict)
{
    dictnode_destroy(dict->root);
    free(dict);
}

void
dict_add (Dict *dict, const char *key, const char *value)
{
    dictnode_add(dict->root, key, value);
}

char *
dict_search_exact (Dict *dict, const char *key)
{
    DictNode *node = dictnode_get(dict->root, key);
    if (node && node->value) {
	return node->value;
    } else {
	return NULL;
    }
}

char *
dict_search_longest (Dict *dict, const char *key)
{
    DictNode *node = dict->root;
    char *value = NULL;

    while (*key != '\0' && node) {
	DictNode *found = dictnode_find_node(node, *key);
	if (found == NULL)
	    break;
	if (found->value)
	    value = found->value;
	node = found->child;
	key++;
    }
    return value;
}

void
dict_traverse (Dict *dict, DictTraverseFunc traverse_func, void *data)
{
    dictnode_traverse(dict->root, traverse_func, data);
}

