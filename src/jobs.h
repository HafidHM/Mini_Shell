#ifndef __JOBS_H
#define __JOBS_H

/* Structure Job */
typedef struct job{
	int num; //Num du job 
    pid_t pid; //Son Pid
    char * jbname; //Son Nom
    int status;  //Son statut : soit : 0 (Stopped), 1(Running),  2(Terminated)
    struct job *next; //Pointeur sur la liste des jobs
}Jobs;

Jobs* ADDjob(pid_t pid, char* name,int status,Jobs* listJobs); //Ajouter un job a laa liste des Jobs
void DisplayJobs(Jobs* listJobs);  //afficher une liste des jobs
void FreeJobs(Jobs* listJobs); //liberer une liste jobs
void changeStatus(pid_t pid);  // changer le status d'un processus terminé en status : 2 (Terminated)


#endif