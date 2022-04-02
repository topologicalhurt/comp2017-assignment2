/**
 * comp2017 - assignment 2
 * <Connor Sinclair>
 * <510427586>
 */

/* TODOS:

1) FIX ALL MEMORY LEAKS
2) IMPLEMENT SNAPSHOTS ETC. PROPERLY
3) IMPLEMENT GENERAL REF'S PROPERLY
4) FIX INVALID ARGS PROVIDED (E.G letters instead of numbers)
5) ASSIGN RELEVANT STATES' TO KEYS
*/

/*
<POTENTIAL MAJOR EFFICIENCY IMPROVEMENTS>
1) USING A HASHED LINKED LIST FOR SEARCHES
2) IMPLEMENTING THIS AS STACK-BASED I.E STATS CALCULATED AT ENTRY TIME
NOT AFTER ENTRY OF DATA (WHICH REQUIRES A SEARCH.) VIA A MIN-HEAP.
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

static entry * dbHead = NULL; // db header
static entry * dbTail = NULL; // db tail
static snapshot * snapHead = NULL; // snapshot header
static snapshot * snapTail = NULL; // snapshot tail
static size_t numSnaps = 0;
static size_t nkeys = 0;

extern inline uint64_t djb2h(const char * str) {
	uint64_t hash = 0x1505;
  uint32_t c = 0;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

extern inline bool is_key(const char * k) {
	  return strlen(k) < MAX_KEY && !isdigit(k[0]);
}

extern inline bool is_num(const char * s) {
	for(size_t i = 0; i < strlen(s); i++) {
		if(!isdigit(s[i])) {
			return false;
		}
	}
	return true;
}

extern inline void uppers(char * line) {
	size_t i = 0;
	while(line[i]) {
		line[i] = toupper(line[i]);
		i++;
	}
}

extern inline bool is_simple(char ** args, const size_t nargs, const size_t start) {
	for(size_t i = start; i < nargs; i++) {
		if(!is_num(args[i])) {
			return false;
		}
	}
	return true;
}

static inline void swap_values(entry ** e, const size_t i, const size_t j) {
  element * tmp;
  tmp = malloc(sizeof(struct element));
  *tmp = ((*e) -> values)[i];
  ((*e) -> values)[i] = ((*e) -> values)[j];
  ((*e) -> values)[j] = *tmp;
  free(tmp);
}

static inline void shallow_copy_entry(entry ** e1, const entry * e2) {
  *e1 = malloc(sizeof(struct entry));
  **e1 = *e2;
}

static inline void copy_values(entry ** e1, const entry * e2) {
	(*e1) -> values = malloc(sizeof(struct element) * e2 -> length);
	memcpy((*e1) -> values, e2 -> values, sizeof(struct element) * e2 -> length);
}

// Deep copy entry by reference
void dcer(entry ** e1, const entry * e2) {
  shallow_copy_entry(e1, e2);
  (*e1) -> values = malloc(sizeof(struct element) * e2 -> length);
  (*e1) -> values = e2 -> values;
}

// Deep copy entry by absolute reference
static inline void dcar(entry ** e1, const entry * e2) {
  /* !!!TODO: NEED TO DEEP COPY FORWARD AND BACKWARD !!!*/
  shallow_copy_entry(e1, e2);
	copy_values(e1, e2);
}

char * read_delim(const char line[], size_t * argc, const size_t start) {

  size_t i = start;
  while(i < MAX_LINE && line[i] != '\0' && line[i] != '\n'
  && line[i++] != ' ') {}

  if(i == 0) return NULL;

  char * ret;
  ret = malloc(MAX_LINE);

  *argc = i;
  i = line[i - 1] == ' ' ? i - 1 : i;
  memcpy(ret, &line[start], i - start);
  ret[i - start] = '\0';
  if(start == 0) uppers(ret); // Command argument -> case sensitive
  char * tmp = realloc(ret, i - start + 1);
  if(!tmp) {
    // Catch Error
  }
  ret = tmp;
  return ret;
}

