#include "netstring.h"

#define TRUE (0)
#define FALSE (!(TRUE))
#define min(a,b) ((a<=b)? a : b)
#define max(a,b) ((a<=b)? b : a)

/* Error Values:                                                      */
#define ERROR_INVALID_SIZE (1)
#define ERROR_MISSING_COLON (2)
#define ERROR_TRUNCATED_STRING (3)
#define ERROR_MISSING_TRAILING_COMMA (4)
#define ERROR_OTHER (5)

/*                                                                    */
/**********************************************************************/

static const char * error_msg[] = {
  "Netstring: SUCCESS",
  "Netstring Error: INVALID_SIZE (1)",
  "Netstring Error: MISSING_COLON (2)",
  "Netstring Error: TRUNCATED_STRING (3)",
  "Netstring Error: MISSING_TRAILING_COMMA (4)",
  "Netstring Error: OTHER (5)"

};

#define string_size(size)    ( ((size) == 0)?  netstring_max_length : (size)  )
#define array_size(count)    ( ((count) == 0)? netstring_max_strings : (count) )
#define buffer_size(size)    ( NETSTRING_PREAMBLE_MAX + size + NETSTRING_EPILOGUE_MAX )


/* A simple macro used to walk the pointer p through the buffer,  */
/* looking for the char immediately following the next NULL char  */
#define next_start(p) { for(; *p != '\0'; p++); p++; }

/* A simple macro used to test for an error, print the error,     */
/* and then return a NULL pointer                                 */
/* Used to make the code more readable                            */
#define return_error(b,v) if (b) { \
      fprintf(stderr, "%s\n", error_msg[v]); return; }


static int read_int(int fd, char *next);
static int fread_int(char *next, FILE *fp);
static void build_strings_array(NETSTRING *ns_p, int h_size);


static int netstring_init_read_mode = NETSTRING_INIT_READ_MIN_SIZE;
extern void netstring_set_init_read(size_t mode) {
   assert(mode > 0);
   assert(mode <= 2);
   netstring_init_read_mode = mode;
}

static int netstring_min_length = NETSTRING_MIN_LENGTH_DEFAULT;
extern void netstring_set_min_length(size_t num) {
   assert(num > 0);
   netstring_min_length = num;
}

static int netstring_max_length = NETSTRING_MAX_LENGTH_DEFAULT;
extern void netstring_set_max_length(size_t num) {
   assert(num > 0);
   return_error((num > NETSTRING_MAX_LENGTH_DEFAULT), ERROR_OTHER);

   netstring_max_length = num;
}

static int netstring_max_strings = NETSTRING_MAX_STRINGS_DEFAULT;
extern void netstring_set_max_strings(size_t num) {
   assert(num > 0);
   netstring_max_strings = num;
}



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

  // Create and prepend the preamble
  preamble_length = sprintf(preamble, "%zu:", ns_p-> strings_length);
  preamble_start = ns_p-> strings[0] - preamble_length;
  memcpy(preamble_start, preamble, preamble_length);

  // Append the epilogue
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
    fprintf(stderr, "Netstring WARNING:  call to netstring_append after netstring_end\n");
    fprintf(stderr, "Netstring WARNING:  implicitly calling\"netstring_resume\"\n");
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


// Writes a netstring to a File or Stream, respectively.
extern void netstring_write(int fd, NETSTRING *ns_p) {
  if (ns_p-> netstring == NULL) {
    fprintf(stderr, "Netstring Warning: calling netstring_write print to netstring_end\n");
    fprintf(stderr, "                   implicitly calling netstring_end\n");
    netstring_end(ns_p);
  }  
  write(fd,ns_p-> netstring,ns_p-> netstring_size);
  return;
}
extern void netstring_fwrite(NETSTRING *ns_p, FILE *fp){
  if (ns_p-> netstring == NULL) {
    fprintf(stderr, "Netstring Warning: calling netstring_fwrite print to netstring_end\n");
    fprintf(stderr, "                   implicitly calling netstring_end\n");
    netstring_end(ns_p);
  }
  fwrite(ns_p-> netstring, sizeof(char), ns_p-> netstring_size, fp);
  return;
}



