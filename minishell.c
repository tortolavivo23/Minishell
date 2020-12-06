#include <stdio.h> //Librería básica
#include <stdlib.h> //Librería básica
#include "parser.h" //Para los tline y tcommand
#include <unistd.h> //Librería general (pipes)
#include <errno.h> //Para usar la función errno
#include <string.h> //Para gestión de strings
#include <sys/types.h> //Librería básica
#include <wait.h> // Para el wait
#include <sys/stat.h>
#include <fcntl.h>


int main(void) {
    pid_t pid;
    int p1[2];
    int p2[2];

    int status;
    char buf[1024];
    tline *line;
    int i, j, outputfd, outputbool, inputfd, inputbool, errfd, errbool;
    //Las variables nos serviran para obtener los descriptores de ficherosy para saber si se ha
    //realizado la redireccion mencionada
    char *input; //Creamos variable con la entrada estandar
    char *output; //Creamos variable con la salida estandar
    char *error; //Creamos variable con la salida de eroutpror estandar



    printf("==> ");
    while (fgets(buf, 1024, stdin)) {
        pipe(p1);
        pipe(p2);
        //cada vez que se ejecute un comando estos valores deben estar reseteados, por eso se incluye en el while
        inputbool = 0;
        outputbool = 0;
        errbool = 0;

        line = tokenize(buf);
        if (line == NULL) {
            continue;
        }
        if (line->redirect_input != NULL) {
            inputbool = 1;
            input = line->redirect_input;
        }
        if (line->redirect_output != NULL) {
            outputbool = 1;
            output = line->redirect_output;
        }
        if (line->redirect_error != NULL) {
            errbool = 1;
            error = line->redirect_error;
        }
        if (line->background) {
            printf("comando a ejecutarse en background\n");
        }
        for (i = 0; i < line->ncommands; i++) {
            //Hago la ejecución para sólo uno con salida estándar, lo cambiaremos
            pid = fork();
            if (pid == 0) {
                //Estamos es en el hijo

                //Si es el primer proceso redirigimos la entrada
                if (i == 0) {
                    close(p2[0]);//Si es el primer proceso cerramos el extremo de lectura del pipe
                    close(p1[1]);
                    close(p1[0]);//Si es el primer proceso cerramos todo el pipe
                    //Comprobacion de la redireccion de la entrada standard
                    if (inputbool == 1) {
                        inputfd = open(input, O_RDONLY);
                        if (inputfd == -1) {
                            printf("Error en la redireccion de entrada");
                            exit(1);
                        } else {
                            dup2(inputfd, 0);
                        }
                    }
                } else {
                    if(i%2==0){
                        close(p1[1]);
                        close (p2[0]);
                        dup2(p1[0],0);
                        close (p1[0]);
                    }
                    else{
                        close (p1[0]);
                        close (p2[1]);
                        dup2(p2[0],0);
                        close(p2[0]);
                    }
                }

                //Si es el último proceso redirigimos la salida
                if (i == line->ncommands - 1) {
                    if(i%2==0){
                        close(p2[1]);
                    }
                    else{
                        close(p1[1]);
                    }
                    //Comprobacion de la redireccion de la salida standard
                    if (outputbool == 1) {
                        outputfd = open(output, O_WRONLY);
                        if (outputfd == -1) {
                            //En caso de erro no se ejecuta el comando, pero no terminara la ejecucion de la shell
                            printf("Error en la redireccion de la salida");
                            exit(1);
                        } else {
                            //Redirigimos el descriptor de fichero 1(salida) al del file abierto
                            dup2(outputfd, 1);
                        }
                    }
                    if (errbool == 1) {
                        errfd = open(error, O_WRONLY);
                        if (errfd == -1) {
                            printf("Error en la redireccion de la salida de error");
                            exit(1);
                        } else {
                            dup2(errfd, 2);
                        }
                    }
                }
                else {
                    if(i%2==0){
                        dup2(p2[1],1);
                        close(p2[1]);
                    }
                    else{
                        dup2(p1[1],1);
                        close(p1[1]);
                    }
                }
                //Por ahora solo ejecutamos el primer comando
                execvp(line->commands[i].argv[0], line->commands[i].argv);
                //Si llega es que se ha producido un error.
                fprintf(stderr, "Error al ejecutar el comando %s\n", strerror(errno));
                exit(1);
            }
            /*
            else {
                //Estamos en el padre, espera a que acabe el hijo
                wait(&status);
                if (WIFEXITED(status) != 0)
                    if (WEXITSTATUS(status) != 0)
                        //Comprobaciones de la correcta ejercucion del comando
                        printf("El comando no se ejecutó correctamente\n");

            }


            //Mantenemos la definicion de los comandos otorgado por el test

            printf("orden %d (%s):\n", i, line->commands[i].filename);
            for (j = 0; j < line->commands[i].argc; j++) {
                printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
            }
             */
        }
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        for (i = 0; i < line->ncommands; i++) {
            wait(NULL);
        }
        printf("==> ");
    }
    return 0;
}

