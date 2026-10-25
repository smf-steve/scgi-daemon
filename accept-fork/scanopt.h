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
#define SO_ARG_TYPE(var)   (var & 0x000f)
#define SO_ARG_NONE        (0x000)
#define SO_ARG_REQUIRED    (0x001)
#define SO_ARG_OPTIONAL    (0x002)


// 1. The supported types for the argument associated with an option
#define SO_VAL_TYPE(var)   (var & 0x00f0)
#define SO_VAL_NULL        (0x000)
#define SO_VAL_TEXT        (0x000)  // the default
#define SO_VAL_SET         (0x010)  // sets the value to 1
#define SO_VAL_UNSET       (0x020)  // clears the value to 0
#define SO_VAL_INT         (0x030)  // conversion done at the end
#define SO_VAL_LONG        (0x040)  // conversion done at the end
#define SO_VAL_FLOAT       (0x050)  // conversion done at the end
#define SO_VAL_DOUBLE      (0x060)  // conversion done at the end
#define SO_VAL_ARRAY       (0x070)  // an array of strings

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
#define SO_DUP_TYPE(var)   (var & 0x0f00)
#define SO_DUP_OVERWRITE   (0x000)  // Use the LAST value associates with an option (DEFAULT)
#define SO_DUP_IGNORE      (0x100)  // Use the First value associated with an option
#define SO_DUP_APPEND      (0x200)  // Append the value so all duplicates can be   (presumes )
#define SO_DUP_ERROR       (0x300)  // Emit an error upon a duplicate option
#define SO_DUP_MAX         (10)     // Maximum number of values that can be appended


// Defaults for 
#define SO_ARG        (SO_ARG_REQUIRED | SO_VAL_TEXT  | SO_DUP_OVERWRITE)
#define SO_OPTIONAL   (SO_ARG_OPTIONAL | SO_VAL_TEXT  | SO_DUP_OVERWRITE)
#define SO_NONE       (SO_ARG_NONE     | SO_VAL_TEXT  | SO_DUP_OVERWRITE)
#define SO_FLAG       (SO_ARG_NONE     | SO_VAL_SET   | SO_DUP_OVERWRITE)
#define SO_NFLAG      (SO_ARG_NONE     | SO_VAL_UNSET | SO_DUP_OVERWRITE)
#define SO_APPEND     (SO_ARG_REQUIRED | SO_VAL_TEXT  | SO_DUP_APPEND)


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

// Compliments are, e.g., flags that are opposite, e.g., rm -f -i
#define SO_COMPLEMENT(x) (-x)

// Use the following to support literal definition of an option_t;
#define SO_OPTION_END()  { 0 }
#define SO_OPTIONS_END() { 0 }


typedef union _opt_value_t {
  char * sVal;
  int    iVal;
  long   lVal;
  float  fVal;
  double dVal;
  char **aVal;
} opt_value_t;

extern opt_value_t optvalue;
extern int optcount;
extern int optmax;

typedef struct _option_t {
  int      id;              // return value from scanopt_next
  int      type;            // OPTION_TYPE | ARG_TYPE | DUP_MODE
  char     *longName;
  char     *shortName;
  char     *envName;

  // Usage (--help) Information
  char     *description;
  char     *format;         // e.g., "number", default "arg"

  // Variable Definition 
  void     *variable;      // Pointer to the appropriate argument type 
  char     *initValue;     // Initial value to the variable
  char     *defaultValue;  // Default value when a optional value is not present

  // Working data:
  char     *value;
  char     *valueArray[SO_DUP_MAX];   // argv-style array of values
  int      count;                     // command-line reference count
} option_t;



// Operational Functions
extern int scanopt_scan(int argc, char *argv[], option_t option_list[]);
extern int scanopt_next(option_t option_list[]);
extern void scanopt_usage_options(option_t option_list[], char *indent);
extern void scanopt_usage_envs(option_t option_list[], char *indent);


// Option Declaration Functions
extern option_t *scanopt_declare_option(option_t option_list[], int id, int argType, char *longName, char *shortname, char *envName, size_t size);
extern option_t *scanopt_declare_complement(option_t option_list[], int id, int argType, char *longName, char *shortname, char *envName, size_t size);
extern option_t *scanopt_declare_env(option_t option_list[], int id, char *envName, size_t size);

extern void scanopt_set_ondup(int value);
  // The default 'on duplication is "SO_DUP_OVERWRITE"
  // This functions changes the default to potentially:
  //   SO_DUP_OVERWRITE, SO_DUP_IGNORE, SO_DUP_APPEND, SO_DUP_ERROR 

// Option Definition Functions
extern void scanopt_assign_variable(option_t *item, void *variable, int type, char *initialVal, char *defaultVal);
extern void scanopt_assign_flag(option_t *item, void *variable);
extern void scanopt_assign_nflag(option_t *item, void *variable);

extern void scanopt_assign_usage(option_t *item, char *format, char *description);


// Access functions
extern option_t *scanopt_find(int id, option_t option_list[]);
extern option_t *scanopt_find_shortName(int ch, option_t option_list[]);
extern option_t *scanopt_find_envName(char *env, option_t option_list[]);


// Secondary API  (called within but could be used by...)
extern void scanopt_initialize(option_t option_list[]);
extern void scanopt_validate(option_t option_list[]);
extern void scanopt_apply_envs(option_t option_list[]);



