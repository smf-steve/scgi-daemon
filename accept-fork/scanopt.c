// Name:  scanopt:  short for scan options
//        switchscan
// Nomenclature:
//    switch
//    flag, banner
//    



/*
Major difference is our versions popt..
  - we don't try to keep the return value as the shortName char
    - we return the INDEX value which is more flexible
  - we don't try to store info into a varialbe
    - we allow/require the user to put the process value there...
*/


// PREMISE:  an option set via an env is just like setting on the cli
//          but this allows for CONFIG file to be used
//          but this also allows simplier control per user


// Issues to handle

//  How to deal with duplicates
//  Argument type, which includes optional


/*
//use cases

// initial value AND Default value;
--debug[=n]   -- set the debug level to n, otherwise set it to 1.
              -- set value to "0", set default to "1"

// from htpd
O_APPEND_DUPLICATE           
-C directive // before reading config
-f config    // Example here is httpd
-c directive // after reading config

// Opposities
How do deal with opposites.

We could add a flag variable  to allow opsites,  witch is what speed is belows
  e.g., -i and -f are opposites
from popt

{ "interactive", 'i', POPT_ARG_VAL, &speed, 0,  // default
{ "force",       'f', POPT_ARG_VAL, &speed, 1,

// Alais

OPTION_ALIAS(o_SOCKET, "", "", '');

{  o_SOCKET,  "", "socket", 's', OPT_REQUIRED}   
{- o_SOCKET,  "", "uds",    '',  OPT_REQUIRED}
  -- ignore the - index extra values, [ensure they are not set ?]
  -- the arg type must be the same: e.g., OPT_REQUIRED
  -- 



// Opposites
{   o_FORCE,  "", "interactive", 'i', OPT_SET,    // Primary
{ - o_FORCE,  "", "force",       'f', OPT_UNSET,
  // used to support opposite
  // OPT_SET causes the primary.value to be a "1"
  // OPT_UNSET causes the primary.value to be a "0"

OPTION_OPPOSITE(o_FORCE, "SCGI_FORCE", OP_SET,   "", '',  \\
                                       OP_UNSET, "", '' )
  // ONLY ONE can have an ENV
  SCGI_FORCE=set
  SCGI_FORCE=unset
  SCGI_FORCE=   // akin to =set
*/




/* Conversion thoughts
    // don't use reg expressions, to compilicated, etc. for one option
    // ip address:  "%d.%d.%d.%d"
    // count = sscanf(str_val, "%d.%d.%d.%d%c", &a, &b, &c, &d, extra);
    // where count == 4
    // would need to use vsscanf after building the valist appropriately
    //
    // above example shows why it might be more work than its work (mwtiw)
    // just support the basic types of the langue

int variable = convert(&var, o_BIND, option_t *option_list);
    // convert :==
    //    option_t *item;
    //    int count;
    //    item = find_option(O_BIND, option_list);
    //    str_value = item.value;
    //    switch (item.ARG_TYPE) {
    //        case OPT_ARG_INT:  
    //           count = sscanf(item.value, "%d%c", (void *) var, extra);
    //           if (count != 1)  error
    //           return 
    //        ...

popt and getop can do the conversions because it is ONLY for ints

*/


// APIs associated with option definition and translation

char *opt_list scanopt_switchNew(int index, char *envName, char *longName, char shortName, int argType);
void scanopt_add_switchOptions(char *opt_list handle, char *initVal, char *defaultVal, int limit, int onDuplicate);
void scanopt_add_switchDescription(char *opt_list handle, char *description, char *argDescript);

static char **  normalize_commandline(int argc,  char *argv[]);
   // returns an argv like structure but with the condition
   // that all strins are lined up
   // you can convert to a string by replacing all \0 with ' ';
   // note the corresponding argc of the new structure will need to 
   // be computed

   // modifies the command line to
   //  - separate single letter options 
   //    *  -ltc<blah> ==>   -l -t -c<blah>
   //  - expand single letter options to equivalent long forms
   //    * -d --> --debug
   //  - connect required values
   //    *  --require <blah> ==>  --require=<blah>
   //  - connect options values when they exist
   //    *  --optional <blah>  ==> --optional=<blah>
   //  - dont'connect options values when they don't exist
   //    *  --optional <blah>  ==> --optional <blah>

// should we consider if this is done by the shell
// i.e., turn this into a utilty
// how would parsing be handled
//    1. get opts would work
//    1. loop to detect option would be

// ```bash
// case (option) {
//    long_arg=${option/#--*=/}
//    short_arg=${option/#-?/}
//   case --banner=* )
//      break;
// 
//   case -l* )
//      break;
// 
// 
// }
// ```


static int inititalize(option_t *option_list[]);
  // update opt_list with any global settings
  //    set the initial value -- not needed??

static sanity_check(option_t *option_list[]);
   // index is unique with option_list
   // only one positive INDEX
   // check for opposistes OPT_SET, OPT_UNSET, no other records

static int load_env(option_t *option_list[]);
  // for each with "envName"
  //   case
  //      required_multiple: copy arg to value
  //      required:          copy arg     to value
  //      optional_present:  copy arg     to value
  //      optional_missing:  copy default to value
  //      none:              nothing
  //   increment usageCount


