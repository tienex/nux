#include <cdefs.h>

typedef void (*fptr_t) (void);

extern const fptr_t __CTOR_LIST__ asm ("__CTOR_LIST__");
extern const fptr_t __CTOR_LIST_END__[];
extern const fptr_t __DTOR_LIST__ asm ("__DTOR_LIST__");
extern const fptr_t __DTOR_LIST_END__[];


static void __attribute__((section (".init"), used))
__do_global_ctors_aux (void)
{
  static int __initialized = 0;

  if (__initialized)
    return;

  __initialized = 1;

  for (const fptr_t * p = __CTOR_LIST_END__ - 1; *p != NULL; p--)
    {
      (**p) ();
    }
}

static void __do_global_dtors_aux (void) __attribute__((used));


static void __attribute__((section (".fini"))) __do_global_dtors_aux (void)
{
  static int __finished = 0;

  if (__finished)
    return;

  for (const fptr_t * p = &__DTOR_LIST__ + 1; p < __DTOR_LIST_END__; p++)
    {
      (**p) ();
    }
}
