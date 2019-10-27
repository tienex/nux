#include "project.h"

#define MSR_IA32_MISC_ENABLE 0x000001a0
#define _MSR_IA32_MISC_ENABLE_XD_DISABLE (1LL << 34)

#define MSR_IA32_EFER 0xc0000080
#define _MSR_IA32_EFER_NXE (1LL << 11)
#define _MSR_IA32_EFER_LME (1LL << 8)

#define CR4_PAE (1 << 5)

#define CR0_PG  (1 << 31)
#define CR0_WP  (1 << 16)

unsigned long read_cr4 (void)
{
  unsigned long reg;

  asm volatile ("mov %%cr4, %0\n" : "=r" (reg));
  return reg;
}

void write_cr4 (unsigned long reg)
{
  asm volatile ("mov %0, %%cr4\n" :: "r" (reg));
}

unsigned long read_cr3 (void)
{
  unsigned long reg;

  asm volatile ("mov %%cr3, %0\n" : "=r" (reg));
  return reg;
}

void write_cr3 (unsigned long reg)
{
  asm volatile ("mov %0, %%cr3\n" :: "r" (reg));
}

unsigned long read_cr0 (void)
{
  unsigned long reg;

  asm volatile ("mov %%cr0, %0\n" : "=r" (reg));
  return reg;
}

void write_cr0 (unsigned long reg)
{
  asm volatile ("mov %0, %%cr0\n" :: "r" (reg));
}

void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
  asm volatile ("cpuid\n" : "+a"(*eax), "=b"(*ebx), "+c"(*ecx), "=d"(*edx));
}

uint64_t rdmsr(uint32_t ecx)
{
  uint32_t edx, eax;


  asm volatile ("rdmsr\n" : "=d"(edx), "=a" (eax) : "c" (ecx));

  return ((uint64_t)edx << 32) | eax;
}

void wrmsr(uint32_t ecx, uint64_t msr)
{
  uint32_t edx, eax;

  eax = (uint32_t)msr;
  edx = msr >> 32;

  asm volatile ("wrmsr\n" :: "c" (ecx), "d" (edx), "a" (eax));
}

void lgdt(uintptr_t ptr)
{
  asm volatile ("lgdtl (%0)\n" :: "r" (ptr));
}

bool cpu_is_intel ()
{
  uint32_t eax, ebx, ecx, edx;

  eax = 0;
  ecx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);

  // GenuineIntel?
  if (ebx == 0x756e6547
      && ecx == 0x6c65746e
      && edx == 0x49656e69)
    return true;
  else
    return false;

  return 1;
}

unsigned intel_cpu_family (void)
{
  unsigned family;
  uint32_t eax, ebx, ecx, edx;

  eax = 1;
  ecx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);

  family = (eax & 0xf00) >> 8;
  family |= (eax & 0xf00000 >> 20);

  return family;
}

unsigned intel_cpu_model (void)
{
  unsigned model;
  uint32_t eax, ebx, ecx, edx;

  eax = 1;
  ecx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);

  model = (eax & 0xf0) >> 4;
  model |= (eax & 0xf0000) >> 16;

  return model;
}

bool cpu_supports_pae (void)
{
  uint32_t eax, ebx, ecx, edx;

  eax = 1;
  ecx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);

  return !!(edx & (1 << 6));
}

bool cpu_supports_longmode (void)
{
  uint32_t eax, ebx, ecx, edx;

  eax = 0x80000001;
  ecx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);
  
  return !!(edx & (1 << 29));
}

bool cpu_supports_1gbpages (void)
{
  uint32_t eax, ebx, ecx, edx;

  eax = 0x80000001;
  ecx = 0;
  cpuid(&eax, &ebx, &ecx, &edx);
  
  return !!(edx & (1 << 26));
}

