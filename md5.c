//
// Created by Jose Menta on 04/09/2022.
//

// glibc version: 2.19
// feature_test_macro para getline y fdopen
#define _POSIX_C_SOURCE >= 200809L

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#define SLAVES 5

/*
typedef struct {
    char * arch_path;
    char * md5_hash;
    int slave_pid;
} arch_hash;*/



int main(int arg_c, char** arg_v){
    // Recibe como argumentos a los nombres de los archivos que se desean procesar
    // Si no recibe ninguno, finaliza
    if(arg_c <= 1){
        perror("Error: No se recibieron archivos");
        exit(1);
    }
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
            case -1:
            {
                perror("Error: Creación del proceso Slave");
                // TODO: Manejar el cierre de archivos, pipes y mallocs
            }
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

    // Creamos un conjunto de fd para los pipes de lectura


    // Guarda la cantidad de archivos a hashear
    int count_arch = arg_c - 1;
    // Indica la cantidad de archivos que faltan hashear
//    int arch_to_hash_left = count_arch;
    // Indica la cantidad de archivos ya hasheados
    int arch_already_hashed = 0;
    // Indica el proximo archivo a hashear
    int index = 1;

    // Iremos enviando el path de los archivos a hashear y leyendo los hasheos
    while(arch_already_hashed < count_arch){
        // Configuramos los conjuntos con los elementos de lectura y escritura a analizar
        fd_set read_fd_set;
        FD_ZERO(&read_fd_set);
        // Creamos un conjunto de fd para los pipes de escritura
        fd_set write_fd_set;
        FD_ZERO(&write_fd_set);
        int max = 0;
        for(int i = 0; i < SLAVES; i++){
            max = (read_fd[i]>max)?read_fd[i]:max;
            FD_SET(read_fd[i], &read_fd_set);
            max = (write_fd[i]>max)?write_fd[i]:max;
            FD_SET(write_fd[i], &write_fd_set);
        }
        // Un solo select con sets de lectura y escritura retorna en los sets
        // los file descriptors disponibles para leer/escribir
        if(select(max+1, &read_fd_set, &write_fd_set, NULL, NULL) == -1){
            perror("Error: Al realizar select()");
            // TODO: Manejar el cierre de archivos, pipes y mallocs
        }
        // Repaso todos los fd de lectura para saber cuales se pueden leer
        for(int i = 0; i<SLAVES; i++){
            if(FD_ISSET(read_fd[i],&read_fd_set)){
                // Si esta disponible este fd para leer, leo el hash md5 obtenido
                // Primero creo un FILE * para poder leer hasta \n
                FILE * read_file = fdopen(read_fd[i], "r");
                if(read_file == NULL){
                    perror("Error: Al intentar abrir el fd read");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                }
                char * arch_hash = NULL;
                size_t arch_hash_len = 0;
                // Lectura del fd hasta \n (printf termina en \n)
                if(getline(&arch_hash, &arch_hash_len, read_file) == -1){
                    //fclose(read_file);
                    perror("Error: Al leer del slave");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                }
                // Imprimo el hasheo del archivo recibido
                if(printf("%s", arch_hash) < 0){
                    // fclose(read_file);
                    perror("Error: Al imprimir el hasheo");
                    // TODO: Manejar el cierre de archivos, pipes y mallocs
                }
                read_fd[i] = dup(read_fd[i]);
                fclose(read_file);
//                getchar();
                // Un archivo menos para hashear
                arch_already_hashed++;
                // Liberamos los recursos para el string y para abrir el archivo
                free(arch_hash);
                // TODO: Ver como cerrar los FILE *
                // fclose(read_file);
            }
        }
        // Una vez que leimos aquellos fd que se podian, vamos a comprobar si podemos enviar nuevos archivos
        // para hashear a los procesos slave
        for(int i = 0; i<SLAVES && index<arg_c; i++){
            // Si el fd esta listo para escribir, entonces le paso el proximo archivo a leer

            if(FD_ISSET(write_fd[i],&write_fd_set)){
                // Tengo que escribir el argumento completo al proceso slave
                size_t len = strlen(arg_v[index]);
                char aux[len+2];
                strcpy(aux,arg_v[index]);
                aux[len]='\n';
                aux[len+1] = '\0';
                size_t curr = 0;
                printf("%s", aux);
                long remaining = len+2; //len+1 porque no debemos escribir el \0, porque del otro lado el getline no lo saca
                //Entonces lleva a que en la proxima lectura lo que vea es el string vacio
                //TODO: Hay 2 maneras de arreglarlo: esta o hacer el getchar en slave, sabiendo que tiene que sacar el \n
                while(remaining>0){
                    //Quiero que se quede bloqueado hasta que termine de escribir
                    ssize_t written = write(write_fd[i],aux+curr,remaining);
                    if(written==-1){
                        perror("Error: Al intentar escribir el argumento");
                        //TODO: Manejar el cierre
                    }
                    curr+=written;
                    remaining-=written;
                }
                index++;

            }
        }
    }

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
        }
        if(WIFEXITED(slave_status) && WEXITSTATUS(slave_status) != 0){
            perror("Error: EL proceso slave no se cerro correctamente");
            //TODO: Manejar el cierre
        }
    }
    return 0;
}