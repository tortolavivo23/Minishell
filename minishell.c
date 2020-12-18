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


struct dato {
    pid_t pid;
    int ind;
};

struct lista { /* lista simple enlazada */
    struct dato *datos;
    struct lista *sig;
};

/* Devuelve la longitud de una lista */
int longitudl(struct lista *l) {
    struct lista *p;
    int n;
    n = 0;
    p = l;
    while (p != NULL) {
        ++n;
        p = p->sig;
    }
    return n;
}

struct lista *creanodo(void) {
    return (struct lista *) malloc(sizeof(struct lista));
}

/* Inserta dato al final de la lista (para colas) */
struct lista *insertafinal(struct lista *l, struct dato *x) {
    struct lista *p,*q;
    q = creanodo(); /* crea un nuevo nodo */
    q->datos = x; /* copiar los datos */
    q->sig = NULL;
    if (l == NULL)
        return q;
    /* la lista argumento no es vacía. Situarse en el último */
    p = l;
    while (p->sig != NULL)
        p = p->sig;
    p->sig = q;
    return l;
}

struct lista *elimina(struct lista *p, struct dato *x, int (*f)(struct dato *a, struct dato *b)) {
    int cond;
    if (p == NULL) /* no hay nada que borrar */
        return p;
    /* compara el dato */
    cond = (*f)(x,p->datos);
    if (cond == 0) { /* encontrado! */
        struct lista *q;
        q = p->sig;
        free(p); /* libera la memoria y hemos perdido el enlace, por eso se guarda en q */
        p = q; /* restaurar p al nuevo valor */
    } else /* no encontrado */
        p->sig = elimina(p->sig,x,f); /* recurre */
    return p;
}

/* anula una lista liberando la memoria */
struct lista *anulalista(struct lista *l) {
    struct lista *p,*q;
    p = l;
    while (p != NULL) {
        q = p->sig; /* para no perder el nodo */
        free(p);
        p = q;
    }
    return NULL;
}

struct dato * datoFinal(struct lista *l) {
    struct lista *p;

    if (l == NULL)
        return NULL;
    /* la lista argumento no es vacía. Situarse en el último */
    p = l;
    while (p->sig != NULL)
        p = p->sig;
    return p->datos;
}




int cd (int argc, char *argv[]);

