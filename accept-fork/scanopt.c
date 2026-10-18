// Name:  scanopt:  short for scan options
//        switchscan


/*
Major difference is our versions popt/getopt..
  - we don't try to keep the return value as the shortName char
    - we return an INDEX value to a structure
  - we don't try to store info into a varialbe
    - we allow/require the user to put the process value there...
  - option types include: required, option, none (as usually)
  - argument types include:  text, int, long, float, & double
  - argument types include the ability to deal with duplicates
    - supports the http approach of -c & -C
  - we have alias/synonmyses
  - we have opposites:  e.g., --force, --inquire
  - we define default values
  - we incorporate Env Variable into the process

  - we internally normalize the argv structure different
*/


// PREMISE:  an option set via an env is just like setting on the cli
//          but this allows for CONFIG file to be used
//          but this also allows simplier control per user



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



#include "scanopt.h"

opt_value_t opt_value;

// To implement:


extern void  scanopt_add_description(option_t item, char *descript, char *format);
extern void  scanopt_add_environment(option_t item, char *envName);
extern void  scanopt_add_values(option_t item, char *initialValue, char *defaultValue);
extern void  scanopt_add_variables(option_t item, void *variable);
extern void  scanop_opt_add_type(option_t item, int type);

//// A set of Getter Functions
//char * scanopt_get_value(option_t item);
//
//// the following validate that the arg type is correct
//int    scanopt_get_value_flag(option_t item);
//int    scanopt_get_value_iVal(option_t item);
//int    scanopt_get_value_lVal(option_t item);
//double scanopt_get_value_fVal(option_t item);
//double scanopt_get_value_dVal(option_t item);
//



extern int scanopt_arg_default;
int scanopt_arg_default = SO_ARG_DEFAULT;

static void scanopt_apply_env(option_t option_list[]);
static void scanopt_first_use(option_t *item, char *value);

static int number_of_options(option_t option_list[]);

#define SO_OPTION_NAME(item)  (item->longName != NULL ? item->longName : item->shortName )

extern void scanopt_validate(option_t option_list[]) {
   // must have at least a longName or shortName
   // shortName is a single character
   // shortname does not use ':' nor '?'
   // check id < 0
   //   -- SO_SYNONYM:     SO_ARG_NONE with SO_VAL_TEXT (i.e., the defualt)
   //   -- SO_COMPLEMENT:  SO_ARG_NONE with SO_VAL_SET or SET_VAL_UNSET

   // NOT checked: if (id > 0), it is unique within the option_list

   // move this to initialize: if SO_VAL_SET or SET_VAL_UNSET is provided,
   //    it must have an inital, or it should be  "0" "1"  respectively
   
   // default value is ONLY associated with a option_argument
   //  no_aurgment,     SET | UNSET, initial value
   //  option_argument, *,           initial value, default value
   //  required_value,  *,           initial value
 
   /// shuld we flip the order of execution of:  validate &

 option_t *item;
 option_t *root_item;
 int i;
 int isError = false;

 for(i = 0; option_list[i].id != 0; i++ ) {
    item = option_list+i;
    if (item->longName == NULL && item->shortName == NULL) {
      fprintf(stderr, "Option needs either a long name or short name\n");
      isError = true;
    }

    if (item->shortName != NULL) {
      if (item->shortName[0] == '\0' || item->shortName[1] != '\0') {
        if (item->longName != NULL) {
          fprintf(stderr, "Short name for \"%s\" must be a single character\n", item->longName);
        } else {
          fprintf(stderr, "Short name for option id=%d must be a single character\n", item->id);
        }
        isError = true;
      }
      switch (item->shortName[0]) {
        case '?':
        case ':':
          fprintf(stderr, "Colon (:) nor Question (?) can not be used as an option\n");
          isError = true;
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
        fprintf(stderr, "Option \"%s\" missing it's complement\n", SO_OPTION_NAME(item));
        isError = true;
      }
      
      switch (SO_VAL_TYPE(item->type)) {
        case SO_VAL_SET:
          if (SO_VAL_TYPE(root_item->type) != SO_VAL_UNSET) {
            fprintf(stderr, "Option \"%s\" improperly defined complement\n", SO_OPTION_NAME(root_item));
            isError = true;
          }
          break;

        case SO_VAL_UNSET:
          if (SO_VAL_TYPE(root_item->type) != SO_VAL_SET) {
            fprintf(stderr, "Option \"%s\" improperly defined complement\n", SO_OPTION_NAME(root_item));
            isError = true;
          }
          break;

        default:
          fprintf(stderr, "Option \"%s\" improperly defined complement\n", SO_OPTION_NAME(item));
          isError = true;
          break;
      }
      if (SO_DUP_TYPE(item->type) != SO_DUP_OVERWRITE) {
        fprintf(stderr, "Complement Option \"%s\" must use \"overwrite\" mode\n", SO_OPTION_NAME(root_item));
        isError = true;
      }
    }
    // END OF COMPLEMENT CONSTRAINTS

  }
  //if (isError == true ) {
  //  exit(1);
  //}
  return;
}

