/* Andrew Lunn 
 * A quick test of the des password reading functions. It should be used to 
 * ensure that echoing is turned off while reading in the users password. */

#include <stdio.h>
#include <stdlib.h>
#include <openssl/opensslconf.h>
#include <openssl/des.h>

int main(int argc, char *argv[])
{
  char buf[128];
  char *prompt="Please enter a passwd\n";
  
  des_read_pw_string(buf,sizeof(buf),prompt,1);
  
  exit (0);  
}
