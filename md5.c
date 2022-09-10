// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "md5.h"

// Proceso Maestro que recibe los archivos y pide su md5sum, el cual lo guarda en un archivo y lo envia a una shm
int main(int arg_c, char** arg_v){
    if(arg_c <= 1){
        perror("ERROR - No se recibieron archivos - Master");
        exit(1);
    }

    // Creamos un arreglo que almacene los fd que leen de slaves
    int read_fd[SLAVES];
    // Creamos un arreglo que almacene los fd que escriban a los slaves
    int write_fd[SLAVES];

    // Creamos los slaves y les configuramos los pipes para leer y escribir
    for(int i = 0; i < SLAVES; i++){
        int mtos_pipe[2]; //Va de Master to Slave
        int stom_pipe[2]; //Va de Slave to Master
        if(pipe(mtos_pipe) == -1){
            perror("ERROR - Al crear el pipe de Master a Slave - Master");
            exit(1);
        }
        if(pipe(stom_pipe) == -1){
            perror("ERROR - Al crear el pipe de Slave a Master - Master");
            exit(1);
        }

        pid_t new_pid = fork();
        switch (new_pid) {
            case -1:
            {
                perror("ERROR - Creación del proceso Slave - Master");
                exit(1);
            }
            case 0:
            {
                close_fd(read_fd, i);
                close_fd(write_fd, i);
                if(close(mtos_pipe[1])== -1){
                    perror("ERROR - Cerrar el extremo de escritura del pipe mtos en Slave - Slave");
                    exit(1);
                }
                if(close(stom_pipe[0])== -1){
                    perror("ERROR - Cerrar el extremo de lectura del pipe stom en Slave - Slave");
                    exit(1);
                }
                if(dup2(mtos_pipe[0], STDIN_FILENO)== -1){
                    perror("ERROR - Duplicar STDIN_FILENO en lectura del pipe mtos en Slave - Slave");
                    exit(1);
                }
                if(dup2(stom_pipe[1], STDOUT_FILENO)== -1){
                    perror("ERROR - Duplicar STDOUT en escritura del pipe stom en Slave - Slave");
                    exit(1);
                }
                if(close(mtos_pipe[0])== -1){
                    perror("ERROR - Cerrar el extremo de lectura del pipe mtos en Slave - Slave");
                    exit(1);
                }
                if(close(stom_pipe[1])== -1){
                    perror("ERROR: Cerrar el extremo de escritura del pipe stom en Slave - Slave");
                    exit(1);
                }
                if(execl("slave", "./slave", NULL)==-1){
                    perror("ERROR: Al crear el proceso slave - Slave");
                    exit(1);
                }
                break;
            }
            // EL proceso padre se queda con los fd requeridos
            default:
            {
                if(close(stom_pipe[1])==-1){
                    perror("ERROR: Cerrar el extremo de escritura del pipe stom en Master - Master");
                    exit(1);
                }
                if(close(mtos_pipe[0])==-1){
                    perror("ERROR: Cerrar el extremo de lectura del pipe mtos en Master - Master");
                    exit(1);
                }
                // Almacenamos los pipes de lectura y escritura para el master
                read_fd[i] = stom_pipe[0];
                write_fd[i] = mtos_pipe[1];
            }
        }
    }


    FILE * resultado_file;
    if((resultado_file = fopen("./resultado.csv", "w+")) == NULL){
        perror("ERROR - No se pudo abrir archivo resultado - Master");
        exit(1);
    }

    // Creacion de share memory con driver
    char * shared_memory = SHM_NAME;
    int shared_memory_fd = shm_open(shared_memory, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if(shared_memory_fd == -1){
        perror("ERROR - Creacion de shm - Master");
        close_shm_file_shmADT(NULL, -1, 0, NULL, NULL,resultado_file);
        exit(1);
    }

    // Asignamos el espacio para la shm
    if(ftruncate(shared_memory_fd, SHM_SIZE(arg_c-1)) == -1){
        perror("ERROR - Falló la asignacion de tamaño en la shared memory - Master");
        close_shm_file_shmADT(NULL, shared_memory_fd, 0, NULL,NULL, resultado_file);
        exit(1);
    }

    // Mapeamos la shared memory init. Si falla, abortamos
    void * shared_memory_map = mmap(NULL, SHM_SIZE(arg_c-1), PROT_WRITE, MAP_SHARED, shared_memory_fd, 0);
    if(shared_memory_map == MAP_FAILED){
        perror("ERROR - Mapeando la shared memory init - Master");
        close_shm_file_shmADT(NULL, shared_memory_fd, 0, NULL,NULL, resultado_file);
        exit(1);
    }

    // Creamos el semaforo para sincronizar la lectura y escritura entre md5 y vista
    sem_t * shm_sem = sem_open(READ_SEM,O_RDWR|O_CREAT|O_EXCL,S_IRWXU,0);
    if(shm_sem==SEM_FAILED){
        perror("ERROR - Al abrir el semaforo para lectura - Master");
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c-1), NULL,NULL, resultado_file);
        exit(1);
    }

    // Imprimimos por pantalla el nombre identificador de la shm, el tamaño del mismo y el nombre del semaforo
    if(printf("%s\n", SHM_NAME) < 0){
        perror("ERROR - Al escribir por pantalla - Master");
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), NULL,shm_sem, resultado_file);
        exit(1);
    }

    if(printf("%lu\n", SHM_SIZE(arg_c-1)) < 0){
        perror("ERROR - Al escribir por pantalla - Master");
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), NULL,shm_sem, resultado_file);
        exit(1);
    }

    if(printf("%s\n", READ_SEM) < 0){
        perror("ERROR - Al escribir por pantalla - Master");
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), NULL,shm_sem, resultado_file);
        exit(1);
    }

    fflush(stdout);

    // Esperamos a que se ejecute el proceso vista
    sleep(SLEEP_TIME);
    shmADT shm;
    if((shm = newShm((char*) shared_memory_map,shm_sem)) == NULL){
        perror("ERROR - Al crear el ADT - Master");
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), NULL,shm_sem, resultado_file);
        exit(1);
    }

    // Indica la cantidad de archivos que faltan hashear
    int arch_to_hash = arg_c - 1;
    // Indicamos el proximo archivo a hashear
    int arg_index = 1;

    // Le asginamos un archivo a cada slave inicialmente
    for(int i = 0; i<SLAVES && arch_to_hash > 0;i++){
        // Primero hay que comprobar que el path a pasar sea un archivo y queden archivos disponibles para hashear
        int status = 0;
        while(arg_index<arg_c && (status = is_file(arg_v[arg_index])) == 0){
            // Dado que es un directorio, no debe considerarse como archivo
            arch_to_hash--;
            arg_index++;
        }
        if(status==-1){
            perror("ERROR - Revisando si el path recibido es un archivo - Master");
            close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
            exit(1);
        }
        // Se obtuvo un archivo o ya no quedan paths por leer
        if(arg_index<arg_c) {
            if(write_to_slave(write_fd[i], arg_v[arg_index]) == -1){
                perror("ERROR - Enviando el path al proceso esclavo");
                close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
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

        for(int i = 0; i < SLAVES; i++){
            // Obtenemos el maximo fd, para usar con select
            max_fd = (read_fd[i]>max_fd)?read_fd[i]:max_fd;
            FD_SET(read_fd[i], &read_fd_set);
        }

        // Un solo select con sets de lectura para saber cuando se retorna el md5
        // los file descriptors disponibles para leer/escribir
        if(select(max_fd+1, &read_fd_set,  NULL, NULL, NULL) == -1){
            perror("ERROR - Al realizar select() - Master");
            close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
            exit(1);
        }
        for(int i = 0; i<SLAVES ; i++){
            if(FD_ISSET(read_fd[i], &read_fd_set)){
                // Si esta disponible este fd para leer, leo el hash md5 obtenido
                // Primero creo un FILE * para poder leer hasta \n
                FILE * read_file = fdopen(read_fd[i], "r");
                if(read_file == NULL){
                    perror("ERROR - Al intentar abrir el fd read - Master");
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    exit(1);
                }
                char * arch_hash = NULL;
                size_t arch_hash_len = 0;

                if(getline(&arch_hash, &arch_hash_len, read_file) == -1) {
                    perror("ERROR - Al leer del slave - Master");
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
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
                    perror("ERROR - Revisando si el path recibido es un archivo - Master");
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm, shm_sem,resultado_file);
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    free(arch_hash);
                    exit(1);
                }
                // Se obtuvo un archivo o ya no quedan paths por leer
                if(arg_index<arg_c) {
                    if(write_to_slave(write_fd[i], arg_v[arg_index]) == -1){
                        perror("ERROR - Enviando el path al proceso esclavo");
                        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem,resultado_file);
                        if(fclose(read_file) == EOF){
                            perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                        }
                        free(arch_hash);
                        exit(1);
                    }
                    arg_index++;
                }

                // Escribimos dentro de shm con funcion de driver
                if(shm_write(arch_hash, shm ) == -1){
                    perror("ERROR - Al guardar el hash en la shm - Master");
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    free(arch_hash);
                    exit(1);
                }

                // Imprimimos el hasheo del archivo recibido a resultados.csv
                if(fprintf(resultado_file,"%s", arch_hash) < 0){
                    perror("ERROR - Al escribir el hasheo en resultados.csv - Master");
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    free(arch_hash);
                    exit(1);
                }
                free(arch_hash);

                // Una vez que leimos, movemos el fd del extremo de salida del pipe para poder cerrarlo dado que hicimos fdopen
                read_fd[i] = dup(read_fd[i]);
                if(read_fd[i] == -1){
                    perror("ERROR - Al mover el fd de lectura - Master");
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    if(fclose(read_file) == EOF){
                        perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    }
                    exit(1);
                }
                if(fclose(read_file) == EOF){
                    perror("ERROR - Cerrando el archivo de lectura del proceso slave - Master");
                    close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
                    exit(1);
                }

                // Un archivo menos para hashear
                arch_to_hash--;
            }
        }
    }

    char final_str[2] = {EOT, '\n'};
    if(shm_write(final_str, shm ) == -1){
        perror("ERROR - Al enviar el fin de transmision a la shm - Master");
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
        exit(1);
    }

    // Cerramos los pipes para enviar el EOF a los procesos slave y asi finalizan
    if(close_fd(read_fd, SLAVES) == -1){
        close_fd(write_fd,SLAVES);
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
        exit(1);
    }

    if(close_fd(write_fd, SLAVES) == -1){
        close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
        exit(1);
    }

    // Esperamos a que se cierren los procesos slave antes de finalizar
    for(int i=0; i<SLAVES; i++){
        int slave_status;
        if(wait(&slave_status) == -1){
            perror("ERROR - Al esperar por un proceso slave - Master");
            close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
            exit(1);
        }
        if(WIFEXITED(slave_status) && WEXITSTATUS(slave_status) != 0){
            perror("ERROR - EL proceso slave no se cerro correctamente - Master");
            close_shm_file_shmADT(shared_memory_map, shared_memory_fd, SHM_SIZE(arg_c - 1), shm,shm_sem, resultado_file);
            exit(1);
        }
    }

    // Liberamos los recursos de la shm y del semaforo
    if(munmap(shared_memory_map, SHM_SIZE(arg_c-1)) == -1){
        perror("ERROR - Desmapeando la shared memory - Master");
        close_shm_file_shmADT(NULL, shared_memory_fd, 0, shm,shm_sem, resultado_file);
        exit(1);
    }

    if(shm_unlink(SHM_NAME)==-1){
        perror("ERROR - Haciendo unlink de la shared memory en md5 - Master");
        close_shm_file_shmADT(NULL, shared_memory_fd, 0, shm,shm_sem, resultado_file);
        exit(1);
    }

    if(close(shared_memory_fd) == -1){
        perror("ERROR - Cerrando el fd de la shared memory info - Master");
        close_shm_file_shmADT(NULL, -1, 0, shm, shm_sem,resultado_file);
        exit(1);
    }

    freeShm(shm);

    if(sem_close(shm_sem)==-1){
        perror("ERROR - Cerrando el semaforo - Master");
        close_shm_file_shmADT(NULL,-1,0,NULL,NULL,resultado_file);
        exit(1);
    }

    if(sem_unlink(READ_SEM)==-1){
        perror("ERROR - Haciendo unlink del semaforo - Master");
        close_shm_file_shmADT(NULL,-1,0,NULL,NULL,resultado_file);
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
        freeShm(shmAdt);
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
// close_pipes: Libera los fd indicados por parametro
// -------------------------------------------------------------------------------------------------------
// Argumentos:
//      fd: Arreglo de fd a cerrar
//      length: Cantidad de fd
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
