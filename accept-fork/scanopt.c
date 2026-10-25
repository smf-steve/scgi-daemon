
// Name:  scanopt:  short for scan options

/*
Major difference is our versions popt/getopt..
  - we don't try to keep the return value as the shortName char
    - we return an INDEX value to a structure
  - we don't try to store info into a variable
    - we allow/require the user to put the process value there...
  - option types include: required, option, none (as usually)
  - argument types include:  text, int, long, float, & double
  - argument types include the ability to deal with duplicates
    - supports the http approach of -c & -C
  - we have alias/synonym
  - we have opposites:  e.g., --force, --inquire
  - we define default values
  - we incorporate Env Variable into the process

  - we internally normalize the argv structure different
*/


// PREMISE:  an option set via an env is just like setting on the cli
//          but this allows for CONFIG file to be used
//          but this also allows simpler control per user

///// NOTES Above

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <getopt.h>
#include <limits.h>
#include <assert.h>

#include <sys/types.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <alloca.h>
#include <stdbool.h>

#include "getopt.h"
#include "scanopt.h"


#define SO_OPTION_NAME(item) (item->longName != NULL ? item->longName : \
                             ((item->envName != NULL)? item->envName : item->shortName))


// The internal global variables
// These are reassertion of the getopt_long global variables, and
//   extensions to those global variables, following the same pattern

extern int    optreset;
extern int    optind;
extern char   *optarg;
opt_value_t   optvalue;   
int           optmax    = 0;   // max number of options and envs
int           optcount  = 0;   // the number of options/envs used
int           optondup  = SO_DUP_OVERWRITE;


static void assign_variable(void *variable, int type, char *val);

static void define_option(option_t *item, char *value, option_t option_list[]);

static void create_optstring(char *opt_string, int length, option_t option_list[]);
// generates the short form optstring as per `getopt_long`
//   e.g., "b:c::d:e:fhl:p:s:e:w:vV"
//
//   prepend ":" to suppress error messages
//   then have a rule to handle ":+"
//      ":" silent mode
//      "+" stop at first non-option argument

static void create_longopt_struct(struct option *long_opt, int length, option_t option_list[]);
// generates the struct option as per `getopt_long`

static void create_longopt_string(char *opt_string, int length, option_t option_list[]);
// generates the long form optstring as per `getopt` command line
//   e.g., "flag,required:,optional::, ...."


static void apply_env(option_t *item, option_t option_list[]);


///////////////////////////////////////////////

// Operational Functions
extern int scanopt_scan(int argc, char *argv[], option_t option_list[]) {
  int           isError = false;
  option_t      *item;

  // Variables associates with the use of `getopt_long`
  int           ch;
  char          *opt_string;
  struct option *long_opts;

  extern int    optreset;
  extern int    optind;
  extern char   *optarg;
  extern int    optmax;

  scanopt_validate(option_list);
  scanopt_initialize(option_list);

  // the following does not work.
  // opt_string     = alloca(sizeof(char) * optmax * 3);   // each option could be "x::"
  // long_opts      = alloca(sizeof(struct option)* optmax);
  opt_string     = malloc(sizeof(char) * optmax * 3);   // each option could be "x::"
  long_opts      = malloc(sizeof(struct option)* optmax);

  create_optstring(opt_string, optmax * 3, option_list);
  create_longopt_struct(long_opts, optmax + 1, option_list);

  opterr = 0;    // Disable messages
  optreset = 1;  // Ensure we start at the befinning
  while ((ch = getopt_long(argc, argv, opt_string, long_opts, NULL)) != -1) {
    // getopt_long return values:
    //   -1  : when argv list is exhausted
    //    0  : when a flag has been set... (will no happen in our case)
    // <256  : Short Option
    // >256  : Long Options
    if (ch <= 0xff) {   // ShortOp was provided by the user
      switch (ch) {
        // If there is an error, should we return: SO_ID_ERROR

        case ':':
          fprintf(stderr, "Missing argument for: %s\n", argv[optind-1]);
          continue;      // or we are done with the loop?
          //return optind;
          break;

        case '?':
          fprintf(stderr, "Unknown option: %s\n", argv[optind-1]);
          continue;     // or we are done with the loop?
          //return optind;
          break;

        default:
          item = scanopt_find_shortName(ch, option_list);
          ch = item->id;
      }
    } else {
      ch = ch >> 8;
      item = scanopt_find(ch, option_list);
    }
    //////////////////////////////////////////////

    assert(item->id != 0);
    item->count ++;

    //////////////////////////////////////////////
    // First use of the option
    if (item->count == 1) {
      optcount ++;
      define_option(item, optarg, option_list);
      continue;
    }

    ///////////////////////////////////////////////
    // Subsequent use of the option
    switch (SO_DUP_TYPE(item->type)) {
      case SO_DUP_ERROR:
        fprintf(stderr, "Invalid duplicate use of --%s\n", SO_OPTION_NAME(item));
        isError = true;
        break;

      case SO_DUP_OVERWRITE:
        define_option(item, optarg, option_list);
        break;

      case SO_DUP_IGNORE:
        // skipping/ignoring the duplicate
        fprintf(stderr, "Warning duplicate use of --%s\n", SO_OPTION_NAME(item));
        break;

      case SO_DUP_APPEND:
        item->valueArray[item->count] = optarg;
        assert(item->count < SO_DUP_MAX );
        break;
  
      default:
        assert(false);
    }
  }

  return optind;
}


