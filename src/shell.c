/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "jobs.h"
#include "csapp.h"


int main()
{
	
	while (1) {
	
		struct cmdline *l;
		//int i, j;
		printf("shell> ");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}
		if (l->seq[0] == NULL){
         	continue;  //Continue meme si on tape une commande vide 
        }
		if(strcmp(l->seq[0][0],"quit")==0 || strcmp(l->seq[0][0], "exit") == 0){ // Test des commande qui permet de quiter
			printf("\nShell exiting.. Goodbye!\n\n");
			kill(0,SIGQUIT);
		}
		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;   //Pour continue le programme
		}

		//if (l->in) printf("in: %s\n", l->in);
		//if (l->out) printf("out: %s\n", l->out);
		//if (l->tachFond) printf("tachFond: %c\n", l->tachFond);
		/* Display each command of the pipe */
		/*
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
			for (j=0; cmd[j]!=0; j++) {
				printf("%s ", cmd[j]);
			}
			printf("\n");
		}
         */
		
		executeCommande(l);   //Executer n'importe quel commande
	
	}
	 return 0;
}
