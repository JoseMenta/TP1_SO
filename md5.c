// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "md5.h"

// Proceso Maestro que recibe los archivos y pide su md5sum, el cual lo guarda en un archivo y lo envia a una shm
int main(int arg_c, char** arg_v) {
    // Desactivamos el buffer para stdout
    setvbuf(stdout, NULL, _IONBF, 0);

    // Recibe por argumentos los nombres de los archivos. Si no recibe ninguno, finaliza
    if (arg_c <= -1) {
        perror("ERROR - No se recibieron archivos - Master");
        exit(1);
    }

    int slaves = (SLAVES <= arg_c - 1) ? SLAVES : arg_c - 1;

    // Creamos un arreglo que almacene los fd que leen de slaves
    int read_fd[slaves];
    // Creamos un arreglo que almacene los fd que escriban a los slaves
    int write_fd[slaves];

    if( create_slaves(read_fd, write_fd, slaves) == -1){
        perror("ERROR - Al crear el pipe de Master a Slave - Master");
        exit(1);
    };


    // Creacion del archivo resultados.csv
    FILE * resultado_file;
    if((resultado_file = fopen("./resultado.csv", "w+")) == NULL){
        perror("ERROR - No se pudo abrir archivo resultado - Master");
        exit(1);
    }

    // Creacion de la shared memory
    char * shared_memory = SHM_NAME;
    int shared_memory_fd = shm_open(shared_memory, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if(shared_memory_fd == -1){
        close_shm_file_shmADT(NULL, -1, 0, NULL, NULL,resultado_file);
        perror("ERROR - Creacion de shm - Master");
        exit(1);
    }

    // Asignamos el espacio para la shm
    if(ftruncate(shared_memory_fd, SHM_SIZE(arg_c-1)) == -1){
        close_shm_file_shmADT(NULL, shared_memory_fd, 0, NULL,NULL, resultado_file);
        perror("ERROR - Falló la asignacion de tamaño en la shared memory - Master");
        exit(1);
    }

    // Mapeamos la shared memory
    void * shared_memory_map = mmap(NULL, SHM_SIZE(arg_c-1), PROT_WRITE, MAP_SHARED, shared_memory_fd, 0);
    if(shared_memory_map == MAP_FAILED){
        close_shm_file_shmADT(NULL, shared_memory_fd, 0, NULL,NULL, resultado_file);
        perror("ERROR - Mapeando la shared memory init - Master");
        exit(1);
    }

    // Creamos el semaforo para sincronizar la lectura y escritura entre master y vista
    sem_t * shm_sem = sem_open(READ_SEM,O_RDWR|O_CREAT|O_EXCL,S_IRWXU,0);
    if(shm_sem==SEM_FAILED){
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c-1), NULL,NULL, resultado_file);
        perror("ERROR - Al abrir el semaforo para lectura - Master");
        exit(1);
    }

    // Imprimimos por pantalla el nombre identificador de la shm, el tamaño de la misma y el nombre del semaforo
    if(printf("%s\n", SHM_NAME) < 0){
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), NULL,shm_sem, resultado_file);
        perror("ERROR - Al escribir por pantalla - Master");
        exit(1);
    }

    if(printf("%lu\n", SHM_SIZE(arg_c-1)) < 0){
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), NULL,shm_sem, resultado_file);
        perror("ERROR - Al escribir por pantalla - Master");
        exit(1);
    }

    if(printf("%s\n", READ_SEM) < 0){
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), NULL,shm_sem, resultado_file);
        perror("ERROR - Al escribir por pantalla - Master");
        exit(1);
    }

    // Esperamos a que se ejecute el proceso vista
    sleep(SLEEP_TIME);

    shmADT shm;
    if((shm = new_shm((char*) shared_memory_map,shm_sem)) == NULL){
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), NULL,shm_sem, resultado_file);
        perror("ERROR - Al crear el ADT - Master");
        exit(1);
    }

    // Indica la cantidad de archivos que faltan hashear
    int arch_to_hash = arg_c - 1;
    // Indicamos el proximo archivo a hashear
    int arg_index = 1;

    // Le asginamos un archivo a cada slave inicialmente
    for(int i = 0; i<slaves && arch_to_hash > 0;i++){
        // Primero hay que comprobar que el path a pasar sea un archivo y queden archivos disponibles para hashear
        int status = 0;
        while(arg_index<arg_c && (status = is_file(arg_v[arg_index])) == 0){
            // Dado que es un directorio, no debe considerarse como archivo
            arch_to_hash--;
            arg_index++;
        }
        if(status==-1){
            close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
            perror("ERROR - Revisando si el path recibido es un archivo - Master");
            exit(1);
        }
        // Se obtuvo un archivo o ya no quedan paths por leer
        if(arg_index<arg_c) {
            if(write_to_slave(write_fd[i], arg_v[arg_index]) == -1){
                close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                perror("ERROR - Enviando el path al proceso slave - Master");
                exit(1);
            }
            arg_index++;
        }
    }

    // Iremos enviando el path de los archivos a hashear y leyendo los hasheos
    while(arch_to_hash > 0){
        // Configuramos los conjuntos con los elementos de lectura
        fd_set read_fd_set;
        FD_ZERO(&read_fd_set);
        int max_fd = 0;

        for(int i = 0; i < slaves; i++){
            // Obtenemos el maximo fd, para usar con select
            max_fd = (read_fd[i]>max_fd)?read_fd[i]:max_fd;
            FD_SET(read_fd[i], &read_fd_set);
        }

        // Un solo select con sets de lectura para saber cuando se retorna el md5
        // los file descriptors disponibles para leer/escribir
        if(select(max_fd+1, &read_fd_set,  NULL, NULL, NULL) == -1){
            close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
            perror("ERROR - Al realizar select() - Master");
            exit(1);
        }
        for(int i = 0; i<slaves ; i++){
            if(FD_ISSET(read_fd[i], &read_fd_set)){
                // Si esta disponible este fd para leer, leemos el hash md5 obtenido
                FILE * read_file = fdopen(read_fd[i], "r");
                if(read_file == NULL){
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    perror("ERROR - Al intentar abrir el fd read - Master");
                    exit(1);
                }

                char * arch_hash = NULL;
                size_t arch_hash_len = 0;
                if(getline(&arch_hash, &arch_hash_len, read_file) == -1) {
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    perror("ERROR - Al leer del proceso slave - Master");
                    exit(1);
                }

                // Dado que ya finalizo hasheando el archivo pasado anteriormente, podemos pasarle el proximo archivo disponible
                // Escribimos al proceso esclavo que nos devolvio el resultado

                // Primero hay que comprobar que el path a pasar sea un archivo y queden archivos disponibles para hashear
                int status = 0;
                while(arg_index<arg_c && (status = is_file(arg_v[arg_index])) == 0){
                    arg_index++;
                    // Dado que es un directorio, no debe considerarse como archivo
                    arch_to_hash--;
                }
                if(status==-1){
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm, shm_sem,resultado_file);
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    free(arch_hash);
                    perror("ERROR - Revisando si el path recibido es un archivo - Master");
                    exit(1);
                }
                // Se obtuvo un archivo o ya no quedan paths por leer
                if(arg_index<arg_c) {
                    if(write_to_slave(write_fd[i], arg_v[arg_index]) == -1){
                        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem,resultado_file);
                        if(fclose(read_file) == EOF){
                            perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                        }
                        free(arch_hash);
                        perror("ERROR - Enviando el path al proceso slave - Master");
                        exit(1);
                    }
                    arg_index++;
                }

                // Escribimos dentro de shm con funcion de driver
                if(shm_write(arch_hash, shm ) == -1){
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    free(arch_hash);
                    perror("ERROR - Al guardar el hash en la shm - Master");
                    exit(1);
                }

                // Imprimimos el hasheo del archivo recibido a resultados.csv
                if(fprintf(resultado_file,"%s", arch_hash) < 0){
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    free(arch_hash);
                    perror("ERROR - Al escribir el hasheo en resultados.csv - Master");
                    exit(1);
                }

                free(arch_hash);

                // Una vez que leimos, movemos el fd del extremo de salida del pipe para poder cerrarlo dado que hicimos fdopen
                read_fd[i] = dup(read_fd[i]);
                if(read_fd[i] == -1){
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    perror("ERROR - Al mover el fd de lectura - Master");
                    exit(1);
                }

                if(fclose(read_file) == EOF){
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    exit(1);
                }

                // Un archivo menos para hashear
                arch_to_hash--;
            }
        }
    }

    // Indicamos que se termino la escritura en la shm enviando EOT
    char final_str[2] = {EOT, '\0'};
    if(shm_write(final_str, shm ) == -1){
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
        perror("ERROR - Al enviar el fin de transmision a la shm - Master");
        exit(1);
    }

    // Cerramos los procesos Slave
    if(close_slaves(read_fd, write_fd, slaves) == -1){
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
        perror("ERROR - Al terminar los procesos Slaves  - Master");
        exit(1);
    }

    // Cerramos todos los recursos utilizados para la shm, el semaforo y el archivo resultados.csv
    if(close_shm_file_shmADT(shared_memory_map,shared_memory_fd, SHM_SIZE(arg_c-1),shm,shm_sem,resultado_file)==-1){
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

    if(write(fd, aux, len+1) == -1){
        perror("ERROR - Al intentar escribir el argumento - Master");
        return -1;
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
// close_shm_file_shmADT: Libera los recursos utilizados para la shm, el archivo de salida y el semaforo
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      shm: Puntero al inicio de la shm. Si es NULL, se ignora
//      shm_fd: File descriptor asociado a la shm. Si es -1, se ignora
//      shm_length: La longitud de la shm
//      sem: Puntero al semaforo utilizado para sincronizar el acceso a la shm. Si es NULL, se ignora
//      file: FILE* utilizado para manejar el archivo de resultado. Si es NULL, se ignora
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no hubo error, -1 si hubo
// -------------------------------------------------------------------------------------------------------
int close_shm_file_shmADT(void* shm, int shm_fd, int shm_length, shmADT shmAdt,sem_t* sem, FILE* file){
    int error = 0;
    if(file != NULL && fclose(file) == EOF){
        perror("ERROR - Cerrando el archivo resultado.csv - Master");
        error = -1;
    }
    if(shmAdt != NULL){
        //Le mandamos la señal de finalizar para vista
        char final_str[2] = {EOT, '\0'};
        if(shm_write(final_str, shmAdt ) == -1){
            perror("ERROR - Al enviar el fin de transmision a la shm - Master");
        }
        free_shm(shmAdt);
    }
    if(sem!=NULL){
        if(sem_close(sem)==-1){
            perror("ERROR - Cerrando el semaforo - Master");
            error = -1;
        }
        if(sem_unlink(READ_SEM)==-1){
            perror("ERROR - Haciendo unlink del semaforo - Master");
            error = -1;
        }
    }
    if(shm != NULL){
        if(munmap(shm, shm_length) == -1){
            perror("ERROR - Desmapeando la shared memory - Master");
            error = -1;
        }
        if(shm_unlink(SHM_NAME)==-1){
            perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
            error = -1;
        }
    }
    if(shm_fd != -1 && close(shm_fd) == -1){
        perror("ERROR - Cerrando el fd de la shared memory - Master");
        error = -1;
    }
    return error;
}


// -------------------------------------------------------------------------------------------------------
// close_fd: Libera los fd indicados por parametro
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      fd: Arreglo de fds a cerrar
//      length: Cantidad de fds
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no hubo error, -1 si lo hubo
// -------------------------------------------------------------------------------------------------------
int close_fd(int * fd, int length){
    int error = 0;
    for(int i = 0; i < length; i++){
        if(close(fd[i]) == -1){
            perror("ERROR - Cerrando un fd - Master");
            error = -1;
        }
    }
    return error;
}

// -------------------------------------------------------------------------------------------------------
// create_slaves: Funcion para crear procesos SLAVE
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      read_fd: Arreglo de fd para lectura desde los slaves
//      write_fd: Arreglo de fd para escritura hacia los slaves
//      slaves: cantidad de slaves
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no hubo error, -1 si lo hubo
// -------------------------------------------------------------------------------------------------------
int create_slaves(int * read_fd, int * write_fd, int slaves){
    // Creamos los slaves y les configuramos los pipes para leer y escribir
    for(int i = 0; i < slaves; i++){
        int mtos_pipe[2]; //Va de Master to Slave
        int stom_pipe[2]; //Va de Slave to Master
        if(pipe(mtos_pipe) == -1){
            perror("ERROR - Al crear el pipe de Master a Slave - Master");
            return -1;
        }
        if(pipe(stom_pipe) == -1){
            perror("ERROR - Al crear el pipe de Slave a Master - Master");
            return -1;
        }

        pid_t new_pid = fork();
        switch (new_pid) {
            case -1:
            {
                perror("ERROR - Creación del proceso Slave - Master");
                return -1;
            }
            case 0:
            {
                // El proceso slave no necesita estos fd, por lo que los cerramos
                close_fd(read_fd, i);
                close_fd(write_fd, i);

                if(close(mtos_pipe[1])== -1){
                    perror("ERROR - Cerrar el extremo de escritura del pipe mtos en Slave - Slave");
                    return -1;
                }
                if(close(stom_pipe[0])== -1){
                    perror("ERROR - Cerrar el extremo de lectura del pipe stom en Slave - Slave");
                    return -1;
                }
                if(dup2(mtos_pipe[0], STDIN_FILENO)== -1){
                    perror("ERROR - Duplicar STDIN_FILENO en lectura del pipe mtos en Slave - Slave");
                    return -1;
                }
                if(dup2(stom_pipe[1], STDOUT_FILENO)== -1){
                    perror("ERROR - Duplicar STDOUT en escritura del pipe stom en Slave - Slave");
                    return -1;
                }
                if(close(mtos_pipe[0])== -1){
                    perror("ERROR - Cerrar el extremo de lectura del pipe mtos en Slave - Slave");
                    return -1;
                }
                if(close(stom_pipe[1])== -1){
                    perror("ERROR: Cerrar el extremo de escritura del pipe stom en Slave - Slave");
                    return -1;
                }
                if(execl("slave", "./slave", NULL)==-1){
                    perror("ERROR: Al crear el proceso slave - Slave");
                    return -1;
                }
                break;
            }
                // EL proceso padre se queda con los fd requeridos
            default:
            {
                if(close(stom_pipe[1])==-1){
                    perror("ERROR: Cerrar el extremo de escritura del pipe stom en Master - Master");
                    return -1;
                }
                if(close(mtos_pipe[0])==-1){
                    perror("ERROR: Cerrar el extremo de lectura del pipe mtos en Master - Master");
                    return -1;
                }
                // Almacenamos los pipes de lectura y escritura para el master
                read_fd[i] = stom_pipe[0];
                write_fd[i] = mtos_pipe[1];
            }
        }
    }
    return 0;
}

// -------------------------------------------------------------------------------------------------------
// close_slaves: Funcion para cerrar los procesos SLAVE
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      read_fd: Arreglo de fd para lectura desde los slaves
//      write_fd: Arreglo de fd para escritura hacia los slaves
//      slaves: cantidad de slaves
// -------------------------------------------------------------------------------------------------------
// Retorno:
//      0 si no hubo error, -1 si lo hubo
// -------------------------------------------------------------------------------------------------------
int close_slaves(int * read_fd, int * write_fd, int slaves){
    int error = 0;
    // Cerramos los pipes para enviar el EOF a los procesos slave y asi estos finalizan
    if(close_fd(read_fd, slaves) == -1){
        perror("ERROR - Al cerrar fd read - Master");
        error = -1;
    }

    if(close_fd(write_fd, slaves) == -1){
        perror("ERROR - Al cerrar fd write - Master");
        error = -1;
    }

    // Esperamos a que se cierren los procesos slave antes de finalizar
    for(int i=0; i<slaves; i++){
        int slave_status;
        if(wait(&slave_status) == -1){
            perror("ERROR - Al esperar por un proceso slave - Master");
            error = -1;
        }
        if(WIFEXITED(slave_status) && WEXITSTATUS(slave_status) != 0){
            perror("ERROR - EL proceso slave no se cerro correctamente - Master");
            error = -1;
        }
    }
    return error;
}