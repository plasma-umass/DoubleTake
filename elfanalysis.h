// -*- C++ -*-

#ifndef SHERIFF_XTRACKER_H
#define SHERIFF_XTRACKER_H

#include <set>

#if !defined(_WIN32)
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mm.h"
#include "elf.h"

class elfanalysis {
  
  enum { PAGE_SIZE = 4096 };
  enum { MAXBUFSIZE = 1024 };

public:

  elfanalysis()
  {
    int    count;
    void * ptr;

    if (NElts == xdefines::PROTECTEDHEAP_SIZE)
      _isHeap = true;

    /* Get current executable file name. */
    count=readlink("/proc/self/exe", _exec_filename, MAXBUFSIZE);
    if (count <= 0 || count >= MAXBUFSIZE)
    {
      fprintf(stderr, "Failed to get current executable file name\n" );
      exit(1);
    }
    _exec_filename[count] = '\0';

    /* Get text segmentations information. */
    _elf_info.hdr = (Elf_Ehdr*)grab_file(_exec_filename, &_elf_info.size);
    if (!_elf_info.hdr) {
      printf("Can't grab file %s\n", _exec_filename);
      exit(1);
    }
  
    /* Get the text segment information and saved to global _textinfo */
    get_textseg_info(&_elf_info);
    parse_elf();
  }

  virtual ~elfanalysis() {
    release_file(_elf_info.hdr, _elf_info.size);
  }
 
  void print_objects_info() {

    char base[MAXBUFSIZE];
    int k = 0;      

    sprintf (base, "addr2line -e %s", _exec_filename);
#if 0  
          unsigned long ipaddr = callsite->getItem(j);
          char command[MAXBUFSIZE];
            
         fprintf(stderr, "\tCall site %d %lx: ", j, ipaddr);
          if(ipaddr >= textStart && ipaddr <= textEnd) {
            sprintf(command, "%s %x", base, ipaddr);
            system(command);
          }
        // Print object information about globals.
        Elf_Sym *symbol = find_symbol(&_elf_info, (intptr_t)object.start);
        if(symbol != NULL) {
          const char * symname = _elf_info.strtab + symbol->st_name;
          fprintf(stderr, "\tGlobal object: name \"%s\", start %lx, size %d\n", symname, symbol->st_value, symbol->st_size);
        }
#endif
  }

  void *grab_file(const char *filename, unsigned long *size) {
    struct stat st;
    void *map;
    int fd;

    fd = open(filename, O_RDONLY);
    if (fd < 0 || fstat(fd, &st) != 0)
      return NULL;

    *size = st.st_size;
    map = MM::allocatePrivate (*size, fd);
    close(fd);

    if (map == MAP_FAILED)
      return NULL;
    return map;
  }

  int check_elf(struct elf_info * _elf_info) 
  {
    Elf_Ehdr *hdr = _elf_info->hdr;

    if (_elf_info->size < sizeof(*hdr)) {
        /* file too small, assume this is an empty .o file */
        return -1;
    }
    
    /* Is this a valid ELF file? */
    if ((hdr->e_ident[EI_MAG0] != ELFMAG0) ||
       (hdr->e_ident[EI_MAG1] != ELFMAG1) ||
       (hdr->e_ident[EI_MAG2] != ELFMAG2) ||
       (hdr->e_ident[EI_MAG3] != ELFMAG3)) {
      /* Not an ELF file - silently ignore it */
      return -1;
   }

   return 0;
  }

  void get_textseg_info(struct elf_info * _elf_info) 
  {
    Elf_Ehdr *hdr = _elf_info->hdr;
    Elf_Shdr *sechdrs;
    int i;
  
    if(check_elf(_elf_info)) {
      printf("errorneous elf file.\n");
      exit(1);
    }
    
    sechdrs =(Elf_Shdr *)((char *)hdr + hdr->e_shoff);
    _elf_info->sechdrs = sechdrs;

    /* Fix endianness in section headers */
    for (i = 0; i < hdr->e_shnum; i++) {
      char *secstrings=(char*)((unsigned long)hdr + sechdrs[hdr->e_shstrndx].sh_offset);
      char *secname;
      secname = secstrings + sechdrs[i].sh_name;

      if (strncmp(secname, ".text", 5) != 0) {
        continue;
      }
   
      textStart = hdr->e_entry;
      textEnd   = hdr->e_entry + sechdrs[i].sh_size;
      assert(textEnd >= textStart);
    }

    return; 
  }

  int parse_elf()
  {
    unsigned int i;
    Elf_Ehdr *hdr = _elf_info.hdr;
    Elf_Shdr *sechdrs;
  
    if (check_elf(&_elf_info)) {
      fprintf (stderr, "Erroneous ELF file.\n");
      exit(-1);
    }
  

    sechdrs =(Elf_Shdr *)((char *)hdr + hdr->e_shoff);
    _elf_info.sechdrs = sechdrs;

    /* Fix endianness in section headers */
    for (i = 0; i < hdr->e_shnum; i++) {
      if (sechdrs[i].sh_type != SHT_SYMTAB)
        continue;
  
      if(sechdrs[i].sh_size == 0) 
        continue;
  
      _elf_info.symtab_start = (Elf_Sym*)((unsigned long)hdr + sechdrs[i].sh_offset);
      _elf_info.symtab_stop  = (Elf_Sym*)((unsigned long)hdr + sechdrs[i].sh_offset + sechdrs[i].sh_size);
      _elf_info.strtab       = (char *)((unsigned long)hdr + sechdrs[sechdrs[i].sh_link].sh_offset);
      break;
    }
  
    return 0; 
  }

  /*
   * Find symbols according to specific address.  
   */
  Elf_Sym * find_symbol(struct elf_info *elf, Elf_Addr addr)
  {
    int bFound = 0;
    Elf_Sym *symbol;
    Elf_Ehdr *hdr = elf->hdr;
  
    for (symbol = elf->symtab_start; symbol < elf->symtab_stop; symbol++) {
      if (symbol->st_shndx >= SHN_LORESERVE)
        continue;
      
      /* Checked only when it is a global variable. */
      if(ELF_ST_BIND(symbol->st_info) != STB_GLOBAL || ELF_ST_TYPE(symbol->st_info) != STT_OBJECT)
        continue;
  
      /* If the addr is in the range of current symbol, that get it. */
      if(addr >= symbol->st_value && addr < (symbol->st_value + symbol->st_size)) {
        bFound = 1;
        break;
      }
    }
  
    if(bFound)
      return symbol;
    else
      return NULL;
  }

  void release_file(void *file, unsigned long size)
  {
    munmap(file, size);
  }

  void finalize() {
  }
  
private:

  struct elf_info _elf_info;

  char _exec_filename[MAXBUFSIZE];
};


#endif
