#include <stdio.h>
#include <stdlib.h>
#include "jspon.h"

void update_array_for_json_parsing(int index, size_t buf_size, char* path, char** bufs, size_t* buf_sizes, char** paths)
{
    buf_sizes[index] = buf_size;
    bufs[index] = malloc(buf_size);
    paths[index] = path;
}

int main()
{
    FILE* f = fopen("3bdseg.json","r");
    char content[65536];
    fread(content,sizeof(char),65536,f);
    //printf("%s\n",content);
    const int LEN = 7;

    const int ID = 0;
    const int USERNAME = 1;
    const int PP = 2;
    const int RANK = 3;
    const int COUNTRY_RANK = 4;
    const int COUNTRY_CODE = 5;
    const int COUNTRY_NAME = 6;

    char* bufs[LEN];
    size_t buf_sizes[LEN];
    char* paths[LEN];

    update_array_for_json_parsing(ID,           32, "id",                       bufs,buf_sizes,paths);
    update_array_for_json_parsing(USERNAME,     64, "username",                 bufs,buf_sizes,paths);
    update_array_for_json_parsing(PP,           16, "statistics.pp",            bufs,buf_sizes,paths);
    update_array_for_json_parsing(RANK,         16, "statistics.global_rank",   bufs,buf_sizes,paths);
    update_array_for_json_parsing(COUNTRY_RANK, 16, "statistics.country_rank",  bufs,buf_sizes,paths);
    update_array_for_json_parsing(COUNTRY_CODE,  8, "country_code",             bufs,buf_sizes,paths);
    update_array_for_json_parsing(COUNTRY_NAME, 64, "country.name",             bufs,buf_sizes,paths);
    
    int ret_val = jspon_get_values(content,7,paths,bufs,buf_sizes);
    if (ret_val) {
        for (size_t i=0; i<LEN; ++i) {
            free(bufs[i]);
        }
        return ret_val;
    }
    printf("username: %s\nid: %s\npp: %s\nrank: %s\ncountry rank: %s\ncountry: %s (%s)\n",bufs[USERNAME], bufs[ID], bufs[PP], bufs[RANK], bufs[COUNTRY_RANK], bufs[COUNTRY_NAME], bufs[COUNTRY_CODE]);
}

