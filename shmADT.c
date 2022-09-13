// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "shmADT.h"

struct shmCDT{
    int index;
    sem_t* sem;
    char* start;
};


// -------------------------------------------------------------------------------------------
// new_shm: Crea un nuevo shmADT para manejar el acceso de una shm entre procesos
// -------------------------------------------------------------------------------------------
// Argumentos:
//      shm_start: Puntero con el inicio a la shm
//      semaphore: Semaforo utilizado para sincronizar los procesos que utilizan la shm
// -------------------------------------------------------------------------------------------
// Retorno: shmADT si no hubo error, o NULL si ocurrio algun error al reservar espacio
// -------------------------------------------------------------------------------------------
shmADT new_shm(char* shm_start, sem_t* sem){
    shmADT  ans  = calloc(1,sizeof (struct shmCDT));
    if(ans==NULL){
        return NULL;
    }
    ans->start = shm_start;
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
//      0 si no hubo error, -1 si lo hubo
// -------------------------------------------------------------------------------------------------------
int shm_write(const char* str,shmADT shm){
    for(int i = 0; str[i]!='\0';i++,(shm->index)++){
        shm->start[shm->index] = str[i];
    }
    shm->start[(shm->index)++] = '\0';
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
//      len: la cantidad maxima de caracteres a escribir en buff (incluyendo \n y/o \0)
//      shm: Estructura de la shared memory
// -------------------------------------------------------------------------------------------
// Retorno: 0 si no hubo error, 1 si finalizo la lectura y -1 si hubo error
// -------------------------------------------------------------------------------------------
int shm_read(char* buff,int len,shmADT shm){
    if(sem_wait(shm->sem)==-1){
        return -1;
    }
    int i=0;
    for(;shm->start[shm->index]!='\0' && shm->start[shm->index]!=EOT && i<len-1; i++,(shm->index)++){
        buff[i] = shm->start[shm->index];
    }
    if(shm->start[shm->index]== EOT){
        buff[i] = '\0';
        return 1;
    }
    (shm->index)++;
    buff[i] = '\0';
    return 0;
}

// -------------------------------------------------------------------------------------------------------
// free_shm: Libera los recursos para almacenar la estructura shm (no libera los recursos auxiliares como
//           la shm que se paso o el semaforo)
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
void free_shm(shmADT shm){
    if(shm==NULL){
        return;
    }
    free(shm);
}

