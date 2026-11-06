#include <getopt.h>
#define SET (1)
#define UNSET (0)

////////////////////////////////////////////////////////////////////////
// Scanopt is a library that aids in the process of
// managing command line options. Each command line
// option has three major properties:
//
//   1. the argument type: required, optional, and none
//   1. the value type: text, set and unset (for flags), count, and then
//                      int, long, float, and double.
//
//   1. the duplicative mode: overwrite, error, ignore, and append
//      - overwrite gives priority to the last definition
//      - ignore gives priority to the first definition
//      - append creates a list of each definition
//
//  Scanopt extends the functionality of `getopt`, which it uses
//  under the covers.  We have extended `getopt` by allowing the 
//  automatic conversion of a command line option to a specific type.
//  (The types are limited to C basic types.)  We have also included
//  the functionality to manage duplicative usage of types on the 
//  command line. 
//
/////////////////////////////////////////////////////////////////////////

// Scanopt functional is based upon the definition (by the user) of
// three tables.  Each of these tables are defined by array initialization
// with the last element filled with zeroes. The first table is required: 'scanopt_names_t'

// Use the following to support literal definition of an scanopt_t;
#define SO_TABLE_END() { 0 }

typedef struct {
  int      id;
  int      type;
  char     *longName;
  char     *shortName;
  char     *envName;
} scanopt_names_t;

// The type field is a bit-field that determines
//   1. If the option is a basic option, a synonym, or a complement
//   1. A basic option is one of: non, required, optional
//
//    type =  00 00 00 xx  : SO_TYPE_BASIC:  xx is the ARG_TYPE
//    type =  xx 00 00 00  : !SO_TYPE_BASIC: xx determines syn or comp

#define SO_SYNONYM(name) (SO_TYPE_SYN | name)
#define SO_SYN(name) SO_SYNONYM(name)

#define SO_COMPLEMENT(variable) (SO_TYPE_COMP | variable)
#define SO_COMP(variable) SO_COMPLEMENT(variable)

#define SO_TYPE_BASIC      (0x00 << 24)
#define SO_TYPE_SYN        (0x80 << 24)
#define SO_TYPE_COMP       (0x90 << 24)

#define SO_ARG_TYPE(var)   (var & 0x000f)
#define SO_ARG_NONE        (0x0000) //getopt.h equiv: no_argument
#define SO_ARG_REQUIRED    (0x0001) //getopt.h equiv: required_argument
#define SO_ARG_OPTIONAL    (0x0002) //getopt.h equiv: optional_argument


// Variable / Flag Definition 
typedef struct {
  int      id;
  int      type;           //
  void     *address ;      // Address in which the value is to be stored 
  char     *initValue;     // Initial value to the variable
  char     *defaultValue;  // Default value when a optional value is not present
} scanopt_variable_t;

// The type field is a bit-field that determines
//   - the type of the variable, and
//   - the mode for dealing with duplicative usage of an option

// Common variable types
#define SO_FLAG       (SO_VAL_SET   | SO_DUP_OVERWRITE)
#define SO_NFLAG      (SO_VAL_UNSET | SO_DUP_OVERWRITE)
#define SO_COUNTER    (SO_VAL_COUNT | SO_DUP_OVERWRITE)
#define SO_APPEND     (SO_VAL_TEXT  | SO_DUP_APPEND)
////
#define SO_ARG        (SO_VAL_TEXT  | SO_DUP_OVERWRITE)
#define SO_OPTIONAL   (SO_VAL_TEXT  | SO_DUP_OVERWRITE)
#define SO_NONE       (SO_VAL_NULL  | SO_DUP_OVERWRITE)

// Basic variable types
#define SO_VAL_TEXT        (0x0000) // Text
#define SO_VAL_NULL        (0x0000)
#define SO_VAL_DEFAULT     (0x0000)
////   Associated with NO ARGS
#define SO_VAL_SET         (0x0010)  // Sets the value (to 1)
#define SO_VAL_UNSET       (0x0020)  // Clears the value (to 0)
#define SO_VAL_COUNT       (0x0030)  // Holds a count value
//// Associated with required and optional ARGS
#define SO_VAL_INT         (0x0040)
#define SO_VAL_LONG        (0x0050)
#define SO_VAL_FLOAT       (0x0060)
#define SO_VAL_DOUBLE      (0x0070)
#define SO_VAL_ARRAY       (0x0080)  // an array of text

// Duplication modes for variable types
#define SO_DUP_DEFAULT     (0x0000)  // The global default is OVERWRITE -- which can be overridden
#define SO_DUP_OVERWRITE   (0x0100)  // Use the LAST value associates with an option (DEFAULT)
#define SO_DUP_IGNORE      (0x0200)  // Use the First value associated with an option
#define SO_DUP_APPEND      (0x0300)  // Append the value so all duplicates can be   (presumes )
#define SO_DUP_ERROR       (0x0400)  // Emit an error upon a duplicate option

#define SO_DUP_MAX         (10)      // Maximum number of values that can be appended


// Usage (--help) Information
typedef struct {
  int      id;
  char     *format;         // e.g., "number", default "arg"
  char     *text;
} scanopt_usage_t; 


typedef union  {
  char * sVal;
  int    iVal;
  long   lVal;
  float  fVal;
  double dVal;
  char **aVal;
} opt_value_t;

/////////////////////////////////////////


// External Global Variables: these are
//   - a reassertion of such variables
//   - an extension of such variables
// from getopt_long
//
// Each defined after each call to scanopt_next

// Reassertion:
extern char       *optarg;
extern int        optind;     // ineffective
extern int        optopt;     // ineffective
extern int        optreset;  

// Extension:
extern char        *optname;   // the name of the option
extern char        *optarg;    // the text of the option's argument
extern opt_value_t optvalue;   // the converted value of 
extern int         optcount;   // the number of times the option was used



typedef struct scanopt_instance_t *  scanopt_t; 

// Scanopt Object creation/destruction
extern scanopt_t scanopt_new(scanopt_names_t names[], scanopt_usage_t usages[], scanopt_variable_t variables[]);
extern void scanopt_validate(scanopt_t instance);
extern void scanopt_destroy(scanopt_t instance);


// Operational Functions
extern int  scanopt_scan(scanopt_t instance, int argc, char *argv[]);
extern void scanopt_apply_environment(scanopt_t instance);

extern int  scanopt_next(scanopt_t instance);
#define SO_ID_ERROR     (0xffff)

//extern int  scanopt_next(scanopt_t instance, int mode);

#define SO_NEXT_RETURN_USED  (0)   // options used on the command line
  // options that appeared on the command line or as an ENV variable
  // but not necessarily those that were provided with an initial value

#define SO_NEXT_RETURN_NOVAR (1)   
  // excludes those options that have a variable 

#define SO_NEXT_RETURN_ALL   (2)  // 


// Usage related Functions
extern void scanopt_usage_options(scanopt_t instance, char *indent);
extern void scanopt_usage_envs(scanopt_t instance, char *indent);


// Miscellaneous Functions
extern void scanopt_set_ondup(scanopt_t instance, int value);
  // The default 'on duplication is "SO_DUP_OVERWRITE"
  // This functions changes the default to potentially:
  //   SO_DUP_OVERWRITE, SO_DUP_IGNORE, SO_DUP_APPEND, SO_DUP_ERROR 

