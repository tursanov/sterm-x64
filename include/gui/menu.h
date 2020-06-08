/* Обработка меню. (c) gsr, А.Попов 2000, 2018 */

#if !defined GUI_MENU_H
#define GUI_MENU_H

#include "sterm.h"
#include "gui/scr.h"

struct menu_item{
	struct menu_item *prev;
	struct menu_item *next;
	const char *text;
	int cmd;
	bool enabled;
};

struct menu{
	int top;
	int left;
	int width;
	int height;
	struct menu_item *head;
	struct menu_item *tail;
	size_t n_items;
	int selected;
	size_t max_len;
	bool centered;
};

extern struct menu_item	*new_menu_item(char *txt, int cmd, bool enabled);
extern struct menu	*new_menu(bool set_active, bool centered);
extern void		release_menu_item(struct menu_item *item);
extern void		release_menu(struct menu *m, bool unset_active);
extern struct menu	*add_menu_item(struct menu *mnu, struct menu_item *itm);
extern bool		enable_menu_item(struct menu *mnu, int cmd, bool enable);
extern bool		draw_menu(struct menu *mnu);
extern int		process_menu(struct menu *mnu, struct kbd_event *e);
extern int		get_menu_command(struct menu *mnu);
extern int		execute_menu(struct menu *mnu);

#endif		/* GUI_MENU_H */
