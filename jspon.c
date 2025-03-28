#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ID_SIZE 512

int jspon_get_values(char* json, size_t path_num, char** paths, char** bufs, size_t* buf_sizes)
{
    bool* found = malloc(path_num);
    for (size_t i=0; i<path_num; ++i) found[i] = false;

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

        int path_id_buf_ptr = 0;
    
        path_stacks[p][path_top] = malloc(MAX_ID_SIZE+1);
        for (size_t i=0; i<path_len; ++i) {
            if (path[i] == '.') {
                path_stacks[p][path_top++][path_id_buf_ptr] = 0;
                path_id_buf_ptr = 0;
                path_stacks[p][path_top] = malloc(MAX_ID_SIZE+1);
                continue;
            }
            path_stacks[p][path_top][path_id_buf_ptr] = path[i];
            ++path_id_buf_ptr;
            if (path_id_buf_ptr > MAX_ID_SIZE) return -3;
        }
        path_stacks[p][path_top++][path_id_buf_ptr] = 0;
    }

    size_t stripped_len = 0;
    bool apos_quotes = false;
    bool quotes = false;
    size_t i=0;
    while  (json[i] != 0) {
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
        ++i;
    }
    char* sjson = malloc(stripped_len+1);
    size_t cur_index = 0;

    int cb_count = 1;
    int max_cb_count = 1;

    apos_quotes = false;
    quotes = false;
    i = 0;
    while  (json[i] != 0) {
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
        ++i;
    }
    char** stack = malloc(max_cb_count * sizeof(char*));
    for (size_t i=0; i<max_cb_count; ++i) {
        stack[i] = malloc(MAX_ID_SIZE+1);
    }
    int top = 0;
    char id_buf[MAX_ID_SIZE+1] = {0};
    int id_buf_ptr = 0;
    int val_buf_ptr = 0;
    bool mod_val = false;
    apos_quotes = false;
    quotes = false;
    int arr = 0;
    int print_next = 0;
    for (size_t i=1; i<stripped_len-1; ++i) {
        if (print_next) {
            --print_next;
            //printf("stuff: %c %d\n", sjson[i], i);
        }
        switch (sjson[i]) {
            case '[':
                if (!quotes && !apos_quotes)
                    ++arr;
                break;
            case ']':
                if (!quotes && !apos_quotes)
                    --arr;
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
                break;
            case '}':
                if (quotes || apos_quotes || arr) break;
                --top;
                //printf("\n--top\n");
                break;
            case ',':
                if (quotes || apos_quotes || arr) break;
                mod_val = false;
                while (id_buf_ptr > 0) { id_buf[--id_buf_ptr] = 0; }
                break;
            case ':':
                if (quotes || apos_quotes || arr) break;
                mod_val = true;
                //printf("\nCOLON: %s\n", id_buf);
                // can be optimised
                bool stack_match = true;
                bool any_match = false;
                for (size_t p=0; p<path_num; ++p) {
                    bool matching = true;
                    //printf("top: %d, dotc: %d\n", top, dot_counts[p]);
                    if (top > dot_counts[p]) continue;
                    for (size_t j=0; j<top; ++j) {
                        if (strcmp(path_stacks[p][j], stack[j])) {
                            //printf("sorgy!!!! %s %s\n", path_stacks[p][j], stack[j]);
                            matching = false;
                            break;
                        }
                    }
                    if (!matching) {
                        continue;
                    }
                    //printf("STACK MATCH FOUND\n");
                    any_match = true;

                    if (dot_counts[p] != top || strcmp(id_buf,path_stacks[p][dot_counts[p]])) {
                        //printf("sorry.. %d %d %s %s\n",dot_counts[p], top, id_buf,path_stacks[p][dot_counts[p]]);
                        continue;
                    }
                    //printf("MATCH FOUND\n");
                    int colon_cb_count = 0;
                    int colon_arr = 0;
                    bool rem_last_quote = false;
                    for(size_t k=i+1 ; k<stripped_len-1; ++k) {
                        //printf("finding val: %c %d %d %d %d\n", sjson[k], quotes, apos_quotes, colon_arr, colon_cb_count);
                        switch (sjson[k]) {
                            case '\'':
                                if (!quotes)
                                    apos_quotes = !apos_quotes;
                                if (k == i+1) {
                                    rem_last_quote = true;
                                    break;
                                }
                                bufs[p][val_buf_ptr++] = sjson[k];
                                if (val_buf_ptr >= buf_sizes[p]) val_buf_ptr=0;
                                break;
                            case '"':
                                if (!apos_quotes)
                                    quotes = !quotes;
                                if (k == i+1) {
                                    rem_last_quote = true;
                                    break;
                                }
                                bufs[p][val_buf_ptr++] = sjson[k];
                                if (val_buf_ptr >= buf_sizes[p]) val_buf_ptr=0;
                                break;
                            case '[':
                                if (quotes || apos_quotes) break;
                                ++colon_arr;
                                bufs[p][val_buf_ptr++] = sjson[k];
                                if (val_buf_ptr >= buf_sizes[p]) val_buf_ptr=0;
                                break;
                            case ']':
                                if (quotes || apos_quotes) break;
                                --colon_arr;
                                bufs[p][val_buf_ptr++] = sjson[k];
                                if (val_buf_ptr >= buf_sizes[p]) val_buf_ptr=0;
                                break;
                            case '{':
                                if (quotes || apos_quotes || colon_arr) {
                                    bufs[p][val_buf_ptr++] = sjson[k];
                                    if (val_buf_ptr >= buf_sizes[p]) val_buf_ptr=0;
                                    break;
                                }
                                ++colon_cb_count;
                                bufs[p][val_buf_ptr++] = sjson[k];
                                if (val_buf_ptr >= buf_sizes[p]) val_buf_ptr=0;
                                break;
                            case '}':
                                if (quotes || apos_quotes || colon_arr) {
                                    bufs[p][val_buf_ptr++] = sjson[k];
                                    if (val_buf_ptr >= buf_sizes[p]) val_buf_ptr=0;
                                    break;
                                }
                                --colon_cb_count;
                                --val_buf_ptr;
                                bufs[p][val_buf_ptr++] = sjson[k];
                                break;
                            case ',':
                                if (quotes || apos_quotes || colon_arr) {
                                    bufs[p][val_buf_ptr++] = sjson[k];
                                    if (val_buf_ptr >= buf_sizes[p]) val_buf_ptr=0;
                                    break;
                                }
                                if (!colon_cb_count) {
                                    if (rem_last_quote && val_buf_ptr > 0) {
                                        bufs[p][--val_buf_ptr] = 0;
                                    }
                                    val_buf_ptr = 0;
                                    //printf("val_buf: %s %zu\n", bufs[p],p);
                                    found[p] = true;
                                    k = stripped_len;
                                    break;
                                }
                            default:
                                bufs[p][val_buf_ptr++] = sjson[k];
                                if (val_buf_ptr >= buf_sizes[p]) val_buf_ptr=0;
                                break;

                            }
                        }
                        break;
                }
                if (any_match) break;
                //printf("id buf: %s\n\n", id_buf);

                mod_val = false;
                int colon_cb_count = 1;
                int colon_arr = 0;
                bool cont = true;
                while (id_buf_ptr > 0) { id_buf[--id_buf_ptr] = 0; }
                for (; i<stripped_len-1 && cont; ++i) {
                    //printf("%c",sjson[i]);
                    switch (sjson[i]) {
                        case '\'':
                            if (!quotes)
                                apos_quotes = !apos_quotes;
                            break;
                        case '"':
                            if (!apos_quotes)
                                quotes = !quotes;
                            break;
                        case '[':
                            if (quotes || apos_quotes) break;
                            ++colon_arr;
                            break;
                        case ']':
                            if (quotes || apos_quotes) break;
                            --colon_arr;
                            break;
                        case '{':
                            if (quotes || apos_quotes || colon_arr) break;
                            ++colon_cb_count;
                            break;
                        case '}':
                            if (quotes || apos_quotes || colon_arr) break;
                            --colon_cb_count;
                            break;
                        case ',':
                            //printf("\nhey %d %d %d %d\n\n", cb_count, arr, quotes, apos_quotes);
                            if (quotes || apos_quotes || colon_arr) break;
                            if (!colon_cb_count) {
                                --top;
                                --i;
                                //printf("\nbreak %d %d %d %c %d\n", quotes, apos_quotes, arr, sjson[i], i);
                                print_next = 4;
                                cont = false;
                                break;
                            }
                    }
                }

                //printf("\n");
                break;
            
            default:
                if (mod_val) {
                    break;
                }
                id_buf[id_buf_ptr++] = sjson[i];
                //printf("to id: %c\n", sjson[i]);
                if (id_buf_ptr >= MAX_ID_SIZE) id_buf_ptr=0;
        }
    }

    int ret_val = 0;

    for (size_t p=0; p<path_num; ++p) {
        while (dot_counts[p] > 0) { free(path_stacks[p][dot_counts[p]--]); }
        if (found[p] == false)
            ret_val = -1;
        free(path_stacks[p][0]);
        free(path_stacks[p]);
    }
    free(path_stacks);
    while (top > 0) { free(stack[--top]); }
    free(stack);
    free(found);
    free(sjson);
    return ret_val;
}