char ** read_args(const char line[], const size_t ll, size_t * nargs) {

  if(ll <= 1 || ll > MAX_LINE) return NULL;

  char ** ret = NULL;
  ret = malloc(sizeof(char*) * (MAX_ARGS));

  for(size_t i = 0; i < MAX_ARGS; i++) {
    ret[i] = malloc(MAX_LINE + 1);
    ret[i][MAX_LINE] = '\0';
  }
  size_t i = 0; size_t j = 0; size_t l;

  while(i + 1 < ll) {
    char * tmp = read_delim(line, &i, i);
    l = strlen(tmp);
    memcpy(ret[j], tmp, l + 1);
    char * tmp2 = realloc(ret[j], l + 1);
    if(!tmp2) {
      // Do something...
    }
    ret[j++] = tmp2;
    free(tmp);
  }
  for(i = j; i < MAX_ARGS; i++) {
    free(ret[i]);
  }
  char ** tmp = realloc(ret, j * sizeof(char*));
  if(!tmp) {
    // Catch Error
  }
  ret = tmp;
  #if DEBUG
    LOG_ARGS(ret, j);
  #endif
  *nargs = j;
  return ret;
}

void expand_entry(entry ** e, const size_t ents) {

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

void destroy_entry(entry ** e) {
  /* !!!TODO: NEED TO FREE FORWARD AND BACKWARD!!! */
  free((*e) -> values);
  free(*e);
}

void destroy_db(entry * head) {
	entry * tmp;
	while(head) {
		tmp = head;
		head = head -> next;
		destroy_entry(&tmp);
	}
}


void destroy_snapshot(snapshot * head) {
	snapshot * tmp;
	while(head) {
		tmp = head;
		head = head -> next;
		// entry * tmp2 = tmp -> entries;
		// printf("\n ENTRY DELETED \n");
		// PRINT_ENTRIES(tmp2);
		destroy_db(tmp -> entries);
		free(tmp);
	}
}

void destroy_args(char *** args, size_t nargs) {
	for(size_t i = 0; i < nargs; i++) {
    free((*args)[i]);
	}
  free(*args);
	*args = NULL;
}

void init_entry(entry ** e) {
	*e = malloc(sizeof(struct entry));
	(*e) -> values = malloc(ENTRY_BUFF * sizeof(struct element));
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

void clean_entry(entry ** e) {
  free((*e) -> values);
	(*e) -> values = malloc(ENTRY_BUFF * sizeof(struct element));
  (*e) -> vSize = 0;
  (*e) -> length = 0;
  (*e) -> isSimp = NULL;
  (*e) -> forward_size = 0;
  (*e) -> forward_max = 0;
  (*e) -> backward_size = 0;
  (*e) -> backward_max = 0;
  (*e) -> forward = NULL;
  (*e) -> backward = NULL;
}

void init_snapshot(snapshot ** s) {
  (*s) = malloc(sizeof(struct snapshot));
	(*s) -> entries = NULL;
  (*s) -> id = 0;
  (*s) -> next = NULL;
  (*s) -> prev = NULL;
}

// OPTIMIZATIONS:
// 1) Iterate over memory instead of copy pasting around it
// 2) expand_entry() doesn't alloc memory properly? Fix.
// 3) Don't just disregard the buffer when popping.

void append_entry(entry ** e, element * ent, const size_t ents) {
	//expand_entry(e, ents);

	// Copy into reallocated memory
	size_t n = ((*e) -> length) + ents;
	element * tmp = malloc(n * sizeof(struct element));
	element * tmp2 = realloc((*e) -> values, n * sizeof(struct element));
	if(!tmp2) {
		// Do something...
	}
	(*e) -> values = tmp2;
	memcpy(&(((*e) -> values)[(*e) -> length]),
	ent, ents * sizeof(struct element));
	(*e) -> length += ents;
	free(tmp);
}

void push_entry(entry ** e, const element * ent, const size_t ents) {
	//expand_entry(e, ents);

	size_t n = ents + ((*e) -> length);
	// Copy elements being added
	element * tmp2 = realloc((*e) -> values, n * sizeof(struct element));
	if(!tmp2) {
		// Do something...
	}
	(*e) -> values = tmp2;
	// Push entries in backwards
	memmove(&((*e) -> values)[ents], (*e) -> values,
	(*e) -> length * sizeof(struct element));
	for(int i = ents - 1; i >= 0; i--) {
		((*e) -> values)[ents - 1 - i] = ent[i];
	}

	(*e) -> length += ents;
}

void set_entry(entry ** e, element * ent, const size_t ents) {
	expand_entry(e, ents);
	memcpy((*e) -> values, ent, ents * sizeof(struct element));
	(*e) -> length = ents;
}

void pop_back_entry(entry ** e) {
	// Basically negates the buffer because we just realloc back down to size
	// If I could be bothered I would fix this
	if (((*e) -> length) == 1) {
		((*e) -> length)--;
		return;
	}
	element * tmp;
	tmp = realloc((*e) -> values, (--((*e) -> length)) * sizeof(struct element));
	if(!tmp) {
		// Do something...
	}
	(*e) -> values = tmp;
}

void pop_front_entry(entry ** e) {
	printf("%d\n\n", ((*e) -> values)[0].value);
	if (((*e) -> length) == 1) {
		((*e) -> length)--;
		return;
	}
	memmove((*e) -> values, &((*e) -> values)[1],
	 (--((*e) -> length)) * sizeof(struct element));
}

void pluck_entry(entry ** e, const int ind) {

	if(ind == ((*e) -> length) - 1) {
		pop_back_entry(e);
		return;
	} else if(ind == 0) {
		pop_front_entry(e);
		return;
	}

	// O(N) space and O(n) time.
	// Better to just iterate over to avoid the auxiliary space.

	// Copy around the element
	element * tmp;
	tmp = malloc((((*e) -> length) - 1) * sizeof(struct element));
	memcpy(tmp, (*e) -> values, ind * sizeof(struct element));
	memcpy(&tmp[ind], &((*e) -> values)[ind + 1], (((*e) -> length) - ind - 1)
 	* sizeof(struct element));
	// Copy back into entry
	memcpy((*e) -> values, tmp, --((*e) -> length) * sizeof(struct element));
	free(tmp);
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
	memcpy((*e) -> key, args[1], strlen(args[1]) + 1);
	(*e) -> isSimp = is_simple(args, nargs - 2, 2);
	if((*e) -> isSimp) return get_vals_from_args(args, nargs - 2);
	else return NULL; // General case...
}

// Append all entries into db
void append_db(entry ** tail, entry ** head, entry ** e) {
	entry * nn;
	dcar(&nn, *e);
	nn -> next = NULL;
	if(*tail) {
		nn -> prev = *tail;
		(*tail) -> next = nn;
	} else {
		nn -> prev = *head;
		*head = *head ? *head : nn;
	}
	*tail = nn;
	nkeys++;
}

// Push entry into the db
void push_db(entry ** head, entry ** tail, entry ** e) {
  entry * nn;
	dcar(&nn, *e);
	nn -> next = *head;
	nn -> prev = NULL;
	if(*head) (*head) -> prev = nn;
	else *tail = nn;
	*head = nn;
	nkeys++;
}

void remove_db(entry ** old, entry ** head, entry ** tail) {

	if(!(*old) || !(*head) || !(*tail)) return;

	entry * oldNext;
	entry * oldPrev;
	oldNext = (*old) -> next;
	oldPrev = (*old) -> prev;

	// First case: Deleting the last remaining node -> set both pointers to NULL
	// Second case: The head is the node to be deleted -> set the head to the forward adj. element
	// Third case: The tail is the node to be deleted -> set the tail to the backward adj. element
	if(*head == *old && *tail == *old) {
		*head = NULL;
		*tail = NULL;
	} else if(*head == *old) {
		*head = oldNext;
	} else if(*tail == *old) {
		*tail = oldPrev;
	}

	// 'bypass' / 'run around' the pointer to be deleted
	if((*old) -> next) {
		oldNext -> prev = oldPrev;
	}

	if((*old) -> prev) {
		oldPrev -> next = oldNext;
	}

	destroy_entry(old);
	nkeys--;
}

// Deep copies all the entries in the db into e
void deep_copy_db(snapshot ** s, entry * tail)  {

	if(!tail) return;

	// entry * tmp = tail;
	// entry * tmp2;
	// copy_values(&tmp2, tail);
	// tmp2 -> prev = NULL;
	// tmp2 -> next = NULL;
	// (*s) -> entries = tmp2;
	//
	// tmp = tmp -> next;
	// tmp2 = tmp2 -> next;

	entry * tmp = tail;

	entry * nh = NULL; // The current ptr. to be copied into as we iterate over db
	entry * nt = NULL; // new tail ~ marked as unused ~
	
	while(tmp) {

		append_db(&nt, &nh, &tmp); // Copy into the current snapshot entry the absolute ref. to db entry
		// copy_values(&tmp2, tmp);
		// entry * last = tmp2;
		// tmp2 -> next = NULL;
		// tmp2 = tmp2 -> next;
		// tmp2 -> prev = last;
		// last -> next = tmp2;
		tmp = tmp -> prev; // Add db entries to snapshot backwards  (reading order)
	}
	dcar(&((*s) -> entries), nh);
	if(nh) destroy_entry(&nh);
}

// ~ CURRENT STATES GIVEN BY DB HEADER. ~

void append_snap(snapshot ** tail, snapshot ** head) {
	snapshot * ns;
	init_snapshot(&ns);
	deep_copy_db(&ns, dbTail);
	ns -> next = NULL;
	if(*tail) {
		ns -> prev = *tail;
		(*tail) -> next = ns;
		ns -> id = (ns -> prev -> id) + 1;
	}	else {
		ns -> prev = *head;
		*head = *head ? *head : ns;
	}
	*tail = ns;
	numSnaps++;
}

// void push_snap(snapshot ** head, entry ** e) {
//   snapshot * ns;
// 	/* TODO */
// }

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
	printf("%s\n", HELP_MSG);
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
	for(size_t i = 0; i < (l/2 + 1); i++) {
		swap_values(e, i, l - i);
	}
}

void cmd_uniq(entry ** e) {

	size_t l = ((*e) -> length) - 1;
	if(l <= 0) return;

	size_t uniqC = 0;
	size_t i = 0;

	for(; i < l; i++) {
		// Overwrite potential non-unique entries with unique entries
		((*e) -> values)[uniqC++] = ((*e) -> values)[i];
		// Continue until next unique entry.
		while((i < l) && (((*e) -> values)[i].value == ((*e) -> values)[i + 1].value)) {i++;}
	}

	// Account for the last value by comparing it to adjacent value
	if(((*e) -> values)[i - 1].value != ((*e) -> values)[l].value) {
		((*e) -> values)[uniqC++] = ((*e) -> values)[l];
	}

	(*e) -> length = uniqC;
	printf(CONFIRMATION_MSG);
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

	printf(CONFIRMATION_MSG);
}

int main(void) {

	char line[MAX_LINE]; // Raw input
	char ** args = NULL;
	size_t nargs;

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

				case BYE:
					command_bye();
					exit(0);
				break;

				case HELP:
					command_help();
				break;

				case SET:

					// Check args
					if(nargs > 2 && is_key((args)[1])) {

							// Key doesn't exist -> make a new entry
							if(!search_db(dbHead, args[1])) {
								entry * tmp = NULL;
								init_entry(&tmp);
								cmd_set(&tmp, args, nargs);
								append_db(&dbTail, &dbHead, &tmp);
								destroy_entry(&tmp);
								break;
							}

							// Modify entry in db
							entry * tmp = search_db(dbHead, args[1]);
							if(tmp) {
								clean_entry(&tmp);
								cmd_set(&tmp, args, nargs);
							}
						}

				break;

				case GET:
					if(nargs == 2 && is_key(args[1])) {
						entry * tmp = search_db(dbHead, args[1]);
						if(tmp) {PRINT_ENTRY(tmp); printf("\n");}
						else printf(FETCH_NONEXISTENT_KEY); // Do something when key not found.
					}
				break;

				case PUSH:
				// Check args
				if(nargs > 2 && is_key((args)[1])) {
					// Modify entry in db
					entry * tmp = search_db(dbHead, args[1]);
					if(tmp) {
						cmd_push(&tmp, args, nargs);
					} else {
						printf(FETCH_NONEXISTENT_KEY);
					}
				}
				break;

				case APPEND:
					// Check args
					if(nargs > 2 && is_key((args)[1])) {
						// Modify entry in db
						entry * tmp = search_db(dbHead, args[1]);
						if(tmp) {
							cmd_append(&tmp, args, nargs);
						} else {
							printf(FETCH_NONEXISTENT_KEY);
						}
					}
				break;

				case SNAPSHOT:
					if(nargs == 1) {
						if(dbHead) {
							append_snap(&snapTail, &snapHead);
							LOG_SNAP(snapHead);
						}
					}
				break;

				case PURGE:
					if(nargs == 2 && is_key(args[1])) {
							entry * tmp;
							tmp = search_db(dbHead, args[1]);
							if(tmp) {
								// Delete from both snapshot and db
								printf(CONFIRMATION_MSG);
							} else {
								printf(FETCH_NONEXISTENT_KEY);
						 }
					}
				break;

				case LIST:
					// FORMAT: <CMD> <CMD>
					if(nargs == 2) {
						// Is this type safe?
						uppers(args[1]);
						switch(djb2h(args[1])) {
							case KEYS:
								if(nkeys > 0) {
									PRINT_KEYS(dbTail);
								} else {
									printf(NO_KEY_LIST);
								}
							break;
							case ENTRIES:
								if(dbTail) {
									PRINT_ENTRIES(dbTail);
								} else {
									printf(NO_KEY_ENTRIES);
								}
							break;
							case SNAPSHOTS:
								if(numSnaps == 0) {
									printf(NO_SNAPSHOTS);
								}
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

				case POP:
					if(nargs == 2 && is_key(args[1])) {
						entry * tmp;
						tmp = search_db(dbHead, args[1]);
						if(tmp) {
							if(tmp -> length == 0) {
								printf(EMPTY_ENTRY);
								break;
							}
							pop_front_entry(&tmp);
						} else {
							printf(FETCH_NONEXISTENT_KEY);
						}
				}
				break;

				case PICK:
					if(nargs == 3 && is_key(args[1])) {
						entry * tmp;
						tmp = search_db(dbHead, args[1]);
						if(tmp) {
							int ind = atoi(args[2]) - 1;
							if(ind >= tmp -> length || ind < 0) {
								printf(INDEX_OUT_OF_RANGE);
								break;
							}
							printf("%d\n\n", (tmp -> values)[ind].value);
						} else {
							printf(FETCH_NONEXISTENT_KEY);
						}
					}
				break;

				case PLUCK:
					if(nargs == 3 && is_key(args[1])) {
						entry * tmp;
						tmp = search_db(dbHead, args[1]);
						if(tmp) {
							int ind = atoi(args[2]) - 1;
							if(ind >= tmp -> length || ind < 0) {
								printf(INDEX_OUT_OF_RANGE);
								break;
							}
							printf("%d\n\n", (tmp -> values)[ind].value);
							pluck_entry(&tmp, ind);
						} else {
							printf(FETCH_NONEXISTENT_KEY);
						}
					}
				break;

				case DEL:
					if(nargs == 2 && is_key(args[1])) {
						entry * tmp;
						tmp = search_db(dbHead, args[1]);
						if(tmp) {
							remove_db(&tmp, &dbHead, &dbTail);
							printf(CONFIRMATION_MSG);
						} else {
							printf(FETCH_NONEXISTENT_KEY);
						}
					}
				break;

				default:
					// Command not recognised
					printf(COMMAND_NOT_FOUND);
					break;
			}

			destroy_args(&args, nargs);
		}
	}
	if (args) destroy_args(&args, nargs);
	if (dbHead) destroy_db(dbHead);
	if (snapHead) destroy_snapshot(snapHead);
	return 0;
}
