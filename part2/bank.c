#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include "string_parser.h"
#include "account.h"

#define MAX_THREAD 10

pthread_t tid[MAX_THREAD], bank_thread;
pthread_mutex_t lock[MAX_THREAD];

int transactions = 0;
int numAccts = 0;
long size = 0;
account *acct_array;

void create_Accounts(const char* inputf) {
	// create array of accounts and initialize them (one by one) from input file
	FILE *inputFile = fopen(inputf, "r");
	
	fscanf(inputFile, "%d", &numAccts); // first line to find number of accounts
	
	char *line_buf = NULL;
	double bal, rate = 0;
	size_t len = 0;

	char heading[12];
	
	for(int i = 0; i < numAccts; i++) {
		account newAcct;
		
		// get rid of empty line_buf
		getline(&line_buf, &len, inputFile);

		// move past index line
		getline(&line_buf, &len, inputFile);

		fscanf(inputFile, "%s", newAcct.account_number);
		fscanf(inputFile, "%s", newAcct.password);
		fscanf(inputFile, "%le", &bal);
		newAcct.balance = bal;
		fscanf(inputFile, "%le", &rate);
		newAcct.reward_rate = rate;

		newAcct.transaction_tracter = 0;
		
		newAcct.ac_lock = lock[i];

		sprintf(heading, "account %d\n", i);
		strcpy(newAcct.out_file, heading);
		
		acct_array[i] = newAcct; 
	}
	
	fclose(inputFile);
	free(line_buf);
}

// check password for account given. 1 if true, 0 if false.
int check_pw(account *acct_array, int *numAccts, char *acct, char *pw) {
	for(int i = 0; i < *numAccts; i++) {
		if(strcmp(acct_array[i].account_number, acct) == 0) {
			if(strcmp(acct_array[i].password, pw) == 0) {
				return 0;
			}
            else {
				return 1;
			}
		}
	} 
}


void* update_balance(void *arg) {
	// balance is the sum of the products of total transaction amounts and reward rates
	char heading[50];
	
	for(int i = 0; i < numAccts; i++)
	{
		acct_array[i].balance += (acct_array[i].transaction_tracter * acct_array[i].reward_rate);
		acct_array[i].transaction_tracter = 0;
		
		sprintf(heading, "Current Balance:\t%.02f\n", acct_array[i].balance);
		strcat(acct_array[i].out_file, heading);
	}
	return NULL;
}


void* process_transaction(void *arg) {
	printf("Read called by %p\n", (void *) pthread_self());
	char** tmp = (char **) arg;
	
	command_line token_buffer;
	char *acct;
	char *acctpw;
	char *acct2;
	double amount;
	int target, target2;

	for(int index = 0; index < ((size/MAX_THREAD)-1); index++)
	{
		token_buffer = str_filler(tmp[index], " ");
		
		acct = token_buffer.command_list[1];
		acctpw = token_buffer.command_list[2];

		// Check that account and password match
		int password_check = check_pw(acct_array, &numAccts, acct, acctpw);
		if(password_check == 1)
		{
			printf("Wrong password: %s request denied\n", token_buffer.command_list[0]);
		}else{
			// Find account object in account array
			for(int i = 0; i < numAccts; i++)
			{
				if(strcmp(acct_array[i].account_number, acct) == 0)
				{
					target = i;
				}
			}

			if(strcmp(token_buffer.command_list[0], "T") == 0)
			{
				acct2 = token_buffer.command_list[3];
				amount = atof(token_buffer.command_list[4]);
		
				for(int j = 0; j < numAccts; j++)
				{
					if(strcmp(acct_array[j].account_number, acct2) == 0)
					{
						target2 = j;
					}
				}

				pthread_mutex_lock(&acct_array[target].ac_lock); // lock this array from other threads so we don't access shared memory at the same time in diff threads
				acct_array[target].balance -= amount;
				acct_array[target].transaction_tracter += amount;
				pthread_mutex_unlock(&acct_array[target].ac_lock); // unlock when done

				pthread_mutex_lock(&acct_array[target2].ac_lock); // same thing as before but this time with target2 for the transaction
				acct_array[target2].balance += amount;
				pthread_mutex_unlock(&acct_array[target2].ac_lock); // unlock again
		
				transactions++;
				//printf("After transfer: %f\n", demand->accts[target].balance);
			}else if(strcmp(token_buffer.command_list[0], "C") == 0)
			{
				printf("Account #%s balance: %f\n", acct_array[target].account_number, acct_array[target].balance);
			
			}else if(strcmp(token_buffer.command_list[0], "D") ==0)
			{
				amount = atof(token_buffer.command_list[3]);
				
				pthread_mutex_lock(&acct_array[target].ac_lock);
				acct_array[target].balance += amount;
				acct_array[target].transaction_tracter += amount;
				pthread_mutex_unlock(&acct_array[target].ac_lock);
				
				transactions++;
				//printf("After deposit: %f\n", demand->accts[target].balance);
			}else if(strcmp(token_buffer.command_list[0], "W") == 0)
			{
				amount = atof(token_buffer.command_list[3]);
				
				pthread_mutex_lock(&acct_array[target].ac_lock);
				acct_array[target].balance -= amount;
				acct_array[target].transaction_tracter += amount;
				pthread_mutex_unlock(&acct_array[target].ac_lock);
				
				transactions++;
				//printf("After withdraw: %f\n", demand->accts[target].balance);
			}else
			{
				printf("Invalid Request\n");
			}
		}
	
		free_command_line(&token_buffer);
		memset(&token_buffer, 0, 0);
	}
	
	return NULL;
}


