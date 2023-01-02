#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>

#include <bfd.h>

#define PROGNAME "nux-payload"
#define PAYLOAD_SECTNAME ".nux-payload"
#define OLDPAYLOAD_SECTNAME ".nux-garbage"

uint64_t squoze (char *);
char *unsquoze(uint64_t enc);

static asymbol **isym, **osym;


/*
  On disk structure with payload information.
*/
struct payload_hdr
{
#define PAYLOAD_HDR_MAGIC squoze ("nux-payload")
  uint64_t magic;
  uint64_t filename;
  uint32_t size;
} __attribute__ ((aligned));


void
report (const char * format, va_list args)
{
  fflush (stdout);
  fprintf (stderr, PROGNAME": ");
  vfprintf (stderr, format, args);
  putc ('\n', stderr);
}

void
non_fatal (const char * format, ...)
{
  va_list args;

  va_start (args, format);
  report (format, args);
  va_end (args);
}

void
fatal (const char * format, ...)
{
  va_list args;

  va_start (args, format);
  report (format, args);
  va_end (args);
  exit (-1);
}

void
bfd_nonfatal (const char *string)
{
  const char *errmsg;

  errmsg = bfd_errmsg (bfd_get_error ());
  fflush (stdout);
  if (string)
    fprintf (stderr, "%s: %s: %s\n", PROGNAME, string, errmsg);
  else
    fprintf (stderr, "%s: %s\n", PROGNAME, errmsg);
}

void
bfd_fatal (const char *string)
{
  bfd_nonfatal (string);
  exit (1);
}

static void
_print_payload (bfd *abfd, asection *sect, void *ptr)
{
  FILE *f = (FILE *)ptr;
  struct payload_hdr hdr;

  if (strcmp (PAYLOAD_SECTNAME, sect->name))
    return;

  bfd_get_section_contents (abfd, sect, &hdr, 0, sizeof (struct payload_hdr));
  char *name = unsquoze (hdr.filename);
  fprintf (f, "%12s: %-10ld %08llx\n", name, hdr.size, sect->vma);
  free (name);
}

static void
_copyhdr_section (bfd *ibfd, asection *isection, void *obfdarg)
{
  bfd *obfd = (bfd *) obfdarg;
  bfd_size_type size;
  sec_ptr osection;

  osection = bfd_make_section_anyway_with_flags (obfd,
						 bfd_section_name (ibfd, isection),
						 bfd_get_section_flags (ibfd, isection));
  if (osection == NULL)
    bfd_fatal ("failed to create section");

  size = bfd_section_size (ibfd, isection);
  size = bfd_convert_section_size (ibfd, isection, obfd, size);
  if (!bfd_set_section_size (obfd, osection, size))
    bfd_fatal ("failed to set section size");

  if (! bfd_set_section_vma (obfd, osection, bfd_section_vma (ibfd, isection)))
    bfd_fatal ("failed to set section vma");

  osection->lma = isection->lma;

  if (!bfd_set_section_alignment (obfd,
				  osection,
				  bfd_section_alignment (ibfd, isection)))
    bfd_fatal ("failed to set section alignment");

  osection->entsize = isection->entsize;

  osection->compress_status = isection->compress_status;

  /* Set a link between input and output section for successive deeper
     copies. */
  isection->output_section = osection;
  isection->output_offset = 0;

  if ((isection->flags & SEC_GROUP) != 0)
    fatal ("SEC_GROUP not supported");

  if (!bfd_copy_private_section_data (ibfd, isection, obfd, osection))
    bfd_fatal ("failed to copy private data");

}

static void
_setreloc_section (bfd *ibfd, asection *isection, void *obfdarg)
{
  bfd *obfd = (bfd *) obfdarg;
  asection *osection = isection->output_section;
  long relsize;
  arelent **rel;
  long relcount;

  if (osection == NULL)
    return;

  if ((ibfd->flags & SEC_GROUP) != 0)
    fatal ("WTF");

  if (relsize == 0)
    bfd_set_reloc (obfd, osection, NULL, 0);

  relsize = bfd_get_reloc_upper_bound (ibfd, isection);
  if (relsize < 0)
    {
      /* Do not complain if the target does not support relocations.  */
      if (relsize == -1 && bfd_get_error () == bfd_error_invalid_operation)
	relsize = 0;
      else
	  bfd_fatal (NULL);
    }
  if (relsize == 0)
    {
      bfd_set_reloc (obfd, osection, NULL, 0);
      osection->flags &= ~SEC_RELOC;
    }
  else
    {
      rel = (arelent **) malloc (relsize);
      if (rel == NULL)
	  fatal ("out of memory");

      relcount = bfd_canonicalize_reloc (ibfd, isection, rel, isym);
      if (relcount < 0)
	bfd_fatal ("relocation count is negative");

      bfd_set_reloc (obfd, osection, relcount == 0 ? NULL : rel, relcount);
      if (relcount == 0)
	{
	  osection->flags &= ~SEC_RELOC;
	  free (rel);
	}
    }
}

