#ifndef SSAVER_H
#define SSAVER_H

#if defined __cplusplus
extern "C" {
#endif


extern void 	init_ssaver(void);
extern void		release_ssaver(void);
extern bool 	draw_ssaver(void);
extern bool 	process_ssaver(struct kbd_event *e);


#endif /* SSAVER_H */
