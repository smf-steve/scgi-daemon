//

// DECODING subroutines
extern char**  scgi_netstring_decode(int fd);
     // fd: file descriptor, from to read the netstring
     // return: an envp
     // not allocates space that must freede freed

extern void scgi_netstring_free(char ** envp);
  // deallocates then envp structure

