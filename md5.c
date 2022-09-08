//
// Created by Jose Menta on 04/09/2022.
//

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

#define SLAVES 5
#define SHM_NAME "/shm"
#define READ_SEM "/read_semaphore"
#define SHM_ELEMENT_SIZE 128
#define SHM_SIZE(elements) (sizeof(char) * SHM_ELEMENT_SIZE * elements + 1)
#define SLEEP_TIME 15

typedef struct {
    int index;
    sem_t* semaphore;
    char* start;
} shm_struct;

int write_to_slave(int fd, const char * file_path);
int is_file(const char * file_path);
int shm_write(const char* str,shm_struct* shm);

int main(int arg_c, char** arg_v){
    // Recibe por argumentos los nombres de los archivos que se desean procesar
    // Si no recibe ninguno, finaliza
    if(arg_c <= 1){
        perror("ERROR - No se recibieron archivos - Master");
        exit(1);
    }


    // Creamos el archivo
    FILE * resultado_file;
    // Si falla, abortamos
    if((resultado_file = fopen("./resultado.csv", "w+")) == NULL){
        perror("ERROR - No se pudo abrir archivo resultado - Master");
        exit(1);
    }

    //TODO: liberar a resultado_file

    // Creacion de share memory con driver
    char * shared_memory = SHM_NAME;
    int shared_memory_fd = shm_open(shared_memory, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

    // Si falla la creacion, abortamos
    if(shared_memory_fd == -1){
        perror("ERROR - Creacion de shm - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    // Asignamos el espacio para la shm
    // Si falla, abortamos
    if(ftruncate(shared_memory_fd, SHM_SIZE(arg_c-1)) == -1){
        perror("ERROR - Falló la asignacion de tamaño en la shared memory - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    // Mapeamos la shared memory init
    // Si falla, abortamos
    void * shared_memory_map = mmap(NULL, SHM_SIZE(arg_c-1), PROT_WRITE, MAP_SHARED, shared_memory_fd, 0);
    if(shared_memory_map == MAP_FAILED){
        perror("ERROR - Mapeando la shared memory init - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    // TODO: Liberar la shm

    //Crea el semaforo para sincronizar la lectura y escritura entre md5 y vista
    sem_t * read_sem = sem_open(READ_SEM,O_RDWR|O_CREAT|O_EXCL,S_IRWXU,0);
    // Si la creacion fallo, abortamos
    if(read_sem==SEM_FAILED){
        perror("ERROR - Al abrir el semaforo para lectura - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
            perror("ERROR - Desmapeando la shared memory - Master");
        }
        if(shm_unlink(SHM_NAME)==-1){
            perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    // Imprimimos por pantalla el nombre identificador de la shm, el tamaño del mismo y el nombre del semafor
    // Si falla, abortamos
    if(printf("%s\n", SHM_NAME) < 0){
        perror("ERROR - Al escribir por pantalla - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(sem_close(read_sem)==-1){
            perror("ERROR - Cerrando el semaforo - Master");
        }
        if(sem_unlink(READ_SEM)==-1){
            perror("ERROR - Haciendo unlink del semaforo - Master");
        }
        if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
            perror("ERROR - Desmapeando la shared memory - Master");
        }
        if(shm_unlink(SHM_NAME)==-1){
            perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    if(printf("%lu\n", SHM_SIZE(arg_c-1)) < 0){
        perror("ERROR - Al escribir por pantalla - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(sem_close(read_sem)==-1){
            perror("ERROR - Cerrando el semaforo - Master");
        }
        if(sem_unlink(READ_SEM)==-1){
            perror("ERROR - Haciendo unlink del semaforo - Master");
        }
        if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
            perror("ERROR - Desmapeando la shared memory - Master");
        }
        if(shm_unlink(SHM_NAME)==-1){
            perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    if(printf("%s\n", READ_SEM) < 0){
        perror("ERROR - Al escribir por pantalla - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(sem_close(read_sem)==-1){
            perror("ERROR - Cerrando el semaforo - Master");
        }
        if(sem_unlink(READ_SEM)==-1){
            perror("ERROR - Haciendo unlink del semaforo - Master");
        }
        if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
            perror("ERROR - Desmapeando la shared memory - Master");
        }
        if(shm_unlink(SHM_NAME)==-1){
            perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    sleep(SLEEP_TIME);

    // Definimos la estructura de la shared memory con la informacion necesaria
    shm_struct shm;
    shm.index = 0;
    shm.semaphore = read_sem;
    shm.start = (char*) shared_memory_map;


    //TODO: sacar, es para desarrollo
    setvbuf(stdout, NULL, _IONBF, 0);

    // Creamos un arreglo que almacene los fd que leen de slaves
    int read_fd[SLAVES];
    // Creamos un arreglo que almacene los fd que escriban a los slaves
    int write_fd[SLAVES];

    // Creamos los slaves y les configuramos los pipes para leer y escribir
    for(int i = 0; i < SLAVES; i++){
        //Creamos los pipes para comunicarse con los procesos slave
        // pipe[0]: extremo para leer
        // pipe[1]: extremo para escribir
        int mtos_pipe[2]; //Va de Master a Slave
        int stom_pipe[2]; //Va de Slave a Master
        if(pipe(mtos_pipe) == -1){
            perror("ERROR - Al crear el pipe de Master a Slave - Master");
            /// ------------------------------------------------------------
            if(fclose(resultado_file) == EOF){
                perror("ERROR - Cerrando el archivo resultado.csv - Master");
            }
            if(sem_close(read_sem)==-1){
                perror("ERROR - Cerrando el semaforo - Master");
            }
            if(sem_unlink(READ_SEM)==-1){
                perror("ERROR - Haciendo unlink del semaforo - Master");
            }
            if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                perror("ERROR - Desmapeando la shared memory - Master");
            }
            if(shm_unlink(SHM_NAME)==-1){
                perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
            }
            if(close(shared_memory_fd) == -1){
                perror("ERROR - Cerrando el fd de la shared memory - Master");
            }
            for(int j = 0; j < i; j++){
                if(close(read_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de lectura - Master");
                }
                if(close(write_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de escritura - Master");
                }
            }
            /// ------------------------------------------------------------
            exit(1);
        }
        if(pipe(stom_pipe) == -1){
            perror("ERROR - Al crear el pipe de Slave a Master - Master");
            /// ------------------------------------------------------------
            if(fclose(resultado_file) == EOF){
                perror("ERROR - Cerrando el archivo resultado.csv - Master");
            }
            if(sem_close(read_sem)==-1){
                perror("ERROR - Cerrando el semaforo - Master");
            }
            if(sem_unlink(READ_SEM)==-1){
                perror("ERROR - Haciendo unlink del semaforo - Master");
            }
            if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                perror("ERROR - Desmapeando la shared memory - Master");
            }
            if(shm_unlink(SHM_NAME)==-1){
                perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
            }
            if(close(shared_memory_fd) == -1){
                perror("ERROR - Cerrando el fd de la shared memory - Master");
            }
            for(int j = 0; j < i; j++){
                if(close(read_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de lectura - Master");
                }
                if(close(write_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de escritura - Master");
                }
            }
            if(close(mtos_pipe[0]) == -1){
                perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Master - Master");
            }
            if(close(mtos_pipe[1]) == -1){
                perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Master - Master");
            }
            /// ------------------------------------------------------------
            exit(1);
        }

        // Creamos el proceso hijo Slave
        pid_t new_pid = fork();
        switch (new_pid) {
            // En caso de error, abortamos
            case -1:
            {
                perror("ERROR - Creación del proceso Slave - Master");
                /// ------------------------------------------------------------
                if(fclose(resultado_file) == EOF){
                    perror("ERROR - Cerrando el archivo resultado.csv - Master");
                }
                if(sem_close(read_sem)==-1){
                    perror("ERROR - Cerrando el semaforo - Master");
                }
                if(sem_unlink(READ_SEM)==-1){
                    perror("ERROR - Haciendo unlink del semaforo - Master");
                }
                if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                    perror("ERROR - Desmapeando la shared memory - Master");
                }
                if(shm_unlink(SHM_NAME)==-1){
                    perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                }
                if(close(shared_memory_fd) == -1){
                    perror("ERROR - Cerrando el fd de la shared memory - Master");
                }
                for(int j = 0; j < i; j++){
                    if(close(read_fd[j]) == -1){
                        perror("ERROR - Al cerrar un fd de lectura - Master");
                    }
                    if(close(write_fd[j]) == -1){
                        perror("ERROR - Al cerrar un fd de escritura - Master");
                    }
                }
                if(close(mtos_pipe[0]) == -1){
                    perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Master - Master");
                }
                if(close(mtos_pipe[1]) == -1){
                    perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Master - Master");
                }
                if(close(stom_pipe[0]) == -1){
                    perror("ERROR - Al cerrar el extremo de lectura del pipe stom en Master - Master");
                }
                if(close(stom_pipe[1]) == -1){
                    perror("ERROR - Al cerrar el extremo de lectura del pipe stom en Master - Master");
                }
                /// ------------------------------------------------------------
                exit(1);
            }
            // El proceso hijo se convierte en el proceso slave
            case 0:
            {
                // TODO: Mover la creacion de slaves al principio para no tener que realizar todo este manejo de cierre
                /// ------------------------------------------------------------
                int error = 0;
                if(fclose(resultado_file) == EOF){
                    perror("ERROR - Cerrando el archivo resultado.csv - Slave");
                    error = 1;
                }
                if(sem_close(read_sem)==-1){
                    perror("ERROR - Cerrando el semaforo - Slave");
                    error = 1;
                }
                if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                    perror("ERROR - Desmapeando la shared memory - Slave");
                    error = 1;
                }
                if(close(shared_memory_fd) == -1){
                    perror("ERROR - Cerrando el fd de la shared memory - Slave");
                    error = 1;
                }
                for(int j = 0; j < i; j++){
                    if(close(read_fd[j]) == -1){
                        perror("ERROR - Al cerrar un fd de lectura - Slave");
                        error = 1;
                    }
                    if(close(write_fd[j]) == -1){
                        perror("ERROR - Al cerrar un fd de escritura - Slave");
                        error = 1;
                    }
                }
                if(error){
                    exit(1);
                }
                /// ------------------------------------------------------------
                //Cerramos el extremo de escritura del pipe mtos
                if(close(mtos_pipe[1])== -1){
                    perror("ERROR - Cerrar el extremo de escritura del pipe mtos en Slave - Slave");
                    /// ------------------------------------------------------------
                    if(close(mtos_pipe[0]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Slave - Slave");
                    }
                    if(close(stom_pipe[0]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe stom en Slave - Slave");
                    }
                    if(close(stom_pipe[1]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe stom en Slave - Slave");
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }
                //Cerramos el extremo de lectura del pipe stom
                if(close(stom_pipe[0])== -1){
                    perror("ERROR - Cerrar el extremo de lectura del pipe stom en Slave - Slave");
                    /// ------------------------------------------------------------
                    if(close(mtos_pipe[0]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Slave - Slave");
                    }
                    if(close(stom_pipe[1]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe stom en Slave - Slave");
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }
                // Llevamos a STDIN el extremo de lectura de mtos
                if(dup2(mtos_pipe[0], STDIN_FILENO)== -1){
                    perror("ERROR - Duplicar STDIN_FILENO en lectura del pipe mtos en Slave - Slave");
                    /// ------------------------------------------------------------
                    if(close(mtos_pipe[0]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Slave - Slave");
                    }
                    if(close(stom_pipe[1]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe stom en Slave - Slave");
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }
                // Llevamos a STDOUT el extremo de escritura de stom
                if(dup2(stom_pipe[1], STDOUT_FILENO)== -1){
                    perror("ERROR - Duplicar STDOUT en escritura del pipe stom en Slave - Slave");
                    /// ------------------------------------------------------------
                    if(close(mtos_pipe[0]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Slave - Slave");
                    }
                    if(close(stom_pipe[1]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe stom en Slave - Slave");
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }
                // Cerramos el extremo de lectura del pipe mtos
                if(close(mtos_pipe[0])== -1){
                    perror("ERROR - Cerrar el extremo de lectura del pipe mtos en Slave - Slave");
                    /// ------------------------------------------------------------
                    if(close(stom_pipe[1]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe stom en Slave - Slave");
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }
                // Cerramos el extremo de escritura del pipe stom
                if(close(stom_pipe[1])== -1){
                    perror("ERROR: Cerrar el extremo de escritura del pipe stom en Slave - Slave");
                    exit(1);
                }
                // Ejecutamos el proceso slave
                if(execl("slave", "./slave", NULL)==-1){
                    perror("ERROR: Al crear el proceso slave - Slave");
                    exit(1);
                }
            }
            // EL proceso padre se queda con los fd requeridos
            default:
            {
                // Cerramos el extremo de escritura del pipe stom
                if(close(stom_pipe[1])==-1){
                    perror("ERROR: Cerrar el extremo de escritura del pipe stom en Master - Master");
                    /// ------------------------------------------------------------
                    if(fclose(resultado_file) == EOF){
                        perror("ERROR - Cerrando el archivo resultado.csv - Master");
                    }
                    if(sem_close(read_sem)==-1){
                        perror("ERROR - Cerrando el semaforo - Master");
                    }
                    if(sem_unlink(READ_SEM)==-1){
                        perror("ERROR - Haciendo unlink del semaforo - Master");
                    }
                    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                        perror("ERROR - Desmapeando la shared memory - Master");
                    }
                    if(shm_unlink(SHM_NAME)==-1){
                        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                    }
                    if(close(shared_memory_fd) == -1){
                        perror("ERROR - Cerrando el fd de la shared memory - Master");
                    }
                    for(int j = 0; j < i; j++){
                        if(close(read_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de lectura - Master");
                        }
                        if(close(write_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de escritura - Master");
                        }
                    }
                    if(close(mtos_pipe[0]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Master - Master");
                    }
                    if(close(mtos_pipe[1]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Master - Master");
                    }
                    if(close(stom_pipe[0]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe stom en Master - Master");
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }
                // Cerramos el extremo de lectura del pipe mtos
                if(close(mtos_pipe[0])==-1){
                    perror("ERROR: Cerrar el extremo de lectura del pipe mtos en Master - Master");
                    /// ------------------------------------------------------------
                    if(fclose(resultado_file) == EOF){
                        perror("ERROR - Cerrando el archivo resultado.csv - Master");
                    }
                    if(sem_close(read_sem)==-1){
                        perror("ERROR - Cerrando el semaforo - Master");
                    }
                    if(sem_unlink(READ_SEM)==-1){
                        perror("ERROR - Haciendo unlink del semaforo - Master");
                    }
                    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                        perror("ERROR - Desmapeando la shared memory - Master");
                    }
                    if(shm_unlink(SHM_NAME)==-1){
                        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                    }
                    if(close(shared_memory_fd) == -1){
                        perror("ERROR - Cerrando el fd de la shared memory - Master");
                    }
                    for(int j = 0; j < i; j++){
                        if(close(read_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de lectura - Master");
                        }
                        if(close(write_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de escritura - Master");
                        }
                    }
                    if(close(mtos_pipe[1]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe mtos en Master - Master");
                    }
                    if(close(stom_pipe[0]) == -1){
                        perror("ERROR - Al cerrar el extremo de lectura del pipe stom en Master - Master");
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }
                // Almacenamos los pipes de lectura y escritura para el master
                read_fd[i] = stom_pipe[0];
                write_fd[i] = mtos_pipe[1];
            }
        }
    }


    // Guarda la cantidad de archivos a hashear (si aparece un directorio, es un archivo menos a hashear)
    int count_arch = arg_c - 1;
    // Indica la cantidad de archivos que faltan hashear
//    int arch_to_hash_left = count_arch;
    // Indica la cantidad de archivos ya hasheados
    int arch_already_hashed = 0;
    // Indica el proximo archivo a hashear
    int arg_index = 1;

    // Le asginamos un archivo a cada slave inicialmente
    for(int i = 0; i<SLAVES && arg_index<arg_c;i++){
        // Primero hay que comprobar que el path a pasar sea un archivo y queden archivos disponibles para hashear
        int status = 0;
        while(arg_index<arg_c && (status = is_file(arg_v[arg_index])) == 0){
            // Dado que es un directorio, no debe considerarse como archivo
            count_arch--;
            arg_index++;
        }
        // Error en lectura de stat
        if(status==-1){
            perror("ERROR - Revisando si el path recibido es un archivo - Master");
            /// ------------------------------------------------------------
            if(fclose(resultado_file) == EOF){
                perror("ERROR - Cerrando el archivo resultado.csv - Master");
            }
            if(sem_close(read_sem)==-1){
                perror("ERROR - Cerrando el semaforo - Master");
            }
            if(sem_unlink(READ_SEM)==-1){
                perror("ERROR - Haciendo unlink del semaforo - Master");
            }
            if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                perror("ERROR - Desmapeando la shared memory - Master");
            }
            if(shm_unlink(SHM_NAME)==-1){
                perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
            }
            if(close(shared_memory_fd) == -1){
                perror("ERROR - Cerrando el fd de la shared memory - Master");
            }
            for(int j = 0; j < SLAVES; j++){
                if(close(read_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de lectura - Master");
                }
                if(close(write_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de escritura - Master");
                }
            }
            /// ------------------------------------------------------------
            exit(1);
        }
        // Se obtuvo un archivo o ya no quedan paths por leer
        if(arg_index<arg_c) {
            if(write_to_slave(write_fd[i], arg_v[arg_index]) == -1){
                perror("ERROR - Enviando el path al proceso esclavo");
                /// ------------------------------------------------------------
                if(fclose(resultado_file) == EOF){
                    perror("ERROR - Cerrando el archivo resultado.csv - Master");
                }
                if(sem_close(read_sem)==-1){
                    perror("ERROR - Cerrando el semaforo - Master");
                }
                if(sem_unlink(READ_SEM)==-1){
                    perror("ERROR - Haciendo unlink del semaforo - Master");
                }
                if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                    perror("ERROR - Desmapeando la shared memory - Master");
                }
                if(shm_unlink(SHM_NAME)==-1){
                    perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                }
                if(close(shared_memory_fd) == -1){
                    perror("ERROR - Cerrando el fd de la shared memory - Master");
                }
                for(int j = 0; j < SLAVES; j++){
                    if(close(read_fd[j]) == -1){
                        perror("ERROR - Al cerrar un fd de lectura - Master");
                    }
                    if(close(write_fd[j]) == -1){
                        perror("ERROR - Al cerrar un fd de escritura - Master");
                    }
                }
                /// ------------------------------------------------------------
                exit(1);
            }
            arg_index++;
        }
    }
    // Iremos enviando el path de los archivos a hashear y leyendo los hasheos
    while(arch_already_hashed < count_arch){
        // Configuramos los conjuntos con los elementos de lectura
        fd_set read_fd_set;
        FD_ZERO(&read_fd_set);
        // Creamos un conjunto de fd para los pipes de escritura
        //fd_set write_fd_set;
        //FD_ZERO(&write_fd_set);
        int max_fd = 0;
        //Obtenemos el maximo fd, para usar con select
        for(int i = 0; i < SLAVES; i++){
            max_fd = (read_fd[i]>max_fd)?read_fd[i]:max_fd;
            FD_SET(read_fd[i], &read_fd_set);
            //max = (write_fd[i]>max)?write_fd[i]:max;
            //FD_SET(write_fd[i], &write_fd_set);
        }
        // Un solo select con sets de lectura para saber cuando se retorna el md5
        // los file descriptors disponibles para leer/escribir
        if(select(max_fd+1, &read_fd_set,  NULL, NULL, NULL) == -1){
            perror("ERROR - Al realizar select() - Master");
            /// ------------------------------------------------------------
            if(fclose(resultado_file) == EOF){
                perror("ERROR - Cerrando el archivo resultado.csv - Master");
            }
            if(sem_close(read_sem)==-1){
                perror("ERROR - Cerrando el semaforo - Master");
            }
            if(sem_unlink(READ_SEM)==-1){
                perror("ERROR - Haciendo unlink del semaforo - Master");
            }
            if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                perror("ERROR - Desmapeando la shared memory - Master");
            }
            if(shm_unlink(SHM_NAME)==-1){
                perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
            }
            if(close(shared_memory_fd) == -1){
                perror("ERROR - Cerrando el fd de la shared memory - Master");
            }
            for(int j = 0; j < SLAVES; j++){
                if(close(read_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de lectura - Master");
                }
                if(close(write_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de escritura - Master");
                }
            }
            /// ------------------------------------------------------------
            exit(1);
        }
        // Repaso todos los fd de lectura para saber cuales se pueden leer (tienen un archivo hasheado)
        for(int i = 0; i<SLAVES ; i++){
            if(FD_ISSET(read_fd[i], &read_fd_set)){
                // Si esta disponible este fd para leer, leo el hash md5 obtenido
                // Primero creo un FILE * para poder leer hasta \n
                FILE * read_file = fdopen(read_fd[i], "r");
                if(read_file == NULL){
                    perror("ERROR - Al intentar abrir el fd read - Master");
                    /// ------------------------------------------------------------
                    if(fclose(resultado_file) == EOF){
                        perror("ERROR - Cerrando el archivo resultado.csv - Master");
                    }
                    if(sem_close(read_sem)==-1){
                        perror("ERROR - Cerrando el semaforo - Master");
                    }
                    if(sem_unlink(READ_SEM)==-1){
                        perror("ERROR - Haciendo unlink del semaforo - Master");
                    }
                    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                        perror("ERROR - Desmapeando la shared memory - Master");
                    }
                    if(shm_unlink(SHM_NAME)==-1){
                        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                    }
                    if(close(shared_memory_fd) == -1){
                        perror("ERROR - Cerrando el fd de la shared memory - Master");
                    }
                    for(int j = 0; j < SLAVES; j++){
                        if(close(read_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de lectura - Master");
                        }
                        if(close(write_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de escritura - Master");
                        }
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }
                char * arch_hash = NULL;
                size_t arch_hash_len = 0;
                // Lectura del fd hasta \n (printf termina en \n)
                if(getline(&arch_hash, &arch_hash_len, read_file) == -1) {
                    perror("ERROR - Al leer del slave - Master");
                    /// ------------------------------------------------------------
                    if(fclose(resultado_file) == EOF){
                        perror("ERROR - Cerrando el archivo resultado.csv - Master");
                    }
                    if(sem_close(read_sem)==-1){
                        perror("ERROR - Cerrando el semaforo - Master");
                    }
                    if(sem_unlink(READ_SEM)==-1){
                        perror("ERROR - Haciendo unlink del semaforo - Master");
                    }
                    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                        perror("ERROR - Desmapeando la shared memory - Master");
                    }
                    if(shm_unlink(SHM_NAME)==-1){
                        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                    }
                    if(close(shared_memory_fd) == -1){
                        perror("ERROR - Cerrando el fd de la shared memory - Master");
                    }
                    for(int j = 0; j < SLAVES; j++){
                        if(close(read_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de lectura - Master");
                        }
                        if(close(write_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de escritura - Master");
                        }
                    }
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }

                // Dado que ya finalizo hasheando el archivo pasado anteriormente, podemos pasarle el proximo archivo disponible
                // Escribimos al proceso esclavo que nos devolvio el resultado

                // Primero hay que comprobar que el path a pasar sea un archivo y queden archivos disponibles para hashear
                int status = 0;
                while(arg_index<arg_c && (status = is_file(arg_v[arg_index])) == 0){
                    arg_index++;
                    // Dado que es un directorio, no debe considerarse como archivo
                    count_arch--;
                }
                // Error en lectura de stat
                if(status==-1){
                    perror("ERROR - Revisando si el path recibido es un archivo - Master");
                    /// ------------------------------------------------------------
                    if(fclose(resultado_file) == EOF){
                        perror("ERROR - Cerrando el archivo resultado.csv - Master");
                    }
                    if(sem_close(read_sem)==-1){
                        perror("ERROR - Cerrando el semaforo - Master");
                    }
                    if(sem_unlink(READ_SEM)==-1){
                        perror("ERROR - Haciendo unlink del semaforo - Master");
                    }
                    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                        perror("ERROR - Desmapeando la shared memory - Master");
                    }
                    if(shm_unlink(SHM_NAME)==-1){
                        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                    }
                    if(close(shared_memory_fd) == -1){
                        perror("ERROR - Cerrando el fd de la shared memory - Master");
                    }
                    for(int j = 0; j < SLAVES; j++){
                        if(close(read_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de lectura - Master");
                        }
                        if(close(write_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de escritura - Master");
                        }
                    }
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    free(arch_hash);
                    /// ------------------------------------------------------------
                    exit(1);
                }
                // Se obtuvo un archivo o ya no quedan paths por leer
                if(arg_index<arg_c) {
                    if(write_to_slave(write_fd[i], arg_v[arg_index]) == -1){
                        perror("ERROR - Enviando el path al proceso esclavo");
                        /// ------------------------------------------------------------
                        if(fclose(resultado_file) == EOF){
                            perror("ERROR - Cerrando el archivo resultado.csv - Master");
                        }
                        if(sem_close(read_sem)==-1){
                            perror("ERROR - Cerrando el semaforo - Master");
                        }
                        if(sem_unlink(READ_SEM)==-1){
                            perror("ERROR - Haciendo unlink del semaforo - Master");
                        }
                        if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                            perror("ERROR - Desmapeando la shared memory - Master");
                        }
                        if(shm_unlink(SHM_NAME)==-1){
                            perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                        }
                        if(close(shared_memory_fd) == -1){
                            perror("ERROR - Cerrando el fd de la shared memory - Master");
                        }
                        for(int j = 0; j < SLAVES; j++){
                            if(close(read_fd[j]) == -1){
                                perror("ERROR - Al cerrar un fd de lectura - Master");
                            }
                            if(close(write_fd[j]) == -1){
                                perror("ERROR - Al cerrar un fd de escritura - Master");
                            }
                        }
                        if(fclose(read_file) == EOF){
                            perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                        }
                        free(arch_hash);
                        /// ------------------------------------------------------------
                        exit(1);
                    }
                    arg_index++;
                }

                // Imprimo el hasheo del archivo recibido
                //if(printf("%s", arch_hash) < 0){
                if(fprintf(resultado_file,"%s", arch_hash) < 0){
                    perror("ERROR - Al escribir el hasheo en resultados.csv - Master");
                    /// ------------------------------------------------------------
                    if(fclose(resultado_file) == EOF){
                        perror("ERROR - Cerrando el archivo resultado.csv - Master");
                    }
                    if(sem_close(read_sem)==-1){
                        perror("ERROR - Cerrando el semaforo - Master");
                    }
                    if(sem_unlink(READ_SEM)==-1){
                        perror("ERROR - Haciendo unlink del semaforo - Master");
                    }
                    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                        perror("ERROR - Desmapeando la shared memory - Master");
                    }
                    if(shm_unlink(SHM_NAME)==-1){
                        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                    }
                    if(close(shared_memory_fd) == -1){
                        perror("ERROR - Cerrando el fd de la shared memory - Master");
                    }
                    for(int j = 0; j < SLAVES; j++){
                        if(close(read_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de lectura - Master");
                        }
                        if(close(write_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de escritura - Master");
                        }
                    }
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    free(arch_hash);
                    /// ------------------------------------------------------------
                    exit(1);
                }
                // Escribo dentro de shm con funcion de driver
                if(shm_write(arch_hash, &shm ) == -1){
                    perror("ERROR - Al guardar el hash en la shm - Master");
                    /// ------------------------------------------------------------
                    if(fclose(resultado_file) == EOF){
                        perror("ERROR - Cerrando el archivo resultado.csv - Master");
                    }
                    if(sem_close(read_sem)==-1){
                        perror("ERROR - Cerrando el semaforo - Master");
                    }
                    if(sem_unlink(READ_SEM)==-1){
                        perror("ERROR - Haciendo unlink del semaforo - Master");
                    }
                    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                        perror("ERROR - Desmapeando la shared memory - Master");
                    }
                    if(shm_unlink(SHM_NAME)==-1){
                        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                    }
                    if(close(shared_memory_fd) == -1){
                        perror("ERROR - Cerrando el fd de la shared memory - Master");
                    }
                    for(int j = 0; j < SLAVES; j++){
                        if(close(read_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de lectura - Master");
                        }
                        if(close(write_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de escritura - Master");
                        }
                    }
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    free(arch_hash);
                    /// ------------------------------------------------------------
                    exit(1);
                }

                free(arch_hash);

                // Una vez que leimos, movemos el fd del extremo de salida del pipe para poder cerrar el
                // fd anterior y cerrarlo dado que hicimos fdopen
                read_fd[i] = dup(read_fd[i]);
                if(read_fd[i] == -1){
                    perror("ERROR - Al mover el fd de lectura - Master");
                    /// ------------------------------------------------------------
                    if(fclose(resultado_file) == EOF){
                        perror("ERROR - Cerrando el archivo resultado.csv - Master");
                    }
                    if(sem_close(read_sem)==-1){
                        perror("ERROR - Cerrando el semaforo - Master");
                    }
                    if(sem_unlink(READ_SEM)==-1){
                        perror("ERROR - Haciendo unlink del semaforo - Master");
                    }
                    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                        perror("ERROR - Desmapeando la shared memory - Master");
                    }
                    if(shm_unlink(SHM_NAME)==-1){
                        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                    }
                    if(close(shared_memory_fd) == -1){
                        perror("ERROR - Cerrando el fd de la shared memory - Master");
                    }
                    for(int j = 0; j < SLAVES; j++){
                        if(close(read_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de lectura - Master");
                        }
                        if(close(write_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de escritura - Master");
                        }
                    }
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }
                if(fclose(read_file) == EOF){
                    perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    /// ------------------------------------------------------------
                    if(fclose(resultado_file) == EOF){
                        perror("ERROR - Cerrando el archivo resultado.csv - Master");
                    }
                    if(sem_close(read_sem)==-1){
                        perror("ERROR - Cerrando el semaforo - Master");
                    }
                    if(sem_unlink(READ_SEM)==-1){
                        perror("ERROR - Haciendo unlink del semaforo - Master");
                    }
                    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                        perror("ERROR - Desmapeando la shared memory - Master");
                    }
                    if(shm_unlink(SHM_NAME)==-1){
                        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
                    }
                    if(close(shared_memory_fd) == -1){
                        perror("ERROR - Cerrando el fd de la shared memory - Master");
                    }
                    for(int j = 0; j < SLAVES; j++){
                        if(close(read_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de lectura - Master");
                        }
                        if(close(write_fd[j]) == -1){
                            perror("ERROR - Al cerrar un fd de escritura - Master");
                        }
                    }
                    /// ------------------------------------------------------------
                    exit(1);
                }
//                getchar();
                // Un archivo menos para hashear
                arch_already_hashed++;
            }
        }
    }
    if(shm_write("\0\n", &shm ) == -1){
        perror("ERROR - Al enviar el fin de transmision a la shm - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(sem_close(read_sem)==-1){
            perror("ERROR - Cerrando el semaforo - Master");
        }
        if(sem_unlink(READ_SEM)==-1){
            perror("ERROR - Haciendo unlink del semaforo - Master");
        }
        if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
            perror("ERROR - Desmapeando la shared memory - Master");
        }
        if(shm_unlink(SHM_NAME)==-1){
            perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - Master");
        }
        for(int j = 0; j < SLAVES; j++){
            if(close(read_fd[j]) == -1){
                perror("ERROR - Al cerrar un fd de lectura - Master");
            }
            if(close(write_fd[j]) == -1){
                perror("ERROR - Al cerrar un fd de escritura - Master");
            }
        }
        /// ------------------------------------------------------------
        exit(1);
    }
    //TODO: solucionamos el problema de que si A se queda mucho tiempo con un md5, no recibe otro hasta terminar
    //Esto es para darle trabajo a los que estan libres
    //El problema es que si B termina muy rapido, va a quedarse bloqueado hasta que md5 le pase algo
    //Seria mejor que B ya tenga trabajo
    //Lo de arriba es lo que solucionamos y lo que dejamos de hacer con esta manera
    // Cerramos los pipes para enviar el EOF a los procesos slave y asi finalizan
    for(int i = 0; i < SLAVES; i++){
        if(close(read_fd[i]) == -1){
            perror("ERROR - Al cerrar un fd de lectura - Master");
            /// ------------------------------------------------------------
            if(fclose(resultado_file) == EOF){
                perror("ERROR - Cerrando el archivo resultado.csv - Master");
            }
            if(sem_close(read_sem)==-1){
                perror("ERROR - Cerrando el semaforo - Master");
            }
            if(sem_unlink(READ_SEM)==-1){
                perror("ERROR - Haciendo unlink del semaforo - Master");
            }
            if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                perror("ERROR - Desmapeando la shared memory - Master");
            }
            if(shm_unlink(SHM_NAME)==-1){
                perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
            }
            if(close(shared_memory_fd) == -1){
                perror("ERROR - Cerrando el fd de la shared memory - Master");
            }
            for(int j = i+1; j < SLAVES; j++){
                if(close(read_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de lectura - Master");
                }
                if(close(write_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de escritura - Master");
                }
            }
            if(close(write_fd[i]) == -1){
                perror("ERROR - Al cerrar un fd de escritura - Master");
            }
            /// ------------------------------------------------------------
            exit(1);
        }
        if(close(write_fd[i]) == -1){
            perror("ERROR - Al cerrar un fd de escritura - Master");
            /// ------------------------------------------------------------
            if(fclose(resultado_file) == EOF){
                perror("ERROR - Cerrando el archivo resultado.csv - Master");
            }
            if(sem_close(read_sem)==-1){
                perror("ERROR - Cerrando el semaforo - Master");
            }
            if(sem_unlink(READ_SEM)==-1){
                perror("ERROR - Haciendo unlink del semaforo - Master");
            }
            if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                perror("ERROR - Desmapeando la shared memory - Master");
            }
            if(shm_unlink(SHM_NAME)==-1){
                perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
            }
            if(close(shared_memory_fd) == -1){
                perror("ERROR - Cerrando el fd de la shared memory - Master");
            }
            for(int j = i+1; j < SLAVES; j++){
                if(close(read_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de lectura - Master");
                }
                if(close(write_fd[j]) == -1){
                    perror("ERROR - Al cerrar un fd de escritura - Master");
                }
            }
            /// ------------------------------------------------------------
            exit(1);
        }
    }

    // Esperamos a que se cierren los procesos slave antes de finalizar
    for(int i=0; i<SLAVES; i++){
        int slave_status;
        if(wait(&slave_status) == -1){
            perror("ERROR - Al esperar por un proceso slave - Master");
            /// ------------------------------------------------------------
            if(fclose(resultado_file) == EOF){
                perror("ERROR - Cerrando el archivo resultado.csv - Master");
            }
            if(sem_close(read_sem)==-1){
                perror("ERROR - Cerrando el semaforo - Master");
            }
            if(sem_unlink(READ_SEM)==-1){
                perror("ERROR - Haciendo unlink del semaforo - Master");
            }
            if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                perror("ERROR - Desmapeando la shared memory - Master");
            }
            if(shm_unlink(SHM_NAME)==-1){
                perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
            }
            if(close(shared_memory_fd) == -1){
                perror("ERROR - Cerrando el fd de la shared memory - Master");
            }
            /// ------------------------------------------------------------
            exit(1);
        }
        if(WIFEXITED(slave_status) && WEXITSTATUS(slave_status) != 0){
            perror("ERROR - EL proceso slave no se cerro correctamente - Master");
            /// ------------------------------------------------------------
            if(fclose(resultado_file) == EOF){
                perror("ERROR - Cerrando el archivo resultado.csv - Master");
            }
            if(sem_close(read_sem)==-1){
                perror("ERROR - Cerrando el semaforo - Master");
            }
            if(sem_unlink(READ_SEM)==-1){
                perror("ERROR - Haciendo unlink del semaforo - Master");
            }
            if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
                perror("ERROR - Desmapeando la shared memory - Master");
            }
            if(shm_unlink(SHM_NAME)==-1){
                perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
            }
            if(close(shared_memory_fd) == -1){
                perror("ERROR - Cerrando el fd de la shared memory - Master");
            }
            /// ------------------------------------------------------------
            exit(1);
        }
    }

    //No es necesario este semaforo, pues puedo hacer unlink de la shm y recien la destruye cuando todos los que la tienen
    //mapeada la eliminan
    //sem_wait(vista_sem);//Si vista entro, esperamos a que termine de utilizar la shm antes de cerrarla
    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
        perror("ERROR - Desmapeando la shared memory - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(sem_close(read_sem)==-1){
            perror("ERROR - Cerrando el semaforo - Master");
        }
        if(sem_unlink(READ_SEM)==-1){
            perror("ERROR - Haciendo unlink del semaforo - Master");
        }
        if(shm_unlink(SHM_NAME)==-1){
            perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    if(shm_unlink(SHM_NAME)==-1){
        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(sem_close(read_sem)==-1){
            perror("ERROR - Cerrando el semaforo - Master");
        }
        if(sem_unlink(READ_SEM)==-1){
            perror("ERROR - Haciendo unlink del semaforo - Master");
        }
        if(close(shared_memory_fd) == -1){
            perror("ERROR - Cerrando el fd de la shared memory - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    if(close(shared_memory_fd) == -1){
        perror("ERROR - Cerrando el fd de la shared memory info - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(sem_close(read_sem)==-1){
            perror("ERROR - Cerrando el semaforo - Master");
        }
        if(sem_unlink(READ_SEM)==-1){
            perror("ERROR - Haciendo unlink del semaforo - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    if(sem_close(read_sem)==-1){
        perror("ERROR - Cerrando el semaforo - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        if(sem_unlink(READ_SEM)==-1){
            perror("ERROR - Haciendo unlink del semaforo - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    if(sem_unlink(READ_SEM)==-1){
        perror("ERROR - Haciendo unlink del semaforo - Master");
        /// ------------------------------------------------------------
        if(fclose(resultado_file) == EOF){
            perror("ERROR - Cerrando el archivo resultado.csv - Master");
        }
        /// ------------------------------------------------------------
        exit(1);
    }

    if(fclose(resultado_file) == EOF){
        perror("ERROR - Cerrando el archivo resultado.csv - Master");
        exit(1);
    }

    exit(0);
}

// -------------------------------------------------------------------------------------------------------
// write_to_slave: Dado un fd y un path, escribe el path en dicho fd
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      fd: El file descriptor a escribir
//      file_path: El path del archivo
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no hubo error, -1 si lo hubo
// -------------------------------------------------------------------------------------------------------
int write_to_slave(int fd, const char * file_path){
    size_t len = strlen(file_path);
    char aux[len + 1];
    strcpy(aux, file_path);
    aux[len] = '\n';

    // Indica la posicion del proximo caracter del path que se debe escribir en el pipe
    size_t curr = 0;
    // Indica la cantidad de caracteres que falta escribir por el pipe
    size_t remaining = len + 1;

    // Quiero que se termine de escribir el mensaje, pudiendo quedarse bloqueado en el caso donde el pipe no tiene espacio suficiente
    while (remaining > 0) {
        //Quiero que se termine de escribir el mensaje, pudiendo quedarse bloqueado en el caso donde el pipe no tiene espacio suficiente
        ssize_t written = write(fd, aux + curr, remaining);
        if (written == -1) {
            perror("ERROR - Al intentar escribir el argumento - Master");
            return -1;
        }
        // Movemos el puntero de escritura al primer caracter del path que no se escribio en el pipe
        curr += written;
        // Actualizamos la cantidad de caracteres que faltan escribir
        remaining -= written;
    }
    return 0;
}

// -------------------------------------------------------------------------------------------------------
// is_file: Revisar si el path es un archivo
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      file_path: El path del archivo
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no es un file, 1 si es file, -1 por error en stat
// -------------------------------------------------------------------------------------------------------
int is_file(const char * file_path){
    struct stat prop;
    if( stat(file_path, &prop) == -1){
        perror("ERROR - En lectura de file_path - Master");
        return -1;
    }
    return S_ISREG(prop.st_mode);
}

// -------------------------------------------------------------------------------------------------------
// shm_write: Escribe un string en la shm, sincronizando el semaforo con el padre
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      str: El string a guardar en la shm
//      shm: Estructura con la informacion de la shm
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no hubo error, -1 si lo hubo
// -------------------------------------------------------------------------------------------------------
int shm_write(const char* str,shm_struct* shm){
    for(int i = 0; str[i]!='\n';i++){
//        shm->start[(shm->index)++] = str[i];
        *((shm->start)++)=str[i];
        if(sem_post(shm->semaphore)==-1){
            perror("ERROR - Al realizar post en el semaforo - Master");
            return -1;
        }
    }
    *((shm->start)++) = '\n';
    if(sem_post(shm->semaphore)==-1){
        perror("ERROR - Al realizar post en el semaforo - Master");
        return -1;
    }
    return 0;
}
