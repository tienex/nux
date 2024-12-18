#ifdef NUXPERF_DECLARE
#define NUXPERF(_s) extern nuxperf_t __perf _s
#endif

#ifdef NUXPERF_DEFINE
#define NUXPERF(_s) nuxperf_t __perf _s = { .name = #_s , .val = 0 }
#endif

NUXPERF(pnux_entry_syscall);
NUXPERF(pnux_entry_pagefault);
NUXPERF(pnux_entry_exception);
NUXPERF(pnux_entry_nmi);
NUXPERF(pnux_entry_timer);
NUXPERF(pnux_entry_irq);
NUXPERF(pnux_entry_ipi);