extern int scanopt_next(option_t option_list[]) {
  static int current_index = 0;
  static int initialized = false;

  option_t *item;
  int      complement;

  if (initialized == false) {
    initialized = true;
    current_index = 0;
  }

    // Not sure of the utility of having a flag of what to return
    // do {
    //   current_index ++;
    //   switch (flag) {

    //     case SO_OPTIONS_NOVAR:
    //       if (item.variable != NULL);
    //         continue;
    //       // merge;

    //     case SO_OPTIONS_DEFINED:
    //       if (item.count == 0)
    //         continue;
    //       break;

    //     case SO_OPTIONS_ALL:
    //       break;
    //    }
    //    break;
    // } while (true);

  do {
    item = &(option_list[current_index]);
    current_index ++;

    if (item->id == 0)   return -1;

    apply_env(item, option_list);

    complement = UNSET;
    if (item->id < 0) {
      complement = SET;
      item = scanopt_find(-item->id, option_list);
    }

  } while (item->count == 0);

  optind = current_index;         // relative to the option_list NOT argv
  // optopt = item.id;            // does not make sense

  optarg = item->value;
  if (complement == SET) {
    if (item->value[0] == '0') {
      optarg = "1";
    } else {
      optarg = "0";
    }
  }

  if (item->variable != NULL) {
    assign_variable(item->variable, SO_VAL_TYPE(item->type), optarg);
  
    if (SO_VAL_TYPE(item->type) == SO_VAL_ARRAY) {
      (* (char ***) item->variable) = item->valueArray;
    }
  }

  switch (SO_VAL_TYPE(item->type)) {
    case SO_VAL_TEXT:   optvalue.sVal = optarg; break;

    case SO_VAL_SET:    //merge
    case SO_VAL_UNSET:  //merge
    case SO_VAL_INT:    optvalue.iVal = atoi(optarg); break;

    case SO_VAL_LONG:   optvalue.lVal = atol(optarg); break;

    case SO_VAL_FLOAT:  optvalue.fVal = (float) atof(optarg); break;

    case SO_VAL_DOUBLE: optvalue.dVal = atof(optarg); break;

    case SO_VAL_ARRAY:  optvalue.aVal = item->valueArray; break;
    default:
      assert(false);
  }

  return (complement == UNSET)? item->id : - item->id; 
}

  
extern void scanopt_usage_options(option_t option_list[], char *indent) {
  option_t item;
  char     *format;

  fprintf(stdout, "Options\n");
  for (int i = 0; option_list[i].id != 0; i++) {
    item = option_list[i];

    // must have at least a longName or shortName to be a proper option
    if (item.longName == NULL && item.shortName == NULL ) {
      continue;
    }

    fprintf(stdout, "%s", indent);

    if (item.format == NULL ) {
      format = "arg";
    } else {
      format = item.format;
    }

    // SHORT NAME
    if (item.shortName != NULL) {
      fprintf(stdout, "-%s", item.shortName);
 
      switch(SO_ARG_TYPE(item.type)) {
        case SO_ARG_OPTIONAL:
          // no space before the option
          fprintf(stdout, "[%s]", format);
          break;

         case SO_ARG_REQUIRED:
          fprintf(stdout, " %s", format);
          break;
      }
      if (item.longName != NULL) {
        fprintf(stdout, ", ");
      }
    }

    /// LONG NAME
    if (item.longName != NULL) {
      fprintf(stdout, "--%s", item.longName);
      switch(SO_ARG_TYPE(item.type)) {
        case SO_ARG_OPTIONAL:
          // no space before the option
          fprintf(stdout, "[=%s]", format);
          break;

        case SO_ARG_REQUIRED:
          fprintf(stdout, "=%s", format);
          break;
      }
    }
    fprintf(stdout, "\n");

    // DESCRIPTION
    if (item.description != NULL) {
      fprintf(stdout, "%s%s%s\n", indent, indent, item.description);
    }
    
    if (item.defaultValue != NULL) {
      fprintf(stdout, "\n%s%sDEFAULT: %s", indent, indent, item.defaultValue);
    }
    if (item.envName != NULL) {
      fprintf(stdout, "\n%s%sENVIRONMENT VARIABLE: %s\n", indent, indent, item.envName);
    }
    fprintf(stdout, "\n");
  }
  return;
}


