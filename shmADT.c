//
// Created by Jose Menta on 08/09/2022.
//
#include "shmADT.h"
#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
#include <errno.h>
struct shmCDT{
    int index;
    sem_t* sem;
    char* start;
};
//#define SEM_NAME "/read_sem"
//static int readIndex = 0;
//static int writeIndex = 0;
//sem_t* sem = NULL;
//TODO: ningun caso de error imprime, eso debe manejarse desde quien llama a las funciones
//No me sirve hacerlo como variables globales, procesos separados tienen acceso a memorias distintas y por lo tanto a distintas variables globales

// -------------------------------------------------------------------------------------------------------
// new_sync: Crea el semaforo que se utiliza para sincronizar el acceso a la shm
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no hubo error, -1 si lo hubo, donde setea errno apropiadamente
// -------------------------------------------------------------------------------------------------------
// Advertencia:
//      Esta funcion solo debe ser llamada por uno de los procesos que van a utilizar el ADT
// -------------------------------------------------------------------------------------------------------
//int new_sync(){
//    sem = sem_open(SEM_NAME,O_RDWR|O_CREAT,S_IRWXU,0);
//    if(sem==SEM_FAILED){
//        return -1;
//    }
//}
// -------------------------------------------------------------------------------------------
// newShm: Crea un nuevo shmADT para manejar al acceso de un shm entre procesos
// -------------------------------------------------------------------------------------------
// Argumentos:
//      shmStart: Puntero con el inicio al shm
//      semaphore: Semaforo utilizado para sincronizar a los procesos que utilizan la shm
// -------------------------------------------------------------------------------------------
// Retorno: shmADT si no hubo error, NULL si ocurrio algun error al reservar espacio o crear el semaforo (errno se setea apropiadamente)
// -------------------------------------------------------------------------------------------
shmADT newShm(char* shmStart, sem_t* sem){
    shmADT  ans  = calloc(1,sizeof (struct shmCDT));
    if(ans==NULL){
        return NULL;
    }
    ans->start = shmStart;
    ans->sem = sem;
    ans->index = 0;
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
//TODO: cuando cambiemos el caracter de finalizacion, hacer que el primer ciclo termine con '\0'
int shm_write(const char* str,shmADT shm){
    for(int i = 0; str[i]!='\n' && str[i]!='\0';i++,(shm->index)++){
        shm->start[shm->index] = str[i];
//        *((shm->start)++)=str[ii];
        if(sem_post(shm->sem)==-1){
//            perror("ERROR - Al realizar post en el semaforo - Master");
            return -1;
        }
    }
    shm->start[(shm->index)++] = '\n';
    if(sem_post(shm->sem)==-1){
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
    for(;(status = sem_wait(shm->sem))==0 && shm->start[shm->index]!='\n' && shm->start[shm->index]!='\0' && shm->start[shm->index]!=EOT && i<n-2;i++,(shm->index)++){
        buff[i] = shm->start[shm->index];
    }
    if(status==-1){
        return -1;
    }
    if(shm->start[shm->index]=='\n'){
        (shm->index)++;
        buff[i] = '\n';
        buff[i+1] = '\0';
        return 0;
        //Aca escribe hasta el index n-1, es decir n caracteres
    }
    //TODO: cambiar por EOT
    if(shm->start[shm->index]==EOT){
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
//    void
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
// -------------------------------------------------------------------------------------------------------
// free_sync: Libera al semaforo que se utiliza para sincronizar el acceso a la shm
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no hubo error, -1 si lo hubo, donde setea errno apropiadamente
// -------------------------------------------------------------------------------------------------------
// Advertencia:
//      Esta funcion solo debe ser llamada por uno de los procesos que van a utilizar el ADT
// -------------------------------------------------------------------------------------------------------
//int free_sync(){
//    int ans = 0;
//    if(sem_close(sem)==-1){
//         ans = -1;
//    }
//    if(sem_unlink(SEM_NAME)==-1){
//        //En el caso donde se cierra por segunda vez, no consideramos que tenga error
//        ans = -1;
//    }
//    return ans;
//}

