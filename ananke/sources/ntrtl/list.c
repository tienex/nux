/** @file
  NT RTL List Utility Functions Implementation

  Additional list utility functions beyond the inline implementations.

  Copyright (C) 2025 ANANKE Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>
#include <ananke/ntrtl.h>

/* ---------------------------------------------------------------
 *  Doubly-Linked List Utilities
 * --------------------------------------------------------------- */

/**
  Get the length of a doubly-linked list.

  @param[in] ListHead  Head of list

  @return Number of entries in list
**/
UINTN
EFIAPI
RtlListLength (
  IN CONST LIST_ENTRY  *ListHead
  )
{
  UINTN Count = 0;
  CONST LIST_ENTRY *Entry;

  for (Entry = ListHead->Flink; Entry != ListHead; Entry = Entry->Flink) {
    Count++;
  }

  return Count;
}

/**
  Reverse a doubly-linked list.

  @param[in,out] ListHead  Head of list to reverse
**/
VOID
EFIAPI
RtlReverseList (
  IN OUT PLIST_ENTRY  ListHead
  )
{
  PLIST_ENTRY Current = ListHead->Flink;

  // Swap Flink and Blink for head
  PLIST_ENTRY Temp = ListHead->Flink;
  ListHead->Flink = ListHead->Blink;
  ListHead->Blink = Temp;

  // Swap Flink and Blink for all entries
  while (Current != ListHead) {
    PLIST_ENTRY Next = Current->Flink;

    Temp = Current->Flink;
    Current->Flink = Current->Blink;
    Current->Blink = Temp;

    Current = Next;
  }
}

/**
  Split a doubly-linked list into two lists.

  Splits the list at the given entry, with the first half remaining in the
  original list and the second half in the new list.

  @param[in,out] ListHead     Head of original list
  @param[in]     SplitEntry   Entry to split at (becomes first in new list)
  @param[out]    NewListHead  Head of new list

  @retval TRUE   List was split successfully
  @retval FALSE  Error occurred
**/
BOOLEAN
EFIAPI
RtlSplitList (
  IN OUT PLIST_ENTRY       ListHead,
  IN     PLIST_ENTRY       SplitEntry,
  OUT    PLIST_ENTRY       NewListHead
  )
{
  if (IsListEmpty(ListHead) || SplitEntry == ListHead) {
    InitializeListHead(NewListHead);
    return FALSE;
  }

  // New list: SplitEntry to ListHead->Blink
  NewListHead->Flink = SplitEntry;
  NewListHead->Blink = ListHead->Blink;

  // Update original list
  ListHead->Blink = SplitEntry->Blink;
  SplitEntry->Blink->Flink = ListHead;

  // Update new list boundaries
  SplitEntry->Blink = NewListHead;
  NewListHead->Blink->Flink = NewListHead;

  return TRUE;
}

/**
  Concatenate two doubly-linked lists.

  Appends the second list to the end of the first list.

  @param[in,out] ListHead1  Head of first list
  @param[in,out] ListHead2  Head of second list (becomes empty)
**/
VOID
EFIAPI
RtlConcatenateLists (
  IN OUT PLIST_ENTRY  ListHead1,
  IN OUT PLIST_ENTRY  ListHead2
  )
{
  if (IsListEmpty(ListHead2)) {
    return;
  }

  // Connect last entry of list1 to first entry of list2
  PLIST_ENTRY List1Last = ListHead1->Blink;
  PLIST_ENTRY List2First = ListHead2->Flink;
  PLIST_ENTRY List2Last = ListHead2->Blink;

  List1Last->Flink = List2First;
  List2First->Blink = List1Last;

  List2Last->Flink = ListHead1;
  ListHead1->Blink = List2Last;

  // List2 is now empty
  InitializeListHead(ListHead2);
}

/* ---------------------------------------------------------------
 *  Single-Linked List Utilities
 * --------------------------------------------------------------- */

/**
  Get the length of a single-linked list.

  @param[in] ListHead  Head of list

  @return Number of entries in list
**/
UINTN
EFIAPI
RtlSListLength (
  IN CONST SINGLE_LIST_ENTRY  *ListHead
  )
{
  UINTN Count = 0;
  CONST SINGLE_LIST_ENTRY *Entry;

  for (Entry = ListHead->Next; Entry != NULL; Entry = Entry->Next) {
    Count++;
  }

  return Count;
}

/**
  Reverse a single-linked list.

  @param[in,out] ListHead  Head of list to reverse
**/
VOID
EFIAPI
RtlReverseSList (
  IN OUT PSINGLE_LIST_ENTRY  ListHead
  )
{
  PSINGLE_LIST_ENTRY Prev = NULL;
  PSINGLE_LIST_ENTRY Current = ListHead->Next;

  while (Current != NULL) {
    PSINGLE_LIST_ENTRY Next = Current->Next;
    Current->Next = Prev;
    Prev = Current;
    Current = Next;
  }

  ListHead->Next = Prev;
}

/**
  Find an entry in a single-linked list.

  @param[in] ListHead  Head of list
  @param[in] Entry     Entry to find

  @retval TRUE   Entry was found in list
  @retval FALSE  Entry not found
**/
BOOLEAN
EFIAPI
RtlFindSListEntry (
  IN CONST SINGLE_LIST_ENTRY  *ListHead,
  IN CONST SINGLE_LIST_ENTRY  *Entry
  )
{
  CONST SINGLE_LIST_ENTRY *Current;

  for (Current = ListHead->Next; Current != NULL; Current = Current->Next) {
    if (Current == Entry) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Get the last entry in a single-linked list.

  @param[in] ListHead  Head of list

  @return Pointer to last entry, or NULL if list is empty
**/
PSINGLE_LIST_ENTRY
EFIAPI
RtlSListGetLast (
  IN CONST SINGLE_LIST_ENTRY  *ListHead
  )
{
  CONST SINGLE_LIST_ENTRY *Current = ListHead;

  if (ListHead->Next == NULL) {
    return NULL;
  }

  while (Current->Next != NULL) {
    Current = Current->Next;
  }

  return (PSINGLE_LIST_ENTRY)Current;
}
