
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

#define SO_ARG_NONE        (0x000)
#define SO_ARG_REQUIRED    (0x001)
#define SO_ARG_OPTIONAL    (0x002)
#define SO_ARG_TYPE(var)   (var & 0x000f)

// Each command line option has three major properties.
// 1. the arguement type: required, optional, and none
// 1. the value type:     text, set and unset (for flags), and then
//                        int, long, float, and double.
//
// 1. the duplicative mode: overwrite, error, ignore, and append
//    overwrite give priority to the last definition
//    ignore gives priority to the first definition
//    append creates a list of options

#define SO_OPTIONS_RETRIEVE_DEFINED (0)
#define SO_OPTIONS_RETRIEVE_NOVAR   (1)   // only those defined, but no var
#define SO_OPTIONS_RETRIEVE_ALL     (2)


#define SO_VAL_TEXT        (0x000)
#define SO_VAL_SET         (0x010)  // set the value to 1
#define SO_VAL_UNSET       (0x020)  // clear the value to 0
#define SO_VAL_INT         (0x030)  // conversion done at the end
#define SO_VAL_LONG        (0x040)  // conversion done at the end
#define SO_VAL_FLOAT       (0x050)  // conversion done at the end
#define SO_VAL_DOUBLE      (0x060)  // conversion done at the end
#define SO_VAL_ARRAY       (0x070)
#define SO_VAL_TYPE(var)   (var & 0x00f0)

#define SO_DUP_OVERWRITE   (0x000)  // Use the LAST value associates with an option (DEFAULT)
#define SO_DUP_IGNORE      (0x100)  // Use the First value assoicated with an option
#define SO_DUP_APPEND      (0x200)  // Append the value so all duplicates can be   (presumes )
#define SO_DUP_ERROR       (0x300)  // Emit an error upon a duplicate option
#define SO_DUP_TYPE(var)   (var & 0x0f00)


#define SO_DUP_MAX         (20)     // Maximum number of values that can be appended
#define SO_ARG_ERROR       (0xF000)
#define SO_ARG_DEFAULT     (SO_ARG_REQUIRED | SO_VAL_TEXT | SO_DUP_OVERWRITE)
extern int scanopt_arg_default;


// Synonym is when there are multiple names for the same option
#define SO_SYNONYM(x)    (-x)

// Compliments are, e.g., flags that are opposite, e.g., rm -f -i
#define SO_COMPLEMENT(x) (-x)

// Alias is a single option that expands into one or more other options
// We don't support such alias (at this time)
// Current thinking is that a separate table needs to be maintained
//   if option is an alias then look up the definition
//   then update each of those options in turn


// Use the following to support literal defintion of an option_t;
#define  SO_OPTION_END()  { 0 }


// Reserved for backward compatability with getopt
#define SO_ID_COLON     (0xffff)
#define SO_ID_QUESTION  (0xffff)
#define SO_ID_ERROR     (0xffff)

typedef union _opt_value_t {
  char * sVal;
  int    iVal;
  long   lVal;
  float  fVal;
  double dVal;
  char **aVal;
} opt_value_t;

extern opt_value_t opt_value;

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
  void     *flag;           // pointer to the appropriate type in which the final value is stored
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






