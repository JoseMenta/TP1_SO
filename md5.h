
#ifndef TP1_SO_MD5_H
#define TP1_SO_MD5_H

// feature_test_macro para getline, fdopen y ftruncate
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include "shmADT.h"

#define SLAVES 5
#define SHM_NAME "/shm"
#define READ_SEM "/read_semaphore"
#define SHM_ELEMENT_SIZE 128
#define SHM_SIZE(elements) (sizeof(char) * SHM_ELEMENT_SIZE * (elements) + 1)
#define SLEEP_TIME 10

int write_to_slave(int fd, const char * file_path);
int is_file(const char * file_path);
int close_shm_file_shmADT(void* shm, int shm_fd, int shm_length, shmADT shmAdt,sem_t* sem, FILE* file);
int close_fd(int * fd, int length);

#endif //TP1_SO_MD5_H
