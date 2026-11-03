#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>

#include "kkt/fd/ad.h"
#include "express.h"
#include "kkt/xml.h"


/* ---------- Структура запроса ---------- */
struct request {
	char **xml;               /* массив XML-строк, завершающийся NULL */
	struct bank_data *pos;    /* массив банковских абзацев */
	size_t pos_count;         /* количество абзацев */
};

/* ---------- Вспомогательные функции ---------- */

static uint64_t read_uint64(const uint8_t *txt, size_t len) {
	uint64_t val = 0;
	for (size_t i = 0; i < len; i++) {
		if (isdigit(txt[i])) {
			val = val * 10 + (txt[i] - '0');
		} else {
			break;
		}
	}
	return val;
}

/* Разбор одного банковского абзаца. Возвращает 0 при успехе, -1 при ошибке. */
static int parse_bank_paragraph(const char *txt, struct bank_data *bd) {
	memset(bd, 0, sizeof(*bd));
	int idx = 0;

	/* Определяем длину req_id (до первой ';') */
	const char *semi = strchr(txt, ';');
	if (!semi) return -1;
	size_t id_len = semi - txt;
	bd->req_id = read_uint64((const uint8_t *)txt, id_len);
	idx = id_len + 1;

	/* term_id ? 6 символов */
	if (strlen(txt + idx) < BANK_TERM_ID_LEN) return -1;
	memcpy(bd->term_id, txt + idx, BANK_TERM_ID_LEN);
	bd->term_id[BANK_TERM_ID_LEN] = 0;
	idx += BANK_TERM_ID_LEN + 1;

	/* op */
	bd->op = txt[idx];
	idx += 2;

	/* ticket */
	bd->ticket = (txt[idx] == 'p');
	idx += 2;

	/* repayment */
	if (txt[idx] == '0') {
		bd->repayment = '0';
		idx += 2;
	} else if (txt[idx] == '1') {
		bd->repayment = '1';
		idx += 2;
		memcpy(bd->prev_blank_nr, txt + idx, BANK_BLANK_NR_LEN);
		bd->prev_blank_nr[BANK_BLANK_NR_LEN] = 0;
		idx += BANK_BLANK_NR_LEN + 2;
	} else {
		bd->repayment = 0;
		idx++;
	}

	/* Документы */
	for (int i = 0; i < BANK_MAX_DOCS; i++) {
		if (!isdigit((unsigned char)txt[idx]))
			break;
		struct bank_doc_info *di = &bd->doc_info[i];
		memcpy(di->blank_nr, txt + idx, BANK_BLANK_NR_LEN);
		di->blank_nr[BANK_BLANK_NR_LEN] = 0;
		idx += BANK_BLANK_NR_LEN + 1;
		uint64_t q = 0, r = 0;
		bool quot = true;
		for (int j = 0; j < BANK_AMOUNT_MAX_LEN; j++) {
			uint8_t b = txt[idx++];
			if (isdigit(b)) {
				b -= 0x30;
				if (quot) {
					q *= 10;
					q += b;
				} else {
					r *= 10;
					r += b;
				}
			} else if (b == '.') {
				quot = false;
			} else if (b == ';' || b == '\0') {
				break;
			}
		}
		idx++;
		if (r < 10)
			r *= 10;
		di->amount = q * 100 + r;
		bd->nr_docs++;
	}

	return 0;
}

/* Добавление bank_data в массив запроса */
static void add_bank_data(struct request *req, const struct bank_data *bd) {
	req->pos = realloc(req->pos, (req->pos_count + 1) * sizeof(struct bank_data));
	req->pos[req->pos_count] = *bd;
	req->pos_count++;
}

/* ---------- Функции обработки лога ---------- */

/* Проверяет, начинается ли строка с метки времени нового блока */
int is_new_entry(const char *line) {
	if (strlen(line) < 13) return 0;
	if (!isdigit(line[0]) || !isdigit(line[1])) return 0;
	if (line[2] != ':') return 0;
	if (!isdigit(line[3]) || !isdigit(line[4])) return 0;
	if (line[5] != ':') return 0;
	if (!isdigit(line[6]) || !isdigit(line[7])) return 0;
	if (line[8] != '.') return 0;
	if (!isdigit(line[9]) || !isdigit(line[10]) || !isdigit(line[11])) return 0;
	if (line[12] != ' ') return 0;
	return 1;
}

