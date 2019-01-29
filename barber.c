#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>

enum err
{
	OK,
	ESEM,
	ESHM,
	EARG,
	EFORK,
	ECRP,
	EOPEN,
	ECLOSE
};


typedef struct arg
{
	int chairs;
	int GenC;
	int GenB;
	int cust_max;
	int randomC;
	int randomB;
} Targ;

typedef struct shstruct
{
	int freeseats;
	int customers;
	int action;
} Tshstruct;

void printerr(int err);

//help
void phelp()
{
	printf("Autor: Michal Kysilko xkysil00\n"
		"xkysil00@stud.fit.vutbr.cz\n"
		"Datum: 1.5.2011\n"
		"Reseni problemu spiciho holice pomoci semaforu\n\n"
		"Program se spousti s parametry Q GenC GenB N F v tomto poradi, kde\n"
		"Q je pocet zidli, GenC a GenB jsou rozsahy pro generovani zakazniku a doby obsluhy\n"
		"N je celkovy pocet zakazniku a F je nazev souboru, do ktereho se uklada vystup.\n"
		"Pokud se F rovna ' - ' je vystup zobrazovan na stdout\n");
	exit(0);
}

//funkce pro uklid pameti a semaforu
void killall(sem_t **semarray, Tshstruct *shm_xkysil00,int shmid_xkysil00)
{
	shmdt(shm_xkysil00);
    	shmctl(shmid_xkysil00, IPC_RMID, NULL);
	sem_close(semarray[0]);
	sem_close(semarray[1]);
    	sem_close(semarray[2]);
    	sem_close(semarray[3]);
	sem_close(semarray[4]);
	sem_close(semarray[5]);
	sem_close(semarray[6]);
    	sem_unlink("/SEM0_xkysil00");
    	sem_unlink("/SEM1_xkysil00");
    	sem_unlink("/SEM2_xkysil00");
    	sem_unlink("/SEM3_xkysil00");
	sem_unlink("/SEM4_xkysil00");
	sem_unlink("/SEM5_xkysil00");
	sem_unlink("/SEM6_xkysil00");
	return;
}


//funkce holice
void barber(sem_t **semarray, Tshstruct *shm_xkysil00, Targ *arg, FILE *f )
{
	while(shm_xkysil00->customers < arg->cust_max)
	{
	sem_wait(semarray[4]);
	shm_xkysil00->action++;
	fprintf(f,"%d: barber: checks\n",shm_xkysil00->action);
	sem_post(semarray[4]);
	sem_wait(semarray[0]);  //zakaznik --

	sem_wait(semarray[1]);  //cekarna --
	sem_post(semarray[5]);
	sem_wait(semarray[4]);
	shm_xkysil00->action++;
	fprintf(f,"%d: barber: ready\n",shm_xkysil00->action);
	sem_post(semarray[4]);
	sem_wait(semarray[4]);
	shm_xkysil00->freeseats++;
	sem_post(semarray[4]);
	
	sem_post(semarray[2]);  //holic ++
	sem_wait(semarray[6]);
	sem_wait(semarray[4]);
	shm_xkysil00->action++;
	fprintf(f,"%d: barber: finished\n",shm_xkysil00->action);
	sem_post(semarray[4]);
	sem_post(semarray[3]);

	sem_post(semarray[1]);  //cekarna ++
	sem_wait(semarray[4]);
	shm_xkysil00->customers++; 
	sem_post(semarray[4]);

	usleep(arg->randomB);
	}
	return;
}

//funkce zakaznika
int customer(sem_t **semarray,int i, Tshstruct *shm_xkysil00, FILE *f)
{	
	sem_wait(semarray[4]);	
	shm_xkysil00->action++;
	fprintf(f,"%d: customer %d: created\n",shm_xkysil00->action, i);
	sem_post(semarray[4]);
	sem_wait(semarray[1]);  //cekarna --
	sem_wait(semarray[4]);
	shm_xkysil00->action++;
	fprintf(f,"%d: customer %d: enters\n",shm_xkysil00->action, i);
	sem_post(semarray[4]);

	if(shm_xkysil00->freeseats > 0)
	{
	sem_wait(semarray[4]);
	shm_xkysil00->freeseats--;
	sem_post(semarray[4]);
	sem_post(semarray[0]);  //zakaznik ++
	
	sem_post(semarray[1]);  //cekarna ++
	sem_wait(semarray[2]);  //holic --
	
	sem_wait(semarray[5]);
	sem_wait(semarray[4]);
	shm_xkysil00->action++;
	fprintf(f,"%d: customer %d: ready\n",shm_xkysil00->action, i);
	sem_post(semarray[4]);
	sem_post(semarray[6]);
	
	sem_wait(semarray[3]);
	sem_wait(semarray[4]);
	shm_xkysil00->action++;
	fprintf(f,"%d: customer %d: served\n",shm_xkysil00->action, i);
	sem_post(semarray[4]);	
	}
	else
	{
	sem_post(semarray[1]);  //cekarna ++
	sem_wait(semarray[4]);
	shm_xkysil00->action++;
	fprintf(f,"%d: customer %d: refused\n",shm_xkysil00->action, i);
	sem_post(semarray[4]);
	sem_wait(semarray[4]);
	shm_xkysil00->customers++; 
	sem_post(semarray[4]);
	}
	return i;
}

