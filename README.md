# TP N° 1 Sistemas Operativos

El trabajo práctico consiste en aprender a utilizar los distintos tipos de IPCs
presentes en un sistema POSIX. Para ello se implementará un sistema que distribuirá el
cómputo del md5 de múltiples archivos entre varios procesos.

## Requerimientos previos

- Contar con una version de Docker nativa
- Acceso a una terminal linux

## Instructivo
Abrir una terminal y ejecutar el comando
```sh
make docker
```
Esto abrira una imagen docker donde correra nuesto proyecto

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
Colocarlos en el comando en el mismo orden de impresion
```sh
#A modo de ejemplo
./vista /shm 1200 /read_semaphore
```
El resultado se podrá ver en terminal y en resultado.csv una vez finalizado el proceso.

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

