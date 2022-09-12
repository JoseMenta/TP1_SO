#ifndef TP1_SO_SHMADT_H
#define TP1_SO_SHMADT_H

#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

#define EOT 0x04

// -------------------------------------------------------------------------------------------------------
// shmADT: ADT que permite controlar la lectura y escritura a un shm compartido por procesos
// -------------------------------------------------------------------------------------------------------
// Notas:
//      Se debe utilizar el mismo shm entre los procesos, indicando el mismo tamaño en ambos
// -------------------------------------------------------------------------------------------------------

typedef struct shmCDT* shmADT;

shmADT new_shm(char* shm_start, sem_t* sem);
int shm_read(char* buff,int n,shmADT shm);
int shm_write(const char* str,shmADT shm);
void free_shm(shmADT shm);

#endif //TP1_SO_SHMADT_H
