
#include "../loader/loader.h"
int fd;

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }


  fd =  open(argv[1], O_RDONLY);
  if(fd == -1) {
    printf("File  %s cannot be opened...  , error code %d\n",argv[1] , fd);
    exit(1);
  }
  load_and_run_elf(argv); 
  loader_cleanup();
  return 0;
}
