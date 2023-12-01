#ifndef ACCOUNT_H_
#define ACCOUNT_H_
#include <pthread.h>

typedef struct {
	char account_number[17];
	char password[9];
    double balance;
    double reward_rate; // this needs to be applied to every transaction?? yes
    
    double transaction_tracter; // what is a tracter

    char out_file[1064]; // when I had it at [64] it would "zsh: trace trap"

    pthread_mutex_t ac_lock;
} account;

typedef struct {
	account *accts; // trust
	int num_accts;
	command_line request;
} transact_req;

#endif
