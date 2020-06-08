#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"


struct key_metric_ex{
	uint8_t key;
	int rusu;
	int latu;
	int rusl;
	int latl;
};

int kbd_lang_ex = lng_rus;
struct key_metric_ex kbd_keys_ex[N_CHAR_KEYS] = {
	{KEY_ESCAPE		,'\x1b'	,'\x1b', '\x1b'	,'\x1b'},
	{KEY_1			,'!'	,'!', '1'	,'1'},
	{KEY_2			,'"'	,'@', '2'	,'2'},
	{KEY_3			,'#'	,'#', '3'	,'3'},
	{KEY_4			,';'	,'�', '4'	,'4'},
	{KEY_5			,'%'	,'%', '5'	,'5'},
	{KEY_6			,':'	,'^', '6'	,'6'},
	{KEY_7			,'?'	,'&', '7'	,'7'},
	{KEY_8			,'*'	,'*', '8'	,'8'},
	{KEY_9			,'('	,'(', '9'	,'9'},
	{KEY_0			,')'	,')', '0'	,'0'},
	{KEY_MINUS		,'-'	,'_', '-'	,'-'},
	{KEY_PLUS		,'='	,'+', '='	,'='},
	{KEY_Q			,'�'	,'Q', '�'	,'q'},
	{KEY_W			,'�'	,'W', '�'	,'w'},
	{KEY_E			,'�'	,'E', '�'	,'e'},
	{KEY_R			,'�'	,'R', '�'	,'r'},
	{KEY_T			,'�'	,'T', '�'	,'t'},
	{KEY_Y			,'�'	,'Y', '�'	,'y'},
	{KEY_U			,'�'	,'U', '�'	,'u'},
	{KEY_I			,'�'	,'I', '�'	,'i'},
	{KEY_O			,'�'	,'O', '�'	,'o'},
	{KEY_P			,'�'	,'P', '�'	,'p'},
	{KEY_LFBRACE	,'�'	,'[', '�'	,'['},
	{KEY_RFBRACE	,'�'	,']', '�'	,']'},
	{KEY_A			,'�'	,'A', '�'	,'a'},
	{KEY_S			,'�'	,'S', '�'	,'s'},
	{KEY_D			,'�'	,'D', '�'	,'d'},
	{KEY_F			,'�'	,'F', '�'	,'f'},
	{KEY_G			,'�'	,'G', '�'	,'g'},
	{KEY_H			,'�'	,'H', '�'	,'h'},
	{KEY_J			,'�'	,'J', '�'	,'j'},
	{KEY_K			,'�'	,'K', '�'	,'k'},
	{KEY_L			,'�'	,'L', '�'	,'l'},
	{KEY_COLON		,'�'	,':', '�'	,';'},
	{KEY_QUOTE		,'�'	,'"', '�'	,'\''},
	{KEY_TILDE		,';'	,'\'', '/'	,'\''},
	{KEY_BKSLASH	,','	,'\\', '.'	,'\\'},
	{KEY_Z			,'�'	,'Z', '�'	,'z'},
	{KEY_X			,'�'	,'X', '�'	,'x'},
	{KEY_C			,'�'	,'C', '�'	,'c'},
	{KEY_V			,'�'	,'V', '�'	,'v'},
	{KEY_B			,'�'	,'B', '�'	,'b'},
	{KEY_N			,'�'	,'N', '�'	,'n'},
	{KEY_M			,'�'	,'M', '�'	,'m'},
	{KEY_COMMA		,'�'	,'<', '�'	,','},
	{KEY_DOT		,'�'	,'>', '�'	,'.'},
	{KEY_SLASH		,','	,'?', '.'	,'/'},
	{KEY_NUMMUL		,'*'	,'*', '*'	,'*'},
	{KEY_SPACE		,' '	,' ', ' '	,' '},
	{KEY_NUMHOME	,'7'	,'7', '7'	,'7'},
	{KEY_NUMUP		,'8'	,'8', '8'	,'8'},
	{KEY_NUMPGUP	,'9'	,'9', '9'	,'9'},
	{KEY_NUMMINUS	,'-'	,'-', '-'	,'-'},
	{KEY_NUMLEFT	,'4'	,'4', '4'	,'4'},
	{KEY_NUMDOT		,'5'	,'5', '5'	,'5'},
	{KEY_NUMRIGHT	,'6'	,'6', '6'	,'6'},
	{KEY_NUMPLUS	,'+'	,'+', '+'	,'+'},
	{KEY_NUMEND		,'1'	,'1', '1'	,'1'},
	{KEY_NUMDOWN	,'2'	,'2', '2'	,'2'},
	{KEY_NUMPGDN	,'3'	,'3', '3'	,'3'},
	{KEY_NUMINS		,'0'	,'0', '0'	,'0'},
	{KEY_NUMDEL		,'.'	,'.', '.'	,'.'},
	{KEY_NUMSLASH	,'/'	,'/', '/'	,'/'},
};

int kbd_get_char_ex(int key)
{
	int i;
	for (i=0; i < N_CHAR_KEYS; i++)
		if (kbd_keys_ex[i].key == key) {
			if (kbd_shift_state & SHIFT_SHIFT)
				return (kbd_lang_ex == lng_rus) ? kbd_keys_ex[i].rusu : kbd_keys_ex[i].latu;
			else
				return (kbd_lang_ex == lng_rus) ? kbd_keys_ex[i].rusl : kbd_keys_ex[i].latl;
		}
	return 0;
}
