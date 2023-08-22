#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

int (*strt_ptr)();

void loader_cleanup() {
    free(ehdr);
    free(phdr);
    close(fd);
}


void load_and_run_elf(char** argv) {

  fd = open(argv[1], O_RDONLY);

  ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));

  int ehdr_error_code = read(fd, ehdr, sizeof(Elf32_Ehdr));

  if(ehdr_error_code == -1) {
    printf("Error reading ELF header\n");
    exit(1);
  }

  Elf32_Addr entry_point = ehdr->e_entry;
  Elf32_Off pht_offset = ehdr->e_phoff;
  Elf32_Half pht_size_of_each = ehdr->e_phentsize;
  Elf32_Half pht_num_entries = ehdr->e_phnum;

  
  phdr = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr));

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

        unsigned int *segment_mem = mmap(NULL , phdr->p_memsz, PROT_READ |PROT_EXEC  , MAP_SHARED ,fd , phdr->p_offset);

        if(segment_mem == MAP_FAILED){
          printf("%s\n", "Memory could not be mapped.\n");
          exit(1);
        }

        unsigned int  offset = entry_point - phdr->p_vaddr;
        void *  real_entry_point = (char *)segment_mem + offset;


        strt_ptr  = real_entry_point;
        int result = strt_ptr();

        printf("User _start return value = %d\n",result);


        munmap(segment_mem, phdr->p_memsz);

        
        break;
      }
      
      }

  }
  }