extern void scanopt_usage_envs(option_t option_list[], char *indent) {
  option_t item;
  char     *format;

  fprintf(stdout, "Environment\n");

  for (int i = 0; option_list[i].id != 0; i++) {
    item = option_list[i];

    if (item.envName == NULL) continue;

    fprintf(stdout, "%s", indent);

    if (item.format == NULL ) {
      format = "arg";
    } else {
      format = item.format;
    }
    
    fprintf(stdout, "%s", item.envName);

    switch(SO_ARG_TYPE(item.type)) {
      case SO_ARG_NONE: break;
      case SO_ARG_OPTIONAL:
        // no space before the option
        fprintf(stdout, "=[%s]", format);
        break;

      case SO_ARG_REQUIRED:
        fprintf(stdout, "=%s", format);
        break;
    }
    fprintf(stdout, "\n");
    // Two main options:
    //   1. it is a covered by a command line option
    //   1. it is NOT covered by a command line option

    if (item.longName != NULL)
      fprintf(stdout, "%s%sSee --%s above.\n", indent, indent, item.longName);

    if (item.longName == NULL && item.shortName != NULL)
      fprintf(stdout, "%s%sSee --%s above.\n", indent, indent, item.shortName);

    if (item.longName == NULL && item.shortName == NULL) {
      if (item.description != NULL) {
        fprintf(stdout, "%s%s%s\n", indent, indent, item.description);
      }
      if (item.defaultValue != NULL) {
        fprintf(stdout, "%s%sDEFAULT: %s\n", indent, indent, item.defaultValue);
      }
      fprintf(stdout, "\n");
    }
  }
  return;
}


// Option Declaration Functions

extern option_t *scanopt_declare_option(option_t option_list[], int id, int argType, char *longName, char *shortName, char *envName, size_t size) {
  // Alternative way to define a option_list incrementally

  option_t *item;

  item = &option_list[optmax]; optmax ++;
  assert(optmax < size);
  assert(item -> id == 0);

  item->id        = id;
  item->type      = argType;
  item->shortName = shortName;
  item->longName  = longName;
  item->envName   = envName;

  return item;
};


extern option_t *scanopt_declare_complement(option_t option_list[], int id, int argType, char *longName, char *shortname, char *envName, size_t size) {
  assert(false);
}

extern option_t *scanopt_declare_env(option_t option_list[], int id, int argType, char *envName, void * variable, size_t size) {
  option_t *item;

  item = scanopt_declare_option(option_list, id, argType, NULL, NULL, envName, size);
  scanopt_assign_variable(item, &variable, argType, NULL, NULL);

  return item;
}


