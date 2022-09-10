# TP N° 1 Sistemas Operativos

El trabajo práctico consiste en aprender a utilizar los distintos tipos de IPCs
presentes en un sistema POSIX. Para ello se implementará un sistema que distribuirá el
cómputo del md5 de múltiples archivos entre varios procesos.

## Requerimientos previos

- Contar con una version de Docker nativa
- Acceso a una terminal linux
- Tener instalado el programa _md5sum_ en dicha terminal

## Instructivo
Descargar la imagen de docker
```sh
docker pull agodio/itba-so:1.0
```
Ubicarse en el directorio donde se descargó el proyecto. Luego, abrir una terminal y ejecutar el comando
```sh
make open_docker
```
Esto abrira un contenedor docker donde correra nuesto proyecto

Una vez dentro existen 2 maneras de ejecutar el proyecto
### Pipeline
 Ejecutar dentro del contenedor Docker
```sh
$ make all
#Ejemplo:   ./md5  ./prueba/*  | ./vista
$ ./md5  [listado de archivos] | ./vista
```

El resultado se podrá ver en terminal y en resultado.csv una vez finalizado el proceso.




### Procesos en simultaneo

Ejecutar dentro del contenedor Docker
```sh
make all
# Ejemplo:   ./md5  ./prueba/*
./md5  [listado de archivos]  &
/shm
1200
/read_semaphore
```

Este comando arrojara 3 valores.
Colocarlos en el comando _vista_ en el mismo orden de impresion (esto debe realizarse dentro de los 15 segundos luego de ejecutar a _md5_)
```sh
#A modo de ejemplo
./vista /shm 1200 /read_semaphore
```
El resultado se podrá ver en terminal y en resultado.csv una vez finalizado el proceso.

## Testing
El archivo Makefile cuenta con la posibilidad de realizar pruebas en el código fuente.
### Valgrind
Se puede ejecutar la herramienta de testeo Valgrind mediante el comando:
```sh
make valgrind
```

### PVS-Studio
#### Instalación
Para poder utilizar el programa depurador PVS-Studio, primero se debe instalar. 
Para ello, utilizar los comandos
```sh
make install_pvs_studio
```

Luego, para poder utilizar el programa, todos los archivos .c deben tener en sus primeras dos lineas, los siguientes comentarios:
```sh
# md5.c [Ejemplo]
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
```
Una forma de agilizar esta tarea, es utilizando los siguientes comandos:
```sh
find . -name "*.c" | while read line; do sed -i '1s/^\(.*\)$/\/\/ This is a personal academic project. Dear PVS-Studio, please check it.\n\1/' "$line"; done
find . -name "*.c" | while read line; do sed -i '2s/^\(.*\)$/\/\/ PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com\n\1/' "$line"; done
```

#### Uso
Se puede ejecutar la herramienta de testeo PVS-Studio mediante el comando:
```sh
make pvs_studio
```

## Resultados
El formato de impresion de resultados respeta el formato CSV (comma-separated values)

***Path al archvio  ,  MD5  ,  Pid del proceso encargado del calculo***

***./tests/C.txt,d41d8cd98f00b204e9800998ecf8427e,50
./tests/A.txt,d41d8cd98f00b204e9800998ecf8427e,48
(...)***


### Links de Interes
- [Acerca de MD5](https://es.wikipedia.org/wiki/MD5 "Acerca de MD5")
- [Acerca de Docker](https://www.docker.com/ "Acerca de Docker")
- [Acerca de  CSV](https://es.wikipedia.org/wiki/Valores_separados_por_comas "Acerca de  CSV")

