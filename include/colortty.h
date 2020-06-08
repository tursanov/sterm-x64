/*
 *	Раскраска вывода на консоль с помощью Escape-последовательностей ANSI.
 *	(c) gsr 2001, 2004
 */

#ifndef COLORTTY_H
#define COLORTTY_H

/* Атрибуты текста */
#define ANSI_PREFIX	"\033["
#define ANSI_SUFFIX	"m"
#define ANSI_DELIM	";"
#define ANSI_BOLD	"01"

/* Цвет текста */
/* Нормальные цвета */
#define TXT_black	"30"
#define TXT_red		"31"
#define TXT_green	"32"
#define TXT_yellow	"33"
#define TXT_blue	"34"
#define TXT_magenta	"35"
#define TXT_cyan	"36"
#define TXT_white	"37"
/* Насыщенные цвета */
#define TXT_BLACK	ANSI_BOLD ANSI_DELIM TXT_black
#define TXT_RED		ANSI_BOLD ANSI_DELIM TXT_red
#define TXT_GREEN	ANSI_BOLD ANSI_DELIM TXT_green
#define TXT_YELLOW	ANSI_BOLD ANSI_DELIM TXT_yellow
#define TXT_BLUE	ANSI_BOLD ANSI_DELIM TXT_blue
#define TXT_MAGENTA	ANSI_BOLD ANSI_DELIM TXT_magenta
#define TXT_CYAN	ANSI_BOLD ANSI_DELIM TXT_cyan
#define TXT_WHITE	ANSI_BOLD ANSI_DELIM TXT_white

/* Цвет фона */
/* Нормальные цвета */
#define BG_black	"40"
#define BG_red		"41"
#define BG_green	"42"
#define BG_yellow	"43"
#define BG_blue		"44"
#define BG_magenta	"45"
#define BG_cyan		"46"
#define BG_white	"47"
/* Насыщенные цвета (или мигающие символы) */
#define BG_BLACK	ANSI_BOLD ANSI_DELIM BG_black
#define BG_RED		ANSI_BOLD ANSI_DELIM BG_red
#define BG_GREEN	ANSI_BOLD ANSI_DELIM BG_green
#define BG_YELLOW	ANSI_BOLD ANSI_DELIM BG_yellow
#define BG_BLUE		ANSI_BOLD ANSI_DELIM BG_blue
#define BG_MAGENTA	ANSI_BOLD ANSI_DELIM BG_magenta
#define BG_CYAN		ANSI_BOLD ANSI_DELIM BG_cyan
#define BG_WHITE	ANSI_BOLD ANSI_DELIM BG_white

/* Макросы для раскраски */
#define _ansi(s)	ANSI_PREFIX s ANSI_SUFFIX

/* Очистка экрана */
#define CLR_SCR		"\033[H\033[J"
/* Установка атрибутов текста по умолчанию */
#define ANSI_DEFAULT	_ansi("00")
/* Установка цвета символов */
#define tBlc		_ansi(TXT_black)
#define tRed		_ansi(TXT_red)
#define tGrn		_ansi(TXT_green)
#define tYlw		_ansi(TXT_yellow)
#define tBlu		_ansi(TXT_blue)
#define tMgn		_ansi(TXT_magenta)
#define tCya		_ansi(TXT_cyan)
#define tWht		_ansi(TXT_white)

#define tBLC		_ansi(TXT_BLACK)
#define tRED		_ansi(TXT_RED)
#define tGRN		_ansi(TXT_GREEN)
#define tYLW		_ansi(TXT_YELLOW)
#define tBLU		_ansi(TXT_BLUE)
#define tMGN		_ansi(TXT_MAGENTA)
#define tCYA		_ansi(TXT_CYAN)
#define tWHT		_ansi(TXT_WHITE)

/* Установка цвета фона */
#define bBlc		_ansi(BG_black)
#define bRed		_ansi(BG_red)
#define bGrn		_ansi(BG_green)
#define bYlw		_ansi(BG_yellow)
#define bBlu		_ansi(BG_blue)
#define bMgn		_ansi(BG_magenta)
#define bCya		_ansi(BG_cyan)
#define bWht		_ansi(BG_white)

/* Очистка строки после курсора */
#define CLR_EOL		ANSI_PREFIX "0K"
#define CLR_SOL		ANSI_PREFIX "1K"

#endif		/* COLORTTY_H */
