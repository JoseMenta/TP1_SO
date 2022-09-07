//
// Created by Jose Menta on 04/09/2022.
//

// feature_test_macro para getline y fdopen
#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define SLAVES 5

int write_to_slave(int fd, const char * file_path);
int is_file(const char * file_path);

int main(int arg_c, char** arg_v){
    // Recibe por argumentos los nombres de los archivos que se desean procesar
    // Si no recibe ninguno, finaliza
    if(arg_c <= 1){
        perror("Error: No se recibieron archivos");
        exit(1);
    }

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
            perror("Error: Al crear el pipe de Master a Slave");
            // TODO: Manejar el cierre de archivos, pipes y mallocs
        }
        if(pipe(stom_pipe) == -1){
            perror("Error: Al crear el pipe de Slave a Master");
            // TODO: Manejar el cierre de archivos, pipes y mallocs
        }

        // Creamos el proceso hijo Slave
        pid_t new_pid = fork();
        switch (new_pid) {
            // En caso de error, abortamos
            case -1:
            {
                perror("Error: Creación del proceso Slave");
                // TODO: Manejar el cierre de archivos, pipes y mallocs
            }
            // El proceso hijo se convierte en el proceso slave
            case 0:
            {
                //Cerramos el extremo de escritura del pipe mtos
                if(close(mtos_pipe[1])== -1){
                    perror("Error: Cerrar el extremo de escritura del pipe mtos en Slave");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                }
                //Cerramos el extremo de lectura del pipe stom
                if(close(stom_pipe[0])== -1){
                    perror("Error: Cerrar el extremo de lectura del pipe stom en Slave");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                }
                // Llevamos a STDIN el extremo de lectura de mtos
                if(dup2(mtos_pipe[0], STDIN_FILENO)== -1){
                    perror("Error: duplicar STDIN_FILENO en lectura del pipe mtos en Slave");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                }
                // Llevamos a STDOUT el extremo de escritura de stom
                if(dup2(stom_pipe[1], STDOUT_FILENO)== -1){
                    perror("Error: duplicar STDOUT en escritura del pipe stom en Slave");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                }
                // Cerramos el extremo de lectura del pipe mtos
                if(close(mtos_pipe[0])== -1){
                    perror("Error: Cerrar el extremo de lectura del pipe mtos en Slave");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                }
                // Cerramos el extremo de escritura del pipe stom
                if(close(stom_pipe[1])== -1){
                    perror("Error: Cerrar el extremo de escritura del pipe stom en Slave");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                }
                // Ejecutamos el proceso slave
                if(execl("slave", "./slave", NULL)==-1){
                    perror("Error: Al crear el proceso slave");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                    exit(1);
                }
            }
            // EL proceso padre se queda con los fd requeridos
            default:
            {
                // Cerramos el extremo de escritura del pipe stom
                if(close(stom_pipe[1])==-1){
                    perror("Error: Cerrar el extremo de escritura del pipe stom en Master");
                    //TODO: Manejar cierre
                }
                // Cerramos el extremo de lectura del pipe mtos
                if(close(mtos_pipe[0])==-1){
                    perror("Error: Cerrar el extremo de lectura del pipe mtos en Master");
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
//        if(!is_file(arg_v
        while(arg_index<arg_c && (status = is_file(arg_v[arg_index])) == 0){
            count_arch--; //Para no considerarlos luego
            arg_index++;
        }
        // Error en lectura de stat
        if(status==-1){
            //TODO: Manejar el cierre del programa
            exit(1);
        }
        // Se obtuvo un archivo o ya no quedan paths por leer
        if(arg_index<arg_c) {
            if(write_to_slave(write_fd[i], arg_v[arg_index]) == -1){
                // TODO: Manejar el cierre de archivos, fd, etc
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
            perror("Error: Al realizar select()");
            // TODO: Manejar el cierre de archivos, pipes y mallocs
        }
        // Repaso todos los fd de lectura para saber cuales se pueden leer (tienen un archivo hasheado)
        for(int i = 0; i<SLAVES ; i++){
            if(FD_ISSET(read_fd[i], &read_fd_set)){
                // Si esta disponible este fd para leer, leo el hash md5 obtenido
                // Primero creo un FILE * para poder leer hasta \n
                FILE * read_file = fdopen(read_fd[i], "r");
                if(read_file == NULL){
                    perror("Error: Al intentar abrir el fd read");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                    exit(1);
                }
                char * arch_hash = NULL;
                size_t arch_hash_len = 0;
                // Lectura del fd hasta \n (printf termina en \n)
                if(getline(&arch_hash, &arch_hash_len, read_file) == -1) {
                    //fclose(read_file);
                    perror("Error: Al leer del slave");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                    exit(1);
                }

                // Dado que ya finalizo hasheando el archivo pasado anteriormente, podemos pasarle el proximo archivo disponible
                // Escribimos al que nos devolvio el resultado

                // Primero hay que comprobar que el path a pasar sea un archivo y queden archivos disponibles para hashear
                int status = 0;
                while(arg_index<arg_c && (status = is_file(arg_v[arg_index])) == 0){
                    arg_index++;
                    count_arch--;
                }
                // Error en lectura de stat
                if(status==-1){
                    //TODO: Manejar el cierre del programa
                    exit(1);
                }
                // Se obtuvo un archivo o ya no quedan paths por leer
                if(arg_index<arg_c) {
                    if(write_to_slave(write_fd[i], arg_v[arg_index]) == -1){
                        // TODO: Manejar el cierre de archivos, fd, etc
                        exit(1);
                    }
                    arg_index++;
                }
                // Imprimo el hasheo del archivo recibido
                if(printf("%s", arch_hash) < 0){
                    // fclose(read_file);
                    perror("Error: Al imprimir el hasheo");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                    exit(1);
                }
                // Una vez que leimos, movemos el fd del extremo de salida del pipe para poder cerrar el
                // fd anterior y cerrarlo dado que hicimos fdopen
                read_fd[i] = dup(read_fd[i]);
                fclose(read_file);
//                getchar();
                // Un archivo menos para hashear
                arch_already_hashed++;
                // Liberamos los recursos para el string y para abrir el archivo
                free(arch_hash);
            }
        }
    }
    //TODO: solucionamos el problema de que si A se queda mucho tiempo con un md5, no recibe otro hasta terminar
    //Esto es para darle trabajo a los que estan libres
    //El problema es que si B termina muy rapido, va a quedarse bloqueado hasta que md5 le pase algo
    //Seria mejor que B ya tenga trabajo
    //Lo de arriba es lo que solucionamos y lo que dejamos de hacer con esta manera
    // Cerramos los pipes para enviar el EOF a los procesos slave y asi finalizan
    for(int i = 0; i < SLAVES; i++){
        if(close(read_fd[i]) == -1){
            perror("Error: Al cerrar un fd de lectura");
            //TODO: Manejar el cierre
        }
        if(close(write_fd[i]) == -1){
            perror("Error: Al cerrar un fd de escritura");
            //TODO: Manejar el cierre
        }
    }
    // Esperamos a que se cierren los procesos slave antes de finalizar
    for(int i=0; i<SLAVES; i++){
        int slave_status;
        if(wait(&slave_status) == -1){
            perror("Error: Al esperar por un proceso slave");
            //TODO: Manejar el cierre
            exit(1);
        }
        if(WIFEXITED(slave_status) && WEXITSTATUS(slave_status) != 0){
            perror("Error: EL proceso slave no se cerro correctamente");
            //TODO: Manejar el cierre
            exit(1);
        }
    }
    return 0;
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
            perror("Error: Al intentar escribir el argumento");
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
        perror("Error - En lectura de file_path");
        return -1;
    }
    return S_ISREG(prop.st_mode);
}