// set the global default for action to take on duplicate
extern void scanopt_set_ondup(int value) {

  extern int optondup;

  switch (value) {
    case SO_DUP_OVERWRITE:
    case SO_DUP_IGNORE:
    case SO_DUP_APPEND:
    case SO_DUP_ERROR:
       optondup = value;
    default:
      fprintf(stderr, "Attempt to assign an illegal value to optoverwrite\n");
      exit(1);
  }
}


// Option Definition Functions

extern void scanopt_assign_variable(option_t *item, void *variable, int type, char *initValue, char *defaultValue) {

  item->type = item->type | type;
  item->variable = variable;
  item->initValue = initValue;
  item->defaultValue = defaultValue;
  return;
}


extern void scanopt_assign_flag(option_t *item, void *variable) {
  scanopt_assign_variable(item, variable, SO_FLAG, "0", NULL);
  return;
}


extern void scanopt_assign_nflag(option_t *item, void *variable) {
  scanopt_assign_variable(item, variable, SO_NFLAG, "1", NULL);
  return;
}


extern void scanopt_assign_usage(option_t *item, char *format, char *description) {
  item->format = format;
  item->description = description;
  return;
}


// Access functions
extern option_t *scanopt_find(int id, option_t option_list[]) {
  int i;

  for(int i = 0; option_list[i].id != 0; i++ ) {
    if (option_list[i].id == id) 
      return &(option_list[i]);
  }

  return NULL;
}


extern option_t *scanopt_find_shortName(int ch, option_t option_list[]) {

  for(int i = 0; option_list[i].id != 0; i++ ) {
    if (option_list[i].shortName == NULL) continue;
    if (option_list[i].shortName[0] == ch) 
      return &(option_list[i]);
  }

  return NULL;
}


extern option_t *scanopt_find_envName(char *name, option_t option_list[]) {

  for(int i = 0; option_list[i].id != 0; i++ ) {
    if (option_list[i].envName == NULL) continue;
    if (strcmp(name, option_list[i].envName) == 0)
      return &(option_list[i]);
  }

  return NULL;
}



// Secondary API  (called within but could be used by...)
extern void scanopt_initialize(option_t option_list[]) {
  option_t item;
  extern int optmax;

  for (optmax = 0; option_list[optmax].id != 0; optmax++) {
    item = option_list[optmax];

    if (item.id < 0) continue;

    // Initialize working data!
    item.value = NULL;
    for(int i = 0; i < SO_DUP_MAX; i++) {
      if (item.valueArray[i] == NULL)  break;
      item.valueArray[i] = NULL;
    }
    item.count = 0;

    item.value = item.initValue;  // this is not really needed

    if (item.variable != NULL) {
      assign_variable(item.variable, SO_VAL_TYPE(item.type), item.initValue);
    }
  }
  return;
}


