// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "vista.h"

// Proceso Vista que lee la shm e imprime su datos por stdout
int main(int arg_c, char ** arg_v){
    // FORMATO: /<shared_memory_name>\n
    //          <shared_memory_size>\n
    //          /<sem_name>\n
    char * shared_memory_name = NULL, * shared_memory_size = NULL, * sem_name = NULL;
    size_t shared_memory_name_len = 0, shared_memory_size_len = 0, sem_name_len = 0;
    size_t len;

    char * memory_to_free[3];
    int memory_to_free_len=0;

    switch (arg_c) {
        // Si no se pasa por argumento, se obtiene por entrada estandar
        case 1: {
            if ((len = getline(&shared_memory_name, &shared_memory_name_len, stdin)) == -1) {
                perror("ERROR - Se esperaba recibir el nombre de la shared memory - View");
                exit(1);
            }
            shared_memory_name[len-1]='\0';
            memory_to_free[memory_to_free_len++] = shared_memory_name;

            if ((len = getline(&shared_memory_size, &shared_memory_size_len, stdin)) == -1){
                perror("ERROR - Se esperaba recibir el tamaño de la shared memory - View");
                free_strs(memory_to_free, memory_to_free_len);
                exit(1);
            }
            shared_memory_size[len-1]='\0';

            memory_to_free[memory_to_free_len++] = shared_memory_size;
            if ((len = getline(&sem_name, &sem_name_len, stdin)) == -1) {
                perror("ERROR - Se esperaba recibir el nombre del semaforo - View");
                free_strs(memory_to_free, memory_to_free_len);
                exit(1);
            }
            sem_name[len-1]='\0';
            memory_to_free[memory_to_free_len++] = sem_name;
            break;
        }
        case 4:
            shared_memory_name = arg_v[1];
            shared_memory_size = arg_v[2];
            sem_name = arg_v[3];
            break;
        default:
            fprintf(stderr, "ERROR - Solo se espera recibir tres o ningún argumento - View\n");
            exit(1);
    }

    // Se conecta al semaforo con el cual se va a trabajar en la shared memory
    sem_t * shared_memory_sem = sem_open(sem_name, O_RDWR);
    if(shared_memory_sem == SEM_FAILED){
        perror("Error - Al abrir el semaforo de la shm - View");
        free_strs(memory_to_free, memory_to_free_len);
        exit(1);
    }

    // Nos conectamos a la shared memory creado por el proceso master
    int shared_memory_fd = shm_open(shared_memory_name, O_RDWR, 0);
    if(shared_memory_fd == -1){
        perror("ERROR - Conectando a la shared memory - View");
        free_strs(memory_to_free, memory_to_free_len);
        if(sem_close(shared_memory_sem) == -1){
            perror("ERROR - Cerrando el semaforo - View");
        }
        exit(1);
    }

    // Obtenemos el tamaño de la shm
    char * endptr=NULL;
    size_t shm_size = strtoul(shared_memory_size, &endptr, 10);
    if(shared_memory_size == endptr){
        perror("ERROR - Lectura del tamaño de shm por argumento - View");
        free_strs(memory_to_free, memory_to_free_len);
        if(sem_close(shared_memory_sem) == -1){
            perror("ERROR - Cerrando el semaforo - View");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - View");
        }
        exit(1);
    }

    free_strs(memory_to_free, memory_to_free_len);

    // Mapeamos la shared memory
    void * shared_memory_map = mmap(NULL, shm_size, PROT_READ, MAP_SHARED, shared_memory_fd, 0);
    if(shared_memory_map == MAP_FAILED){
        perror("ERROR - Mapeando la shared memory - View");
        if(sem_close(shared_memory_sem) == -1){
            perror("ERROR - Cerrando el semaforo - View");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - View");
        }
        exit(1);
    }

    // Definimos la estructura de la shared memory con la informacion necesaria
    shmADT shm;
    if((shm = newShm((char*)shared_memory_map,shared_memory_sem) ) == NULL){
        perror("ERROR - Al crear el ADT - View");
        exit(1);
    }
    int ret_value = read_shared_memory_info(shm);

    // Desmapeamos la shared memory y cerramos su file descriptor
    freeShm(shm);
    if(munmap(shared_memory_map, shm_size) == -1){
        perror("ERROR - Desmapeando la shared memory - View");
        if(sem_close(shared_memory_sem) == -1){
            perror("ERROR - Cerrando el semaforo - View");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - View");
        }
        ret_value = -1;
    }

    if(close(shared_memory_fd) == -1){
        perror("ERROR - Cerrando el fd de la shared memory - View");
        if(sem_close(shared_memory_sem) == -1){
            perror("ERROR - Cerrando el semaforo - View");
        }
        ret_value = -1;
    }

    if( sem_close(shared_memory_sem) == -1) {
        perror("ERROR - Cerrando el semaforo de la shm - View");
        ret_value = -1;
    }

    // Finalizamos la ejecucion del proceso vista. Si hubo algun error, retorna 1; si no, retorna 0
    exit(ret_value * -1);
}

// -------------------------------------------------------------------------------------------
// read_shared_memory_info: Dado un puntero a memoria, lee los datos que se encuentren en la shared memory
//                          info y los imprime con formato
// -------------------------------------------------------------------------------------------
// Argumentos:
//      addr: Puntero al inicio de la shared memory info
// -------------------------------------------------------------------------------------------
// Retorno: 0 si no hubo error, -1 si hubo algun error
// -------------------------------------------------------------------------------------------
int read_shared_memory_info(shmADT shm){
    // Formato de la shared memory
    // <arch_1>,<md5_arch_1>,<slave_pid>\n<arch_2>,<md5_arch_2>,<slave_pid>\n...<arch_n>,<md5_arch_n>,<slave_pid>\n\0
    char shm_output[256];
    int status;
    while((status = shm_read(shm_output,256, shm)) == 0){
        printf("%s", shm_output);
    }
    if(status == -1){
        perror("ERROR - Leyendo de la shm - View");
        return -1;
    }
    return 0;
}


// -------------------------------------------------------------------------------------------
// free_strs: Libera la memoria dinamica utilizada para la lectura por STDIN
// -------------------------------------------------------------------------------------------
// Argumentos:
//      memory_to_free: Arreglo con punteros a la memoria dinamica
//      len: Cantidad de punteros a liberar
// -------------------------------------------------------------------------------------------
// Retorno:
// -------------------------------------------------------------------------------------------
void free_strs(char ** memory_to_free, int len){
    for(int i=0; i<len; i++){
        free(memory_to_free[i]);
    }
}