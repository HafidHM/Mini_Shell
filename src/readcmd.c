/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "readcmd.h"
#include "jobs.h"
#include "csapp.h"
#include <signal.h>
#define Stopped 0      //Processes Stopped in the background
#define Running 1      //Processes Runing in the background
#define Terminated 2   //Processes is terminated

static int count = 0; //Compter le nombre des fils en arriere plan
static int count1 = 0;   // Compter le nombre des fils stoppés 
static int s = 0;         //sert pour tester si on a un signal "SIGTSTP" ou non
static int countJobs = 1; // Compter le nombre des jobs 
Jobs *listJobs = NULL;   // Liste des Jobs 

void executeSimpleCommande(char** cmd){ //Cette fonction permet d'executer une commande Simple (sans pip "|" ou redirection "<" ">" ">>")
	if(strcmp(cmd[0],"jobs") == 0){  //Si la commande est "jobs"

            printf("\n*****************************************\n");
            printf("\n*                 Jobs                  *\n"); 
            printf("\n*****************************************\n");
            printf("[Num]\t\t[Status]\t\t[PID]\t\t[Name]   \n");
            DisplayJobs(listJobs); //afficher la liste des jobs

    }else if (strcmp(cmd[0],"quit")==0 || strcmp(cmd[0], "exit") == 0){ //Si la commande est "quit" ou "exit", on vide la liste des jobs
    	FreeJobs(listJobs);
    }else if (strcmp(cmd[0],"kill")== 0 && strcmp(cmd[1],"-9")== 0){ //Si la commande est kill 
    	int Pid = atoi(cmd[2]);   //Canvertir l’argument du numéro 2 en un entier
    	kill(Pid,SIGKILL); //On Tue le prossecus fils du pid  "Pid"
    	//changeStatus(Pid);
    	printf("The Processes %d is terminated\n",Pid);  

    }else if(strcmp(cmd[0],"kill")== 0 && strcmp(cmd[1],"-2")== 0){
        int Pid = atoi(cmd[2]);   //Canvertir l’argument du numéro 2 en un entier
    	kill(Pid,SIGTSTP); //On stope le prossecus fils du pid  "Pid"
    	changeStatus(Pid); //fonction permet de changer le status d'un processus terminé en status : 2 (Terminated)
    }
    else{
    	int status = execvp(cmd[0],cmd);      //executer une commande simple avec execvp()
	    if(status == -1){   //statut de la commande soit "=0" => sucess  ,  "=-1" => echec                
			printf("%s: Command not found\n", cmd[0]); //Si la commande ne s'exécute pas donc soi la commande n'existe ou non implémentée 
		}
    }
}


void redirectionEntreeSortie(char * in, char * out){ //commande de type redirection ">" "<"
    int descriptor;
    if (in != NULL){   //tester si on a une redirection  "<"
    	descriptor = open(in,O_RDONLY); //Ouvrire le fichier "in" en mode lecture seule
		if(descriptor < 0){  // Tester si on a bien retourner notre descripteur fichier
			printf("%s : Permission denied\n",in); ////Erreur sur le droit de lecture du fichier "in"
		}else{
			dup2(descriptor,0);	 //Changer la sortie standard "stdin" avec le descripteur du fichier "in"
			close(descriptor);   // Fermer le descripteur de notre fichier "in"
		} 
    }

    if(out != NULL){ //tester si on a une redirection vers la sortie standard ">"
		descriptor = open(out,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH); //Ouvrire le fichier "out" en mode ecriture s'il n'existe pas il va le cree 
		if(dup2(descriptor,1) < 0){  // Tester si on a bien retourner notre descripteur fichier 
       	 	printf("%s.txt: Permission denied\n",out);//Erreur sur les droits de manipulation du fichier lu ou crée "out"
		}else{
		    dup2(descriptor, 1); //Changer la sortie standard "stdout" avec le descripteur du fichier "out"
		    close(descriptor);	// Fermer le descripteur de notre fichier "out"
		}
	}
}


