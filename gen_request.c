#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* The following C program generated an example SCGI request. This example    */
/* is pulled directly from the specification of the SCGI protocol definition: */
/*  http://python.ca/scgi/protocol.txt                                        */
/*                                                                            */
/*                                                                            */
/* "70:"                                                                      */
/*    "CONTENT_LENGTH" <00> "27" <00>                                         */
/*    "SCGI" <00> "1" <00>                                                    */
/*    "REQUEST_METHOD" <00> "POST" <00>                                       */
/*    "REQUEST_URI" <00> "/deepthought" <00>                                  */
/* ","                                                                        */
/* "What is the answer to life?"                                              */
/*                                                                            */

#define emit(string) printf("%s", string)
#define emit_null()  printf("%c", '\0')

int main() {

  emit("70:");
  emit("CONTENT_LENGTH"); emit_null(); emit("27"); emit_null();
  emit("SCGI");           emit_null(); emit("1"); emit_null(); 
  emit("REQUEST_METHOD"); emit_null(); emit("POST"); emit_null();
  emit("REQUEST_URI");    emit_null(); emit("/deepthought"); emit_null();
  emit(",");
  emit("What is the answer to life?");
  exit(0);
}

