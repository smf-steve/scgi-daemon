

int default_arg = SO_ARG_NONE | SO_ARG_SET | SO_ARG_OVERWRITE_DUPLICATE;
// These are used to address how to deal with duplications.
// Need to have a global value to update the default for all.
// That way we can ignore individual settings within an item spec, if it is 0
// E.g., if O_Global_DUPLICATE_HANDLING is NON-Zero
//     the default handling set for each item upon creation of that item.


#define SO_ARG_NONE        (0x00)  // 
#define SO_ARG_SET         (0x01)  // set the value to 1
#define SO_ARG_UNSET       (0x02)  // clear the value to 0

#define SO_ARG_REQUIRED    (0x10)
//      unused             (0x11) 
//      unused             (0x12)
#define SO_ARG_TEXT        (0x13)  // type: char *, conversion: none
#define SO_ARG_INT         (0x14)  // "%d"
#define SO_ARG_LONG        (0x15)  // "%ld"
#define SO_ARG_FLOAT       (0x16)  // "%f" 
#define SO_ARG_DOUBLE      (0x17)  // "%lf" 

#define SO_ARG_OPTIONAL    (0x20)  // ORed with above to define all the optional types, e.g., 
//      unused             (0x21)
//      unused             (0x22)
#define SO_ARG_OPT_TEXT    (0x23)  // O_ARG_OPT_TEXT = O_ARG_TEXT | O_OPTIONAL_ARG
#define SO_ARG_OPT_INT     (0x24)  // "%d"
#define SO_ARG_OPT_LONG    (0x25)  // "%ld"
#define SO_ARG_OPT_FLOAT   (0x26)  // "%f" 
#define SO_ARG_OPT_DOUBLE  (0x27)  // "%lf" 

#define SO_ARG_OVERWRITE_DUPLICATE  (0x000)  // Use the LAST value associates with an option (DEFAULT)
#define SO_ARG_ERROR_DUPLICATE      (0x100)  // Emit an error upon a duplicate option
#define SO_ARG_IGNORE_DUPLICATE     (0x200)  // Use the First value assoicated with an option
#define SO_ARG_APPEND_DUPLICATE     (0x300)  // Append the value so all duplicates can be   (presumes )
   // which should be the default..?   
   //
   // I could argue that its APPEND, in this way getop will reture each substring in succively
   //   If I do this than I could do post processing of after all argv is read
   //
   // I could also argue its OVERWRITE, unless the programmer does something
   //   special in the "case" to detect multiple use, this is what it would be

//#define O_ARG_VAL   (4)  
// This provides funcationality similiar to O_OVERWRITE_DUPLICATE
// but two flags share the some value





// If we get ride of formating, which I think is what we are doing
// the above list can be reduced to
// #define SO_ARG_NONE      (0x00)  // type: int, conversion: none  
// #define SO_ARG_REQUIRED  (0x10)  // type: char *, conversion: none
// #define SO_ARG_OPTIONAL  (0x20)

#define SO_ALIAS(x) (- x)

// Use the following to support literal defintion of an option_t;
#define  SO_SWITCH_ADD(i, e, l, s, t)  { i, e, l, s, t }
// should I have more like
//#define  SO_SWITCH_...
#define  SO_SWITCH_END()  { 0 }


extern char *optarg;   //current argument value under consideration: option_spec.value   ---TYPE is WRONG;
extern int optind;     //the next index into argv 
extern int optopt;     // used when an unknown option is obtained  '?' -- optopt
                       // does not appear to be a equivalent for long options
                       // we should return 0 for the index, and fill in the long or short name
                       //      or change this to char *
                       //     but here we need to change the processing... becuase we can have multiple errors to report

extern int opterr;     // enables or disables error messages
extern int optreset;   // set to 1 to start from the beginning


typedef struct _option_t {
  size_t           index;       // o_BIND  -- same as 'val' in popt
                   // index != 0, secondary recorde: index < 0 
                   // reserve index for backward compatbility ?
                   //   ? (63), or : (58)

  char             *longName;   // "bind"
  char             shortName;   // 'b'
  int              argType;     // SO_ARG_REQUIRED
                                // SO_ARG_{type}
                                // SO_{}_DUPLICATE

  char             *envName;    // "SCGI_BIND"

  // Usage Informaion
  char             *description; // "The IP address of the server"
  char             *argForm;     // "<ip address>"

  // working data:  move to end?
  char *           value;        // the initial value (if not NULL) and final (non converted)
  int              count;        // update each time on the cli
                                 // needed for Error and first

} option_t;

// No need to have a separate variable for initial and final value
// This would ONLY be needed if we wanted to use the some argument list over.
// In practice you would reuse the getopt like function again, but it would 
// be against a separate set of argumentes
//   e.g.,    command -a -b -c  subcommand -a -b -c
//   e.g.,  one call form the command, and one call for the subcommand
//
///////////////////


extern option_t * find_option(int index, option_t *option_list[]);

extern void setoption_print_options();


// Secondary API
char * optionList2optString(option_t options_list);
  // generates the short form optstring
  // "b:c::d:e:fhl:p:s:e:w:vV"

struct option * optionList2longopt(option_t options_list);
  // generates the struct option as per the C api

char * optionList2longopt(option_t options_list);
  // generates the long form optstring as per the command line
  // "bind:,documents:foreground, ...."