// zpracovani paramteru prikazove radky
void crossroad(int argc, char *argv[], Targ *arg)
{

	char *endptr;
	
	if( (argc == 2) && (strcmp("-h",argv[1]) == 0))
	phelp();
	
	else if(argc == 6)
	{
	arg->chairs=strtod(argv[1],&endptr);
	if(strlen(endptr) != 0)
	printerr(EARG);

	arg->GenC=strtod(argv[2],&endptr);
	if(strlen(endptr) != 0)
	printerr(EARG);

	arg->GenB=strtod(argv[3],&endptr);
	if(strlen(endptr) != 0)
	printerr(EARG);

	arg->cust_max=strtod(argv[4],&endptr);
	if(strlen(endptr) != 0)
	printerr(EARG);
	
	}
	else
	printerr(EARG);

	//printf("%d\n", arg->cust_gen);
	return;
}

//tisk chybovych vypisu
void printerr(int err)
{
	if(err == ESEM)
	fprintf(stderr, "Chyba pri vytvareni semaforu\n");

	else if(err == ESHM)
	fprintf(stderr, "Chyba pri vytvareni sdilene pameti\n");

	else if(err == EARG)
	fprintf(stderr, "Chybne zadane parametry\n");

	else if(err == EFORK)
	fprintf(stderr, "Doslo k chybe pri behu procesu\n");

	else if(err == ECRP)
	fprintf(stderr, "Doslo k chybe pri vytvareni procesu\n");

	else if(err == EOPEN)
	fprintf(stderr, "Naskytla se chyba pri otevirani souboru\n");

	else if(err == ECLOSE)
	fprintf(stderr, "Nepodarilo se zavrit soubor\n");

	exit(err);
}

//shareMem->sleepC = (rand() % (shareMem->GenC+1))*1000;

