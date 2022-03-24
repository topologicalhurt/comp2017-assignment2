/**
 * comp2017 - assignment 2
 * <Connor Sinclair>
 * <510427586>
 */

/* TODOS:

1) FIX INVALID ARGS PROVIDED (E.G letters instead of numbers)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdint.h>

#include "ymirdb.h"

extern inline uint64_t djb2h(const char * str);
extern inline bool is_key(const char * k);
extern inline bool is_simple(char ** args, const size_t nargs, const size_t start);
extern inline bool is_num(const char * s);
extern inline void destroy_entry(entry * e);
extern inline void shallow_copy_entry(entry ** e1, const entry * e2);
extern inline void dcer(entry ** e1, const entry * e2);
extern inline void dcar(entry ** e1, const entry * e2);

entry * dbHead = NULL; // db header
//entry * tail = NULL; // db tail

// PROCESS KEYS THEN VALS.
void pktv(entry ** e, char ** args, size_t nargs) {
	*e = malloc(sizeof(struct entry));
	memcpy((*e) -> key, args[1], strlen(args[1]));
	(*e) -> length = (nargs -= 2);
	(*e) -> isSimp = is_simple(args, nargs, 2);
	size_t tmpsize = (nargs * sizeof(struct element));
	element * tmps = malloc(tmpsize);
	if((*e) -> isSimp) {
		// Simple struct
		for(size_t i = 0; i < nargs; i++) {
			struct element tmp;
			tmp.type = INTEGER;
			tmp.value = atoi(args[i + 2]);
			tmps[i] = tmp;
		}
		(*e) -> values = malloc(tmpsize);
		memcpy((*e) -> values, tmps, tmpsize);
	} else {
		// General struct
		
	}
	free(tmps);
}

void destroy_db(entry * dbh) {
	entry * tmp;
	while(dbh) {
		tmp = dbh;
		dbh = dbh -> next;
		destroy_entry(tmp);
	}
}

void destroy_args(char ** args, const size_t nargs) {
	for(size_t i = 0; i < nargs; i++) {
    free(args[i]);
	}
  free(args);
}

void append_db(entry ** tail, entry * e) {

}

void push_db(entry ** head, entry * e) {
  // Returns: new head.
  // Insert at front (leftmost) position. Don't mutate e.
  entry * nn;
	dcar(&nn, e);
	nn -> next = *head;
	nn -> prev = NULL;
	if(*head) (*head) -> prev = nn;
	*head = nn;
}

void command_bye() {
	printf("bye\n");
}

void command_help() {
	printf("%s\n", HELP);
}

void cmd_set(entry * e) {
	#if DEBUG
		LOG_ENTRY(e);
	#endif
	//push_dbHead(&dbHead, e);
}

void cmd_push(entry * e) {
	#if DEBUG
		LOG_ENTRY(e);
		push_db(&dbHead, e);
		LOG_LL(dbHead);
	#else
		push_dbHead(&dbHead, e);
	#endif
}

void cmd_append(entry * e) {
	#if DEBUG
		LOG_ENTRY(e);
	#endif
}

element * cmd_get() {
	return NULL;
}

int main(void) {

	char line[MAX_LINE]; // Raw input
	char ** args = NULL; // User supplied arguments
	size_t nargs; // Num. of args (excluding command)
	entry * curre = NULL; // Current entry

	while (true) {
		printf("> ");

		if (NULL == fgets(line, MAX_LINE, stdin)) {
			printf("\n");
			command_bye();
			break;
		}

		args = read_args(line, strlen(line), &nargs);

		if(args) {

			// Control flow based on hashed command
			switch(djb2h(args[0])) {
				case SET:
					// Check args
					if(nargs > 2 && is_key(args[1])) {
						pktv(&curre, args, nargs);
						cmd_set(curre);
					}
					break;
				case GET:
					// Check args
					break;
				case PUSH:
					// Check args
					if(nargs > 2 && is_key(args[1])) {
						pktv(&curre, args, nargs);
						cmd_push(curre);
					}
					break;
				case APPEND:
					// Check args
					if(nargs > 2 && is_key(args[1])) {
						pktv(&curre, args, nargs);
						cmd_append(curre);
					}
					break;
				default:
					// Command not recognised
					printf("ERROR: cmd not found.\n");
					break;
			}

			// Call destructors for 'garbage collection.'
			destroy_entry(curre);
			destroy_args(args, nargs);
		}
	}
	// free all entries
	destroy_db(dbHead);
	return 0;
}
