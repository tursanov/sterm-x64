/*
	sterm channel viewer
	(c) gsr 2000
*/

#ifndef CHNLVIEW_H
#define CHNLVIEW_H

#define SCR_SIZE	1600
#define LINE_SIZE	80

extern void	init_channel_view(void);
extern void	release_channel_view(void);
extern bool	draw_channel(void);
extern void	cv_switch_mode(void);
extern int	process_channel(struct kbd_event *e);
extern int	cv_next_byte(void);
extern bool	channel_sshot(void);
extern bool	release_sshot(void);
extern bool	cv_up(void);
extern bool	cv_dn(void);
extern bool	cv_pg_up(void);
extern bool	cv_pg_dn(void);
extern bool	cv_home(void);
extern bool	cv_end(void);
extern int	cv_make_attr(byte b);
extern bool	cv_draw_tail(void);
extern bool	cv_draw_sshot(void);

#endif		/* CHNLVIEW_H */