static void scanopt_config();

extern int scanopt_scan(int argc, char *argv[], option_t option_list[]) {
  // rescans the command line
  // sets the getopt `optreset` to ensure getopt works from the beginning

  int           isError = false;
  int           count;
  option_t      *item;

  // Variables associates with the use of `getopt_long`
  int           ch;
  char          *opt_string;
  struct option *long_opts;

  extern int    optreset;
  extern int    optind;
  extern char   *optarg;

  int longindex;

  // Some stuff to address our use of `getopt_long`
  optreset = 1;        // Make sure getopt starts at the beginning


  count          = number_of_options(option_list);

  // the following does not work.
  // opt_string     = alloca(sizeof(char) * count * 3);   // each option could be "x::"
  // long_opts      = alloca(sizeof(struct option)* count);

  opt_string     = malloc(sizeof(char) * count * 3);   // each option could be "x::"
  long_opts      = malloc(sizeof(struct option)* count);



  scanopt_validate(option_list);
  scanopt_initialize(option_list);

  scanopt_optstring(opt_string, count * 3, option_list);
  scanopt_longopt_struct(long_opts, count + 1, option_list);


  opterr = 0;  // disable messages
  while ((ch = getopt_long(argc, argv, opt_string, long_opts, &longindex)) != -1) {
    // getopt_long return values:
    //   -1  : when argv list is exhausted
    //    0  : when a flag has been set... (will no happen in our case)
    // <256  : Short Option
    // >256  : Long Options
    if (ch <= 0xff) {   // ShortOp was provided by the user
      switch (ch) {
        case ':':       // missing argument  (58) (0x3a)
          fprintf(stderr, "Missing argument for: %s\n", argv[optind-1]);
          continue;      // or we are don with the loop?
          //return optind;
          break;
        case '?':       // ambigous option   (63) (0x3f)
          fprintf(stderr, "Unknown option: %s\n", argv[optind-1]);
          continue;     // or we are don with the loop?
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
      scanopt_first_use(item, optarg);
      continue;
    }

    // Issue if Dup mode is APPEND, we have already copy the value

    ///////////////////////////////////////////////
    // Subsequent use of the option
    switch (SO_DUP_TYPE(item->type)) {
      case SO_DUP_ERROR:
        fprintf(stderr, "Invalid duplicate use of --%s\n", SO_OPTION_NAME(item));
        isError = true;
        break;

      case SO_DUP_OVERWRITE:
        scanopt_first_use(item, optarg);
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
  
  scanopt_apply_env(option_list);

  return optind;
}


#define SO_OPTION_isEND(x)  x.id != 0

extern void scanopt_usage(option_t option_list[]){
  option_t item;
  char     *format;

  fprintf(stdout, "Options:\n");
  for (int i = 0; option_list[i].id == 0; i++) {
    item = option_list[i];

    // must have at least a longName or shortName
    assert(item.longName != NULL || item.shortName != NULL );

    fprintf(stdout, "\t");

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
          fprintf(stdout, " %s", format);
          break;
      }
    }
    fprintf(stdout, "\n");

    // DESCRIPTION
    if (item.description != NULL) {
      fprintf(stdout, "\t\t%s", item.description);
    }
    
    if (item.defaultValue != NULL) {
      fprintf(stdout, "\t\tDEFAULT: %s\n", item.defaultValue);
    }
    if (item.envName != NULL) {
      fprintf(stdout, "\t\tENVIRONMENT VARIABLE: %s\n", item.envName);
    }
    fprintf(stdout, "\n");
  }
  return;
}



 /*
  Process:
    0. define the structure [passed as a paramater]
    // ! 1. normailize the command line
    2. if optreset == 1
       - optreset = 0;
       - update all generic variables, e.g., optind
    3. load env values
    4. walk argv to update the value for all options via the cli
       a. find corresponding option in structure: item
          * if SO_ERROR_DUPLICATE && item.count > 0
            goto 4. b based up  ':' & '?' rules

          - item.count ++
          - item.value = argv.value  (as appropriate)

    5. walk the structure, using item

       a. item_index ++;
       b. optarg = item.value
       c. update other optarg stuff
       d. update item pointer
       e. return item.index

       * upon next call to scan_option, come here

*/

extern option_t *scanopt_find(int id, option_t option_list[]) {
  int i;

  for(i = 0; option_list[i].id != 0; i++ ) {
    if (option_list[i].id == id) break;
  }

  return &(option_list[i]);
}

extern option_t *scanopt_find_shortName(int ch, option_t option_list[]) {
  int i;

  for(i = 0; option_list[i].id != 0; i++ ) {
    if (option_list[i].shortName == NULL) continue;
    if (option_list[i].shortName[0] == ch) break;
  }

  return &(option_list[i]);
}


static void scanopt_first_use(option_t *item, char *value) {
  // Update the 'value' base up
  //   case
  //      required_multiple: copy value    to item->value
  //      required:          copy value    to item->value
  //      optional_present:  copy value    to item->value
  //      optional_missing:  copy default  to item->value
  //      none:              nothing       but address SET/UNSET

  switch(SO_ARG_TYPE(item->type)) {
    case SO_ARG_NONE:

      switch (SO_VAL_TYPE(item->type)) {
        case SO_VAL_SET:      item->value = "1";   break;
        case SO_VAL_UNSET:    item->value = "0";   break;
      }

    case SO_ARG_OPTIONAL:
      if (value == NULL)
        item->value = item->defaultValue;
      else
        item->value = value;
      break;

    case SO_ARG_REQUIRED: item->value = value; break;
      
    default:
      assert(false);
      break;
  }
  return;
}


extern void scanopt_initialize(option_t option_list[]) {
  // For each item on the list,
  //   initializes the working data

  option_t item;

  for (int i = 0; option_list[i].id != 0; i++) {
    item = option_list[i];

    if (item.id < 0) continue;

    // Initialize working data!
    item.value = NULL;
    for(int j = 0; j < SO_DUP_MAX; j++) {
      if (item.valueArray[j] == NULL)  break;
      item.valueArray[j] = NULL;
    }
    item.count = 0;

    // for _SET and _UNSET must there be an initial value, use.
    item.value = item.initValue;
  }
}

static void scanopt_apply_env(option_t option_list[]) {
  option_t *item;

  for (int i = 0; option_list[i].id != 0; i++) {
    item = option_list+i;

    if (item->id < 0) continue;
    if (item->count > 0) continue;

    if (item->envName != NULL) {
      char *value = getenv(item->envName);
      if (value != NULL) {
        item->count++;
        scanopt_first_use(item, value);
      }
    }
  }
}


static void scanopt_config() {
  printf("not implemented");
  return;
}


//extern int scanopt_next(option_t option_list[], int flag) {
extern int scanopt_next(option_t option_list[]) {

  static int current_index = 0;
  static int initialized = false;

  option_t item;

  if (initialized == false) {
    initialized = true;
    current_index = 0;
  }

    // Not sure of the utility of the following.
    // do {
    //   current_index ++;
    //   switch (flag) {

    //     case SO_OPTIONS_NOVAR:
    //       if (item.flag != NULL);
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
    item = option_list[current_index];
    current_index ++;

    if (item.id == 0)   return -1;
    if (item.count > 0) break;

    // For unused compliments, use the SO_VAL_SET to determine what to return
    if (SO_VAL_TYPE(item.type) == SO_VAL_SET)   break;
    if (SO_VAL_TYPE(item.type) == SO_VAL_UNSET) continue;
  } while (true);

//   at least one of them has been used.
//   if item.type == SO_VAL_SET    && item.value == "1"
//     return item.index
//   if item.type == SO_VAL_SET    && item.value == "0"
//     return - item.index
//   if item.type == SO_VAL_UNSET  && item.value == "1"
//     return item.index
//   if item.type == SO_VAL_UNSET  && item.value == "0"
//     return - item.index


  optarg = item.value;
  optind = current_index;         // relative to the option_list NOT argv
  // optopt = item.shortName[0];  // does not make sense

  switch (SO_VAL_TYPE(item.type)) {
   case SO_VAL_TEXT:   opt_value.sVal = item.value; break;
   case SO_VAL_SET:
   case SO_VAL_UNSET:
   case SO_VAL_INT:    opt_value.iVal = (int) atoi(item.value); break;
   case SO_VAL_LONG:   opt_value.lVal = (long) atoi(item.value); break;
   case SO_VAL_FLOAT:  opt_value.fVal = (float) atof(item.value); break;
   case SO_VAL_DOUBLE: opt_value.dVal = (float) atof(item.value); break;
   case SO_VAL_ARRAY:  opt_value.aVal = item.valueArray; break;
  }
  if (item.flag != NULL) {
    switch (SO_VAL_TYPE(item.type)) {
      case SO_VAL_TEXT:   (* (char **)item.flag) = opt_value.sVal; break;
      case SO_VAL_SET:
      case SO_VAL_UNSET:
      case SO_VAL_INT:    (* (int *)item.flag) = opt_value.iVal; break;
      case SO_VAL_LONG:   (* (int *)item.flag) = opt_value.lVal; break;
      case SO_VAL_FLOAT:  (* (float *)item.flag) = opt_value.fVal; break;
      case SO_VAL_DOUBLE: (* (double *)item.flag) = opt_value.dVal; break;
      case SO_VAL_ARRAY:  (* (char ***)item.flag) = opt_value.aVal; break;

    }
  }
  return item.id;   // sometimes return the compliment.
}

