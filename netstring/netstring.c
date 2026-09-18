#include "netstring.h"

/* Error Values:                                                      */
#define ERROR_SUCCESS (0)
#define ERROR_INVALID_SIZE (1)
#define ERROR_MISSING_COLON (2)
#define ERROR_TRUNCATED_STRING (3)
#define ERROR_MISSING_TRAILING_COMMA (4)
#define ERROR_OTHER (5)

/*                                                                    */
/**********************************************************************/

static const char * error_msg[] = {
  "Netstring Error: SUCCESS (0)",
  "Netstring Error: INVALID_SIZE (1)",
  "Netstring Error: MISSING_COLON (2)",
  "Netstring Error: TRUNCATED_STRING (3)",
  "Netstring Error: MISSING_TRAILING_COMMA (4)",
  "Netstring Error: OTHER (5)"

};



static int netstring_min_length  = NETSTRING_MIN_LENGTH_DEFAULT;
static int netstring_max_length  = NETSTRING_MAX_LENGTH_DEFAULT;
static int netstring_max_strings = NETSTRING_MAX_STRINGS_DEFAULT;


// if cli value, then update the above values
// if env value, then update these values



#define string_size(size)    ( ((size) == 0)?  netstring_max_length : (size)  )
#define array_size(count)    ( ((count) == 0)? netstring_max_strings : (count) )
#define buffer_size(size)    ( NETSTRING_PREAMBLE_MAX + size + NETSRING_EPILOGUE_MAX )



// Alias for netstring_allocate(count, size)
extern NETSTRING *netstring_start(size_t count, size_t str_size) {  
  // Creates a NETSTRING data structure with internal
  //   storages for proper management
  // Initializes the metadata associated with an undefined netstring
  NETSTRING *ns_p        = (NETSTRING *) malloc(sizeof(NETSTRING));

  count                 = array_size(count);
  str_size              = string_size(str_size);

  ns_p-> buffer_asize   = buffer_size(str_size);
  ns_p-> buffer         = (char *) malloc(ns_p-> buffer_asize);

  ns_p-> strings        = (char **) malloc(sizeof(char *) * (count+1));
  ns_p-> strings_asize  = count;

  netstring_restart(ns_p);
  return ns_p;
}


extern void netstring_restart(NETSTRING *ns_p) {
  // Resets the metadata with the NETSTRING to 
  // represent an undefined netstring, while 
  // reusing the underlying allocated data
  ns_p-> netstring      = NULL;
  ns_p-> netstring_size = 0;

  ns_p-> strings[0]     = (ns_p-> buffer) + NETSTRING_PREAMBLE_MAX;
  ns_p-> strings_count  = 0;
  ns_p-> strings_length = 0;

  return;
}


extern void netstring_end(NETSTRING *ns_p) {
  // Finalizes data of a netstring
  // Adds the preamble ( <size> ":" ) and epologue ( ",")
  // Caps the arrays[] structure  (arrays[n] = NULL)
  // Sets the final value of netstring and netstring_length

  char   preamble[NETSTRING_PREAMBLE_MAX];
  size_t preamble_length;
  char   *preamble_start;

  // Create and prepand the preamble
  preamble_length = sprintf(preamble, "%zu:", ns_p-> strings_length);
  preamble_start = ns_p-> strings[0] - preamble_length;
  memcpy(preamble_start, preamble, preamble_length);

  // Append the epologue
  {
    char *temp = ns_p-> strings[ns_p-> strings_count];

    *temp = ',';
    *(temp + 1) = '\0';
  }

  // End the strings[] data structure
  ns_p-> strings[ns_p-> strings_count] = NULL;  

  // Mark the structure as fully defined by setting the netstring value
  ns_p-> netstring      = preamble_start;
  ns_p-> netstring_size = preamble_length + ns_p-> strings_length + 1;

  return;
}

extern void netstring_resume(NETSTRING *ns_p) {
  // Effectively, undoes the netstring_end operation
  // Allows for addition strings to be appended to the netstring

  // Resume location location of strings[] data structure
  ns_p-> strings[ns_p-> strings_count] = ns_p-> strings[0] + ns_p-> strings_length + 1;

  // Mark the structure as not being fully defined by setting the netstring value
 ns_p-> netstring      = NULL;
 ns_p-> netstring_size = 0;
}


extern void netstring_free(NETSTRING *ns_p) {
  // deallocates the internal NETSTRING data structure
  free(ns_p-> buffer);
  free(ns_p-> strings);
}


