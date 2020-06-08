/*  ΅®β  α ‡“ ª«ξη¥©. (c) gsr 2000 */

#if !defined KEYS_H
#define KEYS_H

#include "sysdefs.h"
#include "sterm.h"

struct key_set{
	int key_arg_len;
	int key_level;
	uint8_t *key_group_ptr;
	bool *key_group_map;
	int n_key;
	int key_group_len;
};

extern uint8_t		keys[KEY_BUF_LEN];
extern struct		key_set *kset;

extern void		init_keys(void);
extern void		release_keys(void);
extern void		init_key_sets(int n);
extern bool		switch_key_set(int n);
extern bool		set_keys(char *_keys,int l);
extern void		rollback_key_set(struct key_set *p);
extern void		rollback_keys(bool all);
extern int		get_key_arg_len(void);
extern bool		walk_keys(bool fwd);
extern char		get_key(void);
extern bool		mark_key(void);
extern bool		has_key_group(void);
extern bool		next_key_group(char *id);
extern int		make_key_map(struct key_set *p);
extern bool		is_key(uint8_t c);
extern bool		is_key_delim(int c);
extern bool		is_key_id(int c);

#endif		/* KEYS_H */
