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
	{KEY_4			,';'	,'˝', '4'	,'4'},
	{KEY_5			,'%'	,'%', '5'	,'5'},
	{KEY_6			,':'	,'^', '6'	,'6'},
	{KEY_7			,'?'	,'&', '7'	,'7'},
	{KEY_8			,'*'	,'*', '8'	,'8'},
	{KEY_9			,'('	,'(', '9'	,'9'},
	{KEY_0			,')'	,')', '0'	,'0'},
	{KEY_MINUS		,'-'	,'_', '-'	,'-'},
	{KEY_PLUS		,'='	,'+', '='	,'='},
	{KEY_Q			,'â'	,'Q', '©'	,'q'},
	{KEY_W			,'ñ'	,'W', 'Ê'	,'w'},
	{KEY_E			,'ì'	,'E', '„'	,'e'},
	{KEY_R			,'ä'	,'R', '™'	,'r'},
	{KEY_T			,'Ö'	,'T', '•'	,'t'},
	{KEY_Y			,'ç'	,'Y', '≠'	,'y'},
	{KEY_U			,'É'	,'U', '£'	,'u'},
	{KEY_I			,'ò'	,'I', 'Ë'	,'i'},
	{KEY_O			,'ô'	,'O', 'È'	,'o'},
	{KEY_P			,'á'	,'P', 'ß'	,'p'},
	{KEY_LFBRACE	,'ï'	,'[', 'Â'	,'['},
	{KEY_RFBRACE	,'ö'	,']', 'Í'	,']'},
	{KEY_A			,'î'	,'A', '‰'	,'a'},
	{KEY_S			,'õ'	,'S', 'Î'	,'s'},
	{KEY_D			,'Ç'	,'D', '¢'	,'d'},
	{KEY_F			,'Ä'	,'F', '†'	,'f'},
	{KEY_G			,'è'	,'G', 'Ø'	,'g'},
	{KEY_H			,'ê'	,'H', '‡'	,'h'},
	{KEY_J			,'é'	,'J', 'Æ'	,'j'},
	{KEY_K			,'ã'	,'K', '´'	,'k'},
	{KEY_L			,'Ñ'	,'L', '§'	,'l'},
	{KEY_COLON		,'Ü'	,':', '¶'	,';'},
	{KEY_QUOTE		,'ù'	,'"', 'Ì'	,'\''},
	{KEY_TILDE		,';'	,'\'', '/'	,'\''},
	{KEY_BKSLASH	,','	,'\\', '.'	,'\\'},
	{KEY_Z			,'ü'	,'Z', 'Ô'	,'z'},
	{KEY_X			,'ó'	,'X', 'Á'	,'x'},
	{KEY_C			,'ë'	,'C', '·'	,'c'},
	{KEY_V			,'å'	,'V', '¨'	,'v'},
	{KEY_B			,'à'	,'B', '®'	,'b'},
	{KEY_N			,'í'	,'N', '‚'	,'n'},
	{KEY_M			,'ú'	,'M', 'Ï'	,'m'},
	{KEY_COMMA		,'Å'	,'<', '°'	,','},
	{KEY_DOT		,'û'	,'>', 'Ó'	,'.'},
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
