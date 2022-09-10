GCCFLAGS = -Wall -std=c99 -pthread -lrt

EXEC = md5 slave vista

# Compila, ejecuta en el formato con pipe, muestra los resultados y elimina ejecutables y el archivo csv
show: clean tests compile execute_piped show_results

# Comando final segun lo estipulado por la entrega OJO!! FALTA HACER QUE ENTRE A DOCKER DESDE ACA
all: compile

#borra los ejecutables, csv anterior y shm, ejecuta en el formato pipe, muestra la salida de vista. Deja lo demas para revisar, mejor borrar al principio por caso de error
test: clean tests compile execute_piped

# Borra ejecutables y los resultados
clean: remove_execs remove_results remove_smh_and_semaphore delete

# Realiza el testeo de valgrind
valgrind: clean tests compile run_valgrind

# Realiza el testeo de PVS Studio
pvs_studio: clean tests compile run_pvs_studio show_pvs_studio_results remove_pvs_studio_files

# crea un conjunto de archivos para probar
tests: 
	@mkdir ./tests
	@touch ./tests/A.txt
	@touch ./tests/B.txt
	@touch ./tests/C.txt
	@touch ./tests/D.txt
	@touch ./tests/E.txt
	@touch ./tests/F.txt
	@touch ./tests/G.txt
	@touch ./tests/H.txt
	@mkdir ./tests/DirA
	@mkdir ./tests/DirB
	@mkdir ./tests/DirC
	@mkdir ./tests/DirD


# Compila los archivos necesarios para funcionar
compile:
	@gcc $(GCCFLAGS) md5.c shmADT.c -o md5
	@gcc $(GCCFLAGS) slave.c -o slave
	@gcc $(GCCFLAGS) vista.c shmADT.c -o vista

# Permite ejecutar en el formato con pipe
# Primero se debe hace make compile y tener un directorio tests con los archivos que se desean analizar
execute_piped:
	@./md5 ./tests/* | ./vista

# Muestra los resultados obtenidos en el archivo CSV
show_results:
	@echo "\nFormato CSV: <arch_path>,<arch_md5>,<slave_pid>\n"
	@echo "\nEste es el resultado obtenido:\n"
	@cat ./resultado.csv

# Borra el archivo CSV
remove_results:
	@rm -f resultado.csv

# Borra los ejecutables
remove_execs:
	@rm -f $(EXEC)

remove_smh_and_semaphore:
	@rm -f /dev/shm/*

# Borra los test
delete: 
	@rm -rf ./tests

open_docker:
	@docker run -v "${PWD}:/root" --privileged --rm -ti agodio/itba-so:1.0

run_valgrind:
	@valgrind ./md5 tests/* | ./vista

install_pvs_studio:
	@wget -q -O - https://files.pvs-studio.com/etc/pubkey.txt | apt-key add -
	@wget -O /etc/apt/sources.list.d/viva64.list https://files.pvs-studio.com/etc/viva64.list
	@apt-get install apt-transport-https
	@apt-get update
	@apt-get install pvs-studio
	@pvs-studio-analyzer credentials "PVS-Studio Free" "FREE-FREE-FREE-FREE"

run_pvs_studio:
	@pvs-studio-analyzer trace -- make compile
	@pvs-studio-analyzer analyze
	@plog-converter -a '64:1,2,3;GA:1,2,3;OP:1,2,3' -t tasklist -o report.tasks PVS-Studio.log

show_pvs_studio_results:
	@echo "Errores reportados por PVS Studio:\n"
	@cat report.tasks

remove_pvs_studio_files:
	@rm -f PVS-Studio.log report.tasks strace_out