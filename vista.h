
#ifndef TP1_SO_VISTA_H
#define TP1_SO_VISTA_H

// feature_test_macro para getline
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include "shmADT.h"

#define EOT 0x04
#define ARCH_INFO_SIZE 128

int read_shared_memory_info(shmADT shm);
void free_strs(char ** memory_to_free, int len);

#endif //TP1_SO_VISTA_H