/* Проверяет маркер ["Экспресс" --> ТМ] (8 и 2 байта) */
int is_marker_line(const char *line) {
	const char *p1 = strstr(line, "[\"");
	if (!p1) return 0;
	const char *p2 = strchr(p1 + 2, '"');
	if (!p2) return 0;
	int len1 = p2 - (p1 + 2);
	const char *p3 = strstr(p2, " --> ");
	if (!p3) return 0;
	const char *p4 = strchr(p3 + 5, ']');
	if (!p4) return 0;
	int len2 = p4 - (p3 + 5);
	return (len1 == 8 && len2 == 2);
}

/* Извлекает XML-блоки и POS-абзацы из бинарных данных */
void process_data(unsigned char *data, size_t len, struct request *req) {
	req->xml = NULL;
	req->pos = NULL;
	req->pos_count = 0;

	int xml_count = 0;
	int xml_cap = 0;

	size_t i = 0;
	while (i < len) {
		if (i + 1 < len && data[i] == 0x1b && data[i+1] == 0x79) {
			i += 2;
			size_t start = i;
			while (i + 1 < len && !(data[i] == 0x1b && (data[i+1] == 0x44 || data[i+1] == 0x5c))) {
				i++;
			}
			size_t end = i;
			size_t str_len = end - start;
			char *str = malloc(str_len + 1);
			memcpy(str, data + start, str_len);
			str[str_len] = 0;

			if (xml_count + 1 > xml_cap) {
				xml_cap = xml_cap ? xml_cap * 2 : 4;
				req->xml = realloc(req->xml, xml_cap * sizeof(char *));
			}
			req->xml[xml_count++] = str;

			if (i + 1 < len) i += 2;
		} else if (i + 1 < len && data[i] == 0x1b && data[i+1] == 0x62) {
			i += 2;
			size_t start = i;
			while (i + 1 < len && !(data[i] == 0x1b && data[i+1] == 0x5c)) {
				i++;
			}
			size_t end = i;
			size_t str_len = end - start;
			char *pos_str = malloc(str_len + 1);
			memcpy(pos_str, data + start, str_len);
			pos_str[str_len] = 0;

			/* Разбиваем pos_str по строкам и парсим каждый непустой абзац */
			char *saveptr = NULL;
			char *line = strtok_r(pos_str, "\r\n", &saveptr);
			while (line) {
				/* Пропускаем ведущие пробелы */
				while (*line && isspace((unsigned char)*line)) line++;
				if (*line) {
					struct bank_data bd;
					if (parse_bank_paragraph(line, &bd) == 0) {
						add_bank_data(req, &bd);
					}
				}
				line = strtok_r(NULL, "\r\n", &saveptr);
			}
			free(pos_str);

			if (i + 1 < len) i += 2;
		} else {
			i++;
		}
	}

	if (req->xml) {
		if (xml_count + 1 > xml_cap) {
			req->xml = realloc(req->xml, (xml_count + 1) * sizeof(char *));
		}
		req->xml[xml_count] = NULL;
	}
}

/* Печать содержимого запроса */
void process_request(struct request *req) {

    if (req->pos_count > 0)
    {
        printf("req->pos_count: %lu\n", req->pos_count);
        memcpy(&bd, &req->pos[0], sizeof(bd));
    }
    else
    {
        clear_bank_info();
    }
    
    if (req->xml) {
		for (int i = 0; req->xml[i]; i++) {
            int ecode;
            printf("req->xml = %s\n", req->xml[i]);
            parse_kkt_xml(req->xml[i], false, (kkt_xml_callback_t) kkt_xml_callback, &ecode);
		}
    }

//	printf("XML blocks:\n");
//	if (req->xml) {
//		for (int i = 0; req->xml[i]; i++) {
//			printf("XML[%d]: %s\n", i, req->xml[i]);
//		}
//	} else {
//		printf("No XML blocks\n");
//	}
//
//	printf("POS entries (%zu):\n", req->pos_count);
//	for (size_t i = 0; i < req->pos_count; i++) {
//		struct bank_data *bd = &req->pos[i];
//		printf("  [%zu] req_id=%llu term_id=%s op=%c ticket=%s repayment=%c\n",
//		       i, (unsigned long long)bd->req_id, bd->term_id, bd->op,
//		       bd->ticket ? "true" : "false",
//		       bd->repayment ? bd->repayment : ' ');
//		if (bd->repayment == '1') {
//			printf("       prev_blank_nr=%s\n", bd->prev_blank_nr);
//		}
//		for (size_t j = 0; j < bd->nr_docs; j++) {
//			printf("       doc[%zu]: blank_nr=%s amount=%llu\n",
//			       j, bd->doc_info[j].blank_nr,
//			       (unsigned long long)bd->doc_info[j].amount);
//		}
//	}
}

