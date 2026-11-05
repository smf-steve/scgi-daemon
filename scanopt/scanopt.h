#include <getopt.h>

// From getopt.h, that is exposed to the user of scanopt

// #define required_argument  SO_ARG_REQUIRED
// #define optional_argument  SO_ARG_OPTIONAL
// #define no_argument        SO_ARG_NONE

// extern char *optarg;
// extern int optind;
// extern int optopt;
// extern int opterr;
// extern int optreset;
// extern int optmax;             // Added for scanopt: max number of options to define
// extern int optcount;           // Added for scanopt: number of options defined
// extern opt_value_t optvalue;   // Added for scanopt

////////////////////////////////////////////////////////////////////////
// Scanopt is a library that aids in the process of
// managing command line options. Each command line
// option has there a three major properties:
//
//   1. the argument type: required, optional, and none
//   1. the value type:     text, set and unset (for flags), and then
//                          int, long, float, and double.
//
//   1. the duplicative mode: overwrite, error, ignore, and append
//      overwrite give priority to the last definition
//      ignore gives priority to the first definition
//      append creates a list of options
//
//  Scanopt extends the functionality of `getopt`, which it uses
//  under the covers.  We have extended `getopt` by allowing the 
//  automatic conversion of a command line option to a specific type.
//  (The types are limited to C basic types.)  We have also included
//  the functionality to manage duplicative usage of types on the 
//  command line. 
//

// 1. The three argument types are defined consistent with `getopt`
//    They are associated with the names.type
//    names.type =  0000 0000 0000 000x  : x is the SO_ARG_TYPE
//    names.type =  xxxx yyyy yyyy yyyy  : x is the ARG bla, y is th
#define SO_ARG_TYPE(var)   (var & 0x000f)  // NOT USED
#define SO_ARG_NONE        (0x0000)
#define SO_ARG_REQUIRED    (0x0001)
#define SO_ARG_OPTIONAL    (0x0002)

// Complements are, e.g., flags that are opposite, e.g., rm -f -i
// This are the arg types..
#define SO_TYPE_ROOT       (0x00 << 24)   // SO_TYPE_ROOT
#define SO_TYPE_SYN        (0x80 << 24)
#define SO_TYPE_COMP       (0x90 << 24)

/////////////////////////////////////////

// Defaults for VARABLE TYPES
//  The first is not necessary
#define SO_ARG        (SO_VAL_TEXT  | SO_DUP_OVERWRITE)
#define SO_OPTIONAL   (SO_VAL_TEXT  | SO_DUP_OVERWRITE)
#define SO_NONE       (SO_VAL_NULL  | SO_DUP_OVERWRITE)

// these make sense if we support DUP_MODE
#define SO_FLAG       (SO_VAL_SET   | SO_DUP_OVERWRITE)
#define SO_NFLAG      (SO_VAL_UNSET | SO_DUP_OVERWRITE)
#define SO_COUNTER    (SO_VAL_COUNT | SO_DUP_OVERWRITE)

#define SO_APPEND     (SO_VAL_TEXT  | SO_DUP_APPEND)



///////////////////
// 1. The supported types for the argument associated with an option
//    associated with the variable->type
//
//   so change this to VAR->type 

#define SO_VAL_TEXT        (0x0000)
#define SO_VAL_NULL        (0x0000)  // the default
#define SO_VAL_DEFAULT     (0x0000)  // the default

// Associated with NO ARGS
#define SO_VAL_SET         (0x0010)  // sets the value to 1
#define SO_VAL_UNSET       (0x0020)  // clears the value to 0
#define SO_VAL_COUNT       (0x0030)  // an int

// Associated with ~ NO ARGS
#define SO_VAL_INT         (0x0040)  // conversion done at the end
#define SO_VAL_LONG        (0x0050)  // conversion done at the end
#define SO_VAL_FLOAT       (0x0060)  // conversion done at the end
#define SO_VAL_DOUBLE      (0x0070)  // conversion done at the end
#define SO_VAL_ARRAY       (0x0080)  // an array of text






// Two value types are supported to address common functionality:
//   The SET/UNSET type is provided as a Boolean type to support
//   flags. The type can also be used to conveniently support 
//   complements.
//      E.g., the `-i` (inquiry) and `-f` (force) option 
//      associated with the `rm` utility.
//
//   The ARRAY type is used to support options that can be processed,
//   in total, before or after another option. 
//      E.g., the `http` utility defines a `-C` option and a `-c`
//      option.  Both of these options can appear multiple times
//      on the command-lined.  With all `-C` arguments applied
//      before applying all directives with the configuration file
//      (`-f config`), then the `-c` arguments are applied thereafter.  
//      As such, the `-C` and `-c` values are an array of arguments.
//

// 1. the duplicative mode to address multiple uses of an option.

#define SO_DUP_DEFAULT     (0x0000)  // The global default is OVERWRITE -- which can be overridden
#define SO_DUP_OVERWRITE   (0x0100)  // Use the LAST value associates with an option (DEFAULT)
#define SO_DUP_IGNORE      (0x0200)  // Use the First value associated with an option
#define SO_DUP_APPEND      (0x0300)  // Append the value so all duplicates can be   (presumes )
#define SO_DUP_ERROR       (0x0400)  // Emit an error upon a duplicate option

