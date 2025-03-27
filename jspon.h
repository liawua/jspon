#include <stdlib.h>

size_t jspon_get_array_size(char* json);
int jspon_parse_array(char* json, size_t arr_size, size_t buf_size, char** bufs);
int jspon_get_values(char* json, size_t path_num, char** paths, char** bufs, size_t* buf_sizes);