void executeCommandePipe(struct cmdline *l){ //Exécuter une commande avec multiple pipes
	int nbrPipe=0;
	int** p;
	pid_t* pid = NULL;

	for (int i=0; l->seq[i]!=0; i++) {	//calculer le nombre des pipes 		
			 nbrPipe++; 
	}// nombre de pipes: nbrPipe-1 


     p = (int **) malloc (sizeof(int) * (nbrPipe));  //Allouer les lignes d'un matrice pour n-pipes(= nbrPipe -1 ) 
     pid = (pid_t *) malloc (sizeof(pid_t) * nbrPipe -1 );  // Allouer un tableau dynamique pour enregistré les processus exécutés dans les n-pipes pour les tués après     

     for(int i=0;i<nbrPipe-1;i++){   
			p[i]= (int *) malloc(2*sizeof(int));  //Allouer les colonnes du matrice pour deux case: l'entreé et la sortie de la pipe
			if (pipe(p[i])<0){  //Tester si une pipe est bien creé
				printf("Creation pipe Failed\n");  
			}
	 }

     for (int i = 0; i < nbrPipe; ++i){
      	pid[i] = fork();  //Dans chaque itération on crée un fils pour chaque commande dans la table "cmd"
      	if(pid[i]<0){     // Tester le statut du création d'un fils
            printf("Error creation a Child\n");
      	}else if (pid[i] == 0){  //On est dans le fils.On commence à configurer les pipes, on traite 3 cas principaux
             if (i==0){   //Cas 1 : La première commande, sa sortie doit être l'entrée de la prochaine pipe
              	    redirectionEntreeSortie(l->in,l->out); //Faire une redirection s'elle existe
              	    dup2(p[i][1],STDOUT_FILENO); //Configurer la premiere pipe, sa sortie est enregiistrer  dans le fichier temporaire STDOUT_FILENO
					for(int j=0;j<nbrPipe-1;j++){
						if(j==0){
							close(p[j][0]);  //Fermer l'entrée de la pipe "j" 
						}else{
							close(p[j][0]);  //Fermer l'entrée de la pipe "j"
							close(p[j][1]);  //Fermer la sortie de la pipe "j"
						}
					}
              }else if (i == nbrPipe-1){    //Cas 2: La dernière commande: l'entrée de cette dernière doit être la sortie de la dernière pipe "Indice=nbrPipes-1"
                 redirectionEntreeSortie(l->in,l->out); //Faire une redirection s'elle existe
              	dup2(p[i-1][0],STDIN_FILENO);    ////Configurer la dernière pipe, l'entrée de cette dernière est la sortie de la dernière pipe
              	for(int j=0;j<nbrPipe-1;j++){
						if(j== i-1){
							close(p[j][1]);    //Fermer la sortie de la pipe "j"
						}else{
							close(p[j][0]);    //Fermer l'entrée de la pipe "j"
							close(p[j][1]);    //Fermer la sortie de la pipe "j"
						}
				}
              }else{  //Cas 3: Les commandes au milieu
                    dup2(p[i][1],STDOUT_FILENO);     //Configurer la pipe "i-1" pour qu'elle soit l'entree   
					dup2(p[i-1][0],STDIN_FILENO);    //de la pipe suivante
					for(int j=0;j<nbrPipe-1;j++){
						if(j==i){
							close(p[j][0]);   // Fermer l'entrée de la pipe "j"
						}else if(j==i-1){
							close(p[j][1]);   // Fermer la sortie de la pipe "j"
						}else{
							close(p[j][0]);   // Fermer l'entrée de la pipe "j"
							close(p[j][1]);   // Fermer la sortie de la pipe "j"
						}
					}
              }

               executeSimpleCommande(l->seq[i]);  //Executer une commande simple
               exit(0);   //Fils mort 
      	}
     }
        for(int i =0;i<nbrPipe-1;i++){ //Fermer les entree et sorties de toutes les pipes 
			close(p[i][0]);
			close(p[i][1]);
		}

		for(int i=0;i<nbrPipe;i++){  //Attendre tous les fils  crées 
			waitpid(pid[i],NULL,0);
		}
		for(int i=0;i<nbrPipe-1;i++){ 
			free(p[i]);
		}
		free(p);   //Libérer le matrice des pipe
		free(pid);  //Libérer le pid
}