/* Освобождение памяти */
void free_request(struct request *req) {
	if (req->xml) {
		for (int i = 0; req->xml[i]; i++) {
			free(req->xml[i]);
		}
		free(req->xml);
	}
	if (req->pos) {
		free(req->pos);
	}
	req->xml = NULL;
	req->pos = NULL;
	req->pos_count = 0;
}

/* Парсит время в формате HH:MM:SS.mmm или HH:MM:SS.ff.mmm */
int parse_time_new(const char *str) {
	int h, m, s, ms = 0;
	char *copy = strdup(str);
	if (!copy) return -1;

	char *last_dot = strrchr(copy, '.');
	if (last_dot) {
		ms = atoi(last_dot + 1);
		*last_dot = '\0';
	}

	if (sscanf(copy, "%d:%d:%d", &h, &m, &s) != 3) {
		free(copy);
		return -1;
	}
	free(copy);

	return ((h * 60 + m) * 60 + s) * 1000 + ms;
}

/* ---------- main ---------- */
int adtest(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
		return 1;
	}
	char *dir = argv[1];
	
	clear_bank_info();

	/* Поиск первого файла period*.txt */
	DIR *d = opendir(dir);
	if (!d) {
		perror("opendir");
		return 1;
	}
	struct dirent *ent;
	char period_path[4096] = {0};
	while ((ent = readdir(d))) {
		if (strncmp(ent->d_name, "period", 6) == 0) {
			char *dot = strrchr(ent->d_name, '.');
			if (dot && strcmp(dot, ".txt") == 0) {
				snprintf(period_path, sizeof(period_path), "%s/%s", dir, ent->d_name);
				break;
			}
		}
	}
	closedir(d);
	if (period_path[0] == 0) {
		fprintf(stderr, "No period*.txt file found in %s\n", dir);
		return 1;
	}

	/* Чтение period-файла */
	FILE *pf = fopen(period_path, "r");
	if (!pf) {
		perror("fopen period");
		return 1;
	}
	char date[32] = {0};
	char time_from_str[64] = {0};
	char time_to_str[64] = {0};

	if (!fgets(date, sizeof(date), pf)) {
		fprintf(stderr, "Cannot read date from %s\n", period_path);
		fclose(pf);
		return 1;
	}
	date[strcspn(date, "\r\n")] = 0;

	if (!fgets(time_from_str, sizeof(time_from_str), pf)) {
		fprintf(stderr, "Cannot read time_from from %s\n", period_path);
		fclose(pf);
		return 1;
	}
	time_from_str[strcspn(time_from_str, "\r\n")] = 0;

	if (fgets(time_to_str, sizeof(time_to_str), pf)) {
		time_to_str[strcspn(time_to_str, "\r\n")] = 0;
	} else {
		time_to_str[0] = 0;
	}
	fclose(pf);

	int time_from = parse_time_new(time_from_str);
	if (time_from < 0) {
		fprintf(stderr, "Invalid time_from in period file: '%s'\n", time_from_str);
		return 1;
	}
	int time_to = -1;
	if (time_to_str[0]) {
		time_to = parse_time_new(time_to_str);
		if (time_to < 0) {
			fprintf(stderr, "Invalid time_to in period file: '%s'\n", time_to_str);
			return 1;
		}
	}
	
	/* --- Копирование ad.bin и archive.bin в /home/sterm --- */
