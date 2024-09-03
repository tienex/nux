#include "internal.h"

unsigned long
umap_minaddr (void)
{
  return 0;
}

unsigned long
umap_maxaddr (void)
{
  return 1L << (39 + UMAP_LOG2_L4PTES);
}
