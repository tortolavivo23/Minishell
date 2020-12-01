#include <stdio.h> //Librería básica
#include <stdlib.h> //Librería básica
#include "parser.h" //Para los tline y tcommand
#include <unistd.h> //Librería general (pipes)
#include <errno.h> //Para usar la función errno
#include <string.h> //Para gestión de strings
#include <sys/types.h> //Librería básica
#include <wait.h> // Para el wait


int main(void) {
	pid_t pid;
	int p1[2];
	int p2[2];

	int status;
        char buf[1024];
        tline * line;
        int i,j;
	char *input = "stdin"; //Creamos variable con la entrada estandar
	char *output = "stdout"; //Creamos variable con la salida estandar
	char *error = "stderr"; //Creamos variable con la salida de error estandar


        printf("==> "); 
        while (fgets(buf, 1024, stdin)) {

                line = tokenize(buf);
                if (line==NULL) {
                        continue;
                }
                if (line->redirect_input != NULL) {
			input = line->redirect_input;
                }
                if (line->redirect_output != NULL) {
			output = line->redirect_output;
                }
                if (line->redirect_error != NULL) {
                       error = line->redirect_error;
                }
                if (line->background) {
                        printf("comando a ejecutarse en background\n");
                } 
                for (i=0; i<line->ncommands; i++) {
			//Hago la ejecución para sólo uno con salida estándar, lo cambiaremos
			pid = fork();
			if(pid==0){
				if(i==0){
					execvp(line->commands[i].argv[0],line->commands[i].argv);
					//Si llega es que se ha producido un error.
					fprintf(stderr, "Error al ejecutar el comando %s\n",strerror(errno));
					exit(1);
				}
			}
			else{
				wait (&status);
               			if (WIFEXITED(status) != 0)
                        		if (WEXITSTATUS(status) != 0)
                                		printf("El comando no se ejecutó correctamente\n");
			}
			printf("orden %d (%s):\n", i, line->commands[i].filename);
                        for (j=0; j<line->commands[i].argc; j++) {
                                printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
                        }
                }
                printf("==> "); 
        }
        return 0;
}

