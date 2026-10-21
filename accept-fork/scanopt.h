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
// extern opt_value_t optvalue;   // Added for scanopt
// extern int optmax;             // Added for scanopt: max number of options to define
// extern int optcount;           // Added for scanopt: number of options defined


////////////////////////////////////////////////////////////////////////
// Scanopt is a library that aids in the process of
// managing command line options. Each command line
// option has there a three major properties:
//
//   1. the arguement type: required, optional, and none
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


// 1. The supported type for the argument associated with an option
#define SO_VAL_TYPE(var)   (var & 0x00f0)
#define SO_VAL_TEXT        (0x000)  // the default
#define SO_VAL_SET         (0x010)  // sets the value to 1
#define SO_VAL_UNSET       (0x020)  // clears the value to 0
#define SO_VAL_INT         (0x030)  // conversion done at the end
#define SO_VAL_LONG        (0x040)  // conversion done at the end
#define SO_VAL_FLOAT       (0x050)  // conversion done at the end
#define SO_VAL_DOUBLE      (0x060)  // conversion done at the end
#define SO_VAL_ARRAY       (0x070)  // an array of strings

// Two value types are supported to address common functionality:
//   The SET/UNSET type is provided a Boolean type to support flags
//   The type can also be used to convientely support complements.
//   E.g., the `-i` (inquiry) and `-f` (force) option to the `rm` utility
//
//   The ARRAY type is used to support options that maybe processed,
//   in total, before or after another option. For example, the `http`
//   utility defines a `-C` option and a `-c` options.  Both of these
//   options can appear multiple times on the command-lined, with all
//   `-C` arguments applied before applying all directives with the
//   configuration file (`-f config`).  Then the `-c` arguments are 
//   are applied there after.  As such, the `-C` and `-c` values are
//   an array of arguments.
//

// 1. the duplicative mode to address multiple uses of an option.
#define SO_DUP_TYPE(var)   (var & 0x0f00)
#define SO_DUP_OVERWRITE   (0x000)  // Use the LAST value associates with an option (DEFAULT)
#define SO_DUP_IGNORE      (0x100)  // Use the First value assoicated with an option
#define SO_DUP_APPEND      (0x200)  // Append the value so all duplicates can be   (presumes )
#define SO_DUP_ERROR       (0x300)  // Emit an error upon a duplicate option
#define SO_DUP_MAX         (20)     // Maximum number of values that can be appended


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
// implicitly on the command line.)  You may also retrieve only those
// options that have been defined and were NOT automatically assigned
// to a variable.
//
// Each of these options exist to reduce the size of the switch statement
// that is typically used to inconjection with `scanopt_next` to improve
// readibilty. 

#define SO_OPTIONS_RETRIEVE_DEFINED  (0)
#define SO_OPTIONS_RETRIEVE_ALL      (1)
#define SO_OPTIONS_RETRIEVE_NOVAR    (2)   // only those defined, but no var

#define SO_ID_ERROR     (0xffff)
#define SO_OPTIONS_END() {0}

// Compliments are, e.g., flags that are opposite, e.g., rm -f -i
#define SO_COMPLEMENT(x) (-x)

// Use the following to support literal defintion of an option_t;
#define  SO_OPTION_END()  { 0 }


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
  // MVP
  int      id;              // return value from scanopt_next
  char     *longName;
  char     *shortName;      // typically a char, but to be consistent
  int      type;            // REQUIRED | TEXT | ERROR

  // Usage (--help) Informaion
  char     *description;
  char     *format;         // e.g., "<number>"

  // Env Stuff
  char     *envName;

  // New Stuff
  void     *variable;      // pointer to the appropriate type in which the final value is stored
  char     *initValue;
  char     *defaultValue;

  // Working data:
  char     *value;
  char     *valueArray[2];   // argv-style array of values
  // char     *valueArray[SO_DUP_MAX];   // argv-style array of values
  int      count;                     // command-line reference count
} option_t;



// Main APIs
extern void scanopt_initialize(option_t option_list[]);
extern int scanopt_scan(int argc, char *argv[], option_t option_list[]);

extern int scanopt_next(option_t option_list[]);
//extern int scanopt_next(option_t option_list[], int flag);


extern void scanopt_usage(option_t option_list[]);


// DataStructure APIs
extern void scanopt_validate(option_t option_list[]);
extern option_t *scanopt_find(int id, option_t option_list[]);
  // returns the index into option_list
extern option_t *scanopt_find_shortName(int ch, option_t option_list[]);


extern void  scanopt_add_description(option_t item, char *descript, char *format);
extern void  scanopt_add_environment(option_t item, char *envName);
extern void  scanopt_add_values(option_t item, char *initialValue, char *defaultValue);
extern void  scanopt_add_variables(option_t item, void *variable);
extern void  scanop_opt_add_type(option_t item, int type);




// // Secondary API
extern void scanopt_optstring(char * opt_string, int length, option_t option_list[]);
//   // generates the short form optstring
//   // "b:c::d:e:fhl:p:s:e:w:vV"
//
//   prepend ":" to suppress error messages
//   then have a rule to handle ":+"
//      ":" silent mode
//      "+" stop at first nonoption arguement

extern void scanopt_longopt_struct(struct option *long_opt, int length, option_t option_list[]);
//   // generates the struct option as per the C api
//
// but this one generates a warning because of 'struct option' stems from
// getopt.h


extern void scanopt_longopt(char *long_string, option_t option_list[]);
//   // generates the long form optstring as per the command line
//   // "bind:,documents:foreground, ...."






