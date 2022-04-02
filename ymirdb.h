#ifndef YMIRDB_H
#define YMIRDB_H

#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

// Debug mode
#define DEBUG false

// LIMS
#define MAX_KEY 0x10
#define MAX_LINE 0x400
#define MAX_ARGS (MAX_LINE >> 1) - 1
#define ENTRY_BUFF 0x0f // Num. of entries before realloc
#define KEY_BUFF 0x40 // Num. keys before realloc

// HASHED COMMAND VALUES
#define BYE 0xB87D765
#define HELP 0x17C85BA6E
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
#define DEL 0xB87DD5A
#define PURGE 0x310DD6AAE8
#define PICK 0x17C8A2D4C
#define PLUCK 0x310DD1C7C4
#define POP 0xB8811B4
#define LEN 0xB87FF64
#define FORWARD 0xD0A8CE70717A
#define BACKWARD 0x1AE59691FD3D44
#define TYPE 0x17C8CA487
#define DBSET 0x310CF31DB7
#define DBGET 0x310CF2EAAB
#define DBPUSH 0x652AB5573AB
#define DBAPPEND 0x1AE5AAB32786E3

// MSG'S:
#define FETCH_NONEXISTENT_KEY "no such key\n\n"
#define CONFIRMATION_MSG "ok\n\n"
#define COMMAND_NOT_FOUND "ERROR: cmd not found\n\n"
#define EMPTY_ENTRY "nil\n\n"
#define NO_KEY_LIST "no keys\n\n"
#define NO_KEY_ENTRIES "no entries\n\n"
#define INDEX_OUT_OF_RANGE "index out of range\n\n"
#define NO_SNAPSHOTS "no snapshots\n\n"

const char* HELP_MSG =
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

struct snapshot {
  int id;
  entry* entries;
  snapshot* next;
  snapshot* prev;
};

inline void uppers(char * line);
inline bool is_key(const char * k);
inline bool is_num(const char * s);
inline bool is_simple(char ** args, const size_t nargs, const size_t start);
// TODO: PROPERLY GIVE CREDIT FOR DJB2 HASH IMPLEMENTATION //
inline uint64_t djb2h(const char *str);
char * read_delim(const char line[], size_t * argc, const size_t start);
char ** read_args(const char line[], const size_t ll, size_t * nargs);
inline void dcer(entry ** e1, const entry * e2) __attribute__((unused));

#define LOG_ENTRY(e) {\
  printf("\nENTRY ASSOCIATED WITH KEY: '%s'\n ", e -> key);\
  for(size_t i = 0; i < e -> length; i++) {\
    printf("E%lu: %d ", i, ((e -> values)[i].value));\
  }\
  printf("\n");\
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

#define PRINT_ENTRY(e) {\
  if(e -> length <= 0) {\
    printf("[]\n");\
  } else {\
    printf("[");\
    for(size_t i = 0; i < e -> length - 1; i++) {\
      printf("%d ", (e -> values)[i].value);\
    }\
    printf("%d]\n", (e -> values)[e -> length - 1].value);\
  }\
}

#define PRINT_KEYS(tail) {\
  entry * tmp;\
  tmp = tail;\
  while(tmp) {\
    printf("%s\n", tmp -> key);\
    tmp = tmp -> prev;\
  }\
  printf("\n");\
}

#define PRINT_ENTRIES(tail) {\
  entry * tmp;\
  tmp = tail;\
  while(tmp) {\
    printf("%s ", tmp -> key);\
    PRINT_ENTRY(tmp);\
    tmp = tmp -> prev;\
  }\
  printf("\n");\
}

#define LOG_SNAP(shead) {\
  printf("\nENTIRE DB HISTORY:\n");\
  snapshot * tmp;\
  tmp = shead;\
  while(tmp) {\
    printf("  SNAPSHOT: %d\n", tmp -> id);\
    entry * tmpj;\
    tmpj = tmp -> entries;\
    while(tmpj) {\
      printf("    %s: ", tmpj -> key);\
      PRINT_ENTRY(tmpj);\
      tmpj = tmpj -> next;\
    }\
    tmp = tmp -> next;\
  }\
}

#endif
