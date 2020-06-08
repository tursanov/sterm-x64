#ifndef SSAVER_H
#define SSAVER_H


extern void 	init_ssaver(void);
extern void		release_ssaver(void);
extern bool 	draw_ssaver(void);
extern bool 	process_ssaver(struct kbd_event *e);


#endif /* SSAVER_H */
