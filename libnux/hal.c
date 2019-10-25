#include <nux/nux.h>
#include <nux/hal.h>
#include "internal.h"


pfn_t hal_req_pfnalloc (void)
{
  return pfn_alloc (0);
}

void hal_req_pfnfree (pfn_t pfn)
{
  pfn_free (pfn);
}