bool cpu_supports_nx (void)
{
  bool nx_supported;
  uint64_t efer;
  uint32_t eax, ebx, ecx, edx;

  /* Intel CPUs might have disabled this in MSR. */
  if (cpu_is_intel ()) 
    {
      unsigned family = intel_cpu_family ();
      unsigned model = intel_cpu_model ();

      if ((family >=6) && (family > 6 || model > 0xd))
	{
	  uint64_t misc_enable;

	  misc_enable = rdmsr (MSR_IA32_MISC_ENABLE);
	  if (misc_enable & _MSR_IA32_MISC_ENABLE_XD_DISABLE)
	    {
	      misc_enable &= ~_MSR_IA32_MISC_ENABLE_XD_DISABLE;
	      wrmsr(MSR_IA32_MISC_ENABLE, misc_enable);
	    }
	}
    }
  eax = 0x80000001;
  ecx = 0;
  cpuid (&eax, &ebx, &ecx, &edx);

  nx_supported = !!(edx & (1 << 20));
  if (!nx_supported)
    return false;

  efer = rdmsr (MSR_IA32_EFER);
  wrmsr (MSR_IA32_EFER, efer | _MSR_IA32_EFER_NXE);
  efer = rdmsr (MSR_IA32_EFER);

  return !!(efer & _MSR_IA32_EFER_NXE);
}


typedef uint64_t pte_t;

static bool nx_enabled;

/* 1 Gb direct map in the Payload Page Table. */
#define PAE_DIRECTMAP_START 0
#define PAE_DIRECTMAP_END   (1LL << 30)

#define PTE_P 1LL
#define PTE_W 2LL
#define PTE_PS (1L << 7)
#define PTE_NX (nx_enabled ? 1LL << 63 : 0)

#define L3OFF(_va) (((_va) >> 30) & 0x3)
#define L2OFF(_va) (((_va) >> 21) & 0x1ff)
#define L1OFF(_va) (((_va) >> 12) & 0x1ff)

static pte_t *pae_cr3;
static pte_t *l2s[4];

static void
set_pte (pte_t *ptep, uint64_t pfn, uint64_t flags)
{
  *ptep = (pfn << PAGE_SHIFT) | flags;
}

/* Beware: this returns va, not valid in 32 where va space is 32 and PAE is bigger. */
static void *
pte_getaddr (pte_t *ptep)
{
  pte_t pte = *ptep;

  if (!(pte & PTE_P))
    return NULL;

  return (void *)(uintptr_t)(pte & 0x7ffffffffffff000LL);
}

static uint64_t
pte_getflags (pte_t *ptep)
{
  pte_t pte = *ptep;

  return pte & ~0x7ffffffffffff000LL;
}

static uint64_t
pte_mergeflags (uint64_t fl1, uint64_t fl2)
{
  uint64_t newf;


  //PTE_P always present
  assert (fl1 & fl2 & PTE_P);
  newf = PTE_P;

  // Write must be OR'd.
  newf |= (fl1 | fl2) & PTE_W;

  // NX must be AND'd.
  newf |= fl1 & fl2 & PTE_NX;

  printf ("Merging: %llx (+) %llx = %llx\n", fl1, fl2, newf);
  return newf;
}

void
pae_verify (vaddr_t va, size_t size)
{
  if (va < PAE_DIRECTMAP_END)
    {
      printf ("ELF would overwrite direct physical map");
      exit (-1);
    }
}

void pae_init (void)
{
  int i;

  /* Enable NX */
  assert (cpu_supports_pae ());

  nx_enabled = cpu_supports_nx ();

  /* In PAE is only 64 bytes, but we allocate a full page for it. */
  pae_cr3 = (pte_t *)get_payload_page ();

  /* Set PDPTEs */
  for (i = 0; i < 4; i++)
    {
      uintptr_t l2page = get_payload_page ();

      set_pte(pae_cr3 + i, l2page >> PAGE_SHIFT, PTE_P);
      l2s[i] = (pte_t *)l2page;
    }

  /* Setup Direct map  at 0->1Gb */
  for (i = 0; i < 512; i++)
      set_pte (l2s[0] + i, i, PTE_PS|PTE_W|PTE_P);

  printf ("Using PAE paging (CR3: %08lx, NX: %d).\n", pae_cr3, nx_enabled);
}

