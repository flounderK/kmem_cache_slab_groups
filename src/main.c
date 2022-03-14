#define _GNU_SOURCE
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <string.h>

#define PATHBUF_SIZE 1024

#define DIRPATH "/sys/kernel/slab"

char g_dir_name[PATHBUF_SIZE] = {};
size_t g_filtered_count = 0;

int filter_points_to_dir(const struct dirent* dent){
    if (dent == NULL){
        return 0;
    }
    if (dent->d_type != DT_LNK){
        return 0;
    }

    char pathbuf[PATHBUF_SIZE];
    char full_pathbuf[PATHBUF_SIZE];
    memset(pathbuf, 0, sizeof(pathbuf));
    memset(full_pathbuf, 0, sizeof(full_pathbuf));
    // expand to full path
    snprintf(full_pathbuf, sizeof(full_pathbuf),
             DIRPATH "/%s",
             dent->d_name);

    // -1 to handle off by 1 on null byte from readlink
    if (readlink(full_pathbuf, pathbuf, sizeof(pathbuf) - 1) < 0){
        perror("readlink");
        printf("%s\n", dent->d_name);
    }

    if (strncmp(g_dir_name, pathbuf, sizeof(pathbuf)) == 0){
        //printf("%s -> %s\n", dent->d_name, g_dir_name);
        g_filtered_count += 1;
        return 1;
    }
    return 0;
}

int compar(const struct dirent ** p_dent1,
           const struct dirent ** p_dent2){
    return strncmp((*p_dent1)->d_name,
                   (*p_dent2)->d_name,
                   sizeof((*p_dent2)->d_name));
}

int main (int argc, char *argv[]) {
    struct dirent* dent = NULL;
    DIR* dir_outer = NULL;
    char pathbuf[PATHBUF_SIZE];
    memset(&pathbuf, 0, sizeof(pathbuf));

    dir_outer = opendir(DIRPATH);
    if (dir_outer == NULL){
        perror("opendir");
        exit(1);
    }

    struct dirent **namelist = NULL;
    // iterate through all directory entries
    while ((dent = readdir(dir_outer)) != NULL){
        // skip if this isn't a directory
        if (dent->d_type != DT_DIR){
            continue;
        }

        // set global dir name to allow this filter hack to work
        strncpy(g_dir_name, dent->d_name, sizeof(dent->d_name));
        g_filtered_count = 0;
        scandir(DIRPATH,
                &namelist,
                filter_points_to_dir,
                compar);
        if (namelist == NULL){
            //printf("Namelist was null for %s\n", dent->d_name);
            continue;
        }
        printf("%11s ", g_dir_name);
        for (int i = 0; i < g_filtered_count; i++){
            printf("%s ", namelist[i]->d_name);
        }
        printf("\n");
    }

    return 0;
}