// The Overall objective of this implementation of netstring
// is to support the SCGI protocol.  In this protocol, the
// contents of the SCGI message include: 
//   1. a netstring representing a set of CGI variables
//   2. the body of the HTTP request that was sent to a webserver
// 
// The netstring needs to be decoded by a SCGI server and transformed
// into an `environ`.  The SCGI server than invokes the requisite
// CGI program via a call to `exec`.  The CGI program expects the
// contents of the HTTP request body is present on stdin.
//
// Under this assumption
//   1. we need to take care to ONLY read the netstring
//   2. we need to minimize the overhead in netstring processing, etc.
//
// In this implementation, we offer several different implementation
// of using both read operations on file descriptors (int fd) and
// fread operations on streams (FILE *fp).
//
// The key drivers to these implementations is minimize the initial
// read of a file to obtain the size of the netstring.
//
// The smallest length of a netstring is 3 characters, which represents
// a netstring containing a single string of zero (0) length: `0:,`.
//
// The smallest length of a netstring used within the SCGI protocol is
// 29 characters.  This is because the encoded string must contain at
// least the following 26 characters:
//
//    C O N T E N T _ L E N G T H \0 . \0 S C G I \0 1 . 1 \0
//  
// The preamble of a 32-bit representation of a netstring is at most
// 11 characters. A 32-bit number can be represented using 10 decimal
// digits, and then we need one addition character for the colon ':'.
//
// As such, depending on constraints, the number of characters that can
// be safely retrieved is between 3 and 11.
//
// The following equations defines the number of characters to be
// read on the initial read via a file descriptor:
//
//    min( max( 3, strlen(min-string)), 11)
//
// The equation is evaluated to be:
//      3:  for an arbitrary netstring
//     11:  for an SCGI-compliant netstring
// 
// In general, we don't know the strlen of the minimal `string`.  If we,
// however, set a minimum string length, we can adjust the amount characters
// that can safely read via a file descriptor.
//
// Note that if we use streams (FILE *fp), the stream abstraction will
// perform appropriate buffering WITHOUT fear of over consuming file data.
// We still provided variations of implementation as part of our
// performance analysis and learning approach.


// Approaches
//   BRUTE:       read one character at a time, until ":"
//   MIN_SIZE:    fread(string, 1, size, fp) -- size is either 3 or 11 depending on suite
//                second trigger will need to examine PREABLE_MAX
//   PREAMBLE:    fscanf("%zu:", size)   -- only doe sprintf
//     - default, requires two reads


// Reads a netstring from the given file descriptor
extern void netstring_read(int fd, NETSTRING *ns_p) {
  size_t h_size;         // values from the preamble
  char   colon  = '\0';  // values from the preamble
  char   comma  = '\0';  // values from the preamble

  size_t to_read;        // number chars to read
  int    read_chars;     // number of chars returned
  int    residual;       // extra chars in preamble buffer
  char   *next_p;        // next location with the strings buffer

  assert(NETSTRING_MIN_READ_BUFFER < NETSTRING_PREAMBLE_MAX);

  // Read the PREAMBLE
  switch (netstring_init_read_mode) {
    case NETSTRING_INIT_READ_PREAMBLE:
      fprintf(stderr, "Netstring Warning: Use of the `read preamble` mode is not supported with FDs\n");
      fprintf(stderr, "                   Defaulting to `read brute` mode\n");

      // merge; break;


    case NETSTRING_INIT_READ_BRUTE:
      h_size = read_int(fd, &colon);
        return_error(!(h_size >=0),   ERROR_INVALID_SIZE);
        return_error((colon  != ':'), ERROR_MISSING_COLON);

      next_p = ns_p-> strings[0];
      to_read = h_size;
      break;

    case NETSTRING_INIT_READ_MIN_SIZE: 
      {
        char   preamble_buffer[NETSTRING_PREAMBLE_MAX+1];
        int    init_read_size;

        init_read_size = min(max(NETSTRING_MIN_READ_BUFFER, netstring_min_length), NETSTRING_PREAMBLE_MAX);

        read(fd, preamble_buffer, init_read_size);
        *(preamble_buffer + init_read_size) = '\0';

        // If no ":", we can safely read more into the preamble_buffer
        if (strchr(preamble_buffer, ':') == NULL) {
          read(fd, 
               preamble_buffer + init_read_size, 

               NETSTRING_PREAMBLE_MAX - init_read_size
               );
          init_read_size = NETSTRING_PREAMBLE_MAX;
        }

        h_size = (size_t) strtol(preamble_buffer, &next_p, 10);
          return_error( (*next_p != ':'), ERROR_MISSING_COLON);

        // In the preamble_buffer have ddddd:sssss
        //             preamble_buffer ^    ^
        //                           next_p |
        // residual is what is left in the buffer
        // preamble length is:   next_p - preamble_buffer + 1
        // residual is int_read_size + 1
        residual =  init_read_size - (next_p - preamble_buffer + 1);

        // copy the stuff after the ':'
        strncpy(ns_p-> strings[0], next_p+1, residual);

        next_p  = ns_p->strings[0] + residual;
        to_read =  h_size - residual;
      }
      break;

    default:
      assert(TRUE);
      break;
  }

  // read the rest of the strings and the EPILOGUE
  read_chars = read(fd, next_p, to_read + 1);
  comma  = *(ns_p-> strings[0] + h_size);
    return_error((read_chars != to_read + 1), ERROR_TRUNCATED_STRING);
    return_error((comma != ','), ERROR_MISSING_TRAILING_COMMA);

  build_strings_array(ns_p, h_size);
  ns_p-> strings_length = h_size;

  return;
}


