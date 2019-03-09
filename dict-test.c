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
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "dict.h"

typedef char*		(*SearchFunc)		(Dict *dict, 
						 const char *key);
typedef void		(*TaskFunc)		(Dict *dict,
						 SearchFunc);

typedef void		(*BenchmarkStartFunc)	(const char *message);
typedef void		(*BenchmarkFinishFunc)	(void);

clock_t		benchmark_start_time;
const char*	benchmark_message;

static void
benchmark_start_dummy (const char *message)
{
}

static void
benchmark_finish_dummy (void)
{
}

static void
benchmark_start (const char *message)
{
    benchmark_start_time = clock();
    benchmark_message = message;
}

static void
benchmark_finish (void)
{
    double elapsed;
    elapsed = ((double)clock() - benchmark_start_time) / CLOCKS_PER_SEC;
    fprintf(stderr, "== %-15s: %.2f sec.\n", benchmark_message, elapsed);
}

static void
read_dict (Dict *dict, const char *file_name)
{
    FILE *fp;
    char buf[BUFSIZ];

    fp = fopen(file_name, "r");
    if (fp == NULL) 
	abort();

    while (fgets(buf, BUFSIZ, fp)) {
	char *tabpos;
	buf[strlen(buf)-1] = '\0';
	tabpos = strchr(buf, '\t');
	if (tabpos) {
	    char *key = buf, *value = tabpos + 1;
	    *tabpos = '\0';
	    if (strlen(key) > 0)
		dict_add(dict, key, value);
	}
    }
    fclose(fp);
}

void
search_test (Dict *dict, SearchFunc search_func)
{
    char key[BUFSIZ];

    while (fgets(key, BUFSIZ, stdin)) {
	char *value;
	key[strlen(key)-1] = '\0';

	value = search_func(dict, key);
	if (value)
	    printf("%s\t%s\n", key, value);
    }	
}

static void
print_entry (const char *key, const char *value, void *data)
{
    printf("%s\t%s\n", key, value);
}

static void
do_nothing (const char *key, const char *value, void *data)
{
}

static void
check_entry (const char *key, const char *value, void *data)
{
    Dict *dict = (Dict *)data;
    assert(strcmp(dict_search_exact(dict, key), value) == 0);
}

void
traverse_print_test (Dict *dict, SearchFunc search_func)
{
    dict_traverse(dict, print_entry, dict);
}

void
traverse_test (Dict *dict, SearchFunc search_func)
{
    dict_traverse(dict, do_nothing, dict);
}

void
traverse_check_test (Dict *dict, SearchFunc search_func)
{
    dict_traverse(dict, check_entry, dict);
}

int 
main (int argc, char **argv)
{
    SearchFunc	search_func    = dict_search_exact;
    TaskFunc	task_func      = search_test;
    BenchmarkStartFunc  start  = benchmark_start_dummy;
    BenchmarkFinishFunc finish = benchmark_finish_dummy;

    char *task_name = "search(exact)";
    int ch;

    while ((ch = getopt(argc, argv, "elptcb")) != EOF) {
	switch(ch) {
	case 'e':
	    search_func = dict_search_exact;
	    break;
	case 'l':
	    search_func = dict_search_longest;
	    task_name = "search(longest)";
	    break;
	case 't':
	    task_func = traverse_test;
	    task_name = "traverse";
	    break;
	case 'p':
	    task_func = traverse_print_test;
	    task_name = "traverse(print)";
	    break;
	case 'c':
	    task_func = traverse_check_test;
	    task_name = "traverse(check)";
	    break;
	case 'b':
	    start  = benchmark_start;
	    finish = benchmark_finish;
	}
    }
    argc -= optind;
    argv += optind;
    if (argc != 1) exit(1);

    {
	char *file_name = argv[0];
	Dict *dict = dict_new();

	start("read_dict"); {
	    read_dict(dict, file_name);
	} finish();

	start(task_name); {
	    task_func(dict, search_func);
	} finish();

	dict_destroy(dict);
    }
    return 0;
}