static void
_setcontent_section (bfd *ibfd, asection *isection, void *obfdarg)
{
  bfd *obfd = (bfd *) obfdarg;
  asection *osection = isection->output_section;
  struct section_list *p;
  bfd_size_type size = bfd_get_section_size (isection);

  if (osection == NULL)
    return;

  if (bfd_get_section_flags (ibfd, isection) & SEC_HAS_CONTENTS
      && bfd_get_section_flags (obfd, osection) & SEC_HAS_CONTENTS)
    {
      bfd_byte *memhunk = NULL;

      if (!bfd_get_full_section_contents (ibfd, isection, &memhunk)
	  || !bfd_convert_section_contents (ibfd, isection, obfd,
					    &memhunk, &size))
	bfd_fatal (NULL);

      if (!bfd_set_section_contents (obfd, osection, memhunk, 0, size))
	bfd_fatal (NULL);

      free (memhunk);
    }
}

static void
_get_max_vma (bfd *ibfd, asection *isection, void *ptr)
{
  unsigned long vma, *maxvma = (unsigned long *)ptr;

  if (! bfd_get_section_flags (ibfd, isection) & SEC_ALLOC)
    return;

  vma = bfd_get_section_vma (ibfd, isection) + bfd_section_size (ibfd, isection);
  if (vma > *maxvma)
    *maxvma = vma;
}

static void
_get_max_lma (bfd *ibfd, asection *isection, void *ptr)
{
  unsigned long lma, *maxlma = (unsigned long *)ptr;

  if (! bfd_get_section_flags (ibfd, isection) & SEC_ALLOC)
    return;

  lma = isection->lma + bfd_section_size (ibfd, isection);
  if (lma > *maxlma)
    *maxlma = lma;
}

static void
do_list (bfd *ibfd)
{

}

void
get_payload_addresses (bfd *ibfd, unsigned long *lma, unsigned long *vma)
{
  *lma = 0;
  *vma = 0;
  bfd_map_over_sections (ibfd, _get_max_lma, (void *)lma);
  bfd_map_over_sections (ibfd, _get_max_vma, (void *)vma);
}

asection *
create_payload_section (bfd *obfd, char *filename,
			unsigned long lma, unsigned long vma)
{
  asection *s;
  flagword flags;
  struct stat stat;

  flags = SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS
    | SEC_READONLY | SEC_DATA | SEC_LINKER_CREATED;
  s = bfd_make_section_anyway_with_flags (obfd, PAYLOAD_SECTNAME, flags);
  if (s == NULL)
    fatal ("can't add payload section");

  if (lstat (filename, &stat) < 0)
    fatal ("can't stat %s", filename);

  if (! bfd_set_section_size (obfd, s, sizeof (struct payload_hdr) + stat.st_size))
    fatal ("can't set payload section initial size");

  if (! bfd_set_section_alignment (obfd, s, 0))
    fatal ("can't set payload section alignment");

  if (! bfd_set_section_vma (obfd, s, vma))
    bfd_fatal ("failed to set section vma");

  s->lma = lma;

  /*
    Set output_section to NULL, to differentiate this new payload in
    next section walks. We have in fact no input equivalent.
  */
  s->output_section = NULL;

  return s;
}

void
fill_payload_section (bfd *obfd, asection *s, char *filename)
{
  int r;
  bfd_byte * buf;
  FILE * f;
  struct payload_hdr * hdr;
  struct stat st;
  size_t size;

  f = fopen (filename, "r");
  if (f == NULL)
    fatal ("%s: %s", filename, strerror (errno));

  r = stat (filename, &st);
  if (r < 0)
    fatal ("%s: stat failed: %s", filename, strerror (errno));

  size = sizeof (struct payload_hdr) + st.st_size;

  buf = calloc(1, size);
  if (buf == NULL)
    fatal ("calloc failed");

  hdr = (struct payload_hdr *)buf;
  hdr->magic = PAYLOAD_HDR_MAGIC;
  hdr->filename = squoze (filename);
  hdr->size = st.st_size;

  if (fread ((void *)(hdr + 1), 1, st.st_size, f) == 0 || ferror (f))
    fatal ("%s: fread failed", filename);
  fclose (f);

  if (! bfd_set_section_contents (obfd, s, buf, 0, size))
    bfd_fatal ("setting payload contents");

  free (buf);

}

