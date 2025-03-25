#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ID_SIZE 512
#define MAX_VAL_SIZE 8192

int jspon_get_value(char* json, char* path, char* buf, size_t buf_size)
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
            path_stack[path_top] = malloc(MAX_ID_SIZE+1);
            strncpy(path_stack[path_top], path_id_buf,MAX_ID_SIZE);
            ++path_top;
            while (path_id_buf_ptr > 0) { path_id_buf[--path_id_buf_ptr] = 0; }
            continue;
        }
        path_id_buf[path_id_buf_ptr] = path[i];
        ++path_id_buf_ptr;
    }

    path_stack[path_top] = malloc(MAX_ID_SIZE+1);
    strncpy(path_stack[path_top], path_id_buf, MAX_ID_SIZE);
    
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
                break;
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
    for (size_t i=1; i<stripped_len-1; ++i) {
        switch (sjson[i]) {
            case '"':
            case '\'':
                break;
            case '{':
                ++top;
                while (id_buf_ptr > 0) { id_buf[--id_buf_ptr] = 0; }
                while (val_buf_ptr > 0) { val_buf[--val_buf_ptr] = 0; }
                break;

            case '}':
                --top;
                while (id_buf_ptr > 0) { id_buf[--id_buf_ptr] = 0; }
                while (val_buf_ptr > 0) { val_buf[--val_buf_ptr] = 0; }
                break;
            case ',':
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
                if (top == dot_count) {
                    cb_count = 0;
                    for(++i ; i<stripped_len-1; ++i) {
                        switch (sjson[i]) {
                            case '{':
                                ++cb_count;
                                val_buf[val_buf_ptr++] = sjson[i];
                                break;
                            case '}':
                                --cb_count;
                                --val_buf_ptr;
                                val_buf[val_buf_ptr++] = sjson[i];
                                break;
                            case ',':
                                if (!cb_count) {
                                    strncpy(buf,val_buf,buf_size);
                                    while (path_top > 0) { free(path_stack[--path_top]); }
                                    free(path_stack);
                                    free(sjson);
                                    return 0;
                                }
                            default:
                                val_buf[val_buf_ptr++] = sjson[i];
                                break;

                        }
                    }
                    break;
                }
            break;
            default:
                id_buf[id_buf_ptr++] = sjson[i];
        }
    }
    while (path_top > 0) { free(path_stack[--path_top]); }
    free(path_stack);
    free(sjson);
    return -1;
}

