#ifndef YMIRDB_H
#define YMIRDB_H

// LIMS
#define MAX_KEY 0x10
#define MAX_LINE 0x400
#define MAX_ARGS (MAX_LINE >> 1) - 1
#define ENTRY_BUFF 0x40 // Num. of entries before realloc
#define KEY_BUFF 0x40 // Num. keys before realloc

// HASHED COMMAND VALUES
#define SET 0xB881D31
#define GET 0xB87EA25
#define PUSH 0x17C8A6265
#define APPEND 0x652A5530C1D
#define SNAPSHOT 0x1AE64326BF8B35
#define ROLLBACK 0x1AE639A0CEDE2F
#define LIST 0x17C87FDE1
#define KEYS 0x17C876141
#define ENTRIES 0xD0A87F429F9F
#define SNAPSHOTS 0x377AEA7FEB0F228
#define SUM 0xB881F3A
#define MIN 0xB880429
#define MAX 0xB88032B
#define REV 0xB8818F2
#define UNIQ 0x17C8D0142
#define SORT 0x17C8BEDED
#define DROP 0x17C83C09A
#define CHECKOUT 0x1AE5A29D29F01B
#define DBSET 0x310CF31DB7
#define DBGET 0x310CF2EAAB
#define DBPUSH 0x652AB5573AB
#define DBAPPEND 0x1AE5AAB32786E3

// MSG'S:
#define FETCH_NONEXISTENT_KEY "no such key\n"
#define CONFIRMATION_MSG "ok\n"
#define COMMAND_NOT_FOUND "ERROR: cmd not found\n"
#define KEYS_NOT_FOUND "no keys\n"
#define EMPTY_ENTRY "nil\n"


#define DEBUG false

#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>


#define LOG_ENTRY(e) {\
  printf("\nENTRY ASSOCIATED WITH KEY: '%s'\n ", e -> key);\
  for(size_t i = 0; i < e -> length; i++) {\
    printf("E%lu: %d ", i, ((e -> values)[i].value));\
  }\
  printf("\n");\
}

#define PRINT_ENTRY(e) {\
  printf("[");\
  for(size_t i = 0; i < e -> length - 1; i++) {\
    printf("%d ", (e -> values)[i].value);\
  }\
  printf("%d]\n", (e -> values)[e -> length - 1].value);\
}

#define LOG_VALS(v, n) {\
  printf("\nVALUES: \n ");\
  for(size_t i = 0; i < n; i++) {\
    printf("V%lu: %d ", i, v[i].value);\
  }\
  printf("\n");\
}

#define LOG_ARGS(args, l) {\
  printf("\nARGS: [");\
  for(size_t i = 0; i < l - 1; i++) {\
    printf("%s, ", args[i]);\
  }\
  printf("%s]\n", args[l - 1]);\
}

#define PRINT_KEYS(keys, nkeys) {\
  for(size_t i = 0; i < nkeys; i++) {\
    printf("%s\n", keys[i]);\
  }\
}

#define LOG_LL(head) {\
  printf("\nDB:");\
  size_t i = 0;\
  entry * tmp;\
  tmp = head;\
  while(tmp) {\
    printf("\n  ENTRY # %lu:\n  [", i++);\
    size_t j = 0;\
    size_t l = (tmp -> length) - 1;\
    for(; j < l; j++) {\
      printf("X%lu: %d, ", j, ((tmp -> values)[j]).value);\
    }\
    printf("X%lu: %d]", j, ((tmp -> values)[l]).value);\
    tmp = tmp -> next;\
  }\
  free(tmp);\
  printf("\n");\
}

enum item_type {
    INTEGER=0,
    ENTRY=1
};

typedef struct element element;
typedef struct entry entry;
typedef struct snapshot snapshot;
typedef struct kargs kargs;

struct element {
  enum item_type type;
  union {
    int value;
    struct entry *entry;
  };
};

struct entry {
  char key[MAX_KEY];
  bool isSimp;
  element * values;
  size_t length; // Number of elements in values
  size_t vSize; // Size of values in bytes

  entry* next;
  entry* prev;

  size_t forward_size;
  size_t forward_max;
  entry** forward;  // this entry depends on these

  size_t backward_size;
  size_t backward_max;
  entry** backward; // these entries depend on this
};

inline void destroy_entry(entry ** e) {
  /* !!!TODO: NEED TO FREE FORWARD AND BACKWARD!!! */
  free((*e) -> values);
  free(*e);
  *e = NULL;
}

