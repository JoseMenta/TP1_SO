//
// Created by Jose Menta on 10/09/2022.
//

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

enum {ARGS = 0, STDIN = 1};
int read_shared_memory_info(shmADT shm);

#endif //TP1_SO_VISTA_H
