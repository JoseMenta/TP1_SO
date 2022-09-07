//
// Created by Jose Menta on 04/09/2022.
//

// feature_test_macro para getline
#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MD5SUM_SIZE 32

//proceso slave que recibe el archivo por stdin y retorna su md5 por stdout
int main(int arg_c, char ** arg_v){
    //TODO: sacar, es para desarrollo
    setvbuf(stdout, NULL, _IONBF, 0);

    // Leemos de stdin con getline
    size_t len=0;
    char* line = NULL;

    // Almacena la cantidad de caracteres leidos
    ssize_t line_length;
    while((line_length=getline(&line,&len,stdin))>0){
//        fprintf(stderr,"El proceso %d Recibe el argumento %d y %d",getpid(),line[0],line[1]);
//        if(line[1]==0){
//            fprintf(stderr,"Error, llego un %c como argumento\n",line[0]);
//            continue;
//        }
        // PROCEDIMIENTO
        // Hacer un proceso que se pise con md5sum (/usr/bin) pero le cambio la salida estandar a un pipe para leerlo
        // Leo del pipe el resultado
        // Mando el resultado por stdout

        // Cada vez que leamos un nuevo archivo, creamos un pipe para el proceso md5sum
        int md5sum_pipe[2];
        // Si falla el pipe, se avisa y aborta
        if(pipe(md5sum_pipe) == -1){
            perror("Error: Creación del pipe para md5sum");
            exit(1);
        }

        // TODO: Verificar si hace falta modificar el \n al recibir de master
        line[line_length-1] = '\0';
//        fprintf(stderr,"%s",line);
        // Creamos el proceso hijo
        pid_t pid = fork();
        switch (pid) {
            case -1: {
                // Si tuvimos un error al intentar crear el proceso para el md5sum, se avisa y aborta
                free(line);
                perror("Error: Creacion del procesos para correr md5sum");
                exit(1);
            }
            case 0: {
                // Estamos en el proceso hijo, que va a correr a md5sum
                // Cambiamos a stdin por el pipe
                // Cerramos el extremo de lectura del pipe
                if(close(md5sum_pipe[0]) == -1){
                    free(line);
                    perror("Error: Al cerrar el extremo de lectura del pipe en md5sum");
                    exit(1);
                }
                /*
                // Cerramos la salida a la terminal
                if(close(STDOUT_FILENO) == -1){

                } */
                // Llevamos el extremo de escritura del pipe a STDOUT
                if(dup2(md5sum_pipe[1],STDOUT_FILENO) == -1){
                    free(line);
                    perror("Error: Al copiar el extremo de escritura del pipe en stdout de md5sum");
                    exit(1);
                }
                // Cerramos el extremo de escritura del pipe
                if(close(md5sum_pipe[1]) == -1){
                    free(line);
                    perror("Error: Al cerrar el extremo de escritura del pipe en md5sum");
                    exit(1);
                }
                // Definimos un arreglo de 3 strings para los argumentos de md5sum
                //  0: Path del programa md5sum (estandar)
                //  1: Path del archivo a aplicar md5sum
                //  2: NULL para el funcionamiento de execv
                char* args[3];
                args[0] = "/usr/bin/md5sum";    // Path del archivo
                //TODO: preguntar por el free en los hijos
                //Si tengo que hacerlo, entonces debo mandarle una copia del string de line

                // args[1] = line; //Esto no es constante, me lleva a problemas si se cambia line mientras carga con exec?
                // En este caso no pasa, porque abajo espera a que empiece este proceso
//                char file[line_length+1];//No usamos len, eso es la longitud del buffer, no d}

                // el string
//                strcpy(file,line);
//                args[1] = file; //Esta mal pasar esto? No es un string constante, no se hasta cuando necesita que no se cambie

//                free(line);//En este caso, lo libero aca, para no tener leaks (exec lo soluciona ya?)
                //TODO: Preguntar si esta bien que tanto el padre como el hijo hagan el free
                args[1] = line; // Path del archvo a hashear
                args[2] = NULL; // Indica el final de argumentos
                // Si hubo error al crear md5sum, se alerta y aborta
                if(execv("/usr/bin/md5sum",args) == -1) {//Preguntar por const char** aca
                    perror("Error: no fue posible ejecutar el programa md5sum");
                    exit(1); //con esto, termina el proceso del md5sum con error
                    //TODO: preguntar si este debe liberar recursos o los tiene que liberar el padre
                    //Pregunta seria: malloc duplica lo que tenia el padre en el hijo?
                }
            }
            default: {
                // Estamos en el proceso slave, tenemos que esperar a recibir el resultado por el pipe
                // Cerramos el extremo de escritura del pipe pues solo lee
                if(close(md5sum_pipe[1]) == -1){
                    free(line);
                    perror("Error: Al cerrar el extremo de escritura del pipe en Slave");
                    exit(1);
                }

                // Leemos el resultado de md5sum y lo guardamos en un arreglo de 32 caracteres, +1 para el \0
                char md5_result[MD5SUM_SIZE+1];
                /*char * md5_result;
                size_t size;
                // TODO: Preguntar si se puede usar FILE *
                FILE * md5_file = fdopen(md5sum_pipe[0], "r");
                if(md5_file == NULL){
                    free(line);
                    perror("Error: Al intentar abrir el md5sum");
                    exit(1);
                }
                if(getdelim(&md5_result, &size, ' ', md5_file) == -1){
                    fclose(md5_file);
                    free(line);
                    perror("Error: Al leer el hash de md5sum");
                    exit(1);
                }*/



                // El offset es para ir moviendo la posicion donde se debe ir leyendo y asi el hasheo no se solapa
                char * md5_result_offset = md5_result;
                // Si hubo error leyendo, se alerta y aborta
                // No se puede hacer el wait antes de read
                // Si el pipe no es lo suficientemente grande, slave se bloquea hasta que termine el hijo
                // El hijo se bloquea hasta que lea slave => deadlock
                size_t count;
                size_t remaining = MD5SUM_SIZE;
                //remaining>0 porque dice que puede llevar a errores si remaining es 0
                while(remaining > 0 && (count = read(md5sum_pipe[0], md5_result_offset,remaining)) != 0){
                    // Si ocurre un error de lectura, se avisa y aborta
                    if(count == -1){
                        free(line);
                        perror("Error: Al recibir el resultado de md5sum");
                        exit(1);
                    }
                    remaining-=count;
                    // Va moviendo el offset para no ir pisando lo que ya se almaceno en md5_result
                    md5_result_offset += count;
                }
                // Indicamos el fin de string del hash md5
                md5_result[32] = '\0';


                // Cerramos el extremo de lectura del pipe pues ya leimos el resultado
                if(close(md5sum_pipe[0]) == -1){
                    free(line);
                    //fclose(md5_file);
                    //free(md5_result);
                    perror("Error: Al cerrar el extremo de lectura del pipe en Slave");
                    exit(1);
                }
                // Esperamos a que termina de ejecutarse el proceso md5sum
                int child_status;
                // Si falla waitpid, se alerta y aborta
                if(waitpid(pid, &child_status, 0) == -1){
                    free(line);
                    //fclose(md5_file);
                    //free(md5_result);
                    perror("Error: Fallo waitpid()");
                    exit(1);
                }
                // Si el proceso md5sum falla, se alerta y aborta
                if(WIFEXITED(child_status) && WEXITSTATUS(child_status) != 0){
                    free(line);
                    //fclose(md5_file);
                    //free(md5_result);
                    perror("Error: Al finalizar el proceso md5sum");
                    exit(1);
                }
                // Enviamos por salida estandar (en realidad, lo obtiene el master) el resultado del md5sum
                // TODO: Dejarlo para despues, tenemos que decidir como se hace
                if(printf("%s;%s;%d\n",line,md5_result,getpid()) < 0){
                    free(line);
                    //fclose(md5_file);
                    //free(md5_result);
                    perror("Error: Fallo printf()");
                    exit(1);
                }
                //fclose(md5_file);
                //free(md5_result);
            }
        }
        //Si pasan el \0 en el string, debo hacer esto
//        fprintf(stderr,"el caracter que queda es %d \n",getchar());

    }
    //Liberar los recursos
    free(line);
    return 0;
}

