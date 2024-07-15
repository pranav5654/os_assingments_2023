#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
Elf32_Addr entry_point;
int fd;
int (*strt_ptr)();
void * segment_mem;

int fragmentation = 0 ;
int pages_allocated =0;
int page_faults =0 ;

int seg_num = -1 ;
int currpages = 0;
int curr_size = 0;


void loader_cleanup() {
    free(ehdr);
    free(phdr);
    close(fd);
    munmap(segment_mem , 4096);
}


void load_and_run_elf(char** argv) {

    fd = open(argv[1], O_RDONLY);

    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    
    int ehdr_bytes = read(fd, ehdr, sizeof(Elf32_Ehdr));

    if(ehdr_bytes == -1) {
        printf("Error reading ELF header\n");
        exit(1);
    }

    entry_point = ehdr->e_entry;

    strt_ptr = (int (*)())entry_point;
    
    int result  =  strt_ptr(); 

    if(curr_size < 0) fragmentation -= curr_size;
    else fragmentation += 4096 - curr_size;


    printf("User _start return value = %d\n",result);
    printf("Page Faults: %d\nPages Allocated: %d\nFragmentation: %.2f kb\n", page_faults , pages_allocated , fragmentation/1024.0);

}


static void my_handler(int signum , siginfo_t * si , void * nouse) {
    if(signum == SIGSEGV){
        page_faults ++;

        unsigned int *  fault_addr = (unsigned int * )si->si_addr;
        phdr = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr));

        for (int i = 0; i < (int)ehdr->e_phnum; i++){

            lseek(fd,ehdr->e_phoff + i*ehdr->e_phentsize,SEEK_SET);
            int phdr_bytes = read(fd, phdr, sizeof(Elf32_Phdr));

            if(phdr_bytes == -1) {
                printf("Error reading Program header\n");
                exit(1);
            }

            if( phdr->p_vaddr <= (unsigned int )fault_addr &&  (unsigned int )fault_addr < phdr->p_vaddr + phdr->p_memsz ){
                int page_size = 4096;
                int page_num = phdr->p_memsz / page_size;
                if(phdr->p_memsz % page_size != 0) page_num ++;

                void *page_start = (void *)((unsigned int )fault_addr & ~(page_size - 1));
                pages_allocated += 1;


                segment_mem = mmap(page_start , page_size , PROT_READ | PROT_EXEC | PROT_WRITE ,  MAP_ANONYMOUS | MAP_SHARED , -1 , 0);

                lseek(fd , phdr->p_offset , SEEK_SET);
                read(fd , segment_mem , phdr->p_filesz);

                if (segment_mem == MAP_FAILED) {
                    perror("mmap");
                    exit(1);
                }
                
                if(seg_num == -1){
                    
                    seg_num = i;
                    currpages = 1;
                    curr_size =  phdr->p_memsz  - page_size;
                }
                
                else if(seg_num != i){
                    
                    if(curr_size < 0) fragmentation -= curr_size;
                    else fragmentation += currpages*page_size - curr_size;
                    seg_num = i;
                    currpages = 1 ;    
                    curr_size = phdr->p_memsz - page_size;

                }
                else{
                    currpages ++;
                    curr_size -= page_size;
                }
            }
        }
    }
}


int main(int argc, char** argv) 
{

  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }

  fd =  open(argv[1], O_RDONLY);

  if(fd == -1) {
    printf("File  %s cannot be opened  , error code %d\n",argv[1] , fd);
    exit(1);
  }

  struct sigaction sig;
  memset(&sig, 0, sizeof(sig));
  sig.sa_sigaction = my_handler;
  sig.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &sig, NULL);


  load_and_run_elf(argv); 
  loader_cleanup();

  
  return 0;
}