static int number_of_options(option_t option_list[]) {
   int i;

   for (i = 0; option_list[i].id != 0; i++);
   return i;
}



// // Secondary API
void scanopt_optstring(char * opt_string, int length, option_t option_list[]) {
//   // generates the short form optstring
//   // "b:c::d:e:fhl:p:s:e:w:vV"
  int x; // index into opt_string
  int i; //index into option_list


  opt_string[0] = '+';  // halt on first non-option argument
  opt_string[1] = ':';  // suppress error messages
  x = 2;

  for (i = 0; option_list[i].id != 0; i++) {
    assert(i < length);

    if (option_list[i].shortName != NULL) {
      opt_string[x] = option_list[i].shortName[0]; x++;
      switch (option_list[i].type) {
        case SO_ARG_NONE:  break;
        case SO_ARG_OPTIONAL:
          opt_string[x] = ':'; x++;
          //merge;
        case SO_ARG_REQUIRED:
          opt_string[x] = ':'; x++;
      }
    }
  }
  return;
}


void scanopt_longopt_struct(struct option *long_opt, int length, option_t option_list[]) {
//   // generates the struct option as per the C api
//   // no need to create the flag option..
// but this one generates a warning because of 'struct option' stems from
// getopt.h

  struct option long_item;
  option_t item;
  int i = 0;

  for (i = 0; option_list[i].id != 0; i++) {
    item = option_list[i];
    long_opt[i].name = item.longName;
    long_opt[i].has_arg = SO_ARG_TYPE(item.type);
    long_opt[i].flag = NULL;
    long_opt[i].val = item.id << 8;
   }
// bzero((void *)(long_opt+i), sizeof(struct option));
    long_opt[i].name = 0;
    long_opt[i].has_arg = 0;
    long_opt[i].flag = NULL;
    long_opt[i].val = 0;

 return;
}
