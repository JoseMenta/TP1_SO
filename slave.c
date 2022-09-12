// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "slave.h"

// Proceso Slave que recibe el path del archivo por stdin y retorna su md5 por stdout
int main(int arg_c, char ** arg_v){
    // Desactivamos el buffer para stdout
    setvbuf(stdout,NULL,_IONBF,0);

    // Leemos de stdin con getline
    size_t line_len=0;
    char* line = NULL;
    ssize_t line_length;

    while((line_length=getline(&line,&line_len,stdin))>0){
        line[line_length-1] = '\0';

        size_t command_length = strlen(MD5_COMMAND) + line_length;
        char command[command_length];
        strcpy(command, MD5_COMMAND);
        strcat(command, line);

        // Crea el pipe, fork y exec del comando => el manejo de errores es interno en popen
        FILE *fp = popen(command, "r");
        if (fp == NULL){
            perror("ERROR - Al usar popen - Slave");
            free(line);
            exit(1);
        }

        char*  md5_result = NULL;
        size_t  md5_buff_len = 0;
        ssize_t md5_result_len = 0;
        if((md5_result_len = getdelim(&md5_result, &md5_buff_len, ' ', fp)) == -1){
            perror("ERROR - Al recibir el resultado de md5sum - Slave");
            free(line);
            free(md5_result);
            if(pclose(fp) == -1){
                perror("ERROR - Al cerrar el extremo de lectura del pipe en md5sum - Slave");
            }
            exit(1);
        }

        // Indicamos el fin de string del hash md5
        md5_result[md5_result_len-1] = '\0';

        // Cerramos el popen
        if(pclose(fp) == -1){
            perror("ERROR - Al cerrar el extremo de lectura del pipe - Slave");
            free(md5_result);
            free(line);
            exit(1);
        }
        if(printf("%s,%s,%d\n",line,md5_result,getpid()) < 0){
            perror("ERROR - Fallo printf() - Slave");
            free(md5_result);
            free(line);
            exit(1);
        }
        free(md5_result);
    }
    // Se libera el string que obtiene el path del archivo
    free(line);

    if(errno != 0){
        perror("ERROR - Leyendo el path del archivo - Slave");
        exit(1);
    }
    exit(0);
}