extern void scanopt_validate(option_t option_list[]) {
   // must have at least a longName, shortName, or envName
   // shortName is a single character
   // shortName does not use ':' nor '?'
   // check id < 0
   //   -- SO_COMPLEMENT:  SO_ARG_NONE with SO_VAL_SET or SO_VAL_UNSET
   //       - id must be the opposite

 
   // ARG_TYPE Constraints
   //  ARG_TYPE         VAL_TYPE      INITIAL        DEFAULT
   //  no_argument,     SET | UNSET,  initial value
   //  option_argument,           *,  initial value, default value
   //  required_value,            *,  initial value
 
   // NOT checked: if (id > 0), it is unique within the option_list
   // should we or should we not have an ENV


 option_t *item;
 option_t *root_item;
 int i;
 int isError = false;

 for(i = 0; option_list[i].id != 0; i++ ) {
    item = option_list+i;

    // NAMING CONSTRAINTS
    if (item->longName == NULL && 
        item->shortName == NULL &&
        item->envName == NULL) {
      fprintf(stderr, "Option #%d: Needs a name\n", item->id);
      isError = true;
    }
    if (item->longName != NULL && item->longName[0] == '\0') {
      fprintf(stderr, "Option #%d: Long name can't be \"\"\n", item->id);
      isError = true;
    }
    if (item->envName != NULL && item->envName[0] == '\0') {
      fprintf(stderr, "Option #%d: Environment name can't be \"\"\n", item->id);
      isError = true;
    }

    if (item->shortName != NULL) {
      switch (item->shortName[0]) {
        case '\0':
          fprintf(stderr, "Option #%d: Short name can't be \"\"\n", item->id);
          isError = true;
          break;

        case '?':
        case ':':
          fprintf(stderr, "Option #%d: Colon (:) nor Question (?) can not be used as an option\n", item->id);
          isError = true;
          break;

        default:
          if (item->shortName[1] != '\0') {
            fprintf(stderr, "Option #%d: Short name must be a single character\n", item->id);
            isError = true;
          }
          break;
      }
    }

    // COMPLEMENT CONSTRAINTS
    if (option_list[i].id < 0) {
      if (SO_ARG_TYPE(item->type) != SO_ARG_NONE) {
        fprintf(stderr,"Option \"%s\" must have no arguments\n", SO_OPTION_NAME(item));
        isError = true;
      }

      root_item = scanopt_find(- option_list[i].id, option_list);
      if (root_item->id == 0) {
        fprintf(stderr, "Option \"%s\": Does not have a complement\n", SO_OPTION_NAME(item));
        isError = true;
      }
      
      switch (SO_VAL_TYPE(item->type)) {
        case SO_VAL_SET:
          if (SO_VAL_TYPE(root_item->type) != SO_VAL_UNSET) {
            fprintf(stderr, "Option \"%s\": Improperly defined as a complement\n", SO_OPTION_NAME(root_item));
            isError = true;
          }
          if (root_item->initValue == NULL) {
            fprintf(stderr, "Option \"%s\": Must be assigned an initial value\n", SO_OPTION_NAME(root_item));
            isError = true;
          }
          break;

        case SO_VAL_UNSET:
          if (SO_VAL_TYPE(root_item->type) != SO_VAL_SET) {
            fprintf(stderr, "Option \"%s\": Improperly defined as a complement\n", SO_OPTION_NAME(root_item));
            isError = true;
          }
          if (root_item->initValue == NULL) {
            fprintf(stderr, "Option \"%s\": Must be assigned an initial value\n", SO_OPTION_NAME(root_item));
            isError = true;
          }          
          break;

        default:
          fprintf(stderr, "Option \"%s\": Improperly defined complement\n", SO_OPTION_NAME(item));
          isError = true;
          break;
      }

      if (SO_DUP_TYPE(root_item->type) != SO_DUP_OVERWRITE) {
        fprintf(stderr, "Option \"%s\": Must use \"overwrite\" mode\n", SO_OPTION_NAME(root_item));
        isError = true;
      }
    }
    // END OF COMPLEMENT CONSTRAINTS

    // ARG_TYPE CONSTRAINTS
    switch (SO_ARG_TYPE(item->type)) {
      case SO_ARG_REQUIRED:      
        if (item -> defaultValue != NULL) {
          fprintf(stderr, "Error: superfluous default value assigned to: %s\n", SO_OPTION_NAME(item));
          isError = true;
        }
        break;
      case SO_ARG_OPTIONAL:
        break;

      case SO_ARG_NONE:
        if (item -> defaultValue != NULL) {
          fprintf(stderr, "Error: superfluous default value assigned to: %s\n", SO_OPTION_NAME(item));
          isError = true;
        }
        switch (SO_VAL_TYPE(item->type)) {
          case SO_VAL_NULL:  // merge
          case SO_VAL_SET:   // merge
          case SO_VAL_UNSET: break;
          default:
            fprintf(stderr, "Illegal value type assigned to an option with no arguments: %s\n", SO_OPTION_NAME(item));
            isError = true;
        }
        break;
    }
    // END OF ARG_TYPE CONSTAINTS
  }
  if (isError == true ) {
    exit(1);
  }
  return;
}


extern void scanopt_apply_environment(option_t option_list[]) {
  option_t *item;

  for (int i = 0; option_list[i].id != 0; i++) {
    item = option_list+i;

    if (item->id < 0) continue;
    if (item->count > 0) continue;

    apply_env(item, option_list);

  }
}


