/**
 * comp2017 - assignment 2
 * <Connor Sinclair>
 * <510427586>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdint.h>

#include "ymirdb.h"

extern inline uint64_t djb2h(char * str);
extern inline bool is_key(char * k);
extern inline bool is_simple(char ** args, size_t nargs);

void destroy_entry(entry * e) {
	free(e -> values);
	//free(e -> next);
	//free(e -> prev);
	// Free remaining elements (forward and backward)

}

void command_bye() {
	printf("bye\n");
}

void command_help() {
	printf("%s\n", HELP);
}

void cmd_set(entry e) {
	#if DEBUG
		LOG_ENTRY(e);
	#endif
}

element * cmd_get() {
	return NULL;
}

void cmd_push(entry e) {
	#if DEBUG
		LOG_ENTRY(e);
	#endif
}

void cmd_append(entry e) {
	#if DEBUG
		LOG_ENTRY(e);
	#endif
}

// PROCESS KEYS THEN VALS.
void pktv(entry * e, char ** args, size_t nargs) {
	memcpy(e -> key, args[1], strlen(args[1]));
	e -> isSimp = is_simple(&args[2], nargs - 1);
	e -> length = nargs - 1;
	element * tmps = malloc(e -> length * sizeof(struct element));
	if(e -> isSimp) {
		// Simple struct
		for(int i = 2; i <= nargs; i++) {
			struct element tmp;
			tmp.type = INTEGER;
			tmp.value = atoi(args[i]);
			memcpy(&tmps[i - 2], &tmp, sizeof(struct element));
		}
		e -> values = malloc(e -> length * sizeof(struct element));
		memcpy(&(e -> values[0]), &(tmps[0]), e -> length * sizeof(struct element));
	} else {
		// General struct
	}
	free(tmps);
}

int main(void) {

	char line[MAX_LINE];
	char ** args = NULL;
	size_t nargs; // num of args (excluding command)
	struct entry curre; // current entry

	while (true) {
		printf("> ");

		if (NULL == fgets(line, MAX_LINE, stdin)) {
			printf("\n");
			command_bye();
			return 0;
		}

		args = read_args(line, strlen(line), &nargs);

		if(args) {

			// Control flow based on hashed command
			switch(djb2h(args[0])) {
				case SET:
					// Check args
					if(nargs >= 2 && is_key(args[1])) {
						pktv(&curre, args, nargs);
					}
					cmd_set(curre);
					break;
				case GET:
					// Check args
					break;
				case PUSH:
					// Check args
					if(nargs >= 2 && is_key(args[1])) {
						pktv(&curre, args, nargs);
					}
					cmd_push(curre);
					break;
				case APPEND:
					// Check args
					if(nargs >= 2 && is_key(args[1])) {
						pktv(&curre, args, nargs);
					}
					cmd_append(curre);
					break;
				default:
					// Command not recognised
					printf("ERROR: cmd not found.\n");
					break;
			}

			// DESTRUCTOR's CALLED
			destroy_entry(&curre);
			for(int i = 0; i < nargs; i++) {
				free(args[i]);
			}
			free(args);
		}
	}
	
	free(curre);
	return 0;
}
