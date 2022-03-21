#ifndef YMIRDB_H
#define YMIRDB_H

#define MAX_KEY 16
#define MAX_LINE 1024
#define MAX_ARGS (MAX_LINE >> 1) - 1
#define NUM_BUCKETS 256

// HASHED COMMAND VALUES
#define SET 0x31
#define GET 0x25
#define PUSH 0x65
#define APPEND 0x1D

#define DEBUG true

#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#define LOG_ENTRY(e) {\
  printf("\n");\
  for(size_t i = 0; i < e.length; i++) {\
    printf("E%lu: %d ", i, e.values[i].value);\
  }\
  printf("\n");\
}

#define LOG_ARGS(j, args) {\
  printf("\nARGS: [");\
  for(size_t i = 0; i < j - 1; i++) {\
    printf("%s, ", args[i]);\
  }\
  printf("%s]\n", args[j - 1]);\
}

enum item_type {
    INTEGER=0,
    ENTRY=1
};

typedef struct element element;
typedef struct entry entry;
typedef struct snapshot snapshot;

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
  size_t length;

  entry* next;
  entry* prev;

  size_t forward_size;
  size_t forward_max;
  entry** forward;  // this entry depends on these

  size_t backward_size;
  size_t backward_max;
  entry** backward; // these entries depend on this
};

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

char * read_delim(char line[], size_t * argc, size_t start) {

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
  free(tmp);
  return ret;
}

char ** read_args(char line[], size_t ll, size_t * nargs) {

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
  }
  char ** tmp = realloc(ret, j * sizeof(char*));
  if(!tmp) {
    // Catch Error
  }
  ret = tmp;

  #if DEBUG
    LOG_ARGS(j, ret);
  #endif
  *nargs = j - 1;
  return ret;
}

inline bool is_key(char * k) {
  return strlen(k) < MAX_KEY && !isdigit(k[0]);
}

inline bool is_simple(char ** args, size_t nargs) {
  for(size_t i = 0; i < nargs; i++) {
    if(is_key(args[i])) {
      return false;
    }
  }
  return true;
}

// TODO: PROPERLY GIVE CREDIT FOR DJB2 HASH IMPLEMENTATION //
// FULL CREDIT TO:
inline uint64_t djb2h(char *str) {
  uint64_t hash = 5381;
  int32_t c;
  while ((c = *str++))
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return hash % NUM_BUCKETS;
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