static pte_t *
pae_get_l1p (vaddr_t va)
{
  pte_t *l2p, *l1;
  unsigned l3off = L3OFF(va);
  unsigned l2off = L2OFF(va);
  unsigned l1off = L1OFF(va);

  l2p = l2s[l3off] + l2off;

  l1 = (pte_t *)pte_getaddr(l2p);
  if (l1 == NULL)
    {
      uintptr_t l1page;

      /* Populating L1. */
      l1page = get_payload_page ();

      set_pte(l2p, l1page >> PAGE_SHIFT, PTE_W|PTE_P);
      l1 = (pte_t *)l1page;
      //      printf("populated l1 with %lx\n", l1);
    }

  return l1 + l1off;
}

static uintptr_t
pae_populate_page (vaddr_t va, int w, int x)
{
  pte_t *l1p;
  uint64_t l1f;
  uintptr_t page;

  l1p = pae_get_l1p (va);

  l1f = (w ? PTE_W : 0) | (x ? 0 : PTE_NX) | PTE_P;

  page = (uintptr_t)pte_getaddr (l1p);
  if (pte_getaddr (l1p) == NULL)
    {
      page = get_payload_page ();
      set_pte(l1p, page >> PAGE_SHIFT, l1f);
    } 
  else
    {
      uint64_t newf;
      uint64_t oldf = pte_getflags (l1p);

      newf = pte_mergeflags(l1f, oldf);
      if (newf != oldf)
	{
	  printf("Flags changed from %llx to %llx\n", oldf, newf);
	  set_pte(l1p, page >> PAGE_SHIFT, newf);
	}
    }

  return page;
}

uintptr_t
pae_getphys(vaddr_t va)
{
  uintptr_t page;
  pte_t *l2e, *l1, *l1e;
  unsigned l3off = L3OFF(va);
  unsigned l2off = L2OFF(va);
  unsigned l1off = L1OFF(va);

   l2e = l2s[l3off] + l2off;

  l1 = (pte_t *)pte_getaddr(l2e);
  assert (l1 != NULL);

  l1e = l1 + l1off;

  page = (uintptr_t)pte_getaddr (l1e);
  assert (page != 0);

  return page | (va & ~(PAGE_MASK));
}

void pae_physmap (vaddr_t va, size_t size)
{
  unsigned i, n;
  pte_t *pte;

  n = size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    {
      pte = pae_get_l1p (va + (i << PAGE_SHIFT));
      set_pte (pte, i, PTE_P|PTE_W|PTE_NX);
    }
  
}

#define PAE_LINEAR_SHIFT (PAGE_SHIFT + 9 + 2)
#define PAE_LINEAR_SIZE (1L << PAE_LINEAR_SHIFT)
#define PAE_LINEAR_ALIGN (PAE_LINEAR_SIZE - 1)

void pae_linear (vaddr_t va, size_t size)
{
  int i;
  unsigned l3off = L3OFF(va);
  unsigned l2off = L2OFF(va);

  if (va & PAE_LINEAR_ALIGN) {
    printf ("PAE Linear VA %llx not aligned (align mask: %lx).\n", va, PAE_LINEAR_ALIGN);
    exit (-1);
  }

  if (size < PAE_LINEAR_SIZE) {
    printf ("PAE Linear size %llx too small.\n", size);
    exit (-1);
  }

  for (i = 0; i < 4; i++) {
    pte_t *l2p;
    
    l2p = l2s[l3off] + l2off + i;
    set_pte (l2p, (uint64_t)(uintptr_t)l2s[i] >> PAGE_SHIFT, PTE_NX|PTE_W|PTE_P);
  }
}

