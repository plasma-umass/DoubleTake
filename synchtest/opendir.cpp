#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>


int main(int argc, char ** argv) {
  DIR * dp;
 
  if (argc != 2) {
      fprintf(stderr, "Usage: %s <start directory>\n",argv[0]);
      exit(-1);
   }

      
  fprintf(stderr, "directory: %s DIR size %x\n",argv[1], sizeof(int));
  //fprintf(stderr, "directory: %s DIR size %x\n",argv[1], sizeof(*dp));
  
  dp = opendir(argv[1]);
  
  fprintf(stderr, "directory: %s dp %p\n",argv[1], dp);

  closedir(dp);
  
  fprintf(stderr, "In the end of main\n");

  //while(1);
}

