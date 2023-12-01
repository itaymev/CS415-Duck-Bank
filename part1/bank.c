#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "string_parser.h"
#include "account.h"

int transactions = 0;

account* create_Accounts(const char* inputf) {
	// create array of accounts and initialize them (one by one) from input file
	FILE *inputFile = fopen(inputf, "r");
	
	int numAccts = 0;
	fscanf(inputFile, "%d", &numAccts); // first line to find number of accounts
	
	char *line_buf = NULL;
	double bal, rate = 0;
	size_t len = 0;

	char heading[12];

	account *acct_array;
        acct_array = (account*) malloc(sizeof(account) * (numAccts));
	
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

		sprintf(heading, "account %d\n", i);
		strcpy(newAcct.out_file, heading);
		
		acct_array[i] = newAcct; 
	}
	
	fclose(inputFile);
	free(line_buf);

	return acct_array;
}

int check_pw(account *acct_array, int *numAccts, char *acct, char *pw) {
	// check password for account given. 1 if true, 0 if false.
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
    return 0; // this is to get rid of an annoying compile warning, if we run the code as intended (we assumed no outstanding inputs) then this line will never be reached
}

void* process_transaction(transact_req *demand) {
	// takes a request tokens from input file read in main to process requested transaction
	char *end;
	command_line request = demand->request;
	char *transact_type = request.command_list[0]; // transaction type
	char *acct = request.command_list[1]; // account 1
	char *acctpw = request.command_list[2]; // account 1 password
	char *acct2;
	double amount;
	int target1, target2;

	char heading[50];
	// find account object in account array, account 1
	for (int i = 0; i < demand->num_accts; i++) {
		if(strcmp(demand->accts[i].account_number, acct) == 0) {
			target1 = i;
		}
	}

	if (strcmp(transact_type, "T") == 0) {
        // trade or transact whatever you wanna call it (barter for all the british blokes)
		acct2 = request.command_list[3]; // account 2, if we need it
		amount = strtod(request.command_list[4], &end);
		
		// find recieving account object in account array, account 2 (this is only needed for certain transactions, I think only "T")
		for(int i = 0; i < demand->num_accts; i++) {
			if(strcmp(demand->accts[i].account_number, acct2) == 0) {
				target2 = i;
			}
		}
		
		demand->accts[target1].balance -= amount;
		demand->accts[target2].balance += amount;
		demand->accts[target1].transaction_tracter += amount;
		transactions++;
	}

    else if (strcmp(transact_type, "C") == 0) {
        // check account balance
		printf("Account #%s balance: %f\n", demand->accts[target1].account_number, demand->accts[target1].balance);
	}

    else if (strcmp(transact_type, "D") == 0) {
        // deposit in account 1
		amount = strtod(request.command_list[3], &end);

		demand->accts[target1].balance += amount;
		demand->accts[target1].transaction_tracter += amount;
		transactions++;
	}
    
    else if(strcmp(transact_type, "W") == 0)
	{
		amount = strtod(request.command_list[3], &end);

		demand->accts[target1].balance -= amount;
		demand->accts[target1].transaction_tracter += amount;
		transactions++;
	}else
	{
		printf("Invalid Request\n");
	}
    return NULL; // again getting rid of annoying compile warning
}

void* update_balance(transact_req *timer) {
	// balance is the sum of the products of total transaction amounts and reward rates
	char heading[50];
	
	for(int i = 0; i < timer->num_accts; i++)
	{
		timer->accts[i].balance += (timer->accts[i].transaction_tracter * timer->accts[i].reward_rate);
		timer->accts[i].transaction_tracter = 0;
		
		sprintf(heading, "Current Balance:\t%.02f\n", timer->accts[i].balance);
		strcat(timer->accts[i].out_file, heading);
	}
    return NULL; // why do I even compile with -g
}

int main(int argc, char const *argv[]) {
	// check that there is an argument for an input file
	if(argc != 2)
	{
		printf("Error input file required\n");
		exit(-1);
	}

	// check that the input file actually exists
	FILE *inputFile = fopen(argv[1], "r");
	if(inputFile == NULL)
	{
		printf("Error %s is not a valid input file\n", argv[2]);
		exit(-1);
	}
	
	int numAccts = 0;
	fscanf(inputFile, "%d", &numAccts);

	account *acct_array = create_Accounts(argv[1]);
	transact_req demand;
	demand.accts = acct_array;
	demand.num_accts = numAccts;

	// accounts have been made at this point time to iterate thru lines and process transactions (beast mode)
	
	char *line_buf = NULL;
	size_t len = 0;
	for(int i = 0; i <= (numAccts * 5); i++) {
		// first five lines per account are account work
		getline(&line_buf, &len, inputFile);
	}
	
	
	command_line token_buffer;
	char *acct;
	char *acctpw;
	while(getline(&line_buf, &len, inputFile) != -1) {
		// this reads to end of file (EOF) (man I love tokens)
		token_buffer = str_filler(line_buf, " ");
		demand.request = token_buffer;
	
		acct = token_buffer.command_list[1];
		acctpw = token_buffer.command_list[2];
	
		int valid = check_pw(acct_array, &numAccts, acct, acctpw); // valid account and password
		if(valid == 1) {
            // bad password or bad account # or combination of both
			printf("Incorrect account password: %s request denied\n", token_buffer.command_list[0]);
		}
        else if (valid == 0) {
            // hooray
			process_transaction(&demand);

			if(transactions == 5000) {
				update_balance(&demand);
				transactions = 0;
			}
		}
		
		free_command_line(&token_buffer);
		memset(&token_buffer, 0, 0);
	}

    const char *directoryName = "output";

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
		sprintf(heading, "output/%s.txt", demand.accts[m].account_number);
		FILE *acct_out = fopen(heading, "w+");
		fprintf(acct_out, "%s", demand.accts[m].out_file);
		fclose(acct_out);	
	}


	FILE *outputFile = fopen("output.txt", "w+");
	for(int k = 0; k < numAccts; k++) {
		fprintf(outputFile, "%d balance:\t%.2f\n\n", k, demand.accts[k].balance);
	}

	free(acct_array);
	fclose(inputFile);
	fclose(outputFile);
	free(line_buf);
	exit(0);
}