void pae_populate (vaddr_t va, size_t size, int w, int x)
{
  ssize_t len = size;

  while (len > 0)
    {
      pae_populate_page(va, w, x);

      len -= PAGE_CEILING(va) - va;
      va += PAGE_CEILING(va) - va;

    }
}


void pae_entry (vaddr_t entry)
{
  unsigned long cr4 = read_cr4();
  unsigned long cr3 = read_cr3();
  unsigned long cr0 = read_cr0();

  write_cr4 (cr4 | CR4_PAE);
  printf("CR4: %08lx -> %08lx.\n", cr4, read_cr4());

  write_cr3 ((unsigned long)pae_cr3);
  printf("CR3: %08lx -> %08lx.\n", cr3, read_cr3());

  write_cr0 (cr0 | CR0_PG | CR0_WP);
  printf("CR0: %08lx -> %08lx.\n", cr0, read_cr0());

  ((void (*)())(uintptr_t)entry)();
}


/*
  AMD64 PAE support.
*/

#define PAE64_DIRECTMAP_START 0
#define PAE64_DIRECTMAP_END   (1LL << 30)

#define L4OFF64(_va) (((_va) >> 39) & 0x1ff)
#define L3OFF64(_va) (((_va) >> 30) & 0x1ff)
#define L2OFF64(_va) (((_va) >> 21) & 0x1ff)
#define L1OFF64(_va) (((_va) >> 12) & 0x1ff)

static pte_t *pae64_cr3;

static pte_t *
pae64_get_l3p (vaddr_t va)
{
  pte_t *l4p, *l3p;
  unsigned l4off = L4OFF64(va);
  unsigned l3off = L3OFF64(va);

  l4p = pae64_cr3 + l4off;

  l3p = (pte_t *)pte_getaddr(l4p);
  if (l3p == NULL)
    {
      uintptr_t l3page;

      /* Populating L3. */
      l3page = get_payload_page ();

      set_pte (l4p, l3page >> PAGE_SHIFT, PTE_W|PTE_P);
      l3p = (pte_t *)l3page;
    }

  return l3p + l3off;
}

static pte_t *
pae64_get_l1p (vaddr_t va)
{
  pte_t *l3p, *l2p, *l1p;
  unsigned l2off = L2OFF64(va);
  unsigned l1off = L1OFF64(va);

  l3p = pae64_get_l3p (va);

  l2p = (pte_t *)pte_getaddr(l3p);
  if (l2p == NULL)
    {
      uintptr_t l2page;

      /* Populating L2. */
      l2page = get_payload_page ();

      set_pte (l3p, l2page >> PAGE_SHIFT, PTE_W|PTE_P);
      l2p = (pte_t *)l2page;
    }
  l2p += l2off;

  l1p = (pte_t *)pte_getaddr(l2p);
  if (l1p == NULL)
    {
      uintptr_t l1page;

      /* Populating L1. */
      l1page = get_payload_page ();

      set_pte (l2p, l1page >> PAGE_SHIFT, PTE_W|PTE_P);
      l1p = (pte_t *)l1page;
    }

  return l1p + l1off;
}

void
pae64_verify (vaddr_t va, size_t size)
{
  if (va < PAE64_DIRECTMAP_END)
    {
      printf ("ELF would overwrite direct physical map");
      exit (-1);
    }
}

void pae64_init (void)
{
  pte_t *l3p;

  assert (cpu_supports_longmode ());

  nx_enabled = cpu_supports_nx ();

  pae64_cr3 = (pte_t *)get_payload_page ();


  /* Setup Direct map at 0->1Gb */
  l3p = pae64_get_l3p (0);
  if (cpu_supports_1gbpages ())
    {
      set_pte (l3p, 0, PTE_PS|PTE_W|PTE_P);
    }
  else
    {
      int i;
      pte_t *l2page = (pte_t *)get_payload_page ();

      for (i = 0; i < 512; i++)
	  set_pte(l2page + i, i, PTE_PS|PTE_W|PTE_P);
      set_pte (l3p, (uintptr_t)l2page >> PAGE_SHIFT, PTE_P|PTE_W);
    }

  printf ("Using PAE64 paging (CR3: %08lx, NX: %d).\n", pae64_cr3, nx_enabled);
}

