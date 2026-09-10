# Netstring

# Description: 

  This directory contains the C code that supports the encoding and the decoding of a netstring. The netstring is presumed to contain a sequence of Cstrings.

  This directory also contains the C code that supports the
  encoding and the decoding of a netstring used to in association of the SCGI protocol.  Within this protocol, each CGI-related environment variables is encode as two Cstrings, containg the name and value of the environment variable.  In addition, various other requirements of the SCGI protocol are validated

  Additionally, this directory contains a Makefile that can build two command-line utilities: netstring and scgi_netstring. These utilities can be used as a command-line pipeline to create a SCGI daemon.  (reference ../scgi-launch)


### Utility Synopsis

  ```bash
   $ netstring [-e | --encode]       < text_file  > netstring 
   $ netstring [-d | --decode]       < netstring > text_file
   $ scgi_netstring [-e | --encode]  < env_file  > netstring 
   $ scgi_netstring [-d | --decode]  < netstring > env_file
   ```


## Netstring Description

## SCGI Protocol Description


## C API Synopsis

  ### Netstring
   ```C
   #include "netstring.h"
   ```

  ### SCGI Netstring
   ```C
   #include "netstring.h"
   ```   

## Command Line Examples



## See Also
* netstring
  * https://en.wikipedia.org/wiki/Netstring
* Simple Common Gateway Interface (SCGI):
  * https://en.wikipedia.org/wiki/Simple_Common_Gateway_Interface
  * http://python.ca/scgi/protocol.txt
