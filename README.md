#TP N° 1 Sistemas Operativos

El trabajo práctico consiste en aprender a utilizar los distintos tipos de IPCs
presentes en un sistema POSIX. Para ello se implementará un sistema que distribuirá el
cómputo del md5 de múltiples archivos entre varios procesos.

##Requerimientos previos

- Contar con una version de Docker nativa
- Acceso a una terminal linux

##Instructivo
El siguiente sistema cuenta con dos versiones de ejecucion

###Pipeline
1. Ejecutar dentro del contenedor Docker
```sh
$ make all
#Ejemplo:   ./md5  ./prueba/*  | ./vista
$ ./md5  [listado de archivos] | ./vista
```
2. El resultado se podrá ver en terminal una vez finalizado el proceso.




###Procesos en simunltaneo
1. Abrir dos terminales

3. Situarse dentro de un contenedor docker en cada una

5. Ejecutar dentro de una terminal
```sh
$ make all
# Ejemplo:   ./md5  ./prueba/*
$ ./md5  [listado de archivos]
/shm
/read_semaphore
1200
```

6. Este comando arrojara 3 valores. Colocarlos en el comando en el mismo orden de impresion
```sh
#A modo de ejemplo
./vista /shm /read_semaphore 1200
```
7. El resultado sera visible en la segunda terminal


### Links de Interes
- [Acerca de MD5](https://es.wikipedia.org/wiki/MD5 "Acerca de MD5")