#define SO_DUP_MAX         (10)     // Maximum number of values that can be appended


// During the final processing stage `scanopt_next`, you can specify 
// whether or not your want to retrieve only those options that have
// appeared on the command line or _all_ options.   (Note that if an
// options is defined via an environment variable, it is defined 
// implicitly on the command line.)  If an option did not appear on
// the command line, it still might have been provided with initial.
// You may also retrieve only those options that have been defined 
// and were NOT automatically assigned to a variable.
//
// Each of these options exist to reduce the size of the switch statement
// that is typically used to in conjunction with `scanopt_next` to improve
// readability.

#define SO_OPTIONS_RETRIEVE_DEFINED  (0)
  // options that appeared on the command line or as an ENV variable
  // but not necessarily those that were provided with an initial value

#define SO_OPTIONS_RETRIEVE_NOVAR    (1)   
  // excludes those options that have a variable 

#define SO_OPTIONS_RETRIEVE_ALL      (2)

#define SET (1)
#define UNSET (0)

#define SO_ID_ERROR     (0xffff)


#define SO_SYNONYM(name) (SO_TYPE_SYN | name)
#define SO_SYN(name) SO_SYNONYM(name)

#define SO_COMPLEMENT(variable) (SO_TYPE_COMP | variable)
#define SO_COMP(variable) SO_COMPLEMENT(variable)


// Use the following to support literal definition of an scanopt_t;
#define SO_TABLE_END() { 0 }


typedef union {
  char * sVal;
  int    iVal;
  long   lVal;
  float  fVal;
  double dVal;
  char **aVal;
} opt_value_t;


// Object Data
extern opt_value_t optvalue;
extern int         optcount;   // the number of options/envs used
extern char        *optname;
extern int         optmax;

// extern int    optreset;
// extern int    optind;
// extern char   *optarg;



typedef struct {
  int      id;              // return value from scanopt_next
  int      type;            // OPTION_TYPE | ARG_TYPE | DUP_MODE
  char     *longName;
  char     *shortName;
  char     *envName;
} scanopt_names_t;

// Variable / Flag Definition 
typedef struct {
  int      id;
  int      type;           //
  void     *address ;      // Address in which the value is to be stored 
  char     *initValue;     // Initial value to the variable
  char     *defaultValue;  // Default value when a optional value is not present
} scanopt_variable_t;


// Usage (--help) Information
typedef struct {
  int      id;
  char     *format;         // e.g., "number", default "arg"
  char     *text;
} scanopt_usage_t; 

typedef struct scanopt_instance_t *  scanopt_t; 


extern scanopt_t scanopt_create(int count);
extern scanopt_t scanopt_new(scanopt_names_t names[], scanopt_usage_t usages[], scanopt_variable_t variables[]);
extern void scanopt_destroy(scanopt_t instance);

// Operational Functions
extern int scanopt_scan(scanopt_t instance, int argc, char *argv[]);
extern int scanopt_next(scanopt_t instance);
extern void scanopt_usage_options(scanopt_t instance, char *indent);
extern void scanopt_usage_envs(scanopt_t instance, char *indent);


extern void scanopt_set_ondup(scanopt_t instance, int value);
  // The default 'on duplication is "SO_DUP_OVERWRITE"
  // This functions changes the default to potentially:
  //   SO_DUP_OVERWRITE, SO_DUP_IGNORE, SO_DUP_APPEND, SO_DUP_ERROR 


// Secondary API  (called within but could be used by...)
//extern void scanopt_initialize(scanopt_t options[]);
extern void scanopt_initialize(scanopt_t object, 
                               scanopt_usage_t usages[],
                               scanopt_variable_t variables[]);

extern void scanopt_validate(scanopt_t instance);
extern void scanopt_apply_environment(scanopt_t instance);




// Option Definition Functions
//extern void scanopt_assign_variable(scanopt_t *item, void *variable, int type, char *initValue, char *defaultValue);
//extern void scanopt_assign_flag(scanopt_t *item, void *variable);
//extern void scanopt_assign_nflag(scanopt_t *item, void *variable);

//extern void scanopt_assign_usage(scanopt_t *item, char *format, char *description);


// Access functions
//extern scanopt_t *scanopt_find(int id, scanopt_t options[]);
//extern scanopt_t *scanopt_find_shortName(int ch, scanopt_t options[]);
//extern scanopt_t *scanopt_find_envName(char *env, scanopt_t options[]);

// Option Declaration Functions
//extern scanopt_t *scanopt_declare_option(scanopt_t options[], int id, int argType, char *longName, char *shortname, char *envName, size_t size);
//extern scanopt_t *scanopt_declare_complement(scanopt_t options[], int id, int argType, char *longName, char *shortname, char *envName, size_t size);
//extern scanopt_t *scanopt_declare_env(scanopt_t options[], int id, int argType, char *envName, void * variable, size_t size);
