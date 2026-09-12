#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct _NETSTRING {
  char   *netstring;          // NULL until fully defined
  size_t netstring_size;      // Zero until fully defined
  size_t strings_length;      // Total length of strings within the netstring
  //
  size_t strings_count;       // Number of strings within th netstring
  //
  char   *buffer;             // The `buffer` that contains the netstring
  size_t buffer_asize;        // Allocated size of the buffer
  //
  char   **strings;           // A vector of pointers to the individual 
                              // strings within the netstring
  size_t strings_asize;       // Allocated size of strings[]
  //
} NETSTRING;


// Example of a fully defined netstring containing three strings: "hello", " ", "there"
//
// netstring = < '1' '4' ':' 'h' 'e' 'l' 'l' 'o' '\0' ' ' '\0' 't' 'h' 'e' 'r' 'e' '\0' ','>
// netstring_size   =  18 (14 + 4)
//    preamble : "13:"
//    prologue : ","
// strings_length   =  14
// string_count     =   3
// strings_array[0] =  "hello"
// strings_array[1] =  " "
// strings_array[2] =  "there"
// strings_array[3] =  NULL


// General order of operations associated with netstrings
//
// Example 1:  Building and Sending a netstring
//
//   netstring_start()
//   do {
//     netstring_append()
//   } while ()
//   netstring_append()
//   netstring_end()
//   // Oops! Not quite done building the netstring
//   netstring_resume()
//   do {
//     netstring_append()
//   } while ()
//   netstring_end()
// netstring_[f]write() 
// netstring_free()
//

// Example 2:  Receiving a netstring
//   netstring_allocate()
//   netstring_[f]read()
//   netstring_free()

// Example 3: Receiving and then appending to a netstring
//   netstring_allocate()
//   netstring_[f]read()
//   netstring_resume()
//   do
//     netstring_append()
//   while ()
//   netstring_end()


#ifndef LINE_MAX
#  define LINE_MAX (1024)
#endif
#ifndef NETSTRING_CSTRING_MAX
#  define NETSTRING_CSTRING_MAX (LINE_MAX)
#endif
#ifndef NETSTRING_ARRAY_MAX
#  define NETSTRING_ARRAY_MAX (100)
#endif
#ifndef NETSTRING_EPILOGUE_MAX
#  define NETSTRING_EPILOGUE_MAX (1)
#endif
#ifndef NETSTRING_PREAMBLE_MAX
#  define NETSTRING_PREAMBLE_MAX (11)
#endif
 


#define netstring_allocate(count, size)  netstring_start(count, size)
extern NETSTRING *netstring_start(size_t count, size_t size);  
  // Allocates space for a NETSTRING data structure
  // for 'count' strings of total size of 'string_size'
  // If either count or size is 0, uses the default value
  // Intitialize the metadata


extern void netstring_restart(NETSTRING *ns_p);
  // Resets the associated data for a netstring
  // while reusing the underlying allocated data


extern void netstring_end(NETSTRING *ns_p);
  // Finalizes data of a NETSTRING
  // Adds the preamble ( <size> ":" ) and epologue ( ",")


extern void netstring_resume(NETSTRING *ns_p);
  // Effectively undoes the `netstring_end` operation


extern void netstring_free(NETSTRING *ns_p);
  // deallocates the internal NETSTRING data structure


extern size_t netstring_append(NETSTRING *ns_p, char *str, size_t len);
  // Appends the 'str'ing of length 'len' to the NETSTRING
  // if 'len' is zero, the length of the string is computed


extern void netstring_read(int fd, NETSTRING *ns_p);
extern void netstring_fread(NETSTRING *ns_p, FILE *fp);
  // Reads a netstring from a File or Stream, respectively.
  // Updates the NETSTRING data structure


extern void netstring_write(int fd, NETSTRING *ns_p);
extern void netstring_fwrite(NETSTRING *ns_p, FILE *fp);
  // Writes a netstring to a File or Stream, respectively.



