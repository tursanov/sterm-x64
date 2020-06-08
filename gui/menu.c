/* Обработка меню. (c) gsr, А.Попов 2000, 2018 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/scr.h"
#include "gui/menu.h"
#include "gui/exgdi.h"
#include "paths.h"
#include "sysdefs.h"

struct menu_item *new_menu_item(char *txt, int cmd, bool enabled)
{
	struct menu_item *ret = __new(struct menu_item);
	if (ret != NULL){
		ret->prev = ret->next = NULL;
		ret->text = (txt == NULL) ? NULL : strdup(txt);
		ret->cmd = cmd;
		ret->enabled = enabled;
	}
	return ret;
}

struct menu *new_menu(bool set_active, bool centered)
{
	struct menu *ret = __new(struct menu);
	if (ret != NULL){
		if (set_active)
			menu_active = true;
		ret->head = ret->tail = NULL;
		ret->n_items = 0;
		ret->selected = -1;
		ret->max_len = 0;
		ret->centered = centered;
	}
	return ret;
}

void release_menu_item(struct menu_item *item)
{
	if (item != NULL){
		if (item->text != NULL)
			free((char *)item->text);
		free(item);
	}
}

void release_menu(struct menu *mnu, bool unset_active)
{
	if (mnu != NULL){
		struct menu_item *p = mnu->head;
		for (int i = 0; (p != NULL) && (i < mnu->n_items); i++){
			struct menu_item *pp = p->next;
			release_menu_item(p);
			p = pp;
		}
		free(mnu);
		if (unset_active)
			menu_active = false;
	}
}

static void make_menu_geometry(struct menu *mnu)
{
	mnu->width=mnu->max_len+8;
	mnu->height=mnu->n_items+4;
	mnu->top=(sg->height - mnu->height) / 2;
	mnu->left=(sg->width - mnu->width) / 2;
}

struct menu *add_menu_item(struct menu *mnu, struct menu_item *itm)
{
	if ((mnu != NULL) && (itm != NULL)){
		if (mnu->head == NULL)
			mnu->head = mnu->tail = itm;
		else
			mnu->tail->next = mnu->head->prev = itm;
		itm->prev = mnu->tail;
		itm->next = mnu->head;
		mnu->tail = itm;
		mnu->n_items++;
		if (itm->text != NULL){
			size_t len = strlen(itm->text);
			if (len > mnu->max_len)
				mnu->max_len = len;
		}
		make_menu_geometry(mnu);
/*		if (!mnu->items->enabled)
			menu_move_down(mnu);*/
		if ((mnu->selected == -1) && itm->enabled)
			mnu->selected = mnu->n_items - 1;
	}
	return mnu;
}

static struct menu_item *get_menu_item_by_cmd(struct menu *mnu, int cmd)
{
	struct menu_item *ret = NULL;
	if (mnu != NULL){
		struct menu_item *itm = mnu->head;
		for (int i = 0; i < mnu->n_items; i++, itm = itm->next){
			if (itm->cmd == cmd){
				ret = itm;
				break;
			}
		}
	}
	return ret;
}

static struct menu_item *get_menu_item(struct menu *mnu, int index)
{
	struct menu_item *ret = NULL;
	if (mnu != NULL){
		struct menu_item *itm = mnu->head;
		for (int i = 0; i < mnu->n_items; i++, itm = itm->next){
			if (i == index){
				ret = itm;
				break;
			}
		}
	}
	return ret;
}

bool enable_menu_item(struct menu *mnu, int cmd, bool enable)
{
	bool ret = false;
	struct menu_item *itm = get_menu_item_by_cmd(mnu, cmd);
	if (itm != NULL){
		itm->enabled = enable;
		ret = true;
	}
	return ret;
}

static bool menu_move_up(struct menu *mnu)
{
	bool ret = false;
	if ((mnu != NULL) && (mnu->selected != -1)){
		int n = mnu->selected;
		struct menu_item *itm = get_menu_item(mnu, n);
		for (int i = 0; (i + 1) < mnu->n_items; i++){
			itm = itm->prev;
			n += mnu->n_items - 1;
			n %= mnu->n_items;
			if (itm->enabled){
				mnu->selected = n;
				ret = true;
				break;
			}
		}
	}
	return ret;
}

static bool menu_move_down(struct menu *mnu)
{
	bool ret = false;
	if ((mnu != NULL) && (mnu->selected != -1)){
		int n = mnu->selected;
		struct menu_item *itm = get_menu_item(mnu, n);
		for (int i = 0; (i + 1) < mnu->n_items; i++){
			itm = itm->next;
			n++;
			n %= mnu->n_items;
			if (itm->enabled){
				mnu->selected = n;
				ret = true;
				break;
			}
		}
	}
	return ret;
}

static bool menu_move_home(struct menu *mnu)
{
	bool ret = false;
	if ((mnu != NULL) && (mnu->n_items > 0)){
		struct menu_item *itm = mnu->head;
		for (int i = 0; i < mnu->n_items; i++){
			if (itm->enabled){
				if (i != mnu->selected){
					mnu->selected = i;
					ret = true;
				}
				break;
			}else
				itm = itm->next;
		}
	}
	return ret;
}