///////////////////////////////////////////////////////


static void assign_variable(void *variable, int type, char *val) {

  switch (SO_VAL_TYPE(type)) {
    case SO_VAL_TEXT:   (* (char **) variable) = val; break;

    case SO_VAL_SET:    //merge
    case SO_VAL_UNSET:  //merge
    case SO_VAL_INT:    (* (int *) variable) = atoi(val); break;

    case SO_VAL_LONG:   (* (int *) variable) = atol(val); break;

    case SO_VAL_FLOAT:  (* (float *) variable) = (float) atof(val); break;

    case SO_VAL_DOUBLE: (* (double *) variable) = atof(val); break;

    //case SO_VAL_ARRAY:  (* (char ***) variable) = ???; break;

    default:
      assert(false);
  }
  return;
}


static void apply_env(option_t *item, option_t option_list[]) {

  assert(! (item->id < 0) );  // What do we do for complements

  if (item->envName != NULL) {
    char *value = getenv(item->envName);
    if (value != NULL) {
      item->count++;
      optcount ++;
      define_option(item, value, option_list);
    }
  }
}


static void define_option(option_t *item, char *value, option_t option_list[]) {
  // Update the 'value' base up
  //   case
  //      required_multiple: copy value    to item->value
  //      required:          copy value    to item->value
  //      optional_present:  copy value    to item->value
  //      optional_missing:  copy default  to item->value
  //      none:              nothing       but address SET/UNSET

  switch(SO_ARG_TYPE(item->type)) {
    case SO_ARG_NONE:
      {
         if (item->id < 0) {
          item = scanopt_find(-(item->id), option_list);
         }
         switch (SO_VAL_TYPE(item->type)) {
           case SO_VAL_SET:    item->value = "1";   break;
           case SO_VAL_UNSET:  item->value = "0";   break;
           default:            break;
         }
      }
      break;

    case SO_ARG_OPTIONAL:
      if (value == NULL)
        item->value = item->defaultValue;
      else
        item->value = value;
      break;

    case SO_ARG_REQUIRED: 
      item->value = value;
      break;
      
    default:
      assert(false);
      break;
  }

  if (SO_DUP_TYPE(item->type) == SO_DUP_APPEND) {
    item->valueArray[0] = value;
  }
  return;
}


static void create_optstring(char *opt_string, int length, option_t option_list[]) {
  int x; // index into opt_string
  int i; // index into option_list

  // generates the `ptstring` as per the `getopt` utility
  // which is identical to the `getopt_long` C API
  // e.g.,  "b:c::d:e:fhl:p:s:e:w:vV"

  opt_string[0] = '+';  // halt on first non-option argument
  opt_string[1] = ':';  // suppress error messages
  x = 2;

  for (i = 0; option_list[i].id != 0; i++) {
    if (option_list[i].shortName != NULL) {
      opt_string[x] = option_list[i].shortName[0]; x++;
      switch (SO_ARG_TYPE(option_list[i].type)) {
        case SO_ARG_NONE:  break;
        case SO_ARG_OPTIONAL:
          opt_string[x] = ':'; x++;
          //merge;
        case SO_ARG_REQUIRED:
          opt_string[x] = ':'; x++;
      }
    }
  }
  assert(i <= length);
  return;
}

static void create_longopt_string(char *long_string, int length, option_t option_list[]) {
  // generates the string as per the `get_opt` utility
  // at this time, this function is not needed
  assert(false);
}

static void create_longopt_struct(struct option *long_opt, int length, option_t option_list[]) {
  option_t *item;
  int i = 0;

  // generates the 'struct option' as per the
  // `getopt_long` C API

  for (i = 0; option_list[i].id != 0; i++) {
    item = &(option_list[i]);
    long_opt[i].name    = item->longName;
    long_opt[i].has_arg = SO_ARG_TYPE(item->type);
    long_opt[i].flag    = NULL;
    long_opt[i].val     = item->id << 8;
   }
   long_opt[i].name     = 0;
   long_opt[i].has_arg  = 0;
   long_opt[i].flag     = NULL;
   long_opt[i].val      = 0;

   assert(i < length);
 return;
}
