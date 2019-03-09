#ifndef __DICT_H__
#define __DICT_H__

typedef struct _Dict Dict;
typedef void	(*DictTraverseFunc)	(const char *key,
					 const char *value,
					 void *data);

Dict*		dict_new		(void);
void		dict_destroy		(Dict *dict);
void		dict_add		(Dict *dict, 
					 const char *key, 
					 const char *value);
char*		dict_search_exact	(Dict *dict, 
					 const char *key);
char*		dict_search_longest	(Dict *dict, 
					 const char *key);
void		dict_traverse		(Dict *dict, 
					 DictTraverseFunc traverse_func,
					 void *data);

#endif
