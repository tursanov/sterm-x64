/*
	sterm help viewer
	(c) gsr 2000
*/

#ifndef HELP_H
#define HELP_H

#include "kbd.h"
#include "gui/scr.h"

struct help_line{
	struct help_line *next;
	char *str;
};

extern void	init_help(char *fname);
extern void	release_help(void);
extern bool	draw_help(void);
extern bool	process_help(struct kbd_event *e);
extern char	*chop(char *s);		/* Perl like chop */
extern bool	read_hlp_lines(char *fname);
extern bool	dispose_hlp_lines(void);
extern char	*get_hlp_line(int n);
extern bool	hv_up(void);
extern bool	hv_dn(void);
extern bool	hv_pg_up(void);
extern bool	hv_pg_dn(void);
extern bool	hv_home(void);
extern bool	hv_end(void);
extern bool	draw_hlp_lines(void);
extern bool	draw_hlp_hints(void);
extern void	print_help(void);

#endif		/* HELP_H */
