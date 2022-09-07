//
// Created by Jose Menta on 04/09/2022.
//

// feature_test_macro para getline
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int read_shared_memory_info(void * addr);

int main(int arg_c, char ** arg_v){
    // El proceso master crea dos shared memories: Uno de ellos es de tamaño fijo (init), para indicar el tamaño
    // de la otra shared memory donde se encuentra la informacion (info)
    // FORMATO: /<shared_memory_init>\n
    //          /<shared_memory_info>\n
    char * shared_memory_init = NULL;
    char * shared_memory_info = NULL;
    switch (arg_c) {
        // Si no se paso por argumento, se obtiene por entrada estandar
        case 0:
            int shared_memory_init_len = shared_memory_info_len = 0;
            if(getline(&shared_memory_init, &shared_memory_init_len, stdin) == -1){
                perror("ERROR - Se esperaba recibir el nombre de la shared memory init");
                exit(1);
            }
            if(getline(&shared_memory_info_len, &shared_memory_info_len, stdin) == -1){
                perror("ERROR - Se esperaba recibir el nombre de la shared memory info");
                exit(1);
            }
            break;
        // Si se paso por argumento, se toma el primero
        case 2:
            shared_memory_init = arg_v[1];
            shared_memory_info = arg_v[2];
            break;
        // Si se paso mas de un argumento, abortamos
        default:
            fprintf(stderr, "ERROR - Solo se espera recibir uno o ningún argumento\n");
            exit(1);
            break;
    }

    // Nos conectamos a la shared memory init creado por el proceso master
    int shared_memory_init_fd = shm_open(shared_memory_init, O_RDONLY, 0);
    // Liberamos el espacio dedicado para reservar el nombre de la shared memory init (en el caso de recibir por stdin)
    free(shared_memory_init);
    // Si fallo la conexion, abortamos
    if(shared_memory_init_fd == -1){
        free(shared_memory_info);
        perror("ERROR - Conectando a la shared memory init");
        exit(1);
    }

    // Mapeamos la shared memory init. Si falla, abortamos, finalizando correctamente
    void * shared_memory_init_map = mmap(NULL, sizeof(size_t), PROT_READ, MAP_SHARED, shared_memory_init_fd, 0);
    if(shared_memory_init_map == MAP_FAILED){
        perror("ERROR - Mapeando la shared memory init");
        free(shared_memory_info);
        if(close(shared_memory_init_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory init");
        }
        exit(1);
    }

    // Obtenemos el tamaño de la shared memory info
    size_t * shared_memory_info_size = (size_t *) shared_memory_init_map;

    // Desmapeamos la shared memory init y cerramos su file descriptor
    // Si falla, abortamos
    if(munmap(shared_memory_init_map, sizeof(size_t)) == -1){
        perror("ERROR - Desmapeando la shared memory init");
        free(shared_memory_info);
        if(close(shared_memory_init_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory init");
        }
        exit(1);
    }
    if(close(shared_memory_init_fd) == -1){
        perror("ERROR - Cerrando el fd de la shared memory init");
        exit(1);
    }

    // Ahora, ya estamos en condiciones de trabajar con la shared memory principal, info
    // Nos conectamos a la shared memory info creado por el proceso master
    int shared_memory_info_fd = shm_open(shared_memory_info, O_RDONLY, 0);
    // Liberamos el espacio dedicado para reservar el nombre de la shared memory info (en el caso de recibir por stdin)
    free(shared_memory_info);
    // Si fallo la conexion, abortamos
    if(shared_memory_info_fd == -1){
        perror("ERROR - Conectando a la shared memory info");
        exit(1);
    }

    // Mapeamos la shared memory info. Si falla, abortamos, finalizando correctamente
    void * shared_memory_info_map = mmap(NULL, *shared_memory_info_size, PROT_READ, MAP_SHARED, shared_memory_info_fd, 0);
    if(shared_memory_info_map == MAP_FAILED){
        perror("ERROR - Mapeando la shared memory info");
        if(close(shared_memory_info_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory info");
        }
        exit(1);
    }

    // Leemos la shared memory info hasta obtener un error o hasta finalizar
    int ret_value = read_shared_memory_info(shared_memory_info_map);

    // Desmapeamos la shared memory info y cerramos su file descriptor
    // Si falla, abortamos
    if(munmap(shared_memory_info_map, *shared_memory_info_size) == -1){
        perror("ERROR - Desmapeando la shared memory info");
        if(close(shared_memory_info_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory info");
        }
        exit(1);
    }
    if(close(shared_memory_info_fd) == -1){
        perror("ERROR - Cerrando el fd de la shared memory info");
        exit(1);
    }

    // Finalizamos la ejecucion del proceso vista
    exit(ret_value);
}

// -------------------------------------------------------------------------------------------
// read_shared_memory_info: Dado un puntero a memoria, lee los datos que se encuentren en la shared memory
//                          info y los imprime con formato
// -------------------------------------------------------------------------------------------
// Argumentos:
//      addr: Puntero al inicio de la shared memory info
// -------------------------------------------------------------------------------------------
// Retorno: 0 si no hubo error, 1 si hubo algun error
// -------------------------------------------------------------------------------------------
int read_shared_memory_info(void * addr){
    // Formato de la shared memory
    // <read_semaphore><arch_1>;<md5_arch_1>;<slave_pid>\n<arch_2>;<md5_arch_2>;<slave_pid>\n...<arch_n>;<md5_arch_n>;<slave_pid>\n\0
    // read_semaphore: size_t

    void * addr_offset = addr + sizeof(size_t);
    while(1){
        // Logica
        // wait((size_t *)*addr);
        // Si el *addr=1 y el proximo caracter es \0, retorno (finalizo)
        // En cualquier otro caso, leo hasta *addr bytes del buffer o hasta llegar a un \n
        // Si leo un \n, separo el string para imprimir el hasheo del archivo con formato legible
        // Ante cualquier error, perror de lo ocurrido y retorna 1 (NO EXITEAR)
    }
}