void executeCommandeRediretionOut(char** cmd, char* out){  //Exécuter une commande de redirection de type ">"
	if (strcmp(out,">")!=0) 
	{  	   redirectionEntreeSortie(NULL,out); 
		   executeSimpleCommande(cmd);
	}else{
		perror("Command no implemented\n");  //Au cas ou on a une commande de type ">>" on affiche ce message
	}
}


void executeCommandeRediretionIn(char** cmd, char* in,char* out){  //Exécuter une commande de redirection de type "<"
	redirectionEntreeSortie(in,out);
	executeSimpleCommande(cmd);
}


void handler_CHILD(int sig){ //Cette fonction ramasse les processus  terminés pour éviter la prolifération de zombis
	pid_t fils;
	while((fils = waitpid(-1, NULL, WNOHANG)) > 0) {
            printf("[%d] : terminated in Background\n", fils);
    }
}

void handler_SIGINT(int sig){   // "Ctr+C" Cette fonction s'exécute lorsque le fils reçoit un signal de type SIGINT
    printf("\nSIGINT recus ^^ \n");
    fflush(stdout); //Vider le buffer
    return;   //pour continue dans notre shell
}
void handler_SIGTSTP(int sig){  // "Ctr+Z" :Cette fonction s'exécute lorsque le fils reçoit un signal de type SIGTSTP
    s = 1;
    count1++;   //Incrémenter le nombre des processus stoppés dans l'arrière-plan
    printf("\nSIGTSTP recus ^^\n");
    fflush(stdout);  //Vider le buffer
    return;   //pour continue dans notre shell 
}


Jobs* ADDjob(pid_t pid, char* name, int status, Jobs* listJobs){  //Ajouter un job dans la liste static des jobs "listJobs"
	Jobs* job  =(Jobs*) malloc(sizeof(Jobs));  // Crée un job dynamiquement de type "Jobs"
	if (job == NULL) //Tester la creation d'un job
		exit(EXIT_FAILURE);
	job->pid  = pid;
	job->jbname = malloc(sizeof(char)*(strlen(name)+1));
	strcpy(job->jbname, name);  // pour copie le nom proprement 
	job->status = status;  
	job->num = countJobs;
	job->next = listJobs;  // chainer le job avec la liste "listJobs"
	countJobs++; //Incrimenter le numero pour le job suivant
	return job;  //Retourner le job
}


void DisplayJobs(Jobs* listJobs){ //Affichage récursive de la liste des jobs "listJobs" 
	if (listJobs==NULL){  //tester si on pas encore atteint la fin de la liste 
		return;
	}
		int  ProssStatus = listJobs->status;
		if(ProssStatus==1){  //tester le statut d'un processus
            printf("[%d]+\t\tRunning\t\t%d\t\t%s\t\n", listJobs->num,listJobs->pid,listJobs->jbname);
        }else if(ProssStatus==0){
            printf("[%d]+\t\tStopped\t\t%d\t\t%s \n", listJobs->num,listJobs->pid, listJobs->jbname);
        }else{
            printf("[%d]+\t\tTerminited\t\t%d\t\t%s \n", listJobs->num,listJobs->pid, listJobs->jbname);
        }
	  DisplayJobs(listJobs->next); //pour afficher le job suivant
}


void FreeJobs(Jobs* listJobs){  //
	while (listJobs != NULL) { /* tant que la liste n'est pas vide je libère job par job */
        Jobs *cell = listJobs;
        listJobs = listJobs->next;
        free(cell);
    }
}

void changeStatus(pid_t pid){ //fonction permet de changer le status d'un processus terminé en status : 2 (Terminated)
 Jobs *cell = listJobs;
	if(listJobs!=NULL){
       while (listJobs != NULL) {
             if (listJobs->pid == pid)
              {
              	listJobs->status = Terminated;
              	return;
              } 
	        listJobs = listJobs->next;
       }
	}
	return;
}


