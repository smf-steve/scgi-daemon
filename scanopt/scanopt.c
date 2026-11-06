// Name: scanopt.c
// Synopsis: A library for 'scan'ning command-line 'opt'ion
//
// Brief Description:
//   - an object based implementation to provide similar 
//     functionality as getopt_long
//   - provides short options, long options, and environment variables
//   - provides extend type of options: synonyms and complements
//   - provides value support for: flags, nflags, and counting values
//   - provides management of '--help' usage type information

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <getopt.h>
#include "scanopt.h"

// External Global Variables: these are
//   - a reassertion of such variables
//   - an extension of such variables
// from getopt_long

extern char        *optarg;
extern int         optind;     // ineffective
extern int         optopt;     // ineffective
extern int         optreset;  

extern char        *optname;
extern int         optmax;
extern opt_value_t optval;

char              *optname;
extern char       *optarg;
int               optcount = 0;
opt_value_t       optval;


#define SO_DEFINE_NAME(names) ((names->longName != NULL) ? names->longName : \
                              ((names->envName  != NULL)? names->envName : names->shortName))

// scanopt_names_t *names;
//
// The type field is a bit-field that determines
//   1. If the option is a basic option, a synonym, or a complement
//   1. A basic option is one of: non, required, optional
//
//    type =  00 00 00 xx  : SO_TYPE_BASIC:  xx is the ARG_TYPE
//    type =  xx 00 00 00  : !SO_TYPE_BASIC: xx determines syn or comp
//    type =  00 xx xx xx  : !SO_TYPE_BASIC: xx xx xx is an option id

#define SO_OPTION_NAME(item)  (item->name)
#define SO_OPTION_ROOT(item)  (item->names->type & 0x00FFFFFF)
#define SO_OPTION_TYPE(item)  (item->names->type & (0xFF << 24))
#define SO_OPTION_ARG(item)   (item->names->type & 0xFF)

#define SO_NAME(item, tag)    (item->names->tag)

// scanopt_variables_t *variable;
#define SO_VARIABLE_TYPE(item) ((item->variable == NULL)? SO_VAL_NULL : (SO_VAL_TYPE(item->variable->type)))
#define SO_VARIABLE_DUP(item)  ((item->variable == NULL)? SO_DUP_DEFAULT : (SO_DUP_TYPE(item->variable->type)))
#define SO_INIT_VAL(item)      ((item->variable == NULL)? NULL : (item->variable->initValue))
#define SO_DEFAULT_VAL(item)   ((item->variable == NULL)? NULL : (item->variable->defaultValue))

#define SO_IS_OPTION_ROOT(item)     (SO_OPTION_TYPE(item) == SO_TYPE_ROOT)
#define SO_IS_OPTION_SYN(item)      (SO_OPTION_TYPE(item) == SO_TYPE_SYN)
#define SO_IS_OPTION_COMP(item)     (SO_OPTION_TYPE(item) == SO_TYPE_COMP)

// definitions associated the variable->type
#define SO_VAL_TYPE(type)  (type & 0x00f0)
#define SO_VAL_MASK        (0xff0f)

#define SO_DUP_TYPE(type)  (type & 0x0f00)
#define SO_DUP_MASK        (0xf0ff)

#define SO_IS_DUP_APPEND(item) (SO_VARIABLE_DUP(item) == SO_DUP_APPEND)


typedef struct {
  int      id;              // return value from scanopt_next
  char    *name;
  scanopt_names_t *names;

  // Usage (--help) Information
  scanopt_usage_t *usage;

  // Variable Definition 
  scanopt_variable_t *variable;

  // Working data:
  opt_value_t optval;                   // The optarg converted to the appropriate type
  char        *optarg;                 // The actual text string provided on the command line, subject to overwriting
  char        *argArray[SO_DUP_MAX];   // argv-style array of values
  int         count;                   // command-line reference count
} option_t;


struct scanopt_instance_t {
  opt_value_t   value;   
  int           max;
  int           count;
  int           onDup;
  option_t      options[];
};


// Support Functions
// Find and search functions
static option_t *find_id(int id, option_t options[]);
static option_t *find_shortName(int ch, option_t options[]);
static scanopt_usage_t    *search_usage(int id, scanopt_usage_t usage[]);
static scanopt_variable_t *search_variable(int id, scanopt_variable_t variable[]);


static void assign_variable(option_t *item, void *address);

static void define_option(option_t *item, int complement, char *value);

