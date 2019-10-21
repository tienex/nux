#include "project.h"

struct elf32ph {
	uint32_t type;
	uint32_t off;
	uint32_t va;
	uint32_t pa;
	uint32_t fsize;
	uint32_t msize;
	uint32_t flags;
	uint32_t align;
};

struct elf32sh {
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	uint32_t addr;
	uint32_t off;
	uint32_t size;
	uint32_t lnk;
	uint32_t info;
	uint32_t align;
	uint32_t shent_size;
};

struct elf32hdr {
	uint8_t id[16];
	uint16_t type;
	uint16_t mach;
	uint32_t ver;
	uint32_t entry;
	uint32_t phoff;
	uint32_t shoff;
	uint32_t flags;
	uint16_t eh_size;
	uint16_t phent_size;
	uint16_t phs;
	uint16_t shent_size;
	uint16_t shs;
	uint16_t shstrndx;
} __packed;

#define ET_EXEC		2
#define EM_386		3
#define EV_CURRENT	1

#define SHT_PROGBITS 	1
#define SHT_NOBITS	8

#define PHT_NULL	0
#define PHT_LOAD	1
#define PHT_DYNAMIC	2
#define PHT_INTERP	3
#define PHT_NOTE	4
#define PHT_SHLIB	5
#define PHT_PHDR	6
#define PHT_TLS		7

#define PHF_X		1
#define PHF_W		2
#define PHF_R		4

uintptr_t load_elf32 (void *elfimg)
{
#define ELFOFF(_o) ((void *)((uintptr_t)elfimg + (_o)))
  int i;

  char elfid[] = { 0x7f, 'E', 'L', 'F', };
  struct elf32hdr *hdr = (struct elf32hdr *) elfimg;
  struct elf32ph *ph = (struct elf32ph *) ELFOFF(hdr->phoff);

  if (memcmp (hdr->id, elfid, 4) != 0)
    return (uintptr_t)-1;

  if (hdr->type != ET_EXEC || hdr->ver != EV_CURRENT)
    return (uintptr_t)-1;

  for (i = 0; i < hdr->phs; i++, ph++) {
    switch (ph->type)
      {
      case PHT_LOAD:
	/* Normal load segment. */
	if (ph->va + ph->msize < ph->va)
	  {
	    printf("size of PH too big.");
	    exit (-1);
	  }

	if (ph->fsize)
	  {
	    /*
	      memcpy() to user and populate on fault.
	    */
	    va_copy (ph->va, ELFOFF(ph->off), ph->fsize,
		     !!(ph->flags & PHF_W), !!(ph->flags & PHF_X));
	  }

	if (ph->msize - ph->fsize > 0)
	  {
	    /*
	      memset() to user and populate on fault.
	    */
	    va_memset (ph->va + ph->fsize, 0, ph->msize - ph->fsize,
		       !!(ph->flags & PHF_W), !!(ph->flags & PHF_X));
	  }
	break;
      case PHT_APXH_INFO:
	/* Boot Information segment. */
	printf("Boot Information area at %08lx (size: %ld).\n", ph->va, ph->msize);
	va_info (ph->va, ph->msize);
	break;
      case PHT_APXH_PHYSMAP:
	/* Direct 1:1 PA mapping. */
	printf("Physmap VA area at %08lx (size: %ld).\n", ph->va, ph->msize);
	va_physmap (ph->va, ph->msize);
	break;
      case PHT_APXH_EMPTY:
	printf("Empty VA area at %08lx (size: %ld).\n", ph->va, ph->msize);
	/* Just VA allocation. Leave it. */
	break;
      case PHT_APXH_PFNMAP:
	printf("PFN Map at %08lx (size: %ld).\n", ph->va, ph->msize);
	va_pfnmap (ph->va, ph->msize);
	break;
      case PHT_APXH_STREE:
	printf("S-Tree at %08lx (size: %ld).\n", ph->va, ph->msize);
	va_stree (ph->va, ph->msize);
	break;
      case PHT_APXH_LINEAR:
	printf("Linear Map at %08lx (size: %ld).\n", ph->va, ph->msize);
	va_linear (ph->va, ph->msize);
	break;
      default:
	printf("Ignored segment type %08lx.\n", ph->type);
	break;
      }
  }

  return (uintptr_t) hdr->entry;
}


arch_t get_elf_arch (void *elfimg)
{
    char elfid[] = { 0x7f, 'E', 'L', 'F', };
    struct elf32hdr *hdr = (struct elf32hdr *) elfimg;

    if (memcmp (hdr->id, elfid, 4) != 0)
      return ARCH_INVALID;

    if (hdr->mach == EM_386)
      return ARCH_386;

    return ARCH_UNSUPPORTED;
}
