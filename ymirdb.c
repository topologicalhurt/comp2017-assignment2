/**
 * comp2017 - assignment 2
 * <Connor Sinclair>
 * <510427586>
 */

/* TODOS:

1) FIX INVALID ARGS PROVIDED (E.G letters instead of numbers)
2) ASSIGN RELEVANT STATES' TO KEYS
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
extern inline void destroy_entry(entry ** e);
extern inline void shallow_copy_entry(entry ** e1, const entry * e2);
extern inline void dcer(entry ** e1, const entry * e2);
extern inline void dcar(entry ** e1, const entry * e2);

// static char ** args = NULL;
// static size_t nargs = 0;
static entry * dbHead = NULL; // db header
static entry * dbTail = NULL; // db tail
static entry * curre = NULL; // current entry

static char ** keys = NULL;
static size_t nkeys = 0;
static size_t kallocs = 0;

void init_entry(entry ** e) {
	*e = malloc(sizeof(struct entry));
	(*e) -> values = malloc(ENTRY_BUFF);
	(*e) -> vSize = ENTRY_BUFF;
	(*e) -> length = 0;
	(*e) -> isSimp = NULL;
	(*e) -> next = NULL;
	(*e) -> prev = NULL;
	(*e) -> forward_size = 0;
	(*e) -> forward_max = 0;
	(*e) -> backward_size = 0;
	(*e) -> backward_max = 0;
	(*e) -> forward = NULL;
	(*e) -> backward = NULL;
}

void expand_entry(entry ** e, const size_t ents) {

	#if DEBUG
		printf("LENGTH: %lu, ENTRY SIZE: %lu, VSIZE: %lu",
		(*e) -> length, ents, (*e) -> vSize);
	#endif

	size_t toAdd = ((*e) -> length) + ents;
	if(toAdd >= (*e) -> vSize) {
		element * tmp;
		tmp = realloc((*e) -> values, (ENTRY_BUFF + toAdd) * sizeof(struct element));
		if(!tmp) {
			// Do something...
		}
		(*e) -> vSize += ENTRY_BUFF + toAdd;
		(*e) -> values = tmp;
	}
}

// Determines if key in db already
// Again... so much faster if using hashmap!
bool key_in_db(char * k) {
	for(size_t i = 0; i < nkeys; i++) {
		if(strcmp(k, keys[i]) == 0) {
			return true;
		}
	}
	return false;
}

bool append_key(char *** keys, char * k) {

	if(key_in_db(k)) {
		return false;
	}

	if(++nkeys >= kallocs * KEY_BUFF) {
		char ** tmp;
		tmp = realloc(*keys, (KEY_BUFF * (++kallocs) * sizeof(char *)));
		if(!tmp) {
			// Do something...
		}
		*keys = tmp;
	}
	(*keys)[nkeys - 1] = malloc(strlen(k));
	memcpy((*keys)[nkeys - 1], k, strlen(k));
	return true;
}

void append_entry(entry ** e, element * ent, const size_t ents) {
	expand_entry(e, ents);
	// Copy into reallocated memory
	memcpy(&(((*e) -> values)[(*e) -> length]),
	ent, ents * sizeof(struct element));
	(*e) -> length += ents;
}

void push_entry(entry ** e, element * ent, const size_t ents) {
	expand_entry(e, ents);
	// Shift by number of elements being added
	memmove(&(((*e) -> values)[ents]),
	(*e) -> values, ((*e) -> length) * sizeof(struct element));
	// Copy elements being added
	memcpy((*e) -> values, ent, ents * sizeof(struct element));
	(*e) -> length += ents;
}

void set_entry(entry ** e, element * ent, const size_t ents) {
	// Be aware we may need to destroy and re-init whole structure later on
	free((*e) -> values);
	(*e) -> length = 0;
	(*e) -> values = malloc(ENTRY_BUFF);
	expand_entry(e, ents);
	memcpy((*e) -> values, ent, ents * sizeof(struct element));
	(*e) -> length += ents;
}

element * get_vals_from_args(char ** args, size_t nargs) {
	element * vals = malloc((nargs * sizeof(struct element)));
	for(size_t i = 0; i < nargs; i++) {
		struct element tmp;
		tmp.type = INTEGER;
		tmp.value = atoi(args[i + 2]);
		vals[i] = tmp;
	}
	return vals;
}

// PROCESS KEYS THEN VALS.
// Returns values to add to entry according to command
element * pktv(entry ** e, char ** args, size_t nargs) {
	// Create new entry if key doesn't exist. Otherwise, modify existing entry
	nargs -= 2;
	memcpy((*e) -> key, args[1], strlen(args[1]));
	(*e) -> isSimp = is_simple(args, nargs, 2);
	if((*e) -> isSimp) return get_vals_from_args(args, nargs);
	else return NULL; // General case...
}

void destroy_db(entry * dbh) {
	entry * tmp;
	while(dbh) {
		tmp = dbh;
		dbh = dbh -> next;
		destroy_entry(&tmp);
	}
}

void destroy_args(char ** args, size_t nargs) {
	for(size_t i = 0; i < nargs; i++) {
    free(args[i]);
	}
  free(args);
}

// Append all entries into db
void append_db(entry ** tail, entry ** e) {
	entry * nn;
	dcar(&nn, *e);
	nn -> next = NULL;
	if(*tail) {
		nn -> prev = *tail;
		(*tail) -> next = nn;
	} else {
		nn -> prev = dbHead;
		dbHead = dbHead ? dbHead : nn;
	}
	*tail = nn;
}

// Push entry into the db
void push_db(entry ** head, entry ** e) {
  entry * nn;
	dcar(&nn, *e);
	nn -> next = *head;
	nn -> prev = NULL;
	if(*head) (*head) -> prev = nn;
	else dbTail = nn;
	*head = nn;
}

// Again we could hash the linked list according to key for O(1) search...
entry * search_db(entry * head, char * k) {
	entry * tmp;
	tmp = head;
	while(tmp) {
		if(strcmp(k, tmp -> key) == 0) {
			return tmp;
		}
		tmp = tmp -> next;
	}
	return NULL;
}

void command_bye() {
	printf("bye\n");
}

void command_help() {
	printf("%s\n", HELP);
}

void cmd_set(entry ** e, char ** args, size_t nargs) {
	element * vals;
	vals = pktv(e, args, nargs);
	set_entry(e, vals, nargs - 2);
	#if DEBUG
		LOG_VALS(vals, nargs - 2);
	#endif
	free(vals);
}

element * cmd_get() {
	return NULL;
}

void cmd_push(entry ** e, char ** args, size_t nargs) {
	element * vals;
	vals = pktv(e, args, nargs);
	push_entry(e, vals, nargs - 2);
	#if DEBUG
		LOG_VALS(vals, nargs - 2);
	#endif
	free(vals);
}

void cmd_append(entry ** e, char ** args, size_t nargs) {
	element * vals;
	vals = pktv(e, args, nargs);
	append_entry(e, vals, nargs - 2);
	#if DEBUG
		LOG_VALS(vals, nargs - 2);
	#endif
	free(vals);
}

void cmd_db_set(entry * e) {
	LOG_ENTRY(e);
}

element * cmd_db_get() {
	return NULL;
}

void cmd_db_push(entry * e) {
	LOG_ENTRY(e);
	push_db(&dbHead, &e);
	LOG_LL(dbHead);
}

void cmd_db_append(entry * e) {
	LOG_ENTRY(e);
	append_db(&dbTail, &e);
	LOG_LL(dbHead);
}

int main(void) {

	char line[MAX_LINE]; // Raw input
	char ** args = NULL;
	size_t nargs;
	keys = malloc(KEY_BUFF * sizeof(char *));

	init_entry(&curre);

	while (true) {
		printf("> ");

		if (NULL == fgets(line, MAX_LINE, stdin)) {
			printf("\n");
			command_bye();
			break;
		}

		args = read_args(line, strlen(line), &(nargs));

		if(args) {

			// Control flow based on hashed command
			switch(djb2h(args[0])) {
				case SET:
				// Check args
				if(nargs > 2 && is_key((args)[1])) {
					// Key in db...
					if(!append_key(&keys, args[1])) {
						curre = search_db(dbHead, args[1]);
					} else {
						curre = NULL;
					}
					cmd_set(&curre, args, nargs);
				}
				break;
				case GET:
				// Check args
				break;
				case PUSH:
				// Check args
				if(nargs > 2 && is_key((args)[1])) {
					append_key(&keys, args[1]);
					cmd_push(&curre, args, nargs);
				}
				break;
				case APPEND:
				// Check args
				if(nargs > 2 && is_key((args)[1])) {
					append_key(&keys, args[1]);
					cmd_append(&curre, args, nargs);
				}
				break;
				#if DEBUG
					case DBSET:
						// Check args
						if(nargs == 2 && is_key((args)[1])) {
							cmd_db_set(curre);
						}
						break;
					case DBGET:
						// Check args
						break;
					case DBPUSH:
						// Check args
						if(nargs == 2 && is_key((args)[1])) {
							cmd_db_push(curre);
						}
						break;
					case DBAPPEND:
						// Check args
						if(nargs == 2 && is_key((args)[1])) {
							cmd_db_append(curre);
						}
						break;
				#endif
				default:
					// Command not recognised
					printf("ERROR: cmd not found.\n");
					break;
			}

			#if DEBUG
				if(curre) LOG_ENTRY(curre);
				LOG_ARGS(keys, nkeys);
			#endif

			destroy_args(args, nargs);
		}
	}
	destroy_entry(&curre);
	destroy_args(keys, nkeys);
	destroy_db(dbHead);
	return 0;
}
