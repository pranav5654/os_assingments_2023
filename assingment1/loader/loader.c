#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

int (*strt_ptr)();

//function to clean up the meomory space used
void loader_cleanup() {
    free(ehdr);
    free(phdr);
    close(fd);
}


void load_and_run_elf(char** argv) {
//opening file
  fd = open(argv[1], O_RDONLY);

  ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
// reading file header
  int ehdr_error_code = read(fd, ehdr, sizeof(Elf32_Ehdr));

  if(ehdr_error_code == -1) {
    printf("Error reading ELF header\n");
    exit(1);
  }
// gathering necessary information from the elf header
  Elf32_Addr entry_point = ehdr->e_entry;
  Elf32_Off pht_offset = ehdr->e_phoff;
  Elf32_Half pht_size_of_each = ehdr->e_phentsize;
  Elf32_Half pht_num_entries = ehdr->e_phnum;

  
  phdr = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr));

//iterating through program headers and finding the right one
  for (int i = 0; i < (int)pht_num_entries; i++)
  {
    lseek(fd,pht_offset + i*pht_size_of_each,SEEK_SET);


    int phdr_error_code = read(fd, phdr, sizeof(Elf32_Phdr));

    if(phdr_error_code == -1) {
      printf("Error reading Program header number %d\n" , i+1);
      exit(1);
    }

    if(phdr->p_type == PT_LOAD){
      if( phdr->p_vaddr < entry_point &&  entry_point < phdr->p_vaddr + phdr->p_memsz){

// assingning memory for the segment to be read containing the entry point
        unsigned int *segment_mem = mmap(NULL , phdr->p_memsz, PROT_READ |PROT_EXEC  , MAP_SHARED ,fd , phdr->p_offset);

        if(segment_mem == MAP_FAILED){
          printf("%s\n", "Memory could not be mapped.\n");
          exit(1);
        }
//finding the offset in the loaded memory
        unsigned int  offset = entry_point - phdr->p_vaddr;
        void *  real_entry_point = (char *)segment_mem + offset;

//using function pointer to typecast the entry point into start function
        strt_ptr  = real_entry_point;
        int result = strt_ptr();

        printf("User _start return value = %d\n",result);

// unmapping the mapped memory
        munmap(segment_mem, phdr->p_memsz);
        break;
      }
      }
  }
  }




