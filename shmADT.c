//
// Created by Jose Menta on 08/09/2022.
//
#include "shmADT.h"
#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
struct shmCDT{
    sem_t* semaphore;
    char* start;
};
#define SEM_NAME "/read_sem"
static int readIndex = 0;
static int writeIndex = 0;

//TODO: ningun caso de error imprime, eso debe manejarse desde quien llama a las funciones

// -------------------------------------------------------------------------------------------
// newShm: Crea un nuevo shmADT para manejar al acceso de un shm entre procesos
// -------------------------------------------------------------------------------------------
// Argumentos:
//      shmStart: Puntero con el inicio al shm
//      semaphore: Semaforo utilizado para sincronizar a los procesos que utilizan la shm
// -------------------------------------------------------------------------------------------
// Retorno: shmADT si no hubo error, NULL si ocurrio algun error al reservar espacio (errno se setea apropiadamente)
// -------------------------------------------------------------------------------------------
shmADT newShm(char* shmStart, sem_t* semaphore){
    shmADT  ans  = calloc(1,sizeof (struct shmCDT));
    if(ans==NULL){
        return NULL;
    }
    ans->start = shmStart;
    ans->semaphore = semaphore;
    return ans;
}
// -------------------------------------------------------------------------------------------------------
// shm_write: Escribe un string en la shm, sincronizando el semaforo con el padre
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      str: El string a guardar en la shm
//      shm: Estructura con la informacion de la shm
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no hubo error, -1 si lo hubo, donde setea errno apropiadamente
// -------------------------------------------------------------------------------------------------------
// Advertencia: TODO: revisar esto
//      Esta funcion no es concurrente, es decir no puede llamarse desde varios threads. En caso de hacerlo,
//      el comportamiento es indefinido (por la variable global writeIndex)
// -------------------------------------------------------------------------------------------------------
int shm_write(const char* str,shmADT shm){
    for(int i = 0; str[i]!='\n' && str[i]!='\0';i++){
        shm->start[writeIndex++] = str[i];
//        *((shm->start)++)=str[ii];
        if(sem_post(shm->semaphore)==-1){
//            perror("ERROR - Al realizar post en el semaforo - Master");
            return -1;
        }
    }
    *((shm->start)++) = '\n';
    if(sem_post(shm->semaphore)==-1){
//        perror("ERROR - Al realizar post en el semaforo - Master");
        return -1;
    }
    return 0;
}
// -------------------------------------------------------------------------------------------
// shm_read: Dado un shmADT y un buffer, almacena la proxima linea guardada en buff
// -------------------------------------------------------------------------------------------
// Argumentos:
//      buff: buffer donde se desea guardar la informacion
//      n: la cantidad maxima de caracteres a escribir en buff (incluyendo \n y/o \0)
//      shm: Estructura de la shared memory
// -------------------------------------------------------------------------------------------
// Retorno: 0 si no hubo error, 1 si hubo error (setea errno apropiadamente)
// -------------------------------------------------------------------------------------------

int shm_read(char* buff,int n,shmADT shm){
    int status = 0, i=0;
    for(;(status = sem_wait(shm->semaphore))==0 && shm->start[readIndex]!='\n' && shm->start[readIndex]!='\0' && i<n-2;i++,readIndex++){
        buff[i] = shm->start[readIndex];
    }
    if(status==-1){
        return -1;
    }
    if(shm->start[readIndex]=='\n'){
        readIndex++;
        buff[i] = '\n';
        buff[i+1] = '\0';
        return 0;
        //Aca escribe hasta el index n-1, es decir n caracteres
    }
    if(shm->start[readIndex]=='\0'){
        buff[i] = '\0';
        return 1;
    }
    return 0;
}
// -------------------------------------------------------------------------------------------------------
// freeShm: Libera los recursos para almacenar la estructura shm (no libera los recursos auxiliares como
//          la shm que se paso o el semaforo)
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      shm: Estructura con la informacion
// -------------------------------------------------------------------------------------------------------
// Retorno:
//     void
// -------------------------------------------------------------------------------------------------------
// Advertencia:
//      Deben liberarse los recursos como la shm o el semaforo que se paso en el constructor aparte
// -------------------------------------------------------------------------------------------------------
void freeShm(shmADT shm){
    if(shm==NULL){
        return;
    }
    free(shm);
}