//	{
//		const char *dst_dir = "/home/sterm";
//		const char *files_to_copy[] = { "ad.bin", "archive.bin" };
//		for (size_t i = 0; i < sizeof(files_to_copy) / sizeof(files_to_copy[0]); i++) {
//			char src_path[4096];
//			snprintf(src_path, sizeof(src_path), "%s/%s", dir, files_to_copy[i]);
//			if (access(src_path, F_OK) == 0) {
//				char dst_path[4096];
//				snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_dir, files_to_copy[i]);
//				
//				
//				if (strcmp(src_path, dst_path) != 0) {
//    				if (copy_file(src_path, dst_path) == 0) {
//	    				printf("Copied %s to %s\n", src_path, dst_path);
//		    		} else {
//			    		fprintf(stderr, "Failed to copy %s to %s\n", src_path, dst_path);
//				    }
//				}
//			}
//		}
//	}

	AD_load(1, false);
	

	/* Формирование пути к лог-файлу */
	char log_path[4096];
	snprintf(log_path, sizeof(log_path), "%s/log/xchg-%s.txt", dir, date);

	FILE *f = fopen(log_path, "rb");
	if (!f) {
		perror("fopen log");
		return 1;
	}

	char line[4096];
	unsigned char *data = NULL;
	size_t data_len = 0;
	size_t data_cap = 0;

	int in_entry = 0;
	int entry_matched = 0;

	while (fgets(line, sizeof(line), f)) {
		line[strcspn(line, "\r\n")] = 0;

		if (is_new_entry(line)) {
			/* Обрабатываем предыдущий блок */
			if (in_entry && entry_matched) {
				struct request req;
				process_data(data, data_len, &req);
				if (req.xml != NULL && req.xml[0] != NULL) {
					process_request(&req);
				}
				free_request(&req);
			}
			in_entry = 0;
			entry_matched = 0;
			data_len = 0;

			int h, m, s, ms;
			sscanf(line, "%d:%d:%d.%d", &h, &m, &s, &ms);
			int entry_time_ms = ((h * 60 + m) * 60 + s) * 1000 + ms;

			if (entry_time_ms >= time_from && (time_to == -1 || entry_time_ms < time_to)) {
				if (is_marker_line(line)) {
					entry_matched = 1;
					in_entry = 1;
				}
			}
		} else if (in_entry) {
			/* Разбор hex-дампа */
			char *dash = strstr(line, "--");
			if (dash) {
				*dash = '\0';
				char *part1 = line;
				char *part2 = dash + 2;

				char *token = strtok(part1, " ");
				if (token) token = strtok(NULL, " ");
				int count = 0;
				while (token && count < 8) {
					if (strlen(token) == 2 && isxdigit(token[0]) && isxdigit(token[1])) {
						unsigned int byte;
						sscanf(token, "%2x", &byte);
						if (data_len + 1 > data_cap) {
							data_cap = data_cap ? data_cap * 2 : 1024;
							data = realloc(data, data_cap);
						}
						data[data_len++] = (unsigned char)byte;
						count++;
					}
					token = strtok(NULL, " ");
				}

				token = strtok(part2, " ");
				count = 0;
				while (token && count < 8) {
					if (strlen(token) == 2 && isxdigit(token[0]) && isxdigit(token[1])) {
						unsigned int byte;
						sscanf(token, "%2x", &byte);
						if (data_len + 1 > data_cap) {
							data_cap = data_cap ? data_cap * 2 : 1024;
							data = realloc(data, data_cap);
						}
						data[data_len++] = (unsigned char)byte;
						count++;
					} else {
						break;
					}
					token = strtok(NULL, " ");
				}
			} else {
				char *token = strtok(line, " ");
				if (token) token = strtok(NULL, " ");
				int count = 0;
				while (token && count < 8) {
					if (strlen(token) == 2 && isxdigit(token[0]) && isxdigit(token[1])) {
						unsigned int byte;
						sscanf(token, "%2x", &byte);
						if (data_len + 1 > data_cap) {
							data_cap = data_cap ? data_cap * 2 : 1024;
							data = realloc(data, data_cap);
						}
						data[data_len++] = (unsigned char)byte;
						count++;
					} else {
						break;
					}
					token = strtok(NULL, " ");
				}
			}
		}
	}

	/* Последний блок */
	if (in_entry && entry_matched) {
		struct request req;
		process_data(data, data_len, &req);
		if (req.xml != NULL && req.xml[0] != NULL) {
			process_request(&req);
		}
		free_request(&req);
	}

	fclose(f);
	free(data);
	return 0;
}