static uintptr_t
pae64_populate_page (vaddr_t va, int w, int x)
{
  pte_t *l1p;
  uint64_t l1f;
  uintptr_t page;

  l1p = pae64_get_l1p (va);
  l1f = (w ? PTE_W : 0) | (x ? 0 : PTE_NX) | PTE_P;

  page = (uintptr_t)pte_getaddr (l1p);
  if (page == 0)
    {
      page = get_payload_page ();
      set_pte(l1p, page >> PAGE_SHIFT, l1f);
    } 
  else
    {
      uint64_t newf;
      uint64_t oldf = pte_getflags (l1p);

      newf = pte_mergeflags(l1f, oldf);
      if (newf != oldf)
	{
	  printf("Flags changed from %llx to %llx\n", oldf, newf);
	  set_pte(l1p, page >> PAGE_SHIFT, newf);
	}
    }

  return page;
}

uintptr_t
pae64_getphys(vaddr_t va)
{
  uintptr_t page;
  pte_t *l4p, *l3p, *l2p, *l1p;
  unsigned l4off = L4OFF64(va);
  unsigned l3off = L3OFF64(va);
  unsigned l2off = L2OFF64(va);
  unsigned l1off = L1OFF64(va);

  l4p = pae64_cr3 + l4off;

  l3p = (pte_t *)pte_getaddr(l4p);
  assert (l3p != NULL);
  l3p += l3off;

  l2p = (pte_t *)pte_getaddr(l3p);
  assert (l2p != NULL);
  l2p += l2off;

  l1p = (pte_t *)pte_getaddr(l2p);
  assert (l1p != NULL);
  l1p += l1off;

  page = (uintptr_t)pte_getaddr (l1p);
  assert (page != 0);

  return page |= (va & ~(PAGE_MASK));
}

void pae64_populate (vaddr_t va, size_t size, int w, int x)
{
  ssize_t len = size;

  while (len > 0)
    {
      pae64_populate_page(va, w, x);

      len -= PAGE_CEILING(va) - va;
      va += PAGE_CEILING(va) - va;

    }
}

static uint64_t pae64_gdt[3] __attribute__((aligned(64))) = {
  0,
  0x00a09a0000000000LL,
};

static struct gdtreg {
  uint16_t size;
  uint32_t base;
} __attribute__((aligned(64))) __packed gdtreg = {
  .size = 15,
  .base = (uint32_t)&pae64_gdt,
  
};

void pae64_entry (vaddr_t entry)
{
  unsigned long cr4 = read_cr4();
  unsigned long cr3 = read_cr3();
  unsigned long cr0 = read_cr0();
  uint64_t efer = rdmsr(MSR_IA32_EFER);

  write_cr4 (cr4 | CR4_PAE);
  printf("CR4: %08lx -> %08lx.\n", cr4, read_cr4());

  write_cr3 ((unsigned long)pae64_cr3);
  printf("CR3: %08lx -> %08lx.\n", cr3, read_cr3());

  wrmsr(MSR_IA32_EFER, efer | _MSR_IA32_EFER_LME);
  printf("EFER: %016llx -> %016llx.\n", efer, rdmsr(MSR_IA32_EFER));

  write_cr0 (cr0 | CR0_PG | CR0_WP);
  printf("CR0: %08lx -> %08lx.\n", cr0, read_cr0());

  lgdt ((uintptr_t)&gdtreg);

  asm volatile
    ("ljmp $8,$1f\n"
     ".code64\n"
     "1:\n"
     "mov %0, %%rax\n"
     "jmp *%%rax\n"
     ".code32"
     :: "m"(entry));

  exit (-1);
}
