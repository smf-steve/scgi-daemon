#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//Add asserts for *_asize 
//
//(* blah)
//or 
//*( blah)  <--

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
 
// Example of a fully defined netstring containing three strings: "hello", " ", "there"
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

typedef struct _NETSTRING {
  char   *netstring;          // NULL until fully defined
  size_t netstring_size;      // Zero until fully defined
  size_t strings_length;      // Total length of strings within the netstring
                              // Also the size in the netstring preamble
  size_t strings_count;       // Number of strings within the netstring
  //
  char   *buffer;             // The `buffer` that contains the netstring
  size_t buffer_asize;        // Allocated size of the buffer
  //
  char   **strings;           // A vector of pointers to the individual strings
  size_t strings_asize;       // Allocated size of strings[]
  //
} NETSTRING;



#define netstring_allocate(count, size)  netstring_start(count, size)
extern NETSTRING *netstring_start(size_t count, size_t size);  
  // Allocates space for a NETSTRING data structure
  // for 'count' strings of total size of 'string_size'
  // If either count or size is 0, uses the default value
  // Intitialize the metadata


extern void netstring_restart(NETSTRING *strings_p);
  // Resets the associated data for a netstring
  // while reusing the underlying allocated data


extern void netstring_end(NETSTRING *strings_p);
  // Finalizes data of a NETSTRING
  // Adds the preamble ( <size> ":" ) and epologue ( ",")

extern void netstring_resume(NETSTRING *strings_p);
  // Effectively undoes the `netstring_end` operation

extern void netstring_free(NETSTRING *strings_p);
  // deallocates the internal NETSTRING data structure


extern size_t netstring_append(NETSTRING *strings_p, char *str, size_t len);
  // Appends the 'str'ing of length 'len' to the NETSTRING
  // if 'len' is zero, the length of the string is computed



extern void netstring_read(int fd, NETSTRING *strings_p);
extern void netstring_fread(NETSTRING *strings_p, FILE *fp);
  // Reads a netstring from a File or Stream, respectively.
  // Updates the NETSTRING data structure


extern void netstring_write(int fd, NETSTRING *strings_p);
extern void netstring_fwrite(NETSTRING *strings_p, FILE *fp);
  // Writes a netstring to a File or Stream, respectively.


// General order of operations:
//
// Example 1:  Building and Sending a netstring
//   netstring_start();
//   do 
//     netstring_append();
//   while ();
//   netstring_append();
//   netstring_end();
//   // Oops! Not quite done building the netstring
//   netstring_resume()
//   do 
//     netstring_append();
//   while ();
//   netstring_end()
// netstring_write()
// netstring_free()
//

// Example 2:  Receiving a netstring
//   netstring_allocate()
//   netstring_read()
//   netstring_free()

// Example 3: Receiving and then appending to a netstring
//   netstring_allocate()
//   netstring_read()
//   netstring_resume()
//   do
//     netstring_append()
//   while ();
//   netstring_end()