void executeCommande(struct cmdline *l){
   int background = 0; // Variable pour savoir le statut d'un processus
   signal(SIGCHLD, handler_CHILD); 
   signal(SIGINT,SIG_IGN);   //Ignorer le signal SIGINT
   signal(SIGTSTP,SIG_IGN);  //Ignorer le signal SIGTSTP

   if (l->tachFond == '&'){ //Tester si l'utilisateur a tapé le caractère "&" aprés un programme 
   	   background = 1;  //Pour dire que le processus est on arrière-plan 
   }

    pid_t pid = fork();  //Cree un fils
    signal(SIGINT,handler_SIGINT); // Attendre un signal SIGINT
    signal(SIGTSTP, handler_SIGTSTP); // Attendre un signal SIGTSTP
   
	if (pid<0){ //Tester la creation d'un processus fils
			perror("Error Creation Child ");
	}

    if(pid==0){    //C'est le fils

		        if (l->seq[1] == NULL){  //Commande sans pipes
			    	if (l->out==NULL && l->in==NULL){ // Commande Simple 
			    		executeSimpleCommande(l->seq[0]);
			    	}else if(l->in==NULL){ //Commande avec redirection  ">"
			    		executeCommandeRediretionOut(l->seq[0],l->out);
			    	}else{ //Commande avec redirection  "<""
			    		  executeCommandeRediretionIn(l->seq[0],l->in,l->out);  
			    	}
			    	
			    }else{   //Commande avec pipes "|"
			    	executeCommandePipe(l);
			    }	
			exit(0); //fils mort  
	}else{  //C'est le Pere

		    if (background){ //Si on est on est en arriere plan 
		    	count++;      // Compter le nombre des fils qui s'exécutent dans l'arrière-plan dans la session courante
               printf("[%d]+ %d\n",count,pid);
               listJobs=ADDjob(pid,l->seq[0][0],Running,listJobs); //Ajouter un job
		    }else{
		    	 printf("[%d]\n",pid); //Afficher le Pid du processus qui s'execute en premmier plan 
		    }
		    if(background) {
		      return;
            }
           while(waitpid(pid, NULL, 0|WUNTRACED) == -1){} //Attendre le fils de Pid "pid" 
    		
    		if (s!=1){ //Si le processus n'a pas recus un signal de type SIGTSTP
    			printf("[%d]: termine\n", pid);
    		}else{ //Si oui alors le s=1
              printf("[%d]+\t\tStopped\n",count1);
              s=0; //Réinitialiser le la variable "s"
              listJobs=ADDjob(pid,l->seq[0][0],Stopped,listJobs); //Ajouter un job dans la liste "listJobs"
    		}
    			    
    }
}

static void memory_error(void)
{
	errno = ENOMEM;
	perror(0);
	exit(1);
}


static void *xmalloc(size_t size)
{
	void *p = malloc(size);
	if (!p) memory_error();
	return p;
}


static void *xrealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr, size);
	if (!p) memory_error();
	return p;
}


/* Read a line from standard input and put it in a char[] */
static char *readline(void)
{
	size_t buf_len = 16;
	char *buf = xmalloc(buf_len * sizeof(char));

	if (fgets(buf, buf_len, stdin) == NULL) {
		free(buf);
		return NULL;
	}
	
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	do {
		size_t l = strlen(buf);
		if ((l > 0) && (buf[l-1] == '\n')) {
			l--;
			buf[l] = 0;
			return buf;
		}
		if (buf_len >= (INT_MAX / 2)) memory_error();
		buf_len *= 2;
		buf = xrealloc(buf, buf_len * sizeof(char));
		if (fgets(buf + l, buf_len - l, stdin) == NULL) return buf;
	} while (1);
}


/* Split the string in words, according to the simple shell grammar. */
static char **split_in_words(char *line)
{
	char *cur = line;
	char **tab = 0;
	size_t l = 0;
	char c;

	while ((c = *cur) != 0) {
		char *w = 0;
		char *start;
		switch (c) {
		case ' ':
		case '\t':
			/* Ignore any whitespace */
			cur++;
			break;
		case '<':
			w = "<";
			cur++;
			break;
		case '>':
			w = ">";
			cur++;
			break;
		case '|':
			w = "|";
			cur++;
			break;
		case '&':      
			w = "&";
			cur++;
			break;
		default:
			/* Another word */
			start = cur;
			while (c) {
				c = *++cur;
				switch (c) {
				case 0:
				case ' ':
				case '\t':
				case '<':
				case '>':
				case '|':
				case '&':
					c = 0;
					break;
				default: ;
				}
			}
			w = xmalloc((cur - start + 1) * sizeof(char));
			strncpy(w, start, cur - start);
			w[cur - start] = 0;
		}
		if (w) {
			tab = xrealloc(tab, (l + 1) * sizeof(char *));
			tab[l++] = w;
		}
	}
	tab = xrealloc(tab, (l + 1) * sizeof(char *));
	tab[l++] = 0;
	return tab;
}