void
update_payload_section (bfd *obfd, char *filename)
{
  int r;
  asection * s;
  size_t size, newsize;
  bfd_byte * buf;
  FILE * f;
  struct payload_hdr * hdr;
  struct stat st;

  s = bfd_get_section_by_name (obfd, PAYLOAD_SECTNAME);
  if (s == NULL)
    {
      flagword flags;

      flags = SEC_LINKER_CREATED | SEC_LOAD | SEC_HAS_CONTENTS;
      //	| SEC_READONLY | SEC_DATA;

      s = bfd_make_section_anyway_with_flags (obfd, PAYLOAD_SECTNAME, flags);
      if (s == NULL)
	bfd_fatal ("can't add payload section");
    }

  size = bfd_get_section_size (s);

  if (! (bfd_get_section_flags (obfd, s) & SEC_HAS_CONTENTS) || size == 0)
    buf = NULL;
  else
    {
      if (! bfd_get_full_section_contents (obfd, s, &buf))
	bfd_fatal ("couldn't get payload contents.");
    }

  f = fopen (filename, "r");
  if (f == NULL)
    fatal ("%s: %s", filename, strerror (errno));

  r = stat (filename, &st);
  if (r < 0)
    fatal ("%s: stat failed: %s", filename, strerror (errno));

  newsize = size + sizeof (struct payload_hdr) + st.st_size;

  if (!bfd_set_section_size (obfd, s, newsize))
    bfd_fatal ("failed to update payload section size");

  buf = realloc(buf, newsize);
  if (buf == NULL)
    fatal ("realloc failed");
  buf += size;

  hdr = (struct payload_hdr *)buf;
  hdr->magic = PAYLOAD_HDR_MAGIC;
  hdr->filename = squoze (filename);

  if (fread ((void *)(hdr + 1), 1, st.st_size, f) == 0 || ferror (f))
    fatal ("%s: fread failed", filename);
  fclose (f);

  bfd_set_section_contents (obfd, s, buf, 0, newsize);

  free (buf);
}

/*
  Poor man's objcopy.

  This copies a read-only bfd into a writable bfd, modifying it while
  creating it.

  Bfd does not support read-modify-write, so this is the only way to
  modify a binary. Historically, this has been made in binutils by
  objcopy (that shares code with strip).

  In here we simplify by copying to the same architecture.
*/
void
copy_bfd (bfd *ibfd, bfd *obfd,
	  char *add_file, unsigned long add_lma, unsigned long add_vma)
{
  long symcount;
  long symsize;
  flagword flags;
  asection *psec;

  /*
    Set format.
  */
  if (!bfd_set_format (obfd, bfd_get_format (ibfd)))
    bfd_fatal (NULL);

  /*
    Check sections.
  */
  if (ibfd->sections == NULL)
    fatal ("file has no sections");


  /*
    Set start and flags.
  */
  flags = bfd_get_file_flags (ibfd);
  flags &= bfd_applicable_file_flags (obfd);
  if (!bfd_set_start_address (obfd, bfd_get_start_address (ibfd))
      || !bfd_set_file_flags (obfd, flags))
    bfd_fatal (NULL);


  /*
    Set arch and mach.
  */
  if (bfd_get_arch (ibfd) == bfd_arch_unknown)
    fatal ("unable to recognise the format of the input file");
  if (!bfd_set_arch_mach (obfd, bfd_get_arch (ibfd), bfd_get_mach (ibfd)))
      bfd_fatal (NULL);

  if (!bfd_set_format (obfd, bfd_get_format (ibfd)))
    bfd_fatal (NULL);


  /*
    Copy symbols.
  */
  isym = NULL;
  osym = NULL;

  symsize = bfd_get_symtab_upper_bound (ibfd);
  if (symsize < 0)
    bfd_fatal (NULL);

  osym = isym = (asymbol **) malloc (symsize);
  if (isym == NULL)
    fatal ("out of memory");

  symcount = bfd_canonicalize_symtab (ibfd, isym);
  if (symcount < 0)
      bfd_fatal (NULL);

  if (symcount == 0)
    {
      free (isym);
      osym = isym = NULL;
    }


  /*
    Update sections.
  */

  /* Step 1: add sections. */
  bfd_map_over_sections (ibfd, _copyhdr_section, obfd);

  /* Step 3: add new payload section */
  if (add_file != NULL)
      psec = create_payload_section (obfd, add_file, add_lma, add_vma);

  /* Step 2: copy header data. */
  if (! bfd_copy_private_header_data (ibfd, obfd))
    bfd_fatal ("error in private header data");

  /* Step 4: set symbols. */
  bfd_set_symtab (obfd, osym, symcount);

  bfd_record_phdr (obfd, 1, false, 0, false, 0, false, false, 1, &psec);

  /* Step 5: copy relocations. */
  bfd_map_over_sections (ibfd, _setreloc_section, obfd);

  /* Step 6: copy contents. */
  bfd_map_over_sections (ibfd, _setcontent_section, obfd);

  /* Step 7: set payload contents */
  if (add_file != NULL)
    fill_payload_section (obfd, psec, add_file);

  if (! bfd_copy_private_bfd_data (ibfd, obfd))
    bfd_fatal ("error copying private data");
}


