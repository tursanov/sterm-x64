/*
	sterminal user keys definitions
	(c) gsr 2000
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sterm.h"
#include "keys.h"

/*uint8_t	*key_presets="P44BXTIMS^45NGKE001aJCV]001bVDFGHO11KJH\\12GFD\\13RTQ]001cOIU]^46G^^";*/
static uint8_t	*key_presets = (uint8_t *)"P55K^";

struct key_set key_sets[MAX_WINDOWS];

uint8_t	keys[KEY_BUF_LEN];
char	key_delim[]={'^','^',']','\\','['};
struct key_set *kset=NULL;

void init_keys(void)
{
	memset(keys, 0, sizeof(keys));
	memcpy(keys, key_presets, strlen((char *)key_presets));
	init_key_sets(MAX_WINDOWS);
	switch_key_set(0);
	rollback_keys(true);
}

void release_keys(void)
{
	int i;
	for (i=0; i < MAX_WINDOWS; i++)
		__delete(key_sets[i].key_group_map);
}

void init_key_sets(int n)
{
	int i;
	for (i=0; i < n; i++){
		struct key_set *p=&key_sets[i];
		p->key_arg_len=0;
		p->key_level=0;
		p->key_group_ptr=NULL;
		p->key_group_map=NULL;
		p->n_key=0;
		p->key_group_len=0;
	}
}

bool switch_key_set(int n)
{
	if ((n < 0) || (n >= ASIZE(key_sets)))
		return false;
	else{
		kset=&key_sets[n];
		return true;
	}
}

int make_key_map(struct key_set *p)
{
	int i,n=0;
	if ((p != NULL)  && (p->key_group_ptr != NULL)){
		__delete(p->key_group_map);
		for (; is_key(p->key_group_ptr[n]); n++);
		if (n > 0){
			p->key_group_map = __calloc(n, bool);
			for (i=0; i < n; p->key_group_map[i++]=false);
		}
	}
	return n;
}

bool is_key(uint8_t c)
{
	return (c >= 'A') && (c <= 'Z');
}

bool is_key_delim(int c)
{
	int i;
	for (i=0; i < sizeof(key_delim); i++)
		if (c == key_delim[i])
			return true;
	return false;
}

bool is_key_id(int c)
{
	return !is_key(c) && !is_key_delim(c);
}

void rollback_key_set(struct key_set *p)
{
	if (p != NULL){
		p->key_group_ptr = keys;
		p->n_key=0;
		for (p->key_arg_len=0; is_key_id(p->key_group_ptr[p->key_arg_len+1]); p->key_arg_len++);
		p->key_level=0;
		p->key_group_len=make_key_map(p);
	}
}

void rollback_keys(bool all)
{
	if (all){
		int i;
		for (i=0; i < MAX_WINDOWS; i++)
			rollback_key_set(&key_sets[i]);
	}else
		rollback_key_set(kset);
}

int get_key_arg_len(void)
{
	kset->key_arg_len=0;
	for (; is_key_id(kset->key_group_ptr[kset->n_key+kset->key_arg_len+1]); kset->key_arg_len++);
	return kset->key_arg_len;
}

bool walk_keys(bool fwd)
{
	int i;
	if (keys == NULL)
		return false;
	for (i=0; i < kset->key_group_len; i++){
		if (fwd){
			if (kset->n_key < (kset->key_group_len-1))
				kset->n_key++;
		}else if (kset->n_key > 0)
			kset->n_key--;
		if (!kset->key_group_map[kset->n_key]){
			get_key_arg_len();
			return true;
		}
	}
	return false;
}

char get_key(void)
{
	if (keys == NULL)
		return 0;
	else
		return kset->key_group_ptr[kset->n_key];
}

bool mark_key(void)
{
	if (keys == NULL)
		return false;
	else
		return kset->key_group_map[kset->n_key]=true;
}

bool has_key_group(void)
{
	return (bool)(is_key_id(kset->key_group_ptr[kset->n_key+1]));
}

bool next_key_group(char *id)
{
	char *p;
	if (!has_key_group() || (kset->key_level > 3))
		return false;
	p = (char *)&kset->key_group_ptr[kset->n_key + 1];
	while (*p != key_delim[kset->key_level]){
		if (!strncmp(p,id,strlen(id))){
			if (is_key(*(p+=strlen(id)))){
				kset->key_group_ptr = (uint8_t *)p;
				kset->key_level++;
				kset->n_key=0;
				get_key_arg_len();
				kset->key_group_len=make_key_map(kset);
				return true;
			}
		}
		if ((p=strchr(p,key_delim[kset->key_level+1])) != NULL)
			p++;
		else
			break;
	}
	return false;
}