size_t jspon_get_array_size(char* json)
{
    size_t size = 0;
    bool quotes = false;
    bool apos_quotes = false;
    bool data = false;
    int arr = 0;
    int cb_count = 0;

    size_t i = 1;
    while(json[i]) {
        switch(json[i]) {
            case ' ':
            case '\n':
            case '\t':
                break;
            case '\'':
                if (!quotes) apos_quotes = !apos_quotes;
                break;
            case '"':
                if (!apos_quotes) quotes = !quotes;
                break;
            case '[':
                if (quotes || apos_quotes) break;
                ++arr;
                break;
            case ']':
                if (quotes || apos_quotes) break;
                --arr;
                break;
            case '{':
                if (quotes || apos_quotes || arr) break;
                ++cb_count;
                break;
            case '}':
                if (quotes || apos_quotes || arr) break;
                --cb_count;
                break;
            case ',':
                if (quotes || apos_quotes || arr || cb_count) break;
                ++size;
                break;
            default:
                data = true;
                break;

        }
        ++i;
    }
    if (data) ++size;
    return size;
}

int jspon_parse_array(char* json, size_t arr_size, size_t buf_size, char** bufs)
{
    size_t i = 1;
    size_t buf_ptr = 0;
    size_t buf_index = 0;
    int arr = 0;
    int cb_count = 0;
    bool quotes = false;
    bool apos_quotes = false;
    while (json[i]) {
        switch(json[i]) {
            case ' ':
            case '\n':
            case '\t':
                break;
            case '\'':
                if (!quotes) apos_quotes = !apos_quotes;
                else {
                    if (buf_ptr >= buf_size) buf_ptr = 0;
                    bufs[buf_index][buf_ptr++] = json[i];
                    break;
                }
                break;
            case '"':
                if (!apos_quotes) quotes = !quotes;
                else {
                    if (buf_ptr >= buf_size) buf_ptr = 0;
                    bufs[buf_index][buf_ptr++] = json[i];
                    break;
                }
                break;
            case '[':
                if (buf_ptr >= buf_size) buf_ptr = 0;
                bufs[buf_index][buf_ptr++] = json[i];
                if (quotes || apos_quotes) break;
                ++arr;
                break;
            case ']':
                if (quotes || apos_quotes || arr == 0) break;
                if (buf_ptr >= buf_size) buf_ptr = 0;
                bufs[buf_index][buf_ptr++] = json[i];
                --arr;
                break;
            case '{':
                if (buf_ptr >= buf_size) buf_ptr = 0;
                bufs[buf_index][buf_ptr++] = json[i];
                if (quotes || apos_quotes || arr) break;
                ++cb_count;
                break;
            case '}':
                if (buf_ptr >= buf_size) buf_ptr = 0;
                bufs[buf_index][buf_ptr++] = json[i];
                if (quotes || apos_quotes || arr) break;
                --cb_count;
                break;
            case ',':
                if (quotes || apos_quotes || arr || cb_count) {
                    if (buf_ptr >= buf_size) buf_ptr = 0;
                    bufs[buf_index][buf_ptr++] = json[i];
                    break;
                }
                bufs[buf_index][buf_ptr] = 0;
                buf_ptr = 0;
                if (++buf_index >= arr_size) {
                    return -1;
                }
                break;
            default:
                if (buf_ptr >= buf_size) buf_ptr = 0;
                bufs[buf_index][buf_ptr++] = json[i];
                break;
        }
        ++i;
    }
    bufs[buf_index][buf_ptr] = 0;
    return 0;
}

int jspon_get_array_values(char* json, size_t num_indexes, size_t indexes[], char** bufs)
{
    return 0;
}
