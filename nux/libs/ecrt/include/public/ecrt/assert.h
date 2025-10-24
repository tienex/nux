/*
  EC - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __ecrt_assert_h__
#define __ecrt_assert_h__

#include <cdefs.h>
#include <stdlib.h>
#include <stdio.h>

#define _str(x) #x

#define assert(_e)							\
  do {									\
    if (!(_e)) {							\
      printf("Assertion '"# _e "' failed at "				\
	     __FILE__ ":%d\n", __LINE__);				\
      exit(-1);								\
    }									\
  } while(0)

#endif
