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

void destroy_refs(entry ** e) {
	for(int i = 0; i < FORWARD_MAX; i++) {
		free(((*e) -> forward)[i]);
	}
	for(int i = 0; i < BACKWARD_MAX; i++) {
		free(((*e) -> backward)[i]);
	}
	free((*e) -> forward);
	free((*e) -> backward);
	(*e) -> forward = NULL;
	(*e) -> backward = NULL;
}

void destroy_entry(entry ** e) {
  /* !!!TODO: NEED TO FREE FORWARD AND BACKWARD!!! */
	if((*e) -> forward && (*e) -> backward) destroy_refs(e);
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

void destroy_snapshot(snapshot * s) {
	destroy_db(s -> entries);
	free(s);
}


void destroy_snapshots(snapshot * head) {
	snapshot * tmp;
	while(head) {
		tmp = head;
		head = head -> next;
		destroy_snapshot(tmp);
	}
}

void destroy_args(char *** args, size_t nargs) {
	for(size_t i = 0; i < nargs; i++) {
    free((*args)[i]);
	}
  free(*args);
	*args = NULL;
}

void create_refs(entry ** e) {
	(*e) -> forward = malloc(sizeof(struct entry) * FORWARD_MAX);
	(*e) -> backward = malloc(sizeof(struct entry) * BACKWARD_MAX);
	for(int i = 0; i < FORWARD_MAX; i++) {
		((*e) -> forward)[i] = malloc(sizeof(struct entry));
	}
	for(int i = 0; i < BACKWARD_MAX; i++) {
		((*e) -> backward)[i] = malloc(sizeof(struct entry));
	}
}

void init_entry(entry ** e) {
	*e = malloc(sizeof(struct entry));
	(*e) -> values = malloc(ENTRY_BUFF * sizeof(struct element));
	(*e) -> vSize = ENTRY_BUFF;
	(*e) -> length = 0;
	(*e) -> general = false;
	(*e) -> next = NULL;
	(*e) -> prev = NULL;
	(*e) -> forward_size = 0;
	// (*e) -> forward_max = 0;
	(*e) -> backward_size = 0;
	// (*e) -> backward_max = 0;
	create_refs(e);
}

void clean_entry(entry ** e) {
  free((*e) -> values);
	(*e) -> values = malloc(ENTRY_BUFF * sizeof(struct element));
  (*e) -> vSize = 0;
  (*e) -> length = 0;
  (*e) -> general = false;
  (*e) -> forward_size = 0;
  // (*e) -> forward_max = 0;
  (*e) -> backward_size = 0;
  // (*e) -> backward_max = 0;
	// destroy_refs(e);
	// create_refs(e);
}

void init_snapshot(snapshot ** s) {
  (*s) = malloc(sizeof(struct snapshot));
	(*s) -> entries = NULL;
	(*s) -> tail = NULL;
  (*s) -> id = 1;
  (*s) -> next = NULL;
  (*s) -> prev = NULL;
}

static inline void shallow_copy_entry(entry ** e1, const entry * e2) {
  *e1 = malloc(sizeof(struct entry));
  **e1 = *e2;
}

static inline void shallow_copy_snap(snapshot ** s1, const snapshot * s2) {
	*s1 = malloc(sizeof(struct snapshot));
	**s1 = *s2;
}

static inline void copy_values(entry ** e1, const entry * e2) {
	(*e1) -> values = malloc(sizeof(struct element) * e2 -> length);
	memcpy((*e1) -> values, e2 -> values, sizeof(struct element) * e2 -> length);
}

static inline void copy_refs(entry ** e1, const entry * e2) {
	memcpy((*e1) -> forward, e2 -> forward, sizeof(struct entry) * FORWARD_MAX);
	memcpy((*e1) -> backward, e2 -> backward, sizeof(struct entry) * BACKWARD_MAX);
	for(int i = 0; i < FORWARD_MAX; i++) {
		((*e1) -> forward)[i] = (e2 -> forward)[i];
	}
	for(int i = 0; i < BACKWARD_MAX; i++) {
		((*e1) -> backward)[i] = (e2 -> backward)[i];
	}
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
	copy_refs(e1, e2);
}

void deep_copy_snap(snapshot ** s1, const snapshot * s2) {
	shallow_copy_snap(s1, s2);
	dcar(&((*s1) -> entries), s2 -> entries);
	if(s2 -> tail) dcar(&((*s1) -> tail), s2 -> tail);
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

// OPTIMIZATIONS:
// 1) Iterate over memory instead of copy pasting around it
// 2) expand_entry() doesn't alloc memory properly? Fix.
// 3) Don't just disregard the buffer when popping.

void append_entry(entry ** e, element * ent, const size_t ents) {
	//expand_entry(e, ents);

	// Copy into reallocated memory
	size_t n = ((*e) -> length) + ents;
	// element * tmp = malloc(n * sizeof(struct element));
	element * tmp2 = realloc((*e) -> values, n * sizeof(struct element));
	if(!tmp2) {
		// Do something...
	}
	(*e) -> values = tmp2;
	for(int i = 0; i < ents; i++) {
		((*e) -> values)[(*e) -> length + i] = ent[i];
	}
	// memcpy(&(((*e) -> values)[(*e) -> length]),
	// ent, ents * sizeof(struct element));
	(*e) -> length += ents;
	// free(tmp);
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

// THESE NEED TO WORK WITH GENERAL REF's TOO!

int get_min_of_entry(const entry * e) {
	int min = 0x7FFFFFFF;
	for(int i = 0; i < e -> length; i++) {
		min = (e -> values)[i].value <= min ? (e -> values)[i].value : min;
	}
	return min;
}

int get_max_of_entry(const entry * e) {
	int max = 0x80000000;
	for(int i = 0; i < e -> length; i++) {
		max = (e -> values)[i].value >= max ? (e -> values)[i].value : max;
	}
	return max;
}

int sum_entry(entry * e) {
	int sum = 0;
	for(int i = 0; i < e -> length; i++) {
		sum += (e -> values)[i].value;
	}
	return sum;
}

void remove_db(entry ** old, entry ** head, entry ** tail) {

	if(!(*old)) return;

	// First case: Deleting the last remaining node -> set both pointers to NULL
	// Second case: The head is the node to be deleted -> set the head to the forward adj. element
	// Third case: The tail is the node to be deleted -> set the tail to the backward adj. element
	if(*head == *old && *tail == *old) {
		*head = NULL;
		*tail = NULL;
	} else if(*head == *old) {
		*head = (*old) -> next;
	} else if(*tail == *old) {
		*tail = (*old) -> prev;
	}

	// 'bypass' / 'run around' the pointer to be deleted
	if((*old) -> next) {
		(*old) -> next -> prev = (*old) -> prev;

	}

	if((*old) -> prev) {
		(*old) -> prev -> next = (*old) -> next;
	}

	destroy_entry(old);
}

void remove_snap(snapshot ** old, snapshot ** head, snapshot ** tail) {

	if(!(*old)) return;

	snapshot * oldNext;
	snapshot * oldPrev;
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

	destroy_snapshot(*old);
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

element * get_vals_from_args(char ** args, const size_t nargs, entry ** e, entry * head) {
	memcpy((*e) -> key, args[1], strlen(args[1]) + 1);
	element * vals = malloc((nargs * sizeof(struct element)));
	for(size_t i = 0; i < nargs; i++) {
		struct element tmp;
		if(is_num(args[i + 2])) {
			tmp.type = INTEGER;
			tmp.value = atoi(args[i + 2]);
		} else {
			tmp.type = ENTRY;
			tmp.entry = search_db(head, args[i + 2]);
			(*e) -> forward_size++;
			(*e) -> general = true;
		}
		vals[i] = tmp;
	}
	return vals;
}

// Check's and responds to if key not in db, valid, or self-referential
// -1 if self-referential, 0 if valid, 1 if not in db.
extern inline int8_t is_permitted_entry(char ** args, const size_t nargs,
	entry * head) {
	for(int i = 2; i < nargs; i++) {
		// Cannot be self-referential
		if(is_key(args[i])) {

			entry * tmp;
			tmp = search_db(head, args[i]);

			if(strcmp(args[1], args[i]) == 0) {
				printf(UNPERMITTED_REF);
				return -1;
			}
			if(tmp) return 0;
			else {printf(FETCH_NONEXISTENT_KEY); return 1;}
		}
	}
	return 0;
}

void get_db_from_snap(snapshot * s, entry ** head, entry ** tail) {
	*head = NULL;
	*tail = NULL;
	entry * tmp = s -> tail;
	while(tmp) {
		append_db(tail, head, &tmp);
		tmp = tmp -> prev;
	}
}

void clean_db(entry * head) {
	entry * tmp = head;
	while(tmp) {
		clean_entry(&tmp);
		tmp = tmp -> next;
	}
}

snapshot * get_snap_from_index(snapshot * head, size_t index) {
	size_t i = 0;
	snapshot * tmp = head;
	while(tmp) {
		if(++i == index) {
			return tmp;
		}
		tmp = tmp -> next;
	}
	return NULL;
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
	(*s) -> entries = nh;
	(*s) -> tail = nt;
}

void pluck_snap(snapshot ** sHead, snapshot ** sTail, char * k) {
	//snapshot * next = NULL; // if we delete tmp, the next adjacent member
	snapshot * tmp2;
	//deep_copy_snap(&tmp2, *sHead);
	tmp2 = *sHead;

	while(tmp2) {
		entry * search = search_db(tmp2 -> entries, k);
		if(search) {
			// Only entry in list -> delete snapshot.
			// Otherwise, remove specific entry found.
			// if(!(tmp2 -> entries -> next) && !(tmp2 -> entries -> prev)) {
			// 	next = tmp2 -> next;
			// 	remove_snap(&tmp2, sHead, sTail);
			// 	tmp2 = next;
			// 	continue;
			// } else {
			// 	remove_db(&search, &(tmp2 -> entries), &(tmp2 -> tail));
			// 	search = NULL;
			// }
			remove_db(&search, &(tmp2 -> entries), &(tmp2 -> tail));
			search = NULL;
		}
		tmp2 = tmp2 -> next;
	}
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
}

void command_bye() {
	printf("bye\n");
}

void command_help() {
	printf("%s\n", HELP_MSG);
}

void cmd_set(entry ** e, char ** args, const size_t nargs) {
	element * vals;
	vals = get_vals_from_args(args, nargs - 2, e, dbHead);
	set_entry(e, vals, nargs - 2);
	#if DEBUG
		LOG_VALS(vals, nargs - 2);
	#endif
	free(vals);
	printf(CONFIRMATION_MSG);
}

void cmd_push(entry ** e, char ** args, size_t nargs) {
	element * vals;

	vals = get_vals_from_args(args, nargs - 2, e, dbHead);

	push_entry(e, vals, nargs - 2);
	#if DEBUG
		LOG_VALS(vals, nargs - 2);
	#endif
	free(vals);
	printf(CONFIRMATION_MSG);
}

void cmd_append(entry ** e, char ** args, size_t nargs) {
	element * vals;
	vals = get_vals_from_args(args, nargs - 2, e, dbHead);
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

					if(is_permitted_entry(args, nargs, dbHead) != 0) break;

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

							// Modify existing entry in db
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

				if(is_permitted_entry(args, nargs, dbHead) != 0) break;

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

					if(is_permitted_entry(args, nargs, dbHead) != 0) break;

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
							printf("saved as snapshot %d\n\n", snapTail -> id);
						}
					}
				break;

				case PURGE:
					if(nargs == 2 && is_key(args[1])) {
							entry * tmp;
							tmp = search_db(dbHead, args[1]);
							if(tmp) {
								// Delete from both snapshot and db
								remove_db(&tmp, &dbHead, &dbTail);
								pluck_snap(&snapHead, &snapTail, args[1]);
								nkeys--;
								printf(CONFIRMATION_MSG);
							} else {
								// Remove from just snapshot
								pluck_snap(&snapHead, &snapTail, args[1]);
								printf(CONFIRMATION_MSG);
							}
							#if DEBUG
								LOG_SNAP(snapHead);
							#endif
					}
				break;

				case DROP:
					if(nargs == 2 && is_num(args[1])) {
						snapshot * tmp;
						tmp = get_snap_from_index(snapHead, atoi(args[1]));
						if(tmp) {
							remove_snap(&tmp, &snapHead, &snapTail);
							printf(CONFIRMATION_MSG);
						} else {
							printf(NO_SUCH_SNAPSHOT);
						}
					}
				break;

				case ROLLBACK:
					if(nargs == 2 && is_num(args[1])) {

						snapshot * tmp = get_snap_from_index(snapHead, atoi(args[1]));
						if(tmp) {
							destroy_db(dbHead);
							get_db_from_snap(tmp, &dbHead, &dbTail);
							snapTail = tmp;
							if(tmp -> next) {
								destroy_snapshots(tmp -> next);
								snapTail -> next = NULL;
							}

							printf(CONFIRMATION_MSG);
						} else {
							printf(NO_SUCH_SNAPSHOT);
						}
					}
					break;

				case CHECKOUT:
					if(nargs == 2 && is_num(args[1])) {

						snapshot * tmp = get_snap_from_index(snapHead, atoi(args[1]));
						if(tmp) {
							destroy_db(dbHead);
							get_db_from_snap(tmp, &dbHead, &dbTail);
							printf(CONFIRMATION_MSG);
						} else {
							printf(NO_SUCH_SNAPSHOT);
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
								if(dbTail) {
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
								PRINT_SNAP_IDS(snapTail);
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
								printf(CONFIRMATION_MSG);
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
							nkeys--;
							printf(CONFIRMATION_MSG);
						} else {
							printf(FETCH_NONEXISTENT_KEY);
						}
					}
				break;

				case MIN:
					if(nargs == 2 && is_key(args[1])) {
						entry * tmp;
						tmp = search_db(dbHead, args[1]);
						printf("%d\n\n", get_min_of_entry(tmp));
					}
				break;

				case MAX:
					if(nargs == 2 && is_key(args[1])) {
						entry * tmp;
						tmp = search_db(dbHead, args[1]);
						printf("%d\n\n", get_max_of_entry(tmp));
					}
				break;

				case SUM:
					if(nargs == 2 && is_key(args[1])) {
						entry * tmp;
						tmp = search_db(dbHead, args[1]);
						printf("%d\n\n", sum_entry(tmp));
					}
				break;

				case LEN:
					if(nargs == 2 && is_key(args[1])) {
						entry * tmp;
						tmp = search_db(dbHead, args[1]);
						printf("%zu\n\n", tmp -> length);
					}
				break;

				case TYPE:
					if(nargs == 2 && is_key(args[1])) {
						entry * tmp;
						tmp = search_db(dbHead, args[1]);
						if(tmp -> general) {
							printf(TYPE_GENERAL);
						} else {
							printf(TYPE_SIMPLE);
						}
					}
				break;

				case FORWARD:

				break;

				case BACKWARD:

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
	if (snapHead) destroy_snapshots(snapHead);
	return 0;
}
