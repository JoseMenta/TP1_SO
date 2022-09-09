//
// Created by Jose Menta on 08/09/2022.
//

#ifndef TP1_SO_SHMADT_H
#define TP1_SO_SHMADT_H
// -------------------------------------------------------------------------------------------------------
// shmADT: ADT que permite controlar la lectura y escritura a un shm compartido por procesos
// -------------------------------------------------------------------------------------------------------
// Notas:
//      Se debe utilizar el mismo shm entre los procesos, indicando el mismo tamaño en ambos
// -------------------------------------------------------------------------------------------------------
typedef struct shmCDT* shmADT;
int shm_read(char* buff,int n,shmADT shm);
int shm_write(const char* str,shmADT shm);
int freeShm(shmADT shm);
#endif //TP1_SO_SHMADT_H
