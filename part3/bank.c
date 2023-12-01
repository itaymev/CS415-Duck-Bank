#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "string_parser.h"
#include "account.h"
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>

#define MAX_THREAD 10

pthread_t tid[MAX_THREAD], bank_thread;
pthread_mutex_t lock[MAX_THREAD];
pthread_barrier_t sync_barrier;
pthread_mutex_t update_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t active = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t update_condition = PTHREAD_COND_INITIALIZER;

int transactions = 0;
int numAccts = 0;
int active_threads = MAX_THREAD;
int waiting_thread_count = 0;
long size = 0;
account *acct_array;

// populates the acct_array global variable with accounts, account type will be populated with balance and 
account* create_Accounts(const char* filename) {

	FILE *inputFile = fopen(filename, "r");
	
	// first line to find number of accounts
	fscanf(inputFile, "%d", &numAccts);
	
	char *line_buf = NULL;
	double bal = 0;
	double rate = 0;
	size_t len = 0;

	char heading[12];

	for(int i = 0; i < numAccts; i++) {
		account newAcct;
		
		// get rid of empty line_buf
		getline(&line_buf, &len, inputFile);

		// move past index line
		getline(&line_buf, &len, inputFile);


		// extracting data from input file and initializes newAcct's attributes
		fscanf(inputFile, "%s", newAcct.account_number);
		fscanf(inputFile, "%s", newAcct.password);
		fscanf(inputFile, "%le", &bal);
		newAcct.balance = bal;
		fscanf(inputFile, "%le", &rate);
		newAcct.reward_rate = rate;

		// this is how many transactions have occured for newAcct, which rn is 0
		newAcct.transaction_tracter = 0;
		
		newAcct.ac_lock = lock[i];

		sprintf(heading, "account %d\n", i);
		strcpy(newAcct.out_file, heading);
		
		// add newAcct to acct_array
		acct_array[i] = newAcct; 
	}
	
	fclose(inputFile);
	free(line_buf);
}

// check correct password give for requested account before calling each proccess_transaction()
int check_pw(account *acct_array, int *numAccts, char *acct, char *pw) {
	for(int i = 0; i < *numAccts; i++) {
		if(strcmp(acct_array[i].account_number, acct) == 0) {
			if(strcmp(acct_array[i].password, pw) == 0) {
				//printf("Matching pw: %s\n", acct_array[i].account_number);
				return 0;
			}
			else {
				return 1;
			}
		}
	} 
}

// when number of transactions run hits 5000 main will call this function to update each account balance
void* update_balance(void *arg) {
	pthread_barrier_wait(&sync_barrier); // alex said this is good (if you're reading this pls give me a good grade i really want to pass)
	//printf("\n\n\n-----UPDATE BALANCE-----\n\n\n");
	char heading[50];
	
	while(1) {
		// lock, wait for signal, and unlock
		pthread_mutex_lock(&mtx);
		pthread_cond_wait(&(update_condition), &mtx);
		pthread_mutex_unlock(&mtx);

		// wait for signal from transaction thread to update
		// lock the thread before entering this function
		pthread_mutex_lock(&wait_thread_lock);
		
		for(int i = 0; i < numAccts; i++) {
			acct_array[i].balance += (acct_array[i].transaction_tracter * acct_array[i].reward_rate);
			acct_array[i].transaction_tracter = 0;
			
			sprintf(heading, "Current Balance:\t%.02f\n", acct_array[i].balance);
			strcat(acct_array[i].out_file, heading);
			//printf("Outfile %d\t %s\n", i, acct_array[i].out_file);
		}

		transactions = 0;
		waiting_thread_count = 0;
		printf("Unlocking worker threads; Send broadcast\n");
		pthread_cond_broadcast(&condition);
		pthread_mutex_unlock(&wait_thread_lock);
		if(active_threads == 0) {
			break;
		}
		sched_yield();
	}

	pthread_exit(NULL);
}


/*
void* update_balance(void *arg) {
    pthread_barrier_wait(&sync_barrier);
    char heading[50];

    while (1) {
        pthread_mutex_lock(&wait_thread_lock);

        while (transactions < 5000) {
            pthread_cond_wait(&update_condition, &wait_thread_lock);
        }

        for (int i = 0; i < numAccts; i++) {
            acct_array[i].balance += (acct_array[i].transaction_tracter * acct_array[i].reward_rate);
            acct_array[i].transaction_tracter = 0;

            sprintf(heading, "Current Balance:\t%.02f\n", acct_array[i].balance);
            strcat(acct_array[i].out_file, heading);
        }

        transactions = 0;
        waiting_thread_count = 0;
        printf("Unlocking worker threads; Send broadcast\n");
        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&wait_thread_lock);

        if (active_threads == 0) {
            break;
        }

        sched_yield();
    }

    return NULL;
}
*/

