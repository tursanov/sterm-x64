#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

#define MAX_CODE_LENGTH 4        // "XXY" + null terminator
#define MAX_FILENAME_LENGTH 256
#define MAX_PATTERNS 200
#define MAX_LINE_LENGTH 1024
#define PATTERNS_DIR "/home/sterm/patterns/"
#define PATTERNS_FILE PATTERNS_DIR "s00.txt"

typedef struct {
    char code[MAX_CODE_LENGTH];  // Код шаблона (например "031")
    uint8_t *data;              // Содержимое файла шаблона
    size_t size;                // Размер данных
    bool is_shared;             // Флаг, что данные используются несколькими шаблонами
    bool is_default;            // Флаг, что шаблон для ИНН=000000000000
} pattern_entry;

typedef struct {
    char filename[MAX_FILENAME_LENGTH];
    uint8_t *data;
    size_t size;
    int ref_count;              // Счетчик ссылок на файл
} loaded_file;

static pattern_entry *patterns = NULL;
static loaded_file *loaded_files = NULL;
static int patterns_count = 0;
static int files_count = 0;

// Внутренняя функция для удаления символов перевода строки
static void clean_line(char *line) {
    line[strcspn(line, "\r")] = '\0';
    line[strcspn(line, "\n")] = '\0';
}

// Внутренняя функция для проверки, нужно ли загружать шаблон
static bool should_load_pattern(const char *line, uint64_t target_inn, bool *is_default) {
    char inn_str[20];
    char *equal_pos = strchr(line, '=');
    if (!equal_pos) return false;
    
    size_t inn_len = equal_pos - line;
    if (inn_len >= sizeof(inn_str)) inn_len = sizeof(inn_str) - 1;
    
    strncpy(inn_str, line, inn_len);
    inn_str[inn_len] = '\0';
    
    uint64_t file_inn;
    if (sscanf(inn_str, "%lu", &file_inn) != 1) return false;
    
    *is_default = (file_inn == 0);
    return (*is_default || file_inn == target_inn);
}