static void
usage (FILE *f, int status)
{
  fprintf (f, "Usage: %s [command] [file]\n", PROGNAME);
  fprintf (f, " Manage NUX payloads in 'file'.\n");
  fprintf (f, " Command is one of the following:\n\
  -l, --list             List all payloads (default if no options)\n\
  -a, --add=FILE         Add FILE as a payload\n\
  -D, --remove-all       Remove all payloads\n\
  -h, --help             Display this information\n\
  -V, --version          Display this program's version number\n\
\n");
  exit (status);
}

static void
print_version (void)
{
  printf ("%s %s\n", PROGNAME, VERSION);
  printf ("Copyright (C) 2015 Free Software Foundation, Inc.\n");
  printf ("\
This program is free software; you may redistribute it under the terms of\n\
the GNU General Public License version 3 or (at your option) any later version.\n\
This program has absolutely no warranty.\n");
  exit (0);
}

const struct option long_options[]=
  {
    {"list", 0, NULL, 'l'},
    {"add", 1, NULL, 'a'},
    {"delete-all", 0, NULL, 'D'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {0, no_argument, 0, 0}
  };


int
main (int argc, char * const argv[])
{
  int fd;
  int c;
  unsigned cmdseen;
  char *add, *file;
  bool rdonly;
  bool show_version, removeall, list;

  cmdseen = 0;
  rdonly = true;
  show_version = false;
  removeall = false;
  list = false;
  add = NULL;
  while ((c = getopt_long (argc, argv, "a:lDhV", long_options, NULL)) != EOF)
    {
      switch (c)
	{
	case 'a':
	  cmdseen++;
	  add = optarg;
	  rdonly = false;
	  break;
	case 'l':
	  cmdseen++;
	  list = true;
	  rdonly = true;
	  break;
	case 'D':
	  cmdseen++;
	  removeall = true;
	  rdonly = false;
	  break;
	case 'V':
	  cmdseen++;
	  show_version = true;
	  break;
	case 'h':
	  usage (stdout, 0);
	  break;
	default:
	  usage (stderr, 1);
	}
    }

  if (cmdseen > 1)
    usage (stderr, 1);

  if (cmdseen == 0)
    list = true;

  if (show_version)
    print_version ();

  if (optind + 1 != argc)
    usage (stderr, 1);

    file = argv[optind];


  bfd_init ();

  bfd *ibfd;
  char **obj_matching;

  ibfd = bfd_openr (file, NULL);
  if (ibfd == NULL)
    fatal ("%s: %s", file, bfd_errmsg (bfd_get_error ()));

  if (!bfd_check_format_matches (ibfd, bfd_object, &obj_matching))
    fatal ("%s: Not an object or executable", file);

  if (list)
    {
      bfd_map_over_sections (ibfd, _print_payload, stdout);
    }
  else if (add)
    {
      bfd *obfd;
      unsigned long lma, vma;

      obfd = bfd_openw (file, bfd_get_target (ibfd));
      if (obfd == NULL)
	fatal ("%s: %s", file, bfd_errmsg (bfd_get_error ()));

      get_payload_addresses (ibfd, &lma, &vma);

      copy_bfd (ibfd, obfd, add, lma, vma);

      if (! bfd_close (obfd))
	bfd_fatal ("bfd_close");
    }

  //  else if (removeall)
  //    do_removeall (ibfd);

  //  bfd_close (ibfd);
  close (fd);
}