static void create_optstring(char *opt_string, int length, option_t options[]);
// generates the short form optstring as per `getopt_long`
//   e.g., "b:c::d:e:fhl:p:s:e:w:vV"
//
//   prepend ":" to suppress error messages
//   then have a rule to handle ":+"
//      ":" silent mode
//      "+" stop at first non-option argument

static void create_longopt_struct(struct option *long_opt, int length, option_t options[]);
// generates the struct option as per `getopt_long`

static void create_longopt_string(char *opt_string, int length, option_t options[]);
// generates the long form optstring as per `getopt` command line
//   e.g., "flag,required:,optional::, ...."


static void apply_env(option_t *item);


///////////////////////////////////////////////

extern scanopt_t scanopt_create(int size) {
  size_t  space;

  space = sizeof(struct scanopt_instance_t) + sizeof(option_t)*(size + 1);
  scanopt_t instance = calloc(space, sizeof(char));

  // instance->optvalue.sVal = NULL;
  // upon scanopt_next, should the optvalue structure be filled with all of the 
  // formally global variables associated with getopt_long
  //  if so then within main.c:   options.optarg, options. optval, etc. are defined

  instance->max  = size;
  instance->count = 0;
  instance->onDup = SO_DUP_OVERWRITE;

  // The rest do not need to be initialized, since `calloc`
  // was used to allocate the spaces--all are zeros
  // for (int i = 0; i <= size; i++) {
  //   instance -> options[i].id = 0;
  // }

  return instance;
}

extern void scanopt_destroy(scanopt_t instance) {
   free(instance);
}

extern scanopt_t scanopt_new(scanopt_names_t names[], scanopt_usage_t usage[], scanopt_variable_t variable[]) {
  scanopt_t instance;
  option_t *options;
  option_t *item;

  int i;

  for (i = 0; names[i].id > 0; i++);
  instance = scanopt_create(i);

  assert(i == instance->max);

  options = instance->options;

  for (i = 0; names[i].id != 0; i++) {
    item = options+i; 
    
    item->id       = names[i].id; 
    item->names    = names+i;
    item->name     = SO_DEFINE_NAME(item->names);

    item->usage    = search_usage(item->id, usage);
    item->variable = search_variable(item->id, variable);

    // All is set to zero, so the following is not needed.
    item->optarg = NULL;
    for (int i = 0; i <= SO_DUP_MAX; i++) {
      if (item->argArray[i] == NULL)  break;
      item->argArray[i] = NULL;
    }
    item->count = 0;
    item->optarg = SO_INIT_VAL(item);
  }
  return instance;
}



// Operational Functions
extern int scanopt_scan(scanopt_t instance, int argc, char *argv[]) {
  option_t      *options = instance->options;
  option_t      *item;

  // does optmax need to be moved to an extern to 
  // marry up with getopt_long?

  int           optmax = instance->max;

  int           isError = false;

  // onDup should be set to the instance default?
  // then reset by individual options?
  int           onDup;
  int           complement;

  extern int    optreset;
  extern int    optind;
  extern char   *optarg;

  // Variables associates with the use of `getopt_long`
  int           ch;
  char          opt_string[(optmax+1) * 3];
  struct option long_opts[optmax + 1];

  create_optstring(opt_string, (optmax+1) * 3, options);
  create_longopt_struct(long_opts, optmax + 1, options);

  opterr = 0;    // Disable messages
  optreset = 1;  // Ensure we start at the befinning
  while ((ch = getopt_long(argc, argv, opt_string, long_opts, NULL)) != -1) {
    // getopt_long return values:
    //   -1  : when argv list is exhausted
    //    0  : when a flag has been set... (will no happen in our case)
    // <256  : Short Option
    // >=256 : Long Options
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
          item = find_shortName(ch, options);
          ch = item->id;
      }
    } else {
      ch = ch >> 8;
      item = find_id(ch, options);
    }
    //////////////////////////////////////////////

    assert(item->id != 0);
    assert(!SO_IS_OPTION_SYN(item));

    complement = false;
    if (SO_IS_OPTION_COMP(item)) {
      item = find_id(SO_OPTION_ROOT(item), options);
      complement = true;
    }
    item->count++;

    //////////////////////////////////////////////
    // First use of the option
    if (item->count == 1) {
      instance->count++;
      define_option(item, complement, optarg);
      continue;
    }

    ///////////////////////////////////////////////
    // Subsequent use of the option

    onDup = SO_VARIABLE_DUP(item);
    if (onDup == SO_DUP_DEFAULT) onDup = instance->onDup;
    switch (onDup) {
      case SO_DUP_ERROR:
        fprintf(stderr, "Invalid duplicate use of --%s\n", SO_OPTION_NAME(item));
        isError = true;
        break;

      case SO_DUP_OVERWRITE:
        define_option(item, complement, optarg);
        break;

      case SO_DUP_IGNORE:
        // skipping/ignoring the duplicate
        fprintf(stderr, "Warning duplicate use of --%s\n", SO_OPTION_NAME(item));
        break;

      case SO_DUP_APPEND:
        item->argArray[item->count] = optarg;
        assert(item->count < SO_DUP_MAX );
        break;
  
      default:
        assert(false);
    }
  }

  return optind;
}


