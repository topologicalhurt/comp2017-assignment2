/**
 * comp2017 - assignment 2
 * <Connor Sinclair>
 * <510427586>
 */

/* TODOS:

1) FIX INVALID ARGS PROVIDED (E.G letters instead of numbers)
2) ASSIGN RELEVANT STATES' TO KEYS
3) EXTENSIVE TESTING TO FIND HEAP CORRUPTION BUG
*/

/*
<POTENTIAL MAJOR EFFICIENCY IMPROVEMENTS>
1) USING A HASHED LINKED LIST FOR SEARCHES
2) IMPLEMENTING THIS AS STACK-BASED I.E STATS CALCULATED AT ENTRY TIME
NOT AFTER ENTRY OF DATA (WHICH REQUIRES A SEARCH.)
3) USING A HASHMAP FOR KEY COLLISION
4) USING A MORE EFFICIENT SORT METHOD
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
extern inline void swap_values(entry ** e, const size_t i, const size_t j);
extern inline void destroy_entry(entry ** e);
extern inline void init_entry(entry ** e);
extern inline void clean_entry(entry * e);
extern inline void shallow_copy_entry(entry ** e1, const entry * e2);
extern inline void dcer(entry ** e1, const entry * e2);
extern inline void dcar(entry ** e1, const entry * e2);

// static char ** args = NULL;
// static size_t nargs = 0;
static entry * dbHead = NULL; // db header
static entry * dbTail = NULL; // db tail

static char ** keys = NULL;
static size_t nkeys = 0;
static size_t kallocs = 0;

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
bool key_in_db(const char * k) {
	for(size_t i = 0; i < nkeys; i++) {
		if(strcmp(k, keys[i]) == 0) {
			return true;
		}
	}
	return false;
}

void append_key(char *** keys, const char * k) {

	if(++nkeys >= kallocs * KEY_BUFF) {
		char ** tmp;
		tmp = realloc(*keys, (KEY_BUFF * (++kallocs) * sizeof(char *)));
		if(!tmp) {
			// Do something...
		}
		*keys = tmp;
	}
	(*keys)[nkeys - 1] = malloc(strlen(k) + 1);
	memcpy((*keys)[nkeys - 1], k, strlen(k));
	// (*keys)[nkeys - 1][strlen(k)] = '\0';
}

void append_entry(entry ** e, element * ent, const size_t ents) {
	expand_entry(e, ents);
	// Copy into reallocated memory
	size_t offset = (*e) -> length;
	memcpy(&(((*e) -> values)[offset]),
	ent, ents * sizeof(struct element));
	(*e) -> length += ents;
}

void push_entry(entry ** e, const element * ent, const size_t ents) {
	expand_entry(e, ents);
	// Shift by number of elements being added
	memmove(&(((*e) -> values)[ents]),
	(*e) -> values, ((*e) -> length) * sizeof(struct element));
	// Copy elements being added
	memcpy((*e) -> values, ent, ents * sizeof(struct element));
	(*e) -> length += ents;
}

void set_entry(entry ** e, element * ent, const size_t ents) {
	free((*e) -> values);
	(*e) -> length = ents;
	(*e) -> values = malloc(ENTRY_BUFF * sizeof(struct element));
	expand_entry(e, ents);
	memcpy((*e) -> values, ent, ents * sizeof(struct element));
}

element * get_vals_from_args(char ** args, const size_t nargs) {
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
element * pktv(entry ** e, char ** args, const size_t nargs) {
	// Create new entry if key doesn't exist. Otherwise, modify existing entry
	memcpy((*e) -> key, args[1], strlen(args[1]));
	(*e) -> isSimp = is_simple(args, nargs - 2, 2);
	if((*e) -> isSimp) return get_vals_from_args(args, nargs - 2);
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
	destroy_entry(e);
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
	destroy_entry(e);
}

// Again we could hash the linked list according to key for O(1) search...
entry * search_db(entry * head, const char * k) {
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

void cmd_set(entry ** e, char ** args, const size_t nargs) {
	element * vals;
	vals = pktv(e, args, nargs);
	set_entry(e, vals, nargs - 2);
	#if DEBUG
		LOG_VALS(vals, nargs - 2);
	#endif
	free(vals);
	printf(CONFIRMATION_MSG);
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
	printf(CONFIRMATION_MSG);
}

void cmd_append(entry ** e, char ** args, size_t nargs) {
	element * vals;
	vals = pktv(e, args, nargs);
	append_entry(e, vals, nargs - 2);
	#if DEBUG
		LOG_VALS(vals, nargs - 2);
	#endif
	free(vals);
	printf(CONFIRMATION_MSG);
}

void cmd_rev(entry ** e) {
	size_t l = ((*e) -> length) - 1;
	for(size_t i = 0; i < l/2; i++) {
		swap_values(e, i, l - i);
	}
}

void cmd_uniq(entry ** e) {

	size_t l = ((*e) -> length) - 1;
	if(l <= 0) return;

	LOG_VALS((*e) -> values, (*e) -> length);

	size_t uniqC = 0;
	size_t i = 0;

	for(; i < l; i++) {
		// Overwrite potential non-unique entries with unique entries
		((*e) -> values)[uniqC++] = ((*e) -> values)[i];
		// Continue until next unique entry.
		while(((*e) -> values)[i].value == ((*e) -> values)[i + 1].value) {
			i++;
		}
	}

	// Account for the last value by comparing it to adjacent value
	if(i > 2) {
		if(((*e) -> values)[i - 1].value != ((*e) -> values)[l].value) {
			((*e) -> values)[uniqC++] = ((*e) -> values)[l];
		}
	}

	(*e) -> length = uniqC;
	LOG_VALS((*e) -> values, (*e) -> length);
}

void cmd_sort(entry ** e) {
	// Sort in ascend. order using basic bubble sort
 size_t i, j;
 size_t l = (*e) -> length;
 bool swapped;

 for (i = 0; i < l-1; i++)
 {
   swapped = false;
   for (j = 0; j < l-i-1; j++)
   {
      if (((*e) -> values)[j].value > ((*e) -> values)[j+1].value)
      {
         swap_values(e, j, j + 1);
         swapped = true;
      }
   }
   if (swapped == false) break;
	}
}

void cmd_db_set(entry * e) {
	LOG_ENTRY(e);
}

element * cmd_db_get() {
	return NULL;
}

void cmd_db_push(entry * e) {
	push_db(&dbHead, &e);
}

void cmd_db_append(entry * e) {
	append_db(&dbTail, &e);
}

int main(void) {

	char line[MAX_LINE]; // Raw input
	char ** args = NULL;
	size_t nargs;
	keys = malloc(KEY_BUFF * sizeof(char *));

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
					if(key_in_db(args[1])) {
						// Modify entry in db
						entry * tmp = search_db(dbHead, args[1]);
						if(tmp) {
							cmd_set(&tmp, args, nargs);
						}
					} else {
						entry * tmp;
						init_entry(&tmp);
						append_key(&keys, args[1]);
						cmd_set(&tmp, args, nargs);
						append_db(&dbTail, &tmp);
					}
				}
				break;

				case GET:
					if(nargs == 2 && is_key(args[1])) {
						entry * tmp = search_db(dbHead, args[1]);
						if(tmp) {PRINT_ENTRY(tmp);}
						else printf(FETCH_NONEXISTENT_KEY); // Do something when key not found.
					}
				break;

				case PUSH:
				// Check args
				if(nargs > 2 && is_key((args)[1])) {
					if(key_in_db(args[1])) {
						// Modify entry in db
						entry * tmp = search_db(dbHead, args[1]);
						if(tmp) {
							cmd_push(&tmp, args, nargs);
						}
					} else {
						entry * tmp;
						init_entry(&tmp);
						append_key(&keys, args[1]);
						cmd_push(&tmp, args, nargs);
						append_db(&dbTail, &tmp);
					}
				}
				break;

				case APPEND:
				// Check args
				if(nargs > 2 && is_key((args)[1])) {
					if(key_in_db(args[1])) {
						// Modify entry in db
						entry * tmp = search_db(dbHead, args[1]);
						if(tmp) {
							cmd_append(&tmp, args, nargs);
						}
					} else {
						entry * tmp;
						init_entry(&tmp);
						append_key(&keys, args[1]);
						cmd_append(&tmp, args, nargs);
						append_db(&dbTail, &tmp);
					}
				}
				break;

				case LIST:
					// FORMAT: <CMD> <CMD>
					if(nargs == 2) {
						// This is type safe?
						uppers(args[1]);
						switch(djb2h(args[1])) {
							case KEYS:
								if(nkeys > 0) {
									PRINT_KEYS(keys, nkeys);
								}
							break;
							case ENTRIES:

							break;
							case SNAPSHOTS:

							break;
							default:
								printf(COMMAND_NOT_FOUND);
							break;
						}
					} else {
						printf(COMMAND_NOT_FOUND);
					}
				break;

				case REV:
					if(nargs == 2 && is_key(args[1])) {
							entry * tmp;
							tmp = search_db(dbHead, args[1]);
							if(tmp) {
								cmd_rev(&tmp);
							} else {
								printf(FETCH_NONEXISTENT_KEY);
						 }
					}
				break;

				case UNIQ:
					if(nargs == 2 && is_key(args[1])) {
							entry * tmp;
							tmp = search_db(dbHead, args[1]);
							if(tmp) {
								cmd_uniq(&tmp);
							} else {
								printf(FETCH_NONEXISTENT_KEY);
						 }
					}
				break;

				case SORT:
					if(nargs == 2 && is_key(args[1])) {
							entry * tmp;
							tmp = search_db(dbHead, args[1]);
							if(tmp) {
								cmd_sort(&tmp);
							} else {
								printf(FETCH_NONEXISTENT_KEY);
						 }
					}
				break;

				#if DEBUG
					case DBSET:
						// Check args
						if(nargs == 2 && is_key((args)[1])) {
						}
						break;
					case DBGET:
						// Check args
						break;
					case DBPUSH:
						// Check args
						if(nargs == 2 && is_key((args)[1])) {
						}
						break;
					case DBAPPEND:
						// Check args
						if(nargs == 2 && is_key((args)[1])) {
						}
						break;
				#endif

				default:
					// Command not recognised
					printf(COMMAND_NOT_FOUND);
					break;
			}

			#if DEBUG
				if(nkeys > 0) LOG_ARGS(keys, nkeys);
				if(dbHead) LOG_LL(dbHead);
			#endif

			destroy_args(args, nargs);
		}
	}
	destroy_args(keys, nkeys);
	destroy_db(dbHead);
	return 0;
}
