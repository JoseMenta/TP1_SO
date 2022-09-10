// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "shmADT.h"

struct shmCDT{
    int index;
    sem_t* sem;
    char* start;
};


// -------------------------------------------------------------------------------------------
// newShm: Crea un nuevo shmADT para manejar al acceso de un shm entre procesos
// -------------------------------------------------------------------------------------------
// Argumentos:
//      shmStart: Puntero con el inicio al shm
//      semaphore: Semaforo utilizado para sincronizar a los procesos que utilizan la shm
// -------------------------------------------------------------------------------------------
// Retorno: shmADT si no hubo error, NULL si ocurrio algun error al reservar espacio
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
// shm_write: Escribe un string en la shm
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      str: El string a guardar en la shm
//      shm: Estructura con la informacion de la shm
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no hubo error, -1 si lo hubo, donde setea errno apropiadamente
// -------------------------------------------------------------------------------------------------------
int shm_write(const char* str,shmADT shm){
    for(int i = 0; str[i]!='\n' && str[i]!='\0';i++,(shm->index)++){
        shm->start[shm->index] = str[i];
        if(sem_post(shm->sem)==-1){
            return -1;
        }
    }
    shm->start[(shm->index)++] = '\n';
    if(sem_post(shm->sem)==-1){
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
    if(shm->start[shm->index]== EOT){
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

