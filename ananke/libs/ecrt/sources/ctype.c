/** @file
  eCRT - An embedded C runtime library

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/


/* libkern.h */

int
isspace (int ch)
{
  return (ch == ' ' || (ch >= '\t' && ch <= '\r'));
}