// Внутренняя функция для загрузки файла в память
static uint8_t *load_file(const char *filename, size_t *size) {
    char fullpath[MAX_FILENAME_LENGTH];
    snprintf(fullpath, sizeof(fullpath), "%s%s", PATTERNS_DIR, filename);
    
    FILE *file = fopen(fullpath, "rb");
    if (!file) {
        fprintf(stderr, "Ошибка: Не удалось открыть файл шаблона '%s'\n", fullpath);
        return NULL;
    }
    
    struct stat st;
    if (fstat(fileno(file), &st) != 0) {
        fprintf(stderr, "Ошибка: Не удалось определить размер файла '%s'\n", fullpath);
        fclose(file);
        return NULL;
    }
    
    uint8_t *buffer = malloc(st.st_size);
    if (!buffer) {
        fprintf(stderr, "Ошибка: Не удалось выделить память для файла '%s'\n", fullpath);
        fclose(file);
        return NULL;
    }
    
    if (fread(buffer, 1, st.st_size, file) != (size_t)st.st_size) {
        fprintf(stderr, "Ошибка: Не удалось прочитать файл '%s'\n", fullpath);
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    *size = st.st_size;
    return buffer;
}

// Внутренняя функция для поиска уже загруженного файла
static loaded_file *find_loaded_file(const char *filename) {
    for (int i = 0; i < files_count; i++) {
        if (strcmp(loaded_files[i].filename, filename) == 0) {
            return &loaded_files[i];
        }
    }
    return NULL;
}

// Внутренняя функция для удаления шаблона по индексу
static void remove_pattern(int index) {
    if (index < 0 || index >= patterns_count) return;
    
    // Уменьшаем счетчик ссылок на файл
    if (!patterns[index].is_shared) {
        for (int i = 0; i < files_count; i++) {
            if (loaded_files[i].data == patterns[index].data) {
                loaded_files[i].ref_count--;
                if (loaded_files[i].ref_count == 0) {
                    free(loaded_files[i].data);
                    // Удаляем файл из списка
                    memmove(&loaded_files[i], &loaded_files[i+1], 
                           (files_count - i - 1) * sizeof(loaded_file));
                    files_count--;
                }
                break;
            }
        }
    }
    
    // Удаляем шаблон из списка
    memmove(&patterns[index], &patterns[index+1], 
           (patterns_count - index - 1) * sizeof(pattern_entry));
    patterns_count--;
}

// Внутренняя функция для добавления/замены шаблона
static void add_or_replace_pattern(const char *code, const char *filename, bool is_default) {
    // Сначала проверяем, существует ли уже шаблон с таким кодом
    for (int i = 0; i < patterns_count; i++) {
        if (strcmp(patterns[i].code, code) == 0) {
            // Если новый шаблон имеет приоритет (не default), заменяем старый
            if (!is_default || patterns[i].is_default) {
                printf("Заменяем старый шаблон на более приоритетный - %s=%s\n", code, filename);
                remove_pattern(i);
                break;
            } else {
                // Шаблон уже существует с более высоким приоритетом
                return;
            }
        }
    }
    
    if (patterns_count >= MAX_PATTERNS) {
        fprintf(stderr, "Предупреждение: Достигнуто максимальное количество шаблонов (%d)\n", MAX_PATTERNS);
        return;
    }
    
    // Ищем уже загруженный файл
    loaded_file *file = find_loaded_file(filename);
    
    if (!patterns) {
        patterns = malloc(MAX_PATTERNS * sizeof(pattern_entry));
        if (!patterns) {
            fprintf(stderr, "Ошибка: Не удалось выделить память для хранения шаблонов\n");
            exit(EXIT_FAILURE);
        }
    }
    
    pattern_entry *entry = &patterns[patterns_count];
    strncpy(entry->code, code, MAX_CODE_LENGTH - 1);
    entry->code[MAX_CODE_LENGTH - 1] = '\0';
    entry->is_default = is_default;
    
    if (file) {
        // Файл уже загружен, используем существующие данные
        entry->data = file->data;
        entry->size = file->size;
        entry->is_shared = true;
        file->ref_count++;
    } else {
        // Загружаем новый файл
        size_t size;
        uint8_t *data = load_file(filename, &size);
        if (!data) {
            fprintf(stderr, "Ошибка: Пропускаем шаблон '%s' из-за ошибки загрузки файла\n", code);
            return;
        }
        
        // Добавляем в список загруженных файлов
        if (!loaded_files) {
            loaded_files = malloc(MAX_PATTERNS * sizeof(loaded_file));
            if (!loaded_files) {
                fprintf(stderr, "Ошибка: Не удалось выделить память для списка файлов\n");
                free(data);
                exit(EXIT_FAILURE);
            }
        }
        
        loaded_file *new_file = &loaded_files[files_count];
        strncpy(new_file->filename, filename, MAX_FILENAME_LENGTH - 1);
        new_file->filename[MAX_FILENAME_LENGTH - 1] = '\0';
        new_file->data = data;
        new_file->size = size;
        new_file->ref_count = 1;
        files_count++;
        
        entry->data = data;
        entry->size = size;
        entry->is_shared = false;
    }
    
    patterns_count++;
}

// Освобождает память, занятую шаблонами
void kkt_free_patterns(void) {
    // Освобождаем файлы, на которые нет ссылок
    for (int i = 0; i < files_count; ) {
        if (loaded_files[i].ref_count == 0) {
            free(loaded_files[i].data);
            memmove(&loaded_files[i], &loaded_files[i+1], 
                  (files_count - i - 1) * sizeof(loaded_file));
            files_count--;
        } else {
            i++;
        }
    }
    
    if (loaded_files) {
        free(loaded_files);
        loaded_files = NULL;
    }
    
    if (patterns) {
        free(patterns);
        patterns = NULL;
    }
    
    patterns_count = 0;
    files_count = 0;
    
    printf("Память, занятая шаблонами, освобождена\n");
}



// Перезагружает шаблоны для указанного ИНН
void kkt_reload_patterns(uint64_t inn) {
    // Очищаем старые шаблоны
    kkt_free_patterns();
    
    FILE *file = fopen(PATTERNS_FILE, "r");
    if (!file) {
        fprintf(stderr, "Ошибка: Не удалось открыть файл с шаблонами '%s'\n", PATTERNS_FILE);
        return;
    }
    
    printf("Загружаем шаблоны для ИНН %lu...\n", inn);
    
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        // Пропускаем комментарии и пустые строки
        if (line[0] == ';' || line[0] == '\n') continue;
        
        // Удаляем символы перевода строки
        clean_line(line);
        
        // Проверяем, нужно ли загружать этот шаблон
        bool is_default = false;
        if (!should_load_pattern(line, inn, &is_default)) continue;
        
        // Парсим строку с шаблонами
        char *equal_pos = strchr(line, '=');
        if (!equal_pos) continue;
        
        const char *patterns_str = equal_pos + 1;
        char *token;
        char *rest = (char *)patterns_str;
        
        while ((token = strtok_r(rest, ",", &rest))) {
            char *colon_pos = strchr(token, ':');
            if (!colon_pos) continue;
            
            // Извлекаем код (первые 3 символа до ':')
            char code[MAX_CODE_LENGTH];
            size_t code_len = colon_pos - token;
            if (code_len >= MAX_CODE_LENGTH) continue;
            
            strncpy(code, token, code_len);
            code[code_len] = '\0';
            
            // Извлекаем имя файла
            const char *filename = colon_pos + 1;
            
            add_or_replace_pattern(code, filename, is_default);
        }
    }
    
    fclose(file);
    printf("Загружено %d шаблонов\n", patterns_count);
}

// Находит шаблон по типу документа и индексу
const uint8_t *kkt_find_pattern(uint8_t docType, uint8_t index, size_t *size) {
    if (!patterns || !size) {
        fprintf(stderr, "Ошибка: Шаблоны не загружены или неверный указатель на размер\n");
        return NULL;
    }
    
    char search_code[MAX_CODE_LENGTH];
    snprintf(search_code, sizeof(search_code), "%.2d%d", docType, index);
    
    for (int i = 0; i < patterns_count; i++) {
        if (strcmp(patterns[i].code, search_code) == 0) {
            *size = patterns[i].size;
            return patterns[i].data;
        }
    }
    
    fprintf(stderr, "Предупреждение: Шаблон с кодом '%s' не найден\n", search_code);
    return NULL;
}
