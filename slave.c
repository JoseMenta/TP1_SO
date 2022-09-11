// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "slave.h"

// Proceso slave que recibe el archivo por stdin y retorna su md5 por stdout
int main(int arg_c, char ** arg_v){
    //desactivamos el buffer para stdout
    setvbuf(stdout,NULL,_IONBF,0);
    // Leemos de stdin con getline
    size_t len=0;
    char* line = NULL;

    // Almacena la cantidad de caracteres leidos
    ssize_t line_length;
    while((line_length=getline(&line,&len,stdin))>0){
        line[line_length-1] = '\0';

        size_t command_length = strlen(MD5_COMMAND) + line_length;
        char command[command_length];
        strcpy(command, MD5_COMMAND);
        strcat(command, line);

        // Crea el pipe, fork y exec del comando => manejo de errores interno de popen
        FILE *fp = popen(command, "r");
        if (fp == NULL){
            perror("ERROR - Fallo en popen - Slave");
            free(line);
            exit(1);
        }

        //obtenemos el fd del popen
        int fd_md5sum = fileno(fp);

        char md5_result[MD5SUM_SIZE+1];
        char * md5_result_offset = md5_result;
        size_t count;
        size_t remaining = MD5SUM_SIZE;
        //remaining>0 porque dice que puede llevar a errores si remaining es 0
        while(remaining > 0 && (count = read(fd_md5sum, md5_result_offset,remaining)) != 0){
            // Si ocurre un error de lectura, se avisa y aborta
            if(count == -1){
                perror("ERROR - Al recibir el resultado de md5sum - Slave");
                free(line);
                if(pclose(fp) == -1){
                    perror("ERROR - Al cerrar el extremo de lectura del pipe en md5sum - Slave");
                }
                exit(1);
            }
            remaining-=count;
            // Va moviendo el offset para no ir pisando lo que ya se almaceno en md5_result
            md5_result_offset += count;
        }

        // Indicamos el fin de string del hash md5
        md5_result[32] = '\0';
        // Cerramos el popen
        if(pclose(fp) == -1){
            perror("ERROR - Al cerrar el extremo de lectura del pipe - Slave");
            free(line);
            exit(1);
        }
        if(printf("%s,%s,%d\n",line,md5_result,getpid()) < 0){
            perror("ERROR - Fallo printf() - Slave");
            free(line);
            exit(1);
        }

    }
    if(errno != 0){
        perror("ERROR - Leyendo el path del archivo - Slave");
        free(line);
        exit(1);
    }
    // Se libera el string que obtiene el path del archivo
    free(line);
    exit(0);
}