static int walk_env(int argc, char argv[]option_t *option_list[]);
  // use the env to update the option_list
  // for each:
  //   check for multiple use (> limit), etc.
  //   case
  //      required_multiple: append arg to value
  //      required:          copy arg     to value
  //      optional_present:  copy arg     to value
  //      optional_missing:  copy default to value
  //      none:              nothing
  //   increment usageCount


 // This is called within scanopt_next, but is shown here
  // 1. to remember to normalize the string
  // 2. to illustrate what is going on
  //argv = scanopt_normalize(&argc, argv);

  ////////////////////////////////////////
  // Normalize the command line options
  //   1. expand all condense options
  //   1. autocomplete all longform options
  //   1. merge all options with there values
  //   1. insert -- after all options
  //   1. place option in order
  //      - update the usage count
  //      - print usage on error
  //
  // o::
  //   -o,       -o"",   -ovalue, -o value
  //   -o,       -o,     -ovalue, -o value  -- but seen via bash
  //   NULL      NULL    "value"  "value"

  // option optional_argument
  //   --option, --option=, --option=value,
  //   NULL      ""         "value"
  //   NULL      NULL       "value"         --but viewed via bash



extern option_t * find_option(int index, option_t *option_list[]);

extern int scan_option(int argc, char * const argv, 
                       option_t *option_list[], option_t ** item);
       // returns the index of othe option, i.e., o_BIND
       // modifies "item" to point to the current options item
       //   you could call find_option instead

       // incremently called to return all options 
       //   even those with a (value == NULL)
       // in one case, the programmer should have provide initial values
       // but if not they should do it during this scan


 /* 
  Process:
    0. define the structure [passed as a paramater]  
    1. normailize the command line
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

       b. if no item or error?  (error)
          - use the  option_t * error_1 = {O_ERROR1, "", '?', ""}
          -          option_t * error_2 = {O_ERROR2, "", ':', ""}
          - item.count ++
          - item.value = arv.value (as appropriate)
          * semantically equivalent to 
             SO_ADD(o_ERROR1, "", ':', "") 
             SO_ADD(o_ERROR2, "", '?', "") 

          *  upon next call to scan_option, come here

    5. walk the structure, using item

       a. item_index ++;
       b. optarg = item.value
       c. update other optarg stuff
       d. update item pointer
       e. return item.index

       * upon next call to scan_option, come here

*/



void setoption_print_options(){
  for (int i = 0; option_list[i].name != NULL; i++) {
     item = option_list[i];

     // print the short form
     if (item.short_form != '\0' || item.long_form != NULL ) {
       fprintf("\t");
     } else {
       assert(FALSE);  // it must have a long or short form
     }

     if (item.short_form != '\0') {
       fprintf(stdout, "-%c", item.short_form);
       if (item.arg_name != NULL) {
         assert(item.option_type != no_argument);
         if (item.option_type == optional_argument) {
          fprintf(stdout, "[");
         }
         fprintf(stdout, " %s", item.arg_name);
         if (item.option_type == optional_argument) {
           fprintf(stdout, "]");
         }
       }
       if (item.long_form != NULL) {
         fprintf(stdout, ", ");
       }
     }
     // print the long form
     if (item.long_form != NULL) {
       fprintf(stdout, "-%s", item.long_form);
       if (item.arg_name != NULL) {
         assert(item.option_type != no_argument);
         if (item.option_type == optional_argument) {
          fprintf(stdout, " [");
         }
         fprintf(stdout, "=%s", item.arg_name);
         if (item.option_type == optional_argument) {
           fprintf(stdout, "]");
         }
       }
     }
     fprint(stdout, "\n");
     fprintf(stdout, "\t\t%s", item.description);
     if (item.value != NULL) {
       fprintf(stdin, "\t\tDEFAULT: %s\n", item.value);
     }
     if (item.env_name != NULL) {
       fprintf(stdin, "\t\tENVIRONMENT VARIABLE: %s\n", item.env_name);
     }
     fprintf(stdout, "\n");
  }
  return;
}


setoption_setenv_options() {
  struct option_item *item;

  for (int i = 0; option_list[i].name != NULL; i++) {
    item = option_list[i];

    if (item.env_name != NULL);
      value = getenv(item.env_name);
      if (value != NULL) {
        item.value = value;
      }
  }
}


Maybe these helper APIs

true/false = scanopt_name("--conf=", argv[1]);
  // returns or if there argv[1]

// Issue: at this point the command line has not been normalized.
// So normalize the first one.
// I need to know that it is required

    scanopt_required_value(1);
       // assumes, thruns scanopt_name, if true, obtains the required value

    scanopt_optional_value(1);
       // runs scanopt_name, if true, 

   // sees the next option has the corresponding name

value      = scanopt_recValue("--conf", "-c", 1);
   // returns the required value
   // returns NULL if scanopt_name would return false

value      = scanopt_optValue("--conf", "-c", 1);
   // option must be connected
   // returns NULL