static void freeseq(char ***seq)
{
	int i, j;

	for (i=0; seq[i]!=0; i++) {
		char **cmd = seq[i];

		for (j=0; cmd[j]!=0; j++) free(cmd[j]);
		free(cmd);
	}
	free(seq);
}


/* Free the fields of the structure but not the structure itself */
static void freecmd(struct cmdline *s)
{
	if (s->in) free(s->in);
	if (s->out) free(s->out);
	if (s->seq) freeseq(s->seq);
}


struct cmdline *readcmd(void)
{
	static struct cmdline *static_cmdline = 0;
	struct cmdline *s = static_cmdline;
	char *line;
	char **words;
	int i;
	char *w;
	char **cmd;
	char ***seq;
	size_t cmd_len, seq_len;

	line = readline();
	if (line == NULL) {
		if (s) {
			freecmd(s);
			free(s);
		}
		return static_cmdline = 0;
	}

	cmd = xmalloc(sizeof(char *));
	cmd[0] = 0;
	cmd_len = 0;
	seq = xmalloc(sizeof(char **));
	seq[0] = 0;
	seq_len = 0;

	words = split_in_words(line);
	free(line);

	if (!s)
		static_cmdline = s = xmalloc(sizeof(struct cmdline));
	else
		freecmd(s);
	s->err = 0;
	s->in = 0;
	s->out = 0;
	s->seq = 0;
	s->tachFond = 0;

	i = 0;
	while ((w = words[i++]) != 0) {
		switch (w[0]) {
		case '<':
			/* Tricky : the word can only be "<" */
			if (s->in) {
				s->err = "only one input file supported";
				goto error;
			}
			if (words[i] == 0) {
				s->err = "filename missing for input redirection";
				goto error;
			}
			s->in = words[i++];
			break;
		case '>':
			/* Tricky : the word can only be ">" */
			if (s->out) {
				s->err = "only one output file supported";
				goto error;
			}
			if (words[i] == 0) {
				s->err = "filename missing for output redirection";
				goto error;
			}

				s->out = words[i++];
			break;
		case '|':
			/* Tricky : the word can only be "|" */
			if (cmd_len == 0) {
				s->err = "misplaced pipe";
				goto error;
			}

			seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
			seq[seq_len++] = cmd;
			seq[seq_len] = 0;

			cmd = xmalloc(sizeof(char *));
			cmd[0] = 0;
			cmd_len = 0;
			break;
		case '&':
			/* Tricky : the word can only be "&" */
				if (cmd_len == 0) {
				    s->err = "misplaced &";
				    goto error;
			    }
				s->tachFond =  '&';
			break;
		default:
			cmd = xrealloc(cmd, (cmd_len + 2) * sizeof(char *));
			cmd[cmd_len++] = w;
			cmd[cmd_len] = 0;
		}
	}

	if (cmd_len != 0) {
		seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
		seq[seq_len++] = cmd;
		seq[seq_len] = 0;
	} else if (seq_len != 0) {
		s->err = "misplaced pipe";
		i--;
		goto error;
	} else
		free(cmd);
	free(words);
	s->seq = seq;
	return s;
error:
	while ((w = words[i++]) != 0) {
		switch (w[0]) {
		case '<':
		case '>':
		case '|':
		case '&':
			break;
		default:
			free(w);
		}
	}
	free(words);
	freeseq(seq);
	for (i=0; cmd[i]!=0; i++) free(cmd[i]);
	free(cmd);
	if (s->in) {
		free(s->in);
		s->in = 0;
	}
	if (s->out) {
		free(s->out);
		s->out = 0;
	}
	return s;
}