int main(int argc, char *argv[])
{
	srand(time(NULL));
	pid_t pid;
	pid_t parentpid;
	sem_t *semarray[7];
	Tshstruct *shm_xkysil00;
	int shmid_xkysil00;

	Targ arg;
	crossroad(argc,argv,&arg);
	pid_t child[arg.cust_max];
	

	arg.randomC=(rand()%(arg.GenC*1000+1));
	arg.randomB=(rand()%(arg.GenB*1000+1));	

	char *fn=argv[5];
	FILE *f;

	if(strcmp("-", argv[5]) != 0)
	{
	f=fopen(fn, "w");
	setbuf(f, NULL);
	
	if (f==NULL)
        printerr(EOPEN);
	}
	else
	f=stdout;

	

	//alokace sdilene pameti	
	if((shmid_xkysil00= shmget((key_t)7546, sizeof(Tshstruct), IPC_CREAT | 0666))<0)
	printerr(ESHM);

	if((shm_xkysil00= (Tshstruct*)shmat(shmid_xkysil00, NULL, 0))== (void *) -1)
    	printerr(ESHM);

	//inicializace pojmenovanych semaforu
	if((semarray[0]= sem_open("/SEM0_xkysil00", O_CREAT, S_IRUSR | S_IWUSR,0))== SEM_FAILED)
  	{
    	shmdt(shm_xkysil00);
    	shmctl(shmid_xkysil00, IPC_RMID, NULL);
    	printerr(ESEM);
  	}

	if((semarray[1]= sem_open("/SEM1_xkysil00",O_CREAT, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
  	{
    	shmdt(shm_xkysil00);
    	shmctl(shmid_xkysil00, IPC_RMID, NULL);
    	sem_close(semarray[0]);
    	sem_unlink("/SEM0_xkysil00");
    	printerr(ESEM);
  	}

  	if((semarray[2]= sem_open("/SEM2_xkysil00", O_CREAT,S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
  	{
    	shmdt(shm_xkysil00);
    	shmctl(shmid_xkysil00, IPC_RMID, NULL);
    	sem_close(semarray[0]);
    	sem_close(semarray[1]);
    	sem_unlink("/SEM0_xkysil00");
    	sem_unlink("/SEM1_xkysil00");
    	printerr(ESEM);
  	}
  	if((semarray[3]= sem_open("/SEM3_xkysil00", O_CREAT,S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
  	{
    	shmdt(shm_xkysil00);
    	shmctl(shmid_xkysil00, IPC_RMID, NULL);
    	sem_close(semarray[0]);
    	sem_close(semarray[1]);
    	sem_close(semarray[2]);
    	sem_unlink("/SEM0_xkysil00");
    	sem_unlink("/SEM1_xkysil00");
    	sem_unlink("/SEM2_xkysil00");
    	printerr(ESEM);
  	}
  	if((semarray[4]= sem_open("/SEM4_xkysil00",O_CREAT,S_IRUSR | S_IWUSR, 1))== SEM_FAILED)
  	{
    	shmdt(shm_xkysil00);
    	shmctl(shmid_xkysil00, IPC_RMID, NULL);
	sem_close(semarray[0]);
	sem_close(semarray[1]);
    	sem_close(semarray[2]);
    	sem_close(semarray[3]);
    	sem_unlink("/SEM0_xkysil00");
    	sem_unlink("/SEM1_xkysil00");
    	sem_unlink("/SEM2_xkysil00");
    	sem_unlink("/SEM3_xkysil00");
    	printerr(ESEM);
 	}
	if((semarray[5]= sem_open("/SEM5_xkysil00",O_CREAT,S_IRUSR | S_IWUSR, 0))== SEM_FAILED)
  	{
    	shmdt(shm_xkysil00);
    	shmctl(shmid_xkysil00, IPC_RMID, NULL);
	sem_close(semarray[0]);
	sem_close(semarray[1]);
    	sem_close(semarray[2]);
    	sem_close(semarray[3]);
	sem_close(semarray[4]);
    	sem_unlink("/SEM0_xkysil00");
    	sem_unlink("/SEM1_xkysil00");
    	sem_unlink("/SEM2_xkysil00");
    	sem_unlink("/SEM3_xkysil00");
	sem_unlink("/SEM4_xkysil00");
    	printerr(ESEM);
 	}
	if((semarray[6]= sem_open("/SEM6_xkysil00",O_CREAT,S_IRUSR | S_IWUSR, 0))== SEM_FAILED)
  	{
    	shmdt(shm_xkysil00);
    	shmctl(shmid_xkysil00, IPC_RMID, NULL);
	sem_close(semarray[0]);
	sem_close(semarray[1]);
    	sem_close(semarray[2]);
    	sem_close(semarray[3]);
	sem_close(semarray[4]);
	sem_close(semarray[5]);
    	sem_unlink("/SEM0_xkysil00");
    	sem_unlink("/SEM1_xkysil00");
    	sem_unlink("/SEM2_xkysil00");
    	sem_unlink("/SEM3_xkysil00");
	sem_unlink("/SEM4_xkysil00");
	sem_unlink("/SEM5_xkysil00");
    	printerr(ESEM);
 	}
	

	shm_xkysil00->freeseats= arg.chairs;
	shm_xkysil00->customers= 0;
	shm_xkysil00->action= 0;

	//tvorba procesu pro holice a nasledne i pro zakazniky pomoci funkce fork()
	pid=fork();

	if(pid == 0)
	barber(semarray, shm_xkysil00,&arg,f);
	else
	parentpid=pid;


	if(pid)
	{
	for(int i=1; i<=arg.cust_max && pid; i++)
	{

	pid=fork();
	usleep(arg.randomC);
	if(pid == 0)
	customer(semarray, i, shm_xkysil00,f);

	else if(pid<0)
	printerr(ECRP);
	else
	child[i]=pid;
	}

	//pockam az se dokonci procesy deti a pak uklidim pamet a semafory
	if(pid)
	{
	for(int i=1;i<=arg.cust_max;i++)
    	{
        if((waitpid(child[i], NULL, 0)) < 0 )
        {
	kill(parentpid, SIGTERM);
        kill(child[i], SIGTERM);
        killall(semarray, shm_xkysil00, shmid_xkysil00);
	printerr(EFORK);
	}
        }
	kill(parentpid, SIGTERM);
	killall(semarray, shm_xkysil00, shmid_xkysil00);
	}
	}
	if(f != stdout)
	{
	if (fclose(f)== EOF)
        printerr(ECLOSE);
	}
	
	return 0;
}

