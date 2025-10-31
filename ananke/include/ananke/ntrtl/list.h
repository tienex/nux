/** @file
  NT RTL List Functions

  Intrusive doubly-linked and single-linked list functions following
  Windows NT RTL conventions.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ANANKE_NTRTL_LIST_H__
#define __ANANKE_NTRTL_LIST_H__

/* ---------------------------------------------------------------
 *  Doubly-Linked List Functions
 * --------------------------------------------------------------- */

/**
  Initialize a list head.

  @param[out] ListHead  List head to initialize
**/
static INLINE VOID
InitializeListHead (
    OUT PLIST_ENTRY  ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

/**
  Check if a list is empty.

  @param[in] ListHead  List head

  @retval TRUE   List is empty
  @retval FALSE  List contains entries
**/
static INLINE BOOLEAN
IsListEmpty (
    IN CONST LIST_ENTRY  *ListHead
    )
{
    return (ListHead->Flink == ListHead);
}

/**
  Remove an entry from a list.

  @param[in] Entry  Entry to remove

  @retval TRUE   Entry was in a list and has been removed
  @retval FALSE  Entry was not in a list
**/
static INLINE BOOLEAN
RemoveEntryList (
    IN PLIST_ENTRY  Entry
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;

    return (Flink != Blink);
}

/**
  Insert an entry at the head of a list.

  @param[in,out] ListHead  List head
  @param[in]     Entry     Entry to insert
**/
static INLINE VOID
InsertHeadList (
    IN OUT PLIST_ENTRY  ListHead,
    IN     PLIST_ENTRY  Entry
    )
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}

/**
  Insert an entry at the tail of a list.

  @param[in,out] ListHead  List head
  @param[in]     Entry     Entry to insert
**/
static INLINE VOID
InsertTailList (
    IN OUT PLIST_ENTRY  ListHead,
    IN     PLIST_ENTRY  Entry
    )
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}

/**
  Remove the first entry from a list.

  @param[in,out] ListHead  List head

  @return Pointer to removed entry, or list head if list was empty
**/
static INLINE PLIST_ENTRY
RemoveHeadList (
    IN OUT PLIST_ENTRY  ListHead
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;

    return Entry;
}

/**
  Remove the last entry from a list.

  @param[in,out] ListHead  List head

  @return Pointer to removed entry, or list head if list was empty
**/
static INLINE PLIST_ENTRY
RemoveTailList (
    IN OUT PLIST_ENTRY  ListHead
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;

    return Entry;
}

/**
  Append one list to another (splice).

  @param[in,out] ListHead  Destination list head
  @param[in,out] ListToAppend  List to append (will be empty after operation)
**/
static INLINE VOID
AppendTailList (
    IN OUT PLIST_ENTRY  ListHead,
    IN OUT PLIST_ENTRY  ListToAppend
    )
{
    PLIST_ENTRY ListEnd = ListHead->Blink;

    if (!IsListEmpty(ListToAppend)) {
        ListHead->Blink->Flink = ListToAppend->Flink;
        ListToAppend->Flink->Blink = ListHead->Blink;
        ListHead->Blink = ListToAppend->Blink;
        ListToAppend->Blink->Flink = ListHead;
        InitializeListHead(ListToAppend);
    }
}

/**
  Get the containing structure from a list entry.

  @param[in] Address  Address of list entry
  @param[in] Type     Type of containing structure
  @param[in] Field    Name of list entry field in structure

  @return Pointer to containing structure
**/
#define CONTAINING_RECORD(Address, Type, Field) \
    ((Type *)((CHAR8 *)(Address) - ANX_OFFSETOF(Type, Field)))

/**
  Iterate through a list.

  @param[in] ListHead  List head
  @param[in] Entry     Variable to receive each entry

  Usage:
    LIST_FOR_EACH(ListHead, Entry) {
        MyStruct *Item = CONTAINING_RECORD(Entry, MyStruct, ListEntry);
        // Process Item...
    }
**/
#define LIST_FOR_EACH(ListHead, Entry) \
    for ((Entry) = (ListHead)->Flink; \
         (Entry) != (ListHead); \
         (Entry) = (Entry)->Flink)

