//
// Created by Jose Menta on 04/09/2022.
//

// feature_test_macro para getline
#include "vista.h"

int main(int arg_c, char ** arg_v){
    // FORMATO: /<shared_memory_name>\n
    //          <shared_memory_size>\n
    //          /<sem_name>\n
    char * shared_memory_name = NULL;
    char * shared_memory_size = NULL;
    char * sem_name = NULL;
    size_t shared_memory_name_len = 0, shared_memory_size_len = 0, sem_name_len = 0;
    int data_received = -1;
    size_t len;
    switch (arg_c) {
        // Si no se pasa por argumento, se obtiene por entrada estandar
        case 1: {
            data_received = STDIN;
            if ((len = getline(&shared_memory_name, &shared_memory_name_len, stdin)) == -1) {
                perror("ERROR - Se esperaba recibir el nombre de la shared memory - View");
                exit(1);
            }
            shared_memory_name[len-1]='\0';

            if ((len = getline(&shared_memory_size, &shared_memory_size_len, stdin)) == -1){
                perror("ERROR - Se esperaba recibir el tamaño de la shared memory - View");
                free(shared_memory_name);
                exit(1);
            }
            shared_memory_size[len-1]='\0';

            if ((len = getline(&sem_name, &sem_name_len, stdin)) == -1) {
                perror("ERROR - Se esperaba recibir el nombre del semaforo - View");
                free(shared_memory_name);
                free(shared_memory_size);
                exit(1);
            }
            sem_name[len-1]='\0';
            break;
        }
        // Si se paso por argumento, se toma el primero
        case 4:
            data_received = ARGS;
            shared_memory_name = arg_v[1];
            shared_memory_size = arg_v[2];
            sem_name = arg_v[3];
            break;
        // En otro caso, se aborta
        default:
            fprintf(stderr, "ERROR - Solo se espera recibir tres o ningún argumento - View\n");
            exit(1);
    }

    // Se conecta al semaforo con el cual se va a trabajar en la shared memory
    sem_t * shared_memory_sem = sem_open(sem_name, O_RDWR);
    // Si la conexion falla, abortamos
    if(shared_memory_sem == SEM_FAILED){
        perror("Error - Al abrir el semaforo de la shm - View");
        if(data_received == STDIN){
            free(shared_memory_name);
            free(shared_memory_size);
            free(sem_name);
        }
        exit(1);
    }
    if(data_received == STDIN){
        free(sem_name);
    }

    // Nos conectamos a la shared memory creado por el proceso master
    int shared_memory_fd = shm_open(shared_memory_name, O_RDWR, 0);
    // Liberamos el espacio dedicado para reservar el nombre de la shared memory (en el caso de recibir por stdin)
    if(data_received == STDIN){
        free(shared_memory_name);
    }
    // Si fallo la conexion, abortamos
    if(shared_memory_fd == -1){
        perror("ERROR - Conectando a la shared memory - View");
        if(data_received == STDIN){
            free(shared_memory_size);
        }
        if(sem_close(shared_memory_sem) == -1){
            perror("ERROR - Cerrando el semaforo - View");
        }
        exit(1);
    }

    // Obtenemos el tamaño de la shm
    char * endptr=NULL;
    size_t shm_size = strtoul(shared_memory_size, &endptr, 10);
    // Si fallo la conversion, aborta
    if(shared_memory_size == endptr){
        perror("ERROR - Lectura del tamaño de shm por argumento - View");
        if(data_received == STDIN){
            free(shared_memory_size);
        }
        if(sem_close(shared_memory_sem) == -1){
            perror("ERROR - Cerrando el semaforo - View");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - View");
        }
        exit(1);
    }

    if(data_received == STDIN){
        free(shared_memory_size);
    }

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
    shmADT shm = newShm((char*)shared_memory_map,shared_memory_sem);
//    shm_struct shm;
//    shm.index = 0;
//    shm.semaphore = shared_memory_sem;
//    shm.start = (char*) shared_memory_map;
    int ret_value = read_shared_memory_info(shm);

    // Desmapeamos la shared memory y cerramos su file descriptor
    // Si falla, abortamos
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
    // Finalizamos la ejecucion del proceso vista
    // Si hubo algun error, retorna 1; si no, retorna 0
    exit(ret_value * -1);
}

// -------------------------------------------------------------------------------------------
// shm_read: Dado el puntero a la shared memory y un string, almacena el proximo hasheo de
//           archivo en el string
// -------------------------------------------------------------------------------------------
// Argumentos:
//      buff: String
//      shm: Estructura de la shared memory
// -------------------------------------------------------------------------------------------
// Retorno: 0 si no hubo error y no encontro '\0', 1 si encontro el '\0' o -1 si hubo un error
// -------------------------------------------------------------------------------------------
//TODO: agregar manejo de limites para escribir
//int shm_read(char* buff,  shm){
//    int status = 0, i=0;
//    for(;(status = sem_wait(shm->semaphore))==0 && shm->start[shm->index]!='\n' && shm->start[shm->index]!='\0';i++,(shm->index)++){
//        buff[i] = shm->start[shm->index];
//    }
//    if(status==-1){
//        perror("ERROR - Al realizar wait para el semaforo - View");
//        return -1;
//    }
//    if(shm->start[shm->index]=='\n'){
//        (shm->index)++;
//        buff[i] = '\n';
//        buff[i+1] = '\0';
//        return 0;
//    }
//    //if(shm->start[shm->index]==EOT)
//    if(shm->start[shm->index]=='\0'){
//        buff[i] = '\0';
//        return 1;
//    }
//    return 0;
//}

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