static bool menu_move_end(struct menu *mnu)
{
	bool ret = false;
	if ((mnu != NULL) && (mnu->n_items > 0)){
		struct menu_item *itm = mnu->tail;
		for (int i = mnu->n_items - 1; i >= 0; i--){
			if (itm->enabled){
				if (i != mnu->selected){
					mnu->selected = i;
					ret = true;
				}
				break;
			}else
				itm = itm->prev;
		}
	}
	return ret;
}

static bool draw_menu_items(struct menu *mnu)
{
	if (mnu == NULL)
		return false;
	int w = mnu->max_len + 2;
	FontPtr pFont = CreateFont(_("fonts/terminal10x18.fnt"), false);
	int mh = pFont->max_height + 2;
	int mw = w * pFont->max_width + 20;
	GCPtr pMemGC = CreateMemGC(mw, (mnu->n_items + 1) * mh + 3);
	int x = (DISCX - GetCX(pMemGC) - 5) / 2;
	int y = (DISCY - GetCY(pMemGC)) / 2;
	GCPtr pGC = CreateGC(x, y, mw /*+ 2*/, (mnu->n_items + 1) * mh + 3);
	
	ClearGC(pMemGC, clBtnFace);
	SetFont(pMemGC, pFont);
	SetTextColor(pMemGC, clNavy);
	SetBrushColor(pMemGC, RGB(0, 0x80, 0x80));
	FillBox(pMemGC, 1, 1, mw - 2, mh - 2);
	DrawText(pMemGC, 0, 0, mw, mh, "МЕНЮ (Esc-выход)", 0);
	DrawBorder(pMemGC, 0, mh, GetCX(pMemGC), 1, 1, 
		clBtnShadow, clBtnHighlight);

//	printf("mnu: %p, mnu->selected = %d, mnu->n_items = %d\n", mnu, mnu->selected, mnu->n_items);

	struct menu_item *itm = mnu->head;
	for (int i = 0; (itm != NULL) && (i < mnu->n_items); i++, itm = itm->next){
		SetTextColor(pMemGC, itm->enabled ? clBlack : clGray);
		if ((i == mnu->selected) && itm->enabled){
			SetTextColor(pMemGC, clWhite);
			SetBrushColor(pMemGC, clNavy);
			DrawBorder(pMemGC, 0, mh + i * mh + 2, mw - 1, mh - 1, 1,
				clBtnShadow, clBtnHighlight);
			FillBox(pMemGC, 1, mh + i * mh + 3, mw - 3, mh - 3);
		}
		if (mnu->centered)
			DrawText(pMemGC, 0, mh + i * mh + 2, mw, mh, itm->text, DT_CENTER);
		else
			TextOut(pMemGC, 10, mh + i * mh + 2, itm->text);
	}
	DrawBorder(pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC), 1, 
		clBtnHighlight, clBtnShadow);
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC));
	DeleteGC(pMemGC);
	DeleteGC(pGC);
	DeleteFont(pFont);
	return true;
}

bool draw_menu(struct menu *mnu)
{
	if (mnu != NULL){
		/*char *s="Еsc - выход";
		int col=mnu->left + (mnu->width-strlen(s)) / 2;
		rectangle(mnu->top,mnu->left,mnu->width,mnu->height,CYAN);
		frame(mnu->top+1,mnu->left+2,mnu->width-4,mnu->height-2,fr_semi,BLACK,CYAN);
		title(s,mnu->top+1,col,false,BLACK,CYAN);*/
		return draw_menu_items(mnu);
	}else
		return false;
}

int process_menu(struct menu *mnu,struct kbd_event *e)
{
	if ((mnu == NULL) || (e == NULL))
		return cmd_none;
	if (e->pressed){
		switch (e->key){
			case KEY_DOWN:
			case KEY_NUMDOWN:
			case KEY_NUMPLUS:
				if (menu_move_down(mnu))
					draw_menu_items(mnu);
				return cmd_none;
			case KEY_UP:
			case KEY_NUMUP:
			case KEY_NUMMINUS:
				if (menu_move_up(mnu))
					draw_menu_items(mnu);
				return cmd_none;
			case KEY_HOME:
			case KEY_NUMHOME:
			case KEY_PGUP:
			case KEY_NUMPGUP:
				if (menu_move_home(mnu))
					draw_menu_items(mnu);
				return cmd_none;
			case KEY_END:
			case KEY_NUMEND:
			case KEY_PGDN:
			case KEY_NUMPGDN:
				if (menu_move_end(mnu))
					draw_menu_items(mnu);
				return cmd_none;
			case KEY_ESCAPE:
			case KEY_F10:
				mnu->selected = -1;	/* fall through */
			case KEY_SPACE:
			case KEY_ENTER:
			case KEY_NUMENTER:
				return cmd_exit;
			default:
				return cmd_none;
		}
	}else
		return cmd_none;
}

int get_menu_command(struct menu *mnu)
{
	if ((mnu == NULL) || (mnu->selected < 0))
		return cmd_none;
	else{
		struct menu_item *p = get_menu_item(mnu, mnu->selected);
		return (p && p->enabled) ? p->cmd : cmd_none;
	}
}

int execute_menu(struct menu *mnu)
{
	if (mnu != NULL){
		struct kbd_event e;
		int cmd = cmd_none;
		do
			kbd_wait_event(&e);
		while ((cmd = process_menu(mnu, &e)) == cmd_none);
		return cmd;
	}else
		return cmd_none;
}