/**
  Iterate through a list safely (allows removal during iteration).

  @param[in] ListHead  List head
  @param[in] Entry     Variable to receive each entry
  @param[in] NextEntry Temporary variable for next entry
**/
#define LIST_FOR_EACH_SAFE(ListHead, Entry, NextEntry) \
    for ((Entry) = (ListHead)->Flink, (NextEntry) = (Entry)->Flink; \
         (Entry) != (ListHead); \
         (Entry) = (NextEntry), (NextEntry) = (Entry)->Flink)

/* ---------------------------------------------------------------
 *  Single-Linked List Functions
 * --------------------------------------------------------------- */

/**
  Initialize a single-linked list head.

  @param[out] ListHead  List head to initialize
**/
static INLINE VOID
InitializeSListHead (
    OUT PSINGLE_LIST_ENTRY  ListHead
    )
{
    ListHead->Next = NULL;
}

/**
  Check if a single-linked list is empty.

  @param[in] ListHead  List head

  @retval TRUE   List is empty
  @retval FALSE  List contains entries
**/
static INLINE BOOLEAN
IsSListEmpty (
    IN CONST SINGLE_LIST_ENTRY  *ListHead
    )
{
    return (ListHead->Next == NULL);
}

/**
  Insert an entry at the head of a single-linked list.

  @param[in,out] ListHead  List head
  @param[in]     Entry     Entry to insert
**/
static INLINE VOID
PushEntryList (
    IN OUT PSINGLE_LIST_ENTRY  ListHead,
    IN     PSINGLE_LIST_ENTRY  Entry
    )
{
    Entry->Next = ListHead->Next;
    ListHead->Next = Entry;
}

/**
  Remove an entry from the head of a single-linked list.

  @param[in,out] ListHead  List head

  @return Pointer to removed entry, or NULL if list was empty
**/
static INLINE PSINGLE_LIST_ENTRY
PopEntryList (
    IN OUT PSINGLE_LIST_ENTRY  ListHead
    )
{
    PSINGLE_LIST_ENTRY FirstEntry;

    FirstEntry = ListHead->Next;
    if (FirstEntry != NULL) {
        ListHead->Next = FirstEntry->Next;
    }

    return FirstEntry;
}

/**
  Count the number of entries in a doubly-linked list.

  @param[in] ListHead  List head

  @return Number of entries in list
**/
UINTN
EFIAPI
RtlListLength (
    IN CONST LIST_ENTRY  *ListHead
    );

/**
  Count the number of entries in a single-linked list.

  @param[in] ListHead  List head

  @return Number of entries in list
**/
UINTN
EFIAPI
RtlSListLength (
    IN CONST SINGLE_LIST_ENTRY  *ListHead
    );

/**
  Reverse a doubly-linked list.

  @param[in,out] ListHead  List head
**/
VOID
EFIAPI
RtlReverseList (
    IN OUT PLIST_ENTRY  ListHead
    );

/**
  Reverse a single-linked list.

  @param[in,out] ListHead  List head
**/
VOID
EFIAPI
RtlReverseSList (
    IN OUT PSINGLE_LIST_ENTRY  ListHead
    );

/**
  Split a doubly-linked list into two lists.

  @param[in,out] ListHead     Head of original list
  @param[in]     SplitEntry   Entry to split at
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
    );

/**
  Concatenate two doubly-linked lists.

  @param[in,out] ListHead1  Head of first list
  @param[in,out] ListHead2  Head of second list
**/
VOID
EFIAPI
RtlConcatenateLists (
    IN OUT PLIST_ENTRY  ListHead1,
    IN OUT PLIST_ENTRY  ListHead2
    );

/**
  Find an entry in a single-linked list.

  @param[in] ListHead  Head of list
  @param[in] Entry     Entry to find

  @retval TRUE   Entry was found
  @retval FALSE  Entry not found
**/
BOOLEAN
EFIAPI
RtlFindSListEntry (
    IN CONST SINGLE_LIST_ENTRY  *ListHead,
    IN CONST SINGLE_LIST_ENTRY  *Entry
    );

/**
  Get the last entry in a single-linked list.

  @param[in] ListHead  Head of list

  @return Pointer to last entry, or NULL if list is empty
**/
PSINGLE_LIST_ENTRY
EFIAPI
RtlSListGetLast (
    IN CONST SINGLE_LIST_ENTRY  *ListHead
    );

#endif /* __ANANKE_NTRTL_LIST_H__ */