inline void init_entry(entry ** e) {
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

inline void swap_values(entry ** e, const size_t i, const size_t j) {
  element * tmp;
  tmp = malloc(sizeof(struct element));
  *tmp = ((*e) -> values)[i];
  ((*e) -> values)[i] = ((*e) -> values)[j];
  ((*e) -> values)[j] = *tmp;
  free(tmp);
}

inline void shallow_copy_entry(entry ** e1, const entry * e2) {
  *e1 = malloc(sizeof(struct entry));
  **e1 = *e2;
}

// Deep copy entry by reference
inline void dcer(entry ** e1, const entry * e2) {
  /* !!!TODO: NEED TO DEEP COPY FORWARD AND BACKWARD !!!*/
  shallow_copy_entry(e1, e2);
  (*e1) -> values = malloc(sizeof(struct element) * e2 -> length);
  (*e1) -> values = e2 -> values;
}

// Deep copy entry by absolute reference
inline void dcar(entry ** e1, const entry * e2) {
  /* !!!TODO: NEED TO DEEP COPY FORWARD AND BACKWARD !!!*/
  shallow_copy_entry(e1, e2);
  (*e1) -> values = malloc(sizeof(struct element) * e2 -> length);
  memcpy((*e1) -> values, e2 -> values, sizeof(struct element) * e2 -> length);
}

struct snapshot {
  int id;
  entry* entries;
  snapshot* next;
  snapshot* prev;
};

void uppers(char * line) {
  size_t i = 0;
  while(line[i]) {
    line[i] = toupper(line[i]);
    i++;
  }
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

  if(ll <= 1) return NULL;

  char ** ret;
  ret = malloc(sizeof(char*) * (MAX_ARGS));
  for(size_t i = 0; i < MAX_ARGS; i++) {
    ret[i] = malloc(MAX_LINE);
  }
  size_t i = 0; size_t j = 0; size_t l;

  while(i + 1 < ll) {
    char * tmp = read_delim(line, &i, i);
    l = strlen(tmp);
    memcpy(ret[j], tmp, l);
    ret[j] = realloc(ret[j], l);
    j++;
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

inline bool is_key(const char * k) {
  return strlen(k) < MAX_KEY && !isdigit(k[0]);
}

inline bool is_num(const char * s) {
  for(size_t i = 0; i < strlen(s); i++) {
    if(!isdigit(s[i])) {
      return false;
    }
  }
  return true;
}

inline bool is_simple(char ** args, const size_t nargs, const size_t start) {
  for(size_t i = start; i < nargs; i++) {
    if(!is_num(args[i])) {
      return false;
    }
  }
  return true;
}

// TODO: PROPERLY GIVE CREDIT FOR DJB2 HASH IMPLEMENTATION //
// FULL CREDIT TO:
inline uint64_t djb2h(const char *str) {
  uint64_t hash = 5381;
  int32_t c;
  while ((c = *str++))
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return hash;
}

const char* HELP =
	"BYE   clear database and exit\n"
	"HELP  display this help message\n"
	"\n"
	"LIST KEYS       displays all keys in current state\n"
	"LIST ENTRIES    displays all entries in current state\n"
	"LIST SNAPSHOTS  displays all snapshots in the database\n"
	"\n"
	"GET <key>    displays entry values\n"
	"DEL <key>    deletes entry from current state\n"
	"PURGE <key>  deletes entry from current state and snapshots\n"
	"\n"
	"SET <key> <value ...>     sets entry values\n"
	"PUSH <key> <value ...>    pushes values to the front\n"
	"APPEND <key> <value ...>  appends values to the back\n"
	"\n"
	"PICK <key> <index>   displays value at index\n"
	"PLUCK <key> <index>  displays and removes value at index\n"
	"POP <key>            displays and removes the front value\n"
	"\n"
	"DROP <id>      deletes snapshot\n"
	"ROLLBACK <id>  restores to snapshot and deletes newer snapshots\n"
	"CHECKOUT <id>  replaces current state with a copy of snapshot\n"
	"SNAPSHOT       saves the current state as a snapshot\n"
	"\n"
	"MIN <key>  displays minimum value\n"
	"MAX <key>  displays maximum value\n"
	"SUM <key>  displays sum of values\n"
	"LEN <key>  displays number of values\n"
	"\n"
	"REV <key>   reverses order of values (simple entry only)\n"
	"UNIQ <key>  removes repeated adjacent values (simple entry only)\n"
	"SORT <key>  sorts values in ascending order (simple entry only)\n"
	"\n"
	"FORWARD <key> lists all the forward references of this key\n"
	"BACKWARD <key> lists all the backward references of this key\n"
	"TYPE <key> displays if the entry of this key is simple or general\n";

#endif