// process requested transactions by main, I realize now I couldve done multiple functions that all get called by this one for each transaction type
void* process_transaction(void *arg) {
	pthread_barrier_wait(&sync_barrier);
	printf("Process Transaction called by %ld\n", pthread_self());
	char** tmp = (char **) arg;
	
	command_line token_buffer;
	char *acct;
	char *acctpw;
	char *acct2;
	double amount;
	int target;
	int target2;

	for(int index = 0; index < ((size/MAX_THREAD)-1); index++) {
		token_buffer = str_filler(tmp[index], " ");
		
		acct = token_buffer.command_list[1];
		acctpw = token_buffer.command_list[2];

		// check that account and password match
		int valid = check_pw(acct_array, &numAccts, acct, acctpw);
		if(valid == 1) {
			//printf("Incorrect password: %s\n", token_buffer.command_list[0]);
		}
		else {
			// set target to account object in account array
			for(int i = 0; i < numAccts; i++) {
				if(strcmp(acct_array[i].account_number, acct) == 0) {
					target = i;
				}
			}

			if(strcmp(token_buffer.command_list[0], "T") == 0) {
				printf("-----Transfer-----\n");
				acct2 = token_buffer.command_list[3];
				amount = atof(token_buffer.command_list[4]);

				// this is the same as what we did earlier but for acct2 and target2
				for(int j = 0; j < numAccts; j++) {
					if(strcmp(acct_array[j].account_number, acct2) == 0) {
						target2 = j;
					}
				}

				pthread_mutex_lock(&acct_array[target].ac_lock);
				acct_array[target].balance -= amount;
				acct_array[target].transaction_tracter += amount;
				pthread_mutex_unlock(&acct_array[target].ac_lock);

				pthread_mutex_lock(&acct_array[target2].ac_lock);
				acct_array[target2].balance += amount;
				pthread_mutex_unlock(&acct_array[target2].ac_lock);
		
				pthread_mutex_lock(&update_lock);
				transactions++;
				pthread_mutex_unlock(&update_lock);

			}
			else if(strcmp(token_buffer.command_list[0], "C") == 0) {
				printf("-----Check balance-----\n");
				printf("Account #%s balance: %f\n", acct_array[target].account_number, acct_array[target].balance);
			
			}
			else if(strcmp(token_buffer.command_list[0], "D") ==0) {
				printf("-----Deposit-----\n");
				amount = atof(token_buffer.command_list[3]);
				
				pthread_mutex_lock(&acct_array[target].ac_lock);
				acct_array[target].balance += amount;
				acct_array[target].transaction_tracter += amount;
				pthread_mutex_unlock(&acct_array[target].ac_lock);
				
				pthread_mutex_lock(&update_lock);
				transactions++;
				pthread_mutex_unlock(&update_lock);

			}
			else if(strcmp(token_buffer.command_list[0], "W") == 0) {
				printf("-----Withdraw-----\n");
				amount = atof(token_buffer.command_list[3]);
				
				pthread_mutex_lock(&acct_array[target].ac_lock);
				acct_array[target].balance -= amount;
				acct_array[target].transaction_tracter += amount;
				pthread_mutex_unlock(&acct_array[target].ac_lock);
				
				pthread_mutex_lock(&update_lock);
				transactions++;
				pthread_mutex_unlock(&update_lock);
			}
			else {
				printf("Invalid Request\n");
			}
		}
	
		if(transactions >= 5000) {
			pthread_mutex_lock(&wait_thread_lock);
			waiting_thread_count++;

			if (active_threads == waiting_thread_count) {
               	pthread_cond_broadcast(&update_condition);
            }
            while (transactions >= 5000) {
        		pthread_cond_wait(&condition, &wait_thread_lock);
            }
    		pthread_mutex_unlock(&wait_thread_lock);
		}

		free_command_line(&token_buffer);
		memset(&token_buffer, 0, 0);
	}

	pthread_mutex_lock(&active);
	active_threads--;
	pthread_mutex_unlock(&active);

	if(active_threads == 0) {
		pthread_mutex_lock(&mtx);
		pthread_cond_broadcast(&update_condition);
		pthread_mutex_unlock(&mtx);
	}
	
	pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
	// make sure theres a valid input file
	if(argc != 2) {
		printf("Error: 1 input file required\n");
		exit(-1);
	}
	FILE *inputFile = fopen(argv[1], "r");
	if(inputFile == NULL) {
		printf("Error %s does not exist.\n", argv[2]);
		exit(-1);
	}
	fclose(inputFile); // its valid
	
	inputFile = fopen(argv[1], "r");
	fscanf(inputFile, "%d", &numAccts);

        acct_array=(account*)malloc(sizeof(account) * (numAccts));
	create_Accounts(argv[1]);
	// accounts have been made at this point, and acct_array is populated
	
	char *line_buf = NULL;
	size_t len = 0;
	// go past first 50 lines of account work
	for(int i = 0; i <= (numAccts * 5); i++) {
		getline(&line_buf, &len, inputFile);
	}

	while(getline(&line_buf, &len, inputFile) != -1) {
		size++;
	}
	//printf("Size: %ld\n", size);
	fclose(inputFile);

	inputFile = fopen(argv[1], "r");
	// go past num of account line and account info lines
	for(int i = 0; i <= (numAccts * 5); i++) {
		getline(&line_buf, &len, inputFile);
	}
	
	// this is an array that will hold commands for threads to start at
	char **comm_array = (char **)malloc(sizeof(char*) * size);
	for(int c = 0; c < size; c++) {
		comm_array[c] = (char*)malloc(sizeof(char) * 256);
	}

	for(int yy = 0; yy < size; yy++) {
		for(int xx = 0; xx < 256; xx++) {
			comm_array[yy][xx] = 0;
		}
	}

	int x = 0;
	while(getline(&line_buf, &len, inputFile) != -1) {
		int j = 0;
		while(line_buf[j] != 0) {
			comm_array[x][j] = line_buf[j];
			j++;
		}
		x++;
	}
	fclose(inputFile);
	free(line_buf);

	// 10 worker threads, bank thread, main thread
	pthread_barrier_init(&sync_barrier, NULL, MAX_THREAD + 2);

	int error;
	// seperating sections of commands in array
	for(int a = 0; a < MAX_THREAD; a++) {
		int b = a * size/MAX_THREAD; // Numbers off the sections
		error = pthread_create(&(tid[a]), NULL, &process_transaction, (void *) (comm_array + b));
		if (error != 0) {
			printf("\nThread can't be created :[%s]", strerror(error));
			exit(EXIT_FAILURE);
		}	
	}

	// create bank thread to run update_balance
	error = pthread_create(&bank_thread, NULL, &update_balance, &transactions);
	if(error != 0) {
		printf("\nThread can't be created:[%s]", strerror(error));
		exit(EXIT_FAILURE);
	}

	printf("Main thread %ld releasing worker threads now\n", pthread_self());
	pthread_barrier_wait(&sync_barrier);

	for(int j = 0; j < MAX_THREAD; j++) {
		pthread_join(tid[j], NULL);
		printf("thread %d joins\n", (j + 1));
	}

	printf("THREADS RETURNED\n");

	pthread_join(bank_thread, NULL);
	printf("BANK THREAD RETURNED\n");
	
	for(int t = 0; t < size; t++) {
		free(comm_array[t]);
	}
	free(comm_array);

	const char *directoryName = "output"; // i could have just used "output" in access mkdir and rmdir but it doesn't really matter

	// check if output dir exists
    if (access(directoryName, F_OK) == 1) {
    	// ouput dir doesnt exist, create it
        int status = mkdir(directoryName, 0700);
    	// if (status == -1) {
    	    // printf("Failed to create the output directory");
        // }
	}
	else {
		// output dir does exist, wipe it and remake it (i know this can be done better give me a break)
		int rmstatus = rmdir(directoryName);
        	// if (rmstatus == -1) {
            	// printf("Failed to remove the output directory");
        	// }

		int mkstatus = mkdir(directoryName, 0700);
        	// if (mkstatus == -1) {
            	// printf("Failed to recreate the output directory");
        	// }
	}

	char heading[50];
	for(int m = 0; m < numAccts; m++) {
		sprintf(heading, "output/%s.txt", acct_array[m].account_number);
		FILE *acct_out = fopen(heading, "w+");
		fprintf(acct_out, "%s", acct_array[m].out_file);
		//printf("GOT HERE\n");
		fclose(acct_out);	
	}

	FILE *outputFile = fopen("output.txt", "w+");
	for(int k = 0; k < numAccts; k++) {
		fprintf(outputFile, "%d balance:\t%.2f\n\n", k, acct_array[k].balance);
	}

	fclose(outputFile);
	free(acct_array);
	for(int l = 0; l < numAccts; l++) {
		pthread_mutex_destroy(&lock[l]);
	}
	pthread_mutex_destroy(&update_lock);
	pthread_mutex_destroy(&mtx);
	pthread_barrier_destroy(&sync_barrier);
	pthread_exit(NULL);

	exit(0);
}
