#include "header.h"

//variaveis globais
Config *config;
Stats *stat;
int config_sm_id, stat_sm_id;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void close_server(){
	pthread_mutex_destroy(&mutex);
}

void stat_manager(){
	printf("Gestor de estatisticas!\n");
}

void load_conf(){
	printf("Load Config!\n");
	FILE *f;
	char line[50], aux[50];
	char *token, **strlist;
	int count=0, i=0;

	f = fopen ("config.txt", "r");
	if(f==NULL){
		perror("Erro na leitura do ficheiro!\n");
	}
	if(fscanf(f, "SERVERPORT=%d\n", &(config->port))!=1){
		perror("Error reading port!\n");
	}
	printf("Port: %d\n",config->port);

	if(fscanf(f, "SCHEDULING=%s\n", line)!=1){
		perror("Error reading Scheduling!\n");
	}
	if(strcmp(line, "NORMAL")==0) config->sched=0;
	else if(strcmp(line, "ESTATICO")==0) config->sched=1;
	else if(strcmp(line, "COMPRIMIDO")==0) config->sched=2;

	printf("Schedule: %d\n",config->sched);

	if(fscanf(f, "THREADPOOL=%d\n", &(config->threadp))!=1){
		perror("Error reading threadpool!\n");
	}
	printf("Threadpool: %d\n",config->threadp);

	if(fscanf(f, "ALLOWED=%s\n", line)!=1){
		perror("Error reading allowed compressed files");
	}

	strcpy(aux, line);
	token=strtok(aux, ";");
	while(token != NULL){
		count++;
		token = strtok(NULL, ";");
	}

	strlist=(char**) malloc(count*sizeof(char*));

	token=strtok(line, ";");
	while(token != NULL){
		strlist[i]=strdup(token);
		token = strtok(NULL, ";");
		i++;
	}

	for(i=0; i<count; i++){
		printf("%s\n", strlist[i]);
	}
	//todo: prints de debug com if
	fclose(f);
}

void start_stat_process(){
	pid_t stat_pid, config_pid;
	printf("Entrou start_stat_process\n");
	stat_pid = fork();
	if(stat_pid == -1){
		perror("Error while creating stat process");
	}
	else if(stat_pid == 0){
		printf("Process for statistics created! PID of statistics: %ld\tPID of the father: %ld\n",(long)getpid(), (long)getppid());
		stat_manager();
		printf("processo de gestao saiu\n");
		exit(0);
	}
	printf("isto nao pode repetir\n");
}

void *worker_threads(void){
	printf("Entrou na worker da thread\n");
	usleep(1000);
	pthread_exit(NULL);
}

void start_threads(){
	int i, size=config->threadp;
	pthread_t threads[size];
	int id[size];

	printf("entrei no cria threads\n");
	for(i = 0 ; i<size ;i++) {
		id[i] = i;
		if(pthread_create(&threads[i],NULL,worker_threads,&id[i])==0){
			printf("Thread %d criada!\n", i);
		}
		else
			perror("Erro a criar a thread!\n");
	}
	//espera pela morte das threads
	for (i=0;i<size;i++){
		if(pthread_join(threads[i],NULL) == 0){
			printf("Thread %d juntou-se\n",id[i]);
		}
		else{
			perror("Erro a juntar threads!\n");
		}
	}
}

void start_sm(){

	stat_sm_id = shmget(IPC_PRIVATE, 1, IPC_CREAT | 0766);
	if( stat_sm_id != -1){
		printf("Stat shared mem ID: %d\n",stat_sm_id );
		int stat_ptr = shmat(stat_sm_id,NULL,0);
	}
	else
		perror("Error sm stat!\n");

	if((config_sm_id = shmget(IPC_PRIVATE, sizeof(Config),IPC_CREAT | 0766)) != -1){
		printf("Config shared mem ID: %d\n",config_sm_id);
		config = (Config*) shmat(config_sm_id,NULL,0);
	}
	else
		perror("Error sm config\n");
}


void setup_server(){
	start_sm();
	load_conf();
	start_stat_process();
	start_threads();
	wait(NULL);
}

int main(){
	setup_server();
	close_server();
	return 0;
}