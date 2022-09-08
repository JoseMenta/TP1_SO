# Compila, ejecuta en el formato con pipe, muestra los resultados y elimina ejecutables y el archivo csv
all: compile execute_piped show_results clean

# Compila los archivos necesarios para funcionar
compile:
	gcc -Wall -std=c99 md5.c -o md5 -pthread -lrt
	gcc -Wall -std=c99 slave.c -o slave
	gcc -Wall -std=c99 vista.c -o vista -pthread -lrt

# Permite ejecutar en el formato con pipe
# Primero se debe hace make compile y tener un directorio tests con los archivos que se desean analizar
execute_piped: $(md5) $(vista) $(slave) $(tests)
	@./md5 ./tests/* | ./vista

# Borra ejecutables y los resultados
clean: remove_execs remove_results

# Muestra los resultados obtenidos en el archivo CSV
show_results:
	@echo "\nFormato CSV: <arch_path>,<arch_md5>,<slave_pid>\n"
	@echo "\nEste es el resultado obtenido:\n"
	@cat ./resultado.csv

# Borra el archivo CSV
remove_results:
	@rm resultado.csv

# Borra los ejecutables
remove_execs:
	@rm md5
	@rm slave
	@rm vista

remove_smh_and_semaphore:
	@rm /dev/shm/*