int main(int argc, char const *argv[])
{
	// Check for correct args
	if(argc != 2)
	{
		printf("Error input file required\n");
		exit(-1);
	}

	// Check that file exists
	FILE *inputFile = fopen(argv[1], "r");
	if(inputFile == NULL)
	{
		printf("Error %s does not exist\n", argv[2]);
		exit(-1);
	}
	fclose(inputFile);
	
	inputFile = fopen(argv[1], "r");
	fscanf(inputFile, "%d", &numAccts);

    acct_array=(account*)malloc(sizeof(account) * (numAccts));
	create_Accounts(argv[1]);
	// accounts have been made at this point
	
	char *line_buf = NULL;
	size_t len = 0;
	// iterate first 50 lines of account work
	for(int i = 0; i <= (numAccts * 5); i++)
	{
		getline(&line_buf, &len, inputFile);
	}

	while(getline(&line_buf, &len, inputFile) != -1)
	{
		size++;
	}
	//printf("Size: %ld\n", size);
	fclose(inputFile);

	inputFile = fopen(argv[1], "r");
	// Skip num of account line and account info lines
	for(int i = 0; i <= (numAccts * 5); i++)
	{
		getline(&line_buf, &len, inputFile);
	}
	
	// Create array that will hold commands for threads to start at
	char **comm_array = (char **)malloc(sizeof(char*) * size);
	for(int c = 0; c < size; c++)
	{
		comm_array[c] = (char*)malloc(sizeof(char) * 256);
	}

	for(int s = 0; s < size; s++)
	{
		for(int ss = 0; ss < 256; ss++)
		{
			comm_array[s][ss] = 0;
		}
	}

	int x = 0;
	while(getline(&line_buf, &len, inputFile) != -1)
	{
		int j = 0;
		while(line_buf[j] != 0)
		{
			comm_array[x][j] = line_buf[j];
			j++;
		}
		x++;
	}
	fclose(inputFile);
	free(line_buf);

	int error;
	// Divvying up sections of commands in array
	for(int a = 0; a < MAX_THREAD; a++)
	{
		int b = a * size/MAX_THREAD; // Numbers off the sections
		error = pthread_create(&(tid[a]), NULL, &process_transaction, (void *) (comm_array + b));
		if (error != 0)
		{
			printf("\nThread failed:[%s]", strerror(error));
			exit(EXIT_FAILURE);
		}	
	}

	// Create bank thread to run update_balance
	error = pthread_create(&bank_thread, NULL, &update_balance, &transactions);
	if(error != 0) {
		printf("\nThread failed: [%s]", strerror(error));
		exit(EXIT_FAILURE);
	}



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

	int numUpdates = 0;	
	update_balance(&numUpdates);

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
		sprintf(heading, "output/%s.txt", acct_array[m].account_number); // im sure there is a cleaner way to route these txt files to output but this is fine
		FILE *acct_out = fopen(heading, "w+");
		fprintf(acct_out, "%s", acct_array[m].out_file);
		//printf("HERE\n");
		fclose(acct_out);	
	}

	FILE *outputFile = fopen("output.txt", "w+");
	for(int k = 0; k < numAccts; k++) {
		fprintf(outputFile, "%d balance:\t%.2f\n\n", k, acct_array[k].balance);
	}

	fclose(outputFile);
	free(acct_array);
	exit(0);
}