int jspon_get_values(char* json, size_t path_num, char** paths, char** bufs, size_t* buf_sizes)
{
    char*** path_stacks = malloc(path_num * sizeof(char*));
    size_t* dot_counts = malloc(path_num * sizeof(size_t));
    for (size_t p=0; p<path_num; ++p) {
        char* path = paths[p];
        size_t path_len = strlen(path);
        dot_counts[p] = 0;
        for (size_t i=0; i<path_len; ++i) {
            if (path[i] == '.') {
                ++dot_counts[p];
            }
        }
        path_stacks[p] = malloc((dot_counts[p]+1) * sizeof(char*));
        int path_top = 0;

        char path_id_buf[MAX_ID_SIZE] = {0};
        int path_id_buf_ptr = 0;
    
        for (size_t i=0; i<path_len; i++) {
            if (path[i] == '.') {
                path_stacks[p][path_top] = malloc(MAX_ID_SIZE+1);
                strncpy(path_stacks[p][path_top++], path_id_buf, MAX_ID_SIZE);
                while (path_id_buf_ptr > 0) { path_id_buf[--path_id_buf_ptr] = 0; }
                continue;
            }
            path_id_buf[path_id_buf_ptr] = path[i];
            ++path_id_buf_ptr;
            if (path_id_buf_ptr >= MAX_ID_SIZE) return -3;
        }

        path_stacks[p][path_top] = malloc(MAX_ID_SIZE+1);
        strncpy(path_stacks[p][path_top], path_id_buf, MAX_ID_SIZE);
    }

    size_t json_len = strlen(json);
    size_t stripped_len = 0;
    bool apos_quotes = false;
    bool quotes = false;
    for (size_t i=0; i<json_len; ++i) {
        switch (json[i]) {
            case '"':
                if (!apos_quotes)
                    quotes = !quotes;
                ++stripped_len;
                break;
            case '\'':
                if (!quotes)
                    apos_quotes = !apos_quotes;
                ++stripped_len;
                break;
            case ' ':
                if (quotes || apos_quotes) {
                    ++stripped_len;
                }
            case '\n':
            case '\t':
                break;
            case '{':
                ++stripped_len;
                break;
            case '}':
                ++stripped_len;
            default:
                ++stripped_len;
                break;
        }
    }
    char* sjson = malloc(stripped_len+1);
    size_t cur_index = 0;

    int cb_count = 1;
    int max_cb_count = 1;

    apos_quotes = false;
    quotes = false;
    for (size_t i=0; i <json_len; ++i) {
        switch (json[i]) {
            case '"':
                if (!apos_quotes)
                    quotes = !quotes;
                sjson[cur_index] = json[i];
                ++cur_index;
                break;
            case '\'':
                if (!quotes)
                    apos_quotes = !apos_quotes;
                sjson[cur_index] = json[i];
                ++cur_index;
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
                if (max_cb_count<cb_count)
                    max_cb_count = cb_count;
                sjson[cur_index] = json[i];
                ++cur_index;
                break;
            case '}':
                --cb_count;
                sjson[cur_index] = ',';
                ++cur_index;
            default:
                sjson[cur_index] = json[i];
                ++cur_index;
                break;
        }
    }
    char** stack = malloc(max_cb_count * sizeof(char*));
    for (size_t i=0; i<max_cb_count; ++i) {
        stack[i] = malloc(MAX_ID_SIZE+1);
    }
    int top = 0;
    char id_buf[MAX_ID_SIZE+1] = {0};
    char val_buf[MAX_VAL_SIZE+1] = {0};
    int id_buf_ptr = 0;
    int val_buf_ptr = 0;
    bool mod_val = false;
    apos_quotes = false;
    quotes = false;
    bool arr = false;
    for (size_t i=1; i<stripped_len-1; ++i) {
        switch (sjson[i]) {
            case '[':
                if (!quotes && !apos_quotes)
                    arr = true;
                break;
            case ']':
                if (!quotes && !apos_quotes)
                    arr = false;
                break;
            case '"':
                if (!apos_quotes)
                    quotes = !quotes;
                break;
            case '\'':
                if (!quotes)
                    apos_quotes = !apos_quotes;
                break;
            case '{':
                if (quotes || apos_quotes || arr) break;
                mod_val = false;
                strncpy(stack[top++], id_buf, MAX_ID_SIZE);
                while (id_buf_ptr > 0) { id_buf[--id_buf_ptr] = 0; }
                while (val_buf_ptr > 0) { val_buf[--val_buf_ptr] = 0; }
                break;
            case '}':
                if (quotes || apos_quotes || arr) break;
                --top;
            case ',':
                if (quotes || apos_quotes || arr) break;
                mod_val = false;
                while (id_buf_ptr > 0) { id_buf[--id_buf_ptr] = 0; }
                while (val_buf_ptr > 0) { val_buf[--val_buf_ptr] = 0; }
                break;
            case ':':
               if (quotes || apos_quotes || arr) break;
                // can be optimised
                for (size_t p=0; p<path_num; ++p) {
                    if (dot_counts[p] == top && !strcmp(id_buf,path_stacks[p][dot_counts[p]])) {
                        bool matching = true;
                        for (size_t j=0; j<top && j<dot_counts[p]; ++j) {
                            if (strcmp(path_stacks[p][j], stack[j])) {
                                matching = false;
                                break;
                            }
                        }
                        if (matching) {
                            cb_count = 0;
                            bool rem_last_quote = false;
                            for(size_t k=i+1 ; k<stripped_len-1; ++k) {
                                switch (sjson[k]) {
                                    case '\'':
                                        if (!quotes)
                                            apos_quotes = !apos_quotes;
                                        if (k == i+1) {
                                            rem_last_quote = true;
                                            break;
                                        }
                                        val_buf[val_buf_ptr++] = sjson[k];
                                        if (val_buf_ptr >= MAX_VAL_SIZE) val_buf_ptr=0;
                                        break;
                                    case '"':
                                        if (!apos_quotes)
                                            quotes = !quotes;
                                        if (k == i+1) {
                                            rem_last_quote = true;
                                            break;
                                        }
                                        val_buf[val_buf_ptr++] = sjson[k];
                                        if (val_buf_ptr >= MAX_VAL_SIZE) val_buf_ptr=0;
                                        break;
                                    case '{':
                                        if (quotes || apos_quotes) {
                                            val_buf[val_buf_ptr++] = sjson[k];
                                            if (val_buf_ptr >= MAX_VAL_SIZE) val_buf_ptr=0;
                                            break;
                                        }
                                        ++cb_count;
                                        val_buf[val_buf_ptr++] = sjson[k];
                                        if (val_buf_ptr >= MAX_VAL_SIZE) val_buf_ptr=0;
                                        break;
                                    case '}':
                                        if (quotes || apos_quotes) {
                                            val_buf[val_buf_ptr++] = sjson[k];
                                            if (val_buf_ptr >= MAX_VAL_SIZE) val_buf_ptr=0;
                                            break;
                                        }
                                        --cb_count;
                                        --val_buf_ptr;
                                        val_buf[val_buf_ptr++] = sjson[k];
                                        break;
                                    case ',':
                                        if (quotes || apos_quotes) {
                                            val_buf[val_buf_ptr++] = sjson[k];
                                            if (val_buf_ptr >= MAX_VAL_SIZE) val_buf_ptr=0;
                                            break;
                                        }
                                        if (!cb_count) {
                                            if (rem_last_quote && val_buf_ptr > 0) {
                                                val_buf[--val_buf_ptr] = 0;
                                            }
                                            strncpy(bufs[p],val_buf,buf_sizes[p]);
                                            k = stripped_len;
                                            break;
                                        }
                                    default:
                                        val_buf[val_buf_ptr++] = sjson[k];
                                        if (val_buf_ptr >= MAX_VAL_SIZE) val_buf_ptr=0;
                                        break;

                                }
                            }
                            break;
                        }
                    }
                }
                mod_val = true;
                break;
            
            default:
                if (mod_val) {
                    break;
                }
                id_buf[id_buf_ptr++] = sjson[i];
                if (id_buf_ptr >= MAX_ID_SIZE) id_buf_ptr=0;
        }
    }
    for (size_t p=0; p<path_num; ++p) {
        while (dot_counts[p] > 0) { free(path_stacks[p][dot_counts[p]--]); }
        free(path_stacks[p][0]);
        free(path_stacks[p]);
    }
    free(path_stacks);
    while (top > 0) { free(stack[--top]); }
    free(stack);
    free(sjson);
    return 0;
}