extern int scanopt_next(scanopt_t instance) {
  option_t *options = instance->options;
  int      optmax = instance->max;

  option_t *item;
  int      complement;

  static int current_index = 0;
  static int initialized = false;

  if (initialized == false) {
    initialized = true;
    current_index = 0;
  }

  // scanopt_next can return 
  // 1. all, but no utility
  //    if a syn, then return the root info, but with the compid
  //    if a comp, then return the root info, but with compid
  // 1. only those options that have been applied
  //    (here only the root options are relevant)
  //    but if we return only the root and they have setup 
  //    their arm to be only the nonroot... they will never see it
  //    - but the argument is that the syn and complements are secondary
  // 1. only those options that have been applied
  //    -- and DON'T have an associated variable
  //


    // Not sure of the utility of having a flag of what to return
    // do {
    //   current_index++;
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
    item = options+current_index; current_index++;

    if (item->id == 0) return -1;

    // Information is stored on the root
    // So never return anything that is NOT associated with the root
    if (SO_IS_OPTION_SYN(item)) continue;
    if (SO_IS_OPTION_COMP(item)) continue;

    if (item->count == 0) apply_env(item);
      // Should this be moved up before the continues

  } while (item->count == 0);



  if (item->variable == NULL) {
    // default as a regular text based argument.
    item->optval.sVal = item->optarg;
  } else {
    assign_variable(item, (void *) item->variable->address);
    assign_variable(item, (void *) &(item->optval));
  }

  optname  = SO_OPTION_NAME(item);
  optarg   = item->optarg;
  optval   = item->optval;
  optcount = item->count;
  
  return item->id; 
}

  
extern void scanopt_usage_options(scanopt_t instance, char *indent) {
  option_t *options = instance->options;
  int      optmax   = instance->max;
  option_t *item;
  char     *format;

  scanopt_names_t *names;
  scanopt_usage_t *usage;
  scanopt_variable_t *variable;

  fprintf(stdout, "Options\n");

  for (int i = 0; i < optmax; i++) {
    names = options[i].names;
    usage = options[i].usage;
    variable = options[i].variable;

    // must have at least a longName or shortName to be a proper option
    if (names->longName == NULL && names->shortName == NULL ) {
      continue;
    }

    fprintf(stdout, "%s", indent);

    format = "arg";
    if (usage != NULL && usage->format != NULL)  format = usage->format;


    // SHORT NAME
    if (names->shortName != NULL) {
      fprintf(stdout, "-%s", names->shortName);
 
      switch(names->type) {
        case SO_ARG_OPTIONAL:
          // no space before the option
          fprintf(stdout, "[%s]", format);
          break;

         case SO_ARG_REQUIRED:
          fprintf(stdout, " %s", format);
          break;
      }
      if (names->longName != NULL) {
        fprintf(stdout, ", ");
      }
    }

    /// LONG NAME
    if (names->longName != NULL) {
      fprintf(stdout, "--%s", names->longName);
      switch(names->type) {
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
    if (usage != NULL && usage->text != NULL) {
      fprintf(stdout, "%s%s%s\n", indent, indent, usage->text);
    }
    
    if (variable != NULL && variable->defaultValue != NULL) {
      fprintf(stdout, "\n%s%sDEFAULT: %s", indent, indent, variable->defaultValue);
    }
    if (names->envName != NULL) {
      fprintf(stdout, "\n%s%sENVIRONMENT VARIABLE: %s\n", indent, indent, names->envName);
    }
    fprintf(stdout, "\n");
  }
  return;
}


extern void scanopt_usage_envs(scanopt_t instance, char *indent) {
  option_t *options = instance->options;
  int      optmax   = instance->max;
  option_t *item;

  scanopt_names_t *names;
  scanopt_usage_t *usage;
  scanopt_variable_t *variable;

  char     *format;


  fprintf(stdout, "Environment\n");

  for (int i = 0; i < optmax; i++) {
    names = options[i].names;
    usage = options[i].usage;
    variable = options[i].variable;

    if (names->envName == NULL) continue;

    fprintf(stdout, "%s", indent);

    format = "arg";
    if (usage != NULL && usage->format != NULL) format = usage->format;
    
    fprintf(stdout, "%s", names->envName);

    switch(names->type) {
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

    if (names->longName != NULL)
      fprintf(stdout, "%s%sSee --%s above.\n", indent, indent, names->longName);

    if (names->longName == NULL && names->shortName != NULL)
      fprintf(stdout, "%s%sSee --%s above.\n", indent, indent, names->shortName);

    if (names->longName == NULL && names->shortName == NULL) {
      if (usage->text != NULL) {
        fprintf(stdout, "%s%s%s\n", indent, indent, usage->text);
      }
      if (variable != NULL && variable->defaultValue != NULL) {
        fprintf(stdout, "%s%sDEFAULT: %s\n", indent, indent, variable->defaultValue);
      }
    }
  }
  return;
}



// set the global default for action to take on duplicate
extern void scanopt_set_ondup(scanopt_t instance, int value) {


  switch (value) {
    case SO_DUP_OVERWRITE:
    case SO_DUP_IGNORE:
    case SO_DUP_ERROR:
    case SO_DUP_APPEND:
      instance->onDup = value;
      break;

    default:
      fprintf(stderr, "Attempt to assign an illegal value to optoverwrite\n");
      exit(1);
  }
}



extern void scanopt_validate(scanopt_t instance) {
   // must have at least a longName, shortName, or envName
   // shortName is a single character
   // shortName does not use ':' nor '?'
   // check id < 0
   //   -- SO_COMPLEMENT:  SO_ARG_NONE with SO_VAL_SET or SO_VAL_UNSET
   //       - id must be the opposite

// check to see that SO_COMP() is present in the table
// check to see that SO_SYN() is present in the table
// names that are the null string

// SYN and COMP that have multiple envNames,
// the order matters, so give warning if
// more than one is provided (and are different).

// args to variables->type is 0,1,2, or 0xFF...
//an initial value of 1 if SET
//an initial value of 0 if unset

// SO_SYN only valid in names
// SO_COMP only valid in variables

   // ARG_TYPE Constraints
   //  ARG_TYPE         VAL_TYPE      INITIAL        DEFAULT
   //  no_argument,     SET | UNSET,  initial value
   //  option_argument,           *,  initial value, default value
   //  required_value,            *,  initial value
  
   // NOT checked: if (id > 0), it is unique within the options
   // should we or should we not have an ENV

 option_t  *options = instance->options;
 int       optmax   = instance->max;

 option_t  *item;
 option_t  *root_item;

 scanopt_names_t *names;
 scanopt_variable_t *variable;

 int isError = false;

 for (int i = 0; options[i].id != 0; i++ ) {
    item = options+i;

    names = item->names;
    variable = item->variable;

    // NAMING CONSTRAINTS
    if (names->longName == NULL && 
        names->shortName == NULL &&
        names->envName == NULL) {
      fprintf(stderr, "Option #%d: Needs a name\n", item->id);
      isError = true;
    }
    if (names->longName != NULL && names->longName[0] == '\0') {
      fprintf(stderr, "Option #%d: Long name can't be \"\"\n", item->id);
      isError = true;
    }
    if (names->envName != NULL &&  names->envName[0] == '\0') {
      fprintf(stderr, "Option #%d: Environment name can't be \"\"\n", item->id);
      isError = true;
    }

    if (names->shortName != NULL) {
      switch (names->shortName[0]) {
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
          if (names->shortName[1] != '\0') {
            fprintf(stderr, "Option #%d: Short name must be a single character\n", item->id);
            isError = true;
          }
          break;
      }
    }

   
    // COMPLEMENT CONSTRAINTS
    if (item->id < 0) {
      if (names->type != SO_ARG_NONE) {
        fprintf(stderr,"Option \"%s\" must have no arguments\n", SO_OPTION_NAME(item));
        isError = true;
      }

      root_item = find_id(- options[i].id, options);
      if (root_item->id == 0) {
        fprintf(stderr, "Option \"%s\": Does not have a complement\n", SO_OPTION_NAME(item));
        isError = true;
      }
      
      switch (SO_VARIABLE_TYPE(item)) {
        // perhaps the complement must NOT HAVE any data defined
        case SO_VAL_SET:
          if (SO_VARIABLE_TYPE(root_item) != SO_VAL_UNSET) {
            fprintf(stderr, "Option \"%s\": Improperly defined as a complement\n", SO_OPTION_NAME(root_item));
            isError = true;
          }
          if (SO_DEFAULT_VAL(root_item) == NULL) {
            fprintf(stderr, "Option \"%s\": Must be assigned an initial value\n", SO_OPTION_NAME(root_item));
            isError = true;
          }
          break;

        case SO_VAL_UNSET:
          if (SO_VARIABLE_TYPE(root_item) != SO_VAL_SET) {
            fprintf(stderr, "Option \"%s\": Improperly defined as a complement\n", SO_OPTION_NAME(root_item));
            isError = true;
          }
          if (SO_INIT_VAL(root_item) == NULL) {
            fprintf(stderr, "Option \"%s\": Must be assigned an initial value\n", SO_OPTION_NAME(root_item));
            isError = true;
          }          
          break;

        default:
          fprintf(stderr, "Option \"%s\": Improperly defined complement\n", SO_OPTION_NAME(item));
          isError = true;
          break;
      }

      if (SO_VARIABLE_DUP(root_item) != SO_DUP_OVERWRITE) {
        fprintf(stderr, "Option \"%s\": Must use \"overwrite\" mode\n", SO_OPTION_NAME(root_item));
        isError = true;
      }
    }
    // END OF COMPLEMENT CONSTRAINTS

    // ARG_TYPE CONSTRAINTS
    switch (SO_OPTION_ARG(item)) {
      case SO_ARG_REQUIRED:      
        if (SO_DEFAULT_VAL(item) != NULL) {
          fprintf(stderr, "Error: superfluous default value assigned to: %s\n", SO_OPTION_NAME(item));
          isError = true;
        }
        break;
      case SO_ARG_OPTIONAL:
        break;

      case SO_ARG_NONE:
        if (SO_DEFAULT_VAL(item) != NULL) {
          fprintf(stderr, "Error: superfluous default value assigned to: %s\n", SO_OPTION_NAME(item));
          isError = true;
        }
        switch (SO_VARIABLE_TYPE(item)) {
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


extern void scanopt_apply_environment(scanopt_t instance) {
  option_t *options = instance->options;
  option_t *item;

  for (int i = 0; options[i].id != 0; i++) {
    item = options+i;

    if (item->count == 0) apply_env(item);
  }
}



// Access functions

static scanopt_usage_t *search_usage(int id, scanopt_usage_t usage[]) {

  for (int i = 0; usage[i].id != 0; i++) {
    if (usage[i].id == id) return &(usage[i]);
  }
  return NULL;
}


static scanopt_variable_t *search_variable(int id, scanopt_variable_t variable[]) {

  for (int i = 0; variable[i].id != 0; i++) {
    if (variable[i].id == id) return &(variable[i]);
  }
  return NULL;
}


static option_t *find_id(int id, option_t options[]) {

  for (int i = 0; options[i].id != 0; i++ ) {
    if (options[i].id == id) return &(options[i]);
  }
  return NULL;
}


static option_t *find_shortName(int ch, option_t options[]) {
  option_t        *item;
  scanopt_names_t *names;

  for (int i = 0; options[i].names != 0; i++ ) {
    item = options+i;
    names = item->names;

    if (names->shortName == NULL)  continue;
    if (names->shortName[0] == ch) {
      item  = options+i;
      if (SO_IS_OPTION_SYN(item)) {
        item = find_id(SO_OPTION_ROOT(item), options);
      }
      return item;
    }
  }
  return NULL;
}




///////////////////////////////////////////////////////
// STATIC Support Functions
///////////////////////////////////////////////////////

// Defines item->optarg = value;
static void define_option(option_t *item, int complement, char *value) {
  // Update the 'value' base upon
  //   case
  //      required_multiple: copy value    to item->optarg
  //      required:          copy value    to item->optarg
  //      optional_present:  copy value    to item->optarg
  //      optional_missing:  copy default  to item->optarg
  //      none:              nothing       but address SET/UNSET

  switch (SO_OPTION_ARG(item)) {
    case SO_ARG_NONE:
      switch (SO_VARIABLE_TYPE(item)) { 
        case SO_VAL_SET:
          item->optarg      = (complement == false) ? "1" : "0";
          item->optval.iVal = (complement == false) ?  1  :  0 ;
          break;

        case SO_VAL_UNSET:
          item->optarg      = (complement == false) ? "0" : "1";
          item->optval.iVal = (complement == false) ?  0  :  1 ;
          break;

        case SO_VAL_COUNT:
          item->optval.iVal = atoi(item->optarg) + 1;
          break;

        default: 
          break;
      }
      break;

    case SO_ARG_OPTIONAL:
      if (value == NULL) {
        value = SO_DEFAULT_VAL(item);
      }
      // merge;

     case SO_ARG_REQUIRED:
      if ((SO_VARIABLE_TYPE(item) == SO_VAL_COUNT)) {
        item->optval.iVal = item->optval.iVal + atoi(value);
      } 
      item->optarg = value;
      break;
      
    default:
      assert(false);
      break;
  }

  // this needs to be moved, it is for only first
  // this function was re-factored not to be ONLY first
  if (SO_IS_DUP_APPEND(item)) {
    item->argArray[0] = value;
  }
  return;
}

// Defines item->optarg = ENV, 
static void apply_env(option_t *item) {

  char * envName;
  int complement = false;

  // need to determine what to do with a complement env
  // either: a complement item can't have a env, ignore or error
  // or: we apply, and the last one wins

  envName = SO_NAME(item, envName);

  if (envName != NULL) {
    char *value = getenv(envName);
    if (value != NULL) {
      item->count++;
      define_option(item, complement, value);
    }
  }
}

static void assign_variable(option_t *item, void *address) {

  int  type;
  char *val = item->optarg;

  assert(item->variable != NULL);

  switch (SO_VARIABLE_TYPE(item)) {
    case SO_VAL_TEXT:   (* (char **) address) = (char *) val; break;

    case SO_VAL_SET:
    case SO_VAL_UNSET:
    case SO_VAL_COUNT:  (* (int *) address) = item->optval.iVal; break;

    case SO_VAL_INT:    (* (int *) address) = atoi((char *) val); break;

    case SO_VAL_LONG:   (* (int *) address) = atol((char *) val); break;

    case SO_VAL_FLOAT:  (* (float *) address) = (float) atof((char *) val); break;

    case SO_VAL_DOUBLE: (* (double *) address) = atof((char *)val); break;

    case SO_VAL_ARRAY:  (* (char ***) address) = item->argArray; break;

    default:
      assert(false);
  }
  return;
}




static void create_optstring(char *opt_string, int length, option_t options[]) {
  int x; // index into opt_string
  int i; // index into options
  scanopt_names_t *names;

  // generates the `optstring` as per the `getopt` utility
  // which is identical to the `getopt_long` C API
  // e.g.,  "b:c::d:e:fhl:p:s:e:w:vV"

  opt_string[0] = '+';  // halt on first non-option argument
  opt_string[1] = ':';  // suppress error messages
  x = 2;

  for (i = 0; options[i].id != 0; i++) {
    names = options[i].names;

    if (names->shortName != NULL) {
      opt_string[x] = names->shortName[0]; x++;
      switch (names->type) {
        case SO_ARG_NONE:  break;
        case SO_ARG_OPTIONAL:
          opt_string[x] = ':'; x++;
          //merge;
        case SO_ARG_REQUIRED:
          opt_string[x] = ':'; x++;
      }
    }
  }
  opt_string[x] = '\0';
  return;
}

static void create_longopt_struct(struct option *long_opt, int length, option_t options[]) {
  scanopt_names_t *names;
  option_t        *item;

  int x; // index into long_opt
  int i; // index into options

  int id;
  int type;

  // generates the 'struct option' as per the `getopt_long` C API
  x = 0;
  for (i = 0; options[i].id != 0; i++) {
    item  = options+i;
    names = options[i].names;

    if (names->longName == NULL) continue;

    id   = names->id;
    type = names->type;

    if (SO_IS_OPTION_SYN(item)) {    
      id = SO_OPTION_ROOT(item);
      item = find_id(id, options);
      type = SO_OPTION_ARG(item);
    }

    long_opt[x].name    = names->longName;
    long_opt[x].has_arg = type;
    long_opt[x].flag    = NULL;
    long_opt[x].val     = id << 8;
    x++;
  }
  long_opt[x].name     = 0;
  long_opt[x].has_arg  = 0;
  long_opt[x].flag     = NULL;
  long_opt[x].val      = 0;

  assert(x < length);
  return;
}