int main(void) {
    pid_t pid;
    pid_t pid1;
    int p1[2];
    int p2[2];

    struct lista *l; /* declaración */
    l = NULL; /* inicialización */

    int status;
    char buf[1024];
    char *ruta;
    tline *line;
    int i, j, outputfd, outputbool, inputfd, inputbool, errfd, errbool, ind, backg;
    //Las variables nos serviran para obtener los descriptores de ficherosy para saber si se ha
    //realizado la redireccion mencionada
    char *input; //Creamos variable con la entrada estandar
    char *output; //Creamos variable con la salida estandar
    char *error; //Creamos variable con la salida de eroutpror estandar

    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);

    printf("==> ");
    while (fgets(buf, 1024, stdin)) {
        pipe(p1);
        pipe(p2);
        //cada vez que se ejecute un comando estos valores deben estar reseteados, por eso se incluye en el while
        inputbool = 0;
        outputbool = 0;
        errbool = 0;
        ind = 0; //Para las instrucciones como el cd.
        backg=0;

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
            backg=1;
            pid1 = fork();
            if(pid1!=0){
                printf("==> ");
                continue;
            }
            else{
                struct dato *d1=(struct dato *) malloc(sizeof(struct dato));
                struct dato *d2= datoFinal(l);
            }
        }
        if ((line->ncommands>=1)&&(strcmp(line->commands[0].argv[0], "cd")== 0)) {//Si es un cd, tenemos que hacer la instrucción de otra forma
            ind++;
            if (line->ncommands == 1) {//Si hay más de un comando el cd no se ejecuta
                cd(line->commands[0].argc, line->commands[0].argv);
            }
        }
        for (i = ind; i < line->ncommands; i++) {
            //Ejecucion de los procesos
            pid = fork();
            if (pid == 0) {
                //Estamos es en el hijo
                if(backg==0){
                    //El padre ignora sigint y sigquit si el proceso se realiza en background debe seguir asi,
                    //en caso contrario (fg) deben realizar su accion por defecto:
                    signal(SIGINT,SIG_DFL);
                    signal(SIGQUIT,SIG_DFL);
                }
                if (strcmp(line->commands[i].argv[0], "cd") ==
                    0) {//Si es un cd en medio de muchas instrucciones no tiene que hacer nada
                    exit(0);
                }
                //Redirecciones de la entrada:
                //Si es el primer proceso redirigimos la entrada standar si es necesario
                if (i == 0) {
                    close(p2[0]);//Si es el primer proceso cerramos el extremo de lectura del pipe
                    close(p1[1]);
                    close(p1[0]);//Si es el primer proceso cerramos todo el pipe
                    //Comprobacion de la redireccion de la entrada standard
                    if (inputbool == 1) {
                        inputfd = open(input, O_RDONLY);
                        if (inputfd == -1) {
                            printf("Error en la redireccion de entrada\n");
                            exit(1);
                        } else {
                            dup2(inputfd, 0);
                        }
                    }
                } else {
                    if (i % 2 ==
                        0) { //Si el proceso es par, cerramos el extremo de escritura de p1, el de lectura de p2 y redireccionamos el extremo de lectura de p1 a stdin
                        close(p1[1]);
                        close(p2[0]);
                        dup2(p1[0], 0);
                        close(p1[0]);
                    } else { //Si el proceso es impar hacemos lo mismo pero cambiando los pipes.
                        close(p1[0]);
                        close(p2[1]);
                        dup2(p2[0], 0);
                        close(p2[0]);
                    }
                }
                //Redirecciones de la salida:
                //Si es el último proceso redirigimos la salida standar si es necesario:
                if (i == line->ncommands - 1) {
                    if (i % 2 ==
                        0) { //Si es el último proceso, cerramos el único extremo de escritura que queda abierto.
                        close(p2[1]);
                    } else {
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
                } else {
                    if (i % 2 == 0) { //Redirecciono stdout a la entrada del pipe.
                        dup2(p2[1], 1);
                        close(p2[1]);
                    } else {
                        dup2(p1[1], 1);
                        close(p1[1]);
                    }
                }
                //Ejecutamos el comando
                execvp(line->commands[i].argv[0], line->commands[i].argv);
                //Si llega es que se ha producido un error.
                fprintf(stderr, "Error al ejecutar el comando %s: %s\n", line->commands[i].argv[0],
                        strerror(errno));
                exit(1);
            } else {
                if (i % 2 ==
                    0) { //Cerramos p1 y lo volvemos a abrir sin ningún valor si estamos en una iteración par
                    close(p1[0]);
                    close(p1[1]);
                    pipe(p1);
                } else { //Hacemos lo mismo con p2 si estamos en una iteración impar
                    close(p2[0]);
                    close(p2[1]);
                    pipe(p2);
                }
            }
        }
        //Cerramos el pipe por si se quedara abierto
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);

        for (i = ind; i < line->ncommands; i++) {
            //Esperamos que acaben todos los procesos
            waitpid(pid,&status,0);
        }
        if(backg==1) {
            printf("bg finalizado \n");
            exit(0);
        }
        printf("==> ");
    }
    printf("\n");
    l=anulalista(l);
    return 0;
}

int cd(int argc, char *argv[]){
    char buffer[1024];
    char *ruta;
    if (argc==1) { //Si no se le pasa ruta hacemos cd a home
        ruta = getenv("HOME");
        if (ruta == NULL) { //Si $HOME es nulo, da un error, si no ponemos que se va a intentar el cambio de directorio
            fprintf(stderr, "No existe la variable $HOME\n");
            return (1);
        }
    }
    else if(argc==2){
        ruta = argv[1];
    }
    else{
        printf("Uso: \ncd RUTA\n");
        return (2);
    }
    if (chdir(ruta)!=0) {//Se intenta el cambio de directorio
        fprintf(stderr, "Error al cambiar de directorio: %s\n", strerror(errno));
        return (2);
    }
    printf("El directorio actual es: %s\n", getcwd(buffer, -1));
    return (0);
}

