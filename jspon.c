#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ID_SIZE 64
#define MAX_VAL_SIZE 1024

int get_value(char* json, char* path, char* buf)
{
    size_t path_len = strlen(path);
    int dot_count = 0;
    for (size_t i=0; i<path_len; ++i) {
        if (path[i] == '.') {
            ++dot_count;
        }
    }
    char** path_stack = malloc((dot_count+1) * sizeof(char*));
    int path_top = 0;

    char path_id_buf[MAX_ID_SIZE] = {0};
    int path_id_buf_ptr = 0;
    
    for (size_t i=0; i<path_len; i++) {
        if (path[i] == '.') {
            path_stack[path_top] = malloc(MAX_ID_SIZE);
            strcpy(path_stack[path_top], path_id_buf);
            ++path_top;
            while (path_id_buf_ptr > 0) { path_id_buf[--path_id_buf_ptr] = 0; }
            continue;
        }
        path_id_buf[path_id_buf_ptr] = path[i];
        ++path_id_buf_ptr;
    }

    path_stack[path_top] = malloc(MAX_ID_SIZE);
    strcpy(path_stack[path_top], path_id_buf);
    
    size_t json_len = strlen(json);
    size_t stripped_len = json_len;
    bool apos_quotes = false;
    bool quotes = false;
    for (size_t i=0; i<json_len; ++i) {
        switch (json[i]) {
            case '"':
                if (!apos_quotes)
                    quotes = !quotes;
                break;
            case '\'':
                if (!quotes)
                    apos_quotes = !apos_quotes;
            case ' ':
                if (quotes || apos_quotes)
                    break;
            case '\n':
            case '\t':
                --stripped_len;
                break;
            case '}':
                ++stripped_len;
        }
    }
    char* sjson = malloc(stripped_len+1);
    size_t cur_index = 0;

    int cb_count = 1;

    apos_quotes = false;
    quotes = false;
    for (size_t i=0; i <json_len; ++i) {
        switch (json[i]) {
            case '"':
                if (!apos_quotes)
                    quotes = !quotes;
                break;
            case '\'':
                if (!quotes)
                    apos_quotes = !apos_quotes;
                break;
            case ' ':
                if (quotes || apos_quotes) {
                    sjson[cur_index] = json[i];
                    ++cur_index;
                }
            case '\n':
            case '\t':
                break;
            case '{':
                ++cb_count;
                sjson[cur_index] = json[i];
                ++cur_index;
                break;
            case '}':
                sjson[cur_index] = ',';
                ++cur_index;
            default:
                sjson[cur_index] = json[i];
                ++cur_index;
                break;
        }
    }
    int top = 0;
    char id_buf[MAX_ID_SIZE] = {0};
    char val_buf[MAX_VAL_SIZE] = {0};
    int id_buf_ptr = 0;
    int val_buf_ptr = 0;
    bool mod_val = false;
    bool matching;
    for (size_t i=1; i<stripped_len-1; ++i) {
        switch (sjson[i]) {
            case '"':
            case '\'':
                break;
            case '{':
                mod_val = false;
                ++top;
                while (id_buf_ptr > 0) { id_buf[--id_buf_ptr] = 0; }
                while (val_buf_ptr > 0) { val_buf[--val_buf_ptr] = 0; }
                break;

            case '}':
                --top;
                mod_val = false;
                while (id_buf_ptr > 0) { id_buf[--id_buf_ptr] = 0; }
                while (val_buf_ptr > 0) { val_buf[--val_buf_ptr] = 0; }
                break;
            case ',':
                matching = true;
                if (top == dot_count && !strcmp(path_stack[top], id_buf)) {
                    strcpy(buf,val_buf);
                    goto cleanup;
                }


                mod_val = false;
                while (id_buf_ptr > 0) { id_buf[--id_buf_ptr] = 0; }
                while (val_buf_ptr > 0) { val_buf[--val_buf_ptr] = 0; }
                break;
            case ':':
                if (strcmp(id_buf, path_stack[top])) {
                    cb_count = 0;
                    bool running = true;
                    for(; i<stripped_len-1 && running; ++i) {
                        switch (sjson[i]) {
                            case '{':
                                ++cb_count;
                                break;
                            case '}':
                                --cb_count;
                                break;
                            case ',':
                                if (!cb_count) {
                                    i -= 2;
                                    running=false;
                                }
                                break;
                    }
                }
                break;
            }
            
            mod_val = true;
            break;
            default:
                if (mod_val) {
                    val_buf[val_buf_ptr++] = sjson[i];
                    break;
                }
                id_buf[id_buf_ptr++] = sjson[i];
        }
    }
cleanup:
    while (path_top > 0) { free(path_stack[--path_top]); }
    free(path_stack);
    free(sjson);
    if (matching) return 0;
    return -1;
}