// netstring_fread mimics netstring_read
// Reads a netstring from the given STREAM
extern void netstring_fread(NETSTRING *ns_p, FILE *fp) {
  size_t h_size;         // values from the preamble
  char   colon  = '\0';  // values from the preamble
  char   comma  = '\0';  // values from the preamble

  size_t to_read;        // number chars to read
  int    read_chars;     // number of chars returned
  int    residual;       // extra chars in preamble buffer
  char   *next_p;        // next location with the strings buffer


  assert(NETSTRING_MIN_READ_BUFFER < NETSTRING_PREAMBLE_MAX);

  // Read the PREAMBLE
  switch (netstring_init_read_mode) {
    case NETSTRING_INIT_READ_PREAMBLE:
      fscanf(fp, "%zu:", &h_size);

      next_p = ns_p-> strings[0];
      to_read = h_size;
      break;

    case NETSTRING_INIT_READ_BRUTE:
      h_size = fread_int(&colon, fp);
         return_error(!(h_size >=0), ERROR_INVALID_SIZE);
         return_error((colon  != ':'), ERROR_MISSING_COLON);

      next_p = ns_p-> strings[0];
      to_read = h_size;
      break;

    case NETSTRING_INIT_READ_MIN_SIZE:
      {
        char   preamble_buffer[NETSTRING_PREAMBLE_MAX+1];
        int    init_read_size;

        init_read_size = min(max(NETSTRING_MIN_READ_BUFFER, netstring_min_length), NETSTRING_PREAMBLE_MAX);

        fread(preamble_buffer, sizeof(char), init_read_size, fp);
        *(preamble_buffer + init_read_size) = '\0';

        // If no ":", we can safely read more into the preamble_buffer
        if (strchr(preamble_buffer, ':') == NULL) {
          fread(
                preamble_buffer + init_read_size, 
                sizeof(char),
                NETSTRING_PREAMBLE_MAX - init_read_size,
                fp);
          init_read_size = NETSTRING_PREAMBLE_MAX;
        }

        h_size = (size_t) strtol(preamble_buffer, &next_p, 10);
          return_error( (*next_p != ':'), ERROR_MISSING_COLON);

        residual =  init_read_size - (next_p - preamble_buffer + 1);

        // copy the stuff after the ':'
        strncpy(ns_p-> strings[0], next_p+1, residual);

        next_p  = ns_p->strings[0] + residual;
        to_read =  h_size - residual;
      }
      break;

    default:
      assert(TRUE);
      break;
  }

  // read the rest of the strings and the EPILOGUE
  read_chars = fread(next_p, sizeof(char), to_read + 1, fp);
  comma  = *(ns_p-> strings[0] + h_size);
    return_error((read_chars != to_read + 1), ERROR_TRUNCATED_STRING);
    return_error((comma != ','), ERROR_MISSING_TRAILING_COMMA);

  build_strings_array(ns_p, h_size);
  ns_p-> strings_length = h_size;

  return;
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



// Support functions to read a int, on char at a time
// via read and fread.
static int read_int(int fd, char *next_char) {
  // reads a number from the file descriptor (fd)
  // returns
  //   - the value of the number
  //   - updates the next_char parameter
  int  number = 0;
  char digit;

  read(fd, &digit, 1);
  while ( digit >= '0' && digit <= '9') {
    number = number * 10 + ( digit - '0');
    read(fd, &digit, 1);
  }

  *next_char   = digit;
  return  number;
}



static int fread_int(char *next_char, FILE *fp) {
    // reads a number from the file descriptor (fd)
    // returns
    //   - the value of the number
    //   - updates the next_char parameter

   int  number = 0;
   char digit;

   fread(&digit, 1, 1, fp);
   while ( digit >= '0' && digit <= '9') {
     number = number * 10 + ( digit - '0');
     fread(&digit, 1, 1, fp);
   }

   *next_char   = digit;
   return  number;
}

