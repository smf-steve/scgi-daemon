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




#define  array_size(count)    ( ((count) == 0)?  NETSTRING_ARRAY_MAX : (count) )
#define  string_size(size)    ( ((size) == 0)? NETSTRING_CSTRING_MAX : (size)  )




// Calculate the buffer_size need to store a netstring in total.
// This calculation is based upon the anticipated number of 
//   strings of a given unified length.
// Additional space is added for the netstring's PREAMBLE and EPOLOGUE.
static size_t buffer_size(size_t count, size_t str_size) {
  // if either count or str_size is 0, the associated MAX is used.

  int value;

  count    = array_size(count);
  str_size = string_size(str_size);

  value    = NETSTRING_PREAMBLE_MAX;
  value   += count * str_size;
  value   += NETSTRING_EPILOGUE_MAX; 

  return value;
}



// #define netstring_allocate(count, size)  netstring_start(count, size)
extern NETSTRING *netstring_start(size_t count, size_t str_size) {  
  // Returns a pointer to a initialized NETSTRING data structure
  //   Sufficient space is allocated for a data structure containing
  //   'count' strings each of size 'string_size'.

  //    If either count or size is 0, uses the default value

  // Intitialize the metadata
  NETSTRING *p        = (NETSTRING *) malloc(sizeof(NETSTRING));

  p-> buffer_asize   = buffer_size(count, str_size);
  p-> buffer         = (char *) malloc(p-> buffer_asize);

  p-> strings        = (char **) malloc(sizeof(char *) * (count+1));
  p-> strings_asize  = count;

  netstring_restart(p);
  return p;
}


extern void netstring_restart(NETSTRING *p) {
  // Reuses the metadata with the NETSTRING
  // while reusing the underlying allocated data
  p-> netstring      = NULL;
  p-> netstring_size = 0;

  p-> strings[0]     = (p-> buffer) + NETSTRING_PREAMBLE_MAX;
  p-> strings_count  = 0;
  p-> strings_length = 0;

  return;
}


extern void netstring_end(NETSTRING *p) {
  // Finalizes data of a netstring
  // Adds the preamble ( <size> ":" ) and epologue ( ",")
  // Caps the arrays[] structure  (arrays[n] = NULL)
  // Sets the final value of netstring and netstring_length

  char   preamble[NETSTRING_PREAMBLE_MAX];
  size_t preamble_length;
  char   *preamble_start;

  // Create and prepand the preamble
  preamble_length = sprintf(preamble, "%zu:", p-> strings_length);
  preamble_start = p-> strings[0] - preamble_length;
  memcpy(preamble_start, preamble, preamble_length);

  // Append the epologue
  {
    char *temp = p-> strings[p-> strings_count];

    *temp = ',';
    *(temp + 1) = '\0';
  }

  // End the strings[] data structure
  p-> strings[p-> strings_count] = NULL;  

  // Mark the structure as fully defined by setting the netstring value
  p-> netstring      = preamble_start;
  p-> netstring_size = preamble_length + p->strings_length + 1;

  return;
}

extern void netstring_resume(NETSTRING *p) {

  // Resume location location of strings[] data structure
  p-> strings[p-> strings_count] = p->strings[0] + p->strings_length + 1;

  // Mark the structure as not being fully defined by setting the netstring value
  p-> netstring      = NULL;
  p-> netstring_size = 0;
}


extern void netstring_free(NETSTRING *p) {
  // deallocates the internal NETSTRING data structure
  free(p-> buffer);
  free(p-> strings);
}


extern size_t netstring_append(NETSTRING *p, char *str, size_t len) {
  // Appends the 'str'ing of length 'len' to the NETSTRING
  // if 'len' is zero, the length of the string is computed

  int  count;
  char *temp;  // a pointer at the end of the strings == strings[count]

  if (str == NULL) { str = ""; }
  if (len == 0) { len = strlen(str); }

  count = p->strings_count;              // Determine the location to place the string 
  temp = p->strings[count];              // based upon working count

  strncpy(temp, str, len);               // Copy the string to the buffer
  temp += len;  *temp = '\0';            // Append the NULL separator

  p->strings_length += len + 1;          // Update the total length

  count ++; temp++;                      // Update the working count and location
  p->strings[count] = temp;                
  p->strings_count  = count;
  
  assert(temp < p-> buffer + p-> buffer_asize);
  assert(p-> strings_count < p-> strings_asize);

  return (len + 1);
}

// Writes a netstring to a File or Stream, respectively.
extern void netstring_write(int fd, NETSTRING *p) {

  write(fd, p-> netstring, p-> netstring_size);
  return;
}
extern void netstring_fwrite(NETSTRING *p, FILE *fp){

  fwrite(p-> netstring, 1, p-> netstring_size, fp);
  return;
}


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


/* A simple macro used to walk the pointer p through the buffer,  */
/* looking for the char immediately following the next NULL char  */
#define next_start(p) { for(; *p != '\0'; p++); p++; }


/* A simple macro used to test for an error, print the error,     */
/* and then return a NULL pointer                                 */
/* Used to make the code more readable                            */
#define return_error(b,v) if (b) { \
      fprintf(stderr, "%s\n", error_msg[v]); return; }

extern void netstring_read(int fd, NETSTRING *p) {
  int  retval;
  char h_size;
  char colon  = '\0';

  /* Syntax:    P ->    <h_size> ":" <header> "," <body>         */
  /*                                                              */
  /*   Read the <h_size> and the ":".                             */
  /*   Place the header and the "," into the buffer               */
  /*   Leave the body on stdin.                                   */
  { 
    h_size = read_int(fd, &colon);                    return_error(!(h_size >=0), ERROR_INVALID_SIZE);
                                                      return_error((colon  != ':'), ERROR_MISSING_COLON);

    retval = read(fd, p->strings[0], h_size + 1);     return_error((retval != h_size+1), ERROR_TRUNCATED_STRING);
    retval = *(p->strings[0]+ h_size);                return_error((retval != ','), ERROR_MISSING_TRAILING_COMMA);    
  } 


  /* Syntax:    <header> ->  "CONTENT_LENGTH" '\0' <_value> '\0' */
  /*                          ( _name '\0' _value '\0' )+         */
  /*                                                              */
  /*   Walk the buffer to create an array of strings              */
  /*   Ensure the first _name is "CONTENT_LENGTH"                 */
  {
    char *p_start, *p_end;    // Walker pointers 
    int count = 0;             

    p_start = p->strings[0];
    p_end = p_start + h_size;
    
    while (p_start < p_end) {
      next_start(p_start);
      count ++;
      p->strings[count] = p_start;
    }

    assert( *p_end == ',');    // We should be right on the end of the netstring
                               // we check the protocol error above

    p->strings[count] = NULL;
    p->strings_count = count;

  }

  p->strings_length = h_size;
  netstring_end(p);

  return;
}


extern void netstring_fread(NETSTRING *p, FILE *fp);
  // Reads a netstring from a File or Stream, respectively.
  // Updates the NETSTRING data structure

// Save for later



