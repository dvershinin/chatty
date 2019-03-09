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
#include <glib.h>
#include "dict.h"

struct _Dict {
    GList *list;
};

typedef struct  {
    char *key;
    char *value;
} Pair;

static Pair *
pair_new (const char *key, const char *value)
{
    Pair *pair = malloc(sizeof(pair));
    if (pair == NULL)
	abort();
    pair->key   = strdup(key);
    pair->value = strdup(value);
    return pair;
}

Dict *
dict_new (void)
{
    Dict *dict = malloc(sizeof(Dict));
    if (dict == NULL)
	abort();
    dict->list = NULL;
    return dict;
}

static void 
pair_destroy (Pair *pair)
{
    free(pair->key);
    free(pair->value);
}

void
dict_destroy (Dict *dict)
{
    GList *list = dict->list;
    while (list) {
	Pair *pair = (Pair *)(list->data);
	pair_destroy(pair);
	list = list->next;
    }
    g_list_free(dict->list);
    free(dict);
}

void
dict_add (Dict *dict, const char *key, const char *value)
{
    Pair *pair = pair_new(key, value);
    dict->list = g_list_prepend(dict->list, pair);
}

char *
dict_search_exact (Dict *dict, const char *key)
{
    GList *list = dict->list;
    while (list) {
	Pair *pair = (Pair *)(list->data);
	if (strcmp(key, pair->key) == 0) {
	    return pair->value;
	}
	list = list->next;
    }
    return NULL;
}

char *
dict_search_longest (Dict *dict, const char *key)
{
    char *value = NULL;
    int maxlen = 0;
    GList *list = dict->list;
    while (list) {
	Pair *pair = (Pair *)(list->data);
	int len = strlen(pair->key);
	if (len > maxlen && strncmp(key, pair->key, len) == 0) {
	    value = pair->value;
	}
	list = list->next;
    }
    return value;
}

void
dict_traverse (Dict *dict, DictTraverseFunc traverse_func, void *data)
{
    GList *list = dict->list;
    while (list) {
	Pair *pair = (Pair *)(list->data);
	traverse_func(pair->key, pair->value, data);
	list = list->next;
    }
}