extern size_t netstring_append(NETSTRING *ns_p, char *str, size_t len) {
  // Appends the 'str'ing of length 'len' to the NETSTRING
  // if 'len' is zero, the length of the string is computed

  int  count;
  char *temp;  // a pointer at the end of the strings == strings[count]

  if (str == NULL) { str = ""; }
  if (len == 0) { len = strlen(str); }

  count = ns_p-> strings_count;          // Determine the location to place the string 
  temp  = ns_p-> strings[count];         // based upon working count

  if (temp == NULL) {
    // implicitly call netstring_resume
    fprintf(stderr, "WARNING: netstring: call to netstring_append after netstring_end, implicitly call \"netstring_resume\"\n");
    netstring_resume(ns_p);
  }
  strncpy(temp, str, len);               // Copy the string to the buffer
  temp += len;  *temp = '\0';            // Append the NULL separator

  ns_p-> strings_length += len + 1;      // Update the total length

  count ++; temp++;                      // Update the working count and location
  ns_p-> strings[count] = temp;                
  ns_p-> strings_count  = count;
  
  assert(temp < ns_p-> buffer + ns_p-> buffer_asize);
  assert(ns_p-> strings_count < ns_p-> strings_asize);

  return (len + 1);
}


extern void netstring_write(int fd, NETSTRING *ns_p) {
  // Writes a netstring to a File or Stream, respectively.
  write(fd,ns_p-> netstring,ns_p-> netstring_size);
  return;
}
extern void netstring_fwrite(NETSTRING *ns_p, FILE *fp){

  fwrite(ns_p-> netstring, 1,ns_p-> netstring_size, fp);
  return;
}


/* A simple macro used to walk the pointer p through the buffer,  */
/* looking for the char immediately following the next NULL char  */
#define next_start(p) { for(; *p != '\0'; p++); p++; }


/* A simple macro used to test for an error, print the error,     */
/* and then return a NULL pointer                                 */
/* Used to make the code more readable                            */
#define return_error(b,v) if (b) { \
      fprintf(stderr, "%s\n", error_msg[v]); return; }


static int read_int(int fd, char *next) {
    // reads from the file descriptor (fd)
    // - a number 
    // - the next char
    // returns the next char from the file descriptor

   int  number = 0;
   char digit;

   read(fd, &digit, 1);
   while ( digit >= '0' && digit <= '9') {
     number = number * 10 + ( digit - '0');
     read(fd, &digit, 1);
   }

   *next   = digit;
   return  number;
}


static void build_strings_array(NETSTRING *ns_p, int h_size) {
  char *start_p, *end_p;    // Walker pointers 
  int count = 0;             

  start_p = ns_p-> strings[0];
  end_p   = start_p + h_size;
  
  while (start_p < end_p) {
    next_start(start_p);
    count ++;
   ns_p-> strings[count] = start_p;
  }
  ns_p-> strings_count = count;

  assert( *end_p == ',');    // We should have the final ',' per the netstring protocol
  return;
}


static int fread_int(FILE *fp, char *next) {
    // reads from the file descriptor (fd)
    // - a number 
    // - the next char
    // returns the next char from the file descriptor

   int  number = 0;
   char digit;

   fread(&digit, 1, 1, fp);
   while ( digit >= '0' && digit <= '9') {
     number = number * 10 + ( digit - '0');
     fread(&digit, 1, 1, fp);
   }

   *next   = digit;
   return  number;
}


extern void netstring_read(int fd, NETSTRING *ns_p) {
  // Reads a netstring from the given file
  int  retval;
  int h_size;
  char colon  = '\0';

  /* Syntax:    P ->    <h_size> ":" <header> "," <body>          */
  /*                                                              */
  /*   Read the <h_size> and the ":".                             */
  /*   Place the header and the "," into the buffer               */
  /*   Leaves the body on stdin.                                  */
  { 
    h_size = read_int(fd, &colon);                        return_error(!(h_size >=0), ERROR_INVALID_SIZE);
                                                          return_error((colon  != ':'), ERROR_MISSING_COLON);

    retval = read(fd, ns_p-> strings[0], h_size + 1);     return_error((retval != h_size+1), ERROR_TRUNCATED_STRING);
    retval = *(ns_p-> strings[0]+ h_size);                return_error((retval != ','), ERROR_MISSING_TRAILING_COMMA);    
  } 

  build_strings_array(ns_p, h_size);
  
  ns_p-> strings_length = h_size;

  return;
}


extern void netstring_fread(NETSTRING *ns_p, FILE *fp) {
  // Reads a netstring from the given STREAM
  int  retval;
  int h_size;
  char colon  = '\0';

  /* Syntax:    P ->    <h_size> ":" <header> "," <body>          */
  /*                                                              */
  /*   Read the <h_size> and the ":".                             */
  /*   Place the header and the "," into the buffer               */
  /*   Leaves the body on stdin.                                  */
  { 
    h_size = fread_int(&colon, fp);                       return_error(!(h_size >=0), ERROR_INVALID_SIZE);
                                                          return_error((colon  != ':'), ERROR_MISSING_COLON);

    retval = fread(ns_p-> strings[0], h_size + 1, 1, fp); return_error((retval != h_size+1), ERROR_TRUNCATED_STRING);
    retval = *(ns_p-> strings[0]+ h_size);                return_error((retval != ','), ERROR_MISSING_TRAILING_COMMA);    
  } 

  build_strings_array(ns_p, h_size);

  ns_p-> strings_length = h_size;

  return;
}
