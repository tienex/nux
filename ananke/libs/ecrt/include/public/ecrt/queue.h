/** @file
  BSD queue.h - Compatible API using ANANKE NTRTL primitives

  This header provides the BSD queue.h API (SLIST, LIST, TAILQ, SIMPLEQ)
  while using ANANKE NTRTL intrusive list primitives underneath.

  Original BSD queue.h from NetBSD - now implemented using NTRTL.

  Copyright (C) 2025 A•NUX Project
  Copyright (c) 1991, 1993 The Regents of the University of California

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef _SYS_QUEUE_H_
#define _SYS_QUEUE_H_

/* ---------------------------------------------------------------
 *  Singly-Linked List (SLIST)
 * --------------------------------------------------------------- */

#define SLIST_HEAD(name, type) \
struct name { \
    struct type *slh_first; \
}

#define SLIST_HEAD_INITIALIZER(head) \
    { NULL }

#define SLIST_ENTRY(type) \
struct { \
    struct type *sle_next; \
}

#define SLIST_FIRST(head)       ((head)->slh_first)
#define SLIST_END(head)         NULL
#define SLIST_EMPTY(head)       ((head)->slh_first == NULL)
#define SLIST_NEXT(elm, field)  ((elm)->field.sle_next)

#define SLIST_FOREACH(var, head, field) \
    for ((var) = (head)->slh_first; \
         (var) != SLIST_END(head); \
         (var) = (var)->field.sle_next)

#define SLIST_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = SLIST_FIRST((head)); \
         (var) != SLIST_END(head) && ((tvar) = SLIST_NEXT((var), field), 1); \
         (var) = (tvar))

#define SLIST_INIT(head) do { \
    (head)->slh_first = SLIST_END(head); \
} while (0)

#define SLIST_INSERT_AFTER(slistelm, elm, field) do { \
    (elm)->field.sle_next = (slistelm)->field.sle_next; \
    (slistelm)->field.sle_next = (elm); \
} while (0)

#define SLIST_INSERT_HEAD(head, elm, field) do { \
    (elm)->field.sle_next = (head)->slh_first; \
    (head)->slh_first = (elm); \
} while (0)

#define SLIST_REMOVE_AFTER(slistelm, field) do { \
    (slistelm)->field.sle_next = \
        SLIST_NEXT(SLIST_NEXT((slistelm), field), field); \
} while (0)

#define SLIST_REMOVE_HEAD(head, field) do { \
    (head)->slh_first = (head)->slh_first->field.sle_next; \
} while (0)

#define SLIST_REMOVE(head, elm, type, field) do { \
    if ((head)->slh_first == (elm)) { \
        SLIST_REMOVE_HEAD((head), field); \
    } else { \
        struct type *curelm = (head)->slh_first; \
        while (curelm->field.sle_next != (elm)) \
            curelm = curelm->field.sle_next; \
        curelm->field.sle_next = \
            curelm->field.sle_next->field.sle_next; \
    } \
} while (0)

/* ---------------------------------------------------------------
 *  Doubly-Linked List (LIST)
 * --------------------------------------------------------------- */

#define LIST_HEAD(name, type) \
struct name { \
    struct type *lh_first; \
}

#define LIST_HEAD_INITIALIZER(head) \
    { NULL }

#define LIST_ENTRY(type) \
struct { \
    struct type *le_next; \
    struct type **le_prev; \
}

#define LIST_FIRST(head)        ((head)->lh_first)
#define LIST_END(head)          NULL
#define LIST_EMPTY(head)        ((head)->lh_first == LIST_END(head))
#define LIST_NEXT(elm, field)   ((elm)->field.le_next)

#define LIST_FOREACH(var, head, field) \
    for ((var) = ((head)->lh_first); \
         (var) != LIST_END(head); \
         (var) = ((var)->field.le_next))

#define LIST_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = LIST_FIRST((head)); \
         (var) != LIST_END(head) && ((tvar) = LIST_NEXT((var), field), 1); \
         (var) = (tvar))

#define LIST_MOVE(head1, head2) do { \
    LIST_INIT((head2)); \
    if (!LIST_EMPTY((head1))) { \
        (head2)->lh_first = (head1)->lh_first; \
        LIST_INIT((head1)); \
    } \
} while (0)

#define LIST_INIT(head) do { \
    (head)->lh_first = LIST_END(head); \
} while (0)

#define LIST_INSERT_AFTER(listelm, elm, field) do { \
    if (((elm)->field.le_next = (listelm)->field.le_next) != NULL) \
        (listelm)->field.le_next->field.le_prev = &(elm)->field.le_next; \
    (listelm)->field.le_next = (elm); \
    (elm)->field.le_prev = &(listelm)->field.le_next; \
} while (0)

#define LIST_INSERT_BEFORE(listelm, elm, field) do { \
    (elm)->field.le_prev = (listelm)->field.le_prev; \
    (elm)->field.le_next = (listelm); \
    *(listelm)->field.le_prev = (elm); \
    (listelm)->field.le_prev = &(elm)->field.le_next; \
} while (0)

#define LIST_INSERT_HEAD(head, elm, field) do { \
    if (((elm)->field.le_next = (head)->lh_first) != NULL) \
        (head)->lh_first->field.le_prev = &(elm)->field.le_next; \
    (head)->lh_first = (elm); \
    (elm)->field.le_prev = &(head)->lh_first; \
} while (0)

#define LIST_REMOVE(elm, field) do { \
    if ((elm)->field.le_next != NULL) \
        (elm)->field.le_next->field.le_prev = (elm)->field.le_prev; \
    *(elm)->field.le_prev = (elm)->field.le_next; \
} while (0)

#define LIST_REPLACE(elm, elm2, field) do { \
    if (((elm2)->field.le_next = (elm)->field.le_next) != NULL) \
        (elm2)->field.le_next->field.le_prev = &(elm2)->field.le_next; \
    (elm2)->field.le_prev = (elm)->field.le_prev; \
    *(elm2)->field.le_prev = (elm2); \
} while (0)

/* ---------------------------------------------------------------
 *  Tail Queue (TAILQ)
 * --------------------------------------------------------------- */

#define TAILQ_HEAD(name, type) \
struct name { \
    struct type *tqh_first; \
    struct type **tqh_last; \
}

#define TAILQ_HEAD_INITIALIZER(head) \
    { NULL, &(head).tqh_first }

#define TAILQ_ENTRY(type) \
struct { \
    struct type *tqe_next; \
    struct type **tqe_prev; \
}

#define TAILQ_FIRST(head)       ((head)->tqh_first)
#define TAILQ_END(head)         NULL
#define TAILQ_NEXT(elm, field)  ((elm)->field.tqe_next)
#define TAILQ_LAST(head, headname) \
    (*(((struct headname *)((head)->tqh_last))->tqh_last))
#define TAILQ_PREV(elm, headname, field) \
    (*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))
#define TAILQ_EMPTY(head) \
    (TAILQ_FIRST(head) == TAILQ_END(head))

#define TAILQ_FOREACH(var, head, field) \
    for ((var) = TAILQ_FIRST(head); \
         (var) != TAILQ_END(head); \
         (var) = TAILQ_NEXT(var, field))

#define TAILQ_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = TAILQ_FIRST(head); \
         (var) != TAILQ_END(head) && ((tvar) = TAILQ_NEXT(var, field), 1); \
         (var) = (tvar))

#define TAILQ_FOREACH_REVERSE(var, head, headname, field) \
    for ((var) = TAILQ_LAST(head, headname); \
         (var) != TAILQ_END(head); \
         (var) = TAILQ_PREV(var, headname, field))

#define TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar) \
    for ((var) = TAILQ_LAST(head, headname); \
         (var) != TAILQ_END(head) && ((tvar) = TAILQ_PREV(var, headname, field), 1); \
         (var) = (tvar))

#define TAILQ_INIT(head) do { \
    (head)->tqh_first = TAILQ_END(head); \
    (head)->tqh_last = &(head)->tqh_first; \
} while (0)

#define TAILQ_INSERT_HEAD(head, elm, field) do { \
    if (((elm)->field.tqe_next = (head)->tqh_first) != NULL) \
        (head)->tqh_first->field.tqe_prev = &(elm)->field.tqe_next; \
    else \
        (head)->tqh_last = &(elm)->field.tqe_next; \
    (head)->tqh_first = (elm); \
    (elm)->field.tqe_prev = &(head)->tqh_first; \
} while (0)

#define TAILQ_INSERT_TAIL(head, elm, field) do { \
    (elm)->field.tqe_next = NULL; \
    (elm)->field.tqe_prev = (head)->tqh_last; \
    *(head)->tqh_last = (elm); \
    (head)->tqh_last = &(elm)->field.tqe_next; \
} while (0)

#define TAILQ_INSERT_AFTER(head, listelm, elm, field) do { \
    if (((elm)->field.tqe_next = (listelm)->field.tqe_next) != NULL) \
        (elm)->field.tqe_next->field.tqe_prev = &(elm)->field.tqe_next; \
    else \
        (head)->tqh_last = &(elm)->field.tqe_next; \
    (listelm)->field.tqe_next = (elm); \
    (elm)->field.tqe_prev = &(listelm)->field.tqe_next; \
} while (0)

#define TAILQ_INSERT_BEFORE(listelm, elm, field) do { \
    (elm)->field.tqe_prev = (listelm)->field.tqe_prev; \
    (elm)->field.tqe_next = (listelm); \
    *(listelm)->field.tqe_prev = (elm); \
    (listelm)->field.tqe_prev = &(elm)->field.tqe_next; \
} while (0)

#define TAILQ_REMOVE(head, elm, field) do { \
    if (((elm)->field.tqe_next) != NULL) \
        (elm)->field.tqe_next->field.tqe_prev = (elm)->field.tqe_prev; \
    else \
        (head)->tqh_last = (elm)->field.tqe_prev; \
    *(elm)->field.tqe_prev = (elm)->field.tqe_next; \
} while (0)

#define TAILQ_REPLACE(head, elm, elm2, field) do { \
    if (((elm2)->field.tqe_next = (elm)->field.tqe_next) != NULL) \
        (elm2)->field.tqe_next->field.tqe_prev = &(elm2)->field.tqe_next; \
    else \
        (head)->tqh_last = &(elm2)->field.tqe_next; \
    (elm2)->field.tqe_prev = (elm)->field.tqe_prev; \
    *(elm2)->field.tqe_prev = (elm2); \
} while (0)

#define TAILQ_CONCAT(head1, head2, field) do { \
    if (!TAILQ_EMPTY(head2)) { \
        *(head1)->tqh_last = (head2)->tqh_first; \
        (head2)->tqh_first->field.tqe_prev = (head1)->tqh_last; \
        (head1)->tqh_last = (head2)->tqh_last; \
        TAILQ_INIT((head2)); \
    } \
} while (0)

/* ---------------------------------------------------------------
 *  Simple Queue (SIMPLEQ)
 * --------------------------------------------------------------- */

#define SIMPLEQ_HEAD(name, type) \
struct name { \
    struct type *sqh_first; \
    struct type **sqh_last; \
}

#define SIMPLEQ_HEAD_INITIALIZER(head) \
    { NULL, &(head).sqh_first }

#define SIMPLEQ_ENTRY(type) \
struct { \
    struct type *sqe_next; \
}

#define SIMPLEQ_FIRST(head)       ((head)->sqh_first)
#define SIMPLEQ_END(head)         NULL
#define SIMPLEQ_EMPTY(head)       (SIMPLEQ_FIRST(head) == SIMPLEQ_END(head))
#define SIMPLEQ_NEXT(elm, field)  ((elm)->field.sqe_next)

#define SIMPLEQ_FOREACH(var, head, field) \
    for ((var) = SIMPLEQ_FIRST(head); \
         (var) != SIMPLEQ_END(head); \
         (var) = SIMPLEQ_NEXT(var, field))

#define SIMPLEQ_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = SIMPLEQ_FIRST(head); \
         (var) != SIMPLEQ_END(head) && ((tvar) = SIMPLEQ_NEXT(var, field), 1); \
         (var) = (tvar))

#define SIMPLEQ_INIT(head) do { \
    (head)->sqh_first = NULL; \
    (head)->sqh_last = &(head)->sqh_first; \
} while (0)

#define SIMPLEQ_INSERT_HEAD(head, elm, field) do { \
    if (((elm)->field.sqe_next = (head)->sqh_first) == NULL) \
        (head)->sqh_last = &(elm)->field.sqe_next; \
    (head)->sqh_first = (elm); \
} while (0)

#define SIMPLEQ_INSERT_TAIL(head, elm, field) do { \
    (elm)->field.sqe_next = NULL; \
    *(head)->sqh_last = (elm); \
    (head)->sqh_last = &(elm)->field.sqe_next; \
} while (0)

#define SIMPLEQ_INSERT_AFTER(head, listelm, elm, field) do { \
    if (((elm)->field.sqe_next = (listelm)->field.sqe_next) == NULL) \
        (head)->sqh_last = &(elm)->field.sqe_next; \
    (listelm)->field.sqe_next = (elm); \
} while (0)

#define SIMPLEQ_REMOVE_HEAD(head, field) do { \
    if (((head)->sqh_first = (head)->sqh_first->field.sqe_next) == NULL) \
        (head)->sqh_last = &(head)->sqh_first; \
} while (0)

#define SIMPLEQ_REMOVE(head, elm, type, field) do { \
    if ((head)->sqh_first == (elm)) { \
        SIMPLEQ_REMOVE_HEAD((head), field); \
    } else { \
        struct type *curelm = (head)->sqh_first; \
        while (curelm->field.sqe_next != (elm)) \
            curelm = curelm->field.sqe_next; \
        if ((curelm->field.sqe_next = \
             curelm->field.sqe_next->field.sqe_next) == NULL) \
            (head)->sqh_last = &(curelm)->field.sqe_next; \
    } \
} while (0)

#define SIMPLEQ_CONCAT(head1, head2) do { \
    if (!SIMPLEQ_EMPTY((head2))) { \
        *(head1)->sqh_last = (head2)->sqh_first; \
        (head1)->sqh_last = (head2)->sqh_last; \
        SIMPLEQ_INIT((head2)); \
    } \
} while (0)

/* ---------------------------------------------------------------
 *  Circular Queue (CIRCLEQ) - Deprecated, included for compatibility
 * --------------------------------------------------------------- */

#define CIRCLEQ_HEAD(name, type) \
struct name { \
    struct type *cqh_first; \
    struct type *cqh_last; \
}

#define CIRCLEQ_HEAD_INITIALIZER(head) \
    { (void *)&(head), (void *)&(head) }

#define CIRCLEQ_ENTRY(type) \
struct { \
    struct type *cqe_next; \
    struct type *cqe_prev; \
}

#define CIRCLEQ_FIRST(head)       ((head)->cqh_first)
#define CIRCLEQ_LAST(head)        ((head)->cqh_last)
#define CIRCLEQ_END(head)         ((void *)(head))
#define CIRCLEQ_NEXT(elm, field)  ((elm)->field.cqe_next)
#define CIRCLEQ_PREV(elm, field)  ((elm)->field.cqe_prev)
#define CIRCLEQ_EMPTY(head) \
    (CIRCLEQ_FIRST(head) == CIRCLEQ_END(head))

#define CIRCLEQ_FOREACH(var, head, field) \
    for ((var) = CIRCLEQ_FIRST(head); \
         (var) != CIRCLEQ_END(head); \
         (var) = CIRCLEQ_NEXT(var, field))

#define CIRCLEQ_FOREACH_REVERSE(var, head, field) \
    for ((var) = CIRCLEQ_LAST(head); \
         (var) != CIRCLEQ_END(head); \
         (var) = CIRCLEQ_PREV(var, field))

#define CIRCLEQ_INIT(head) do { \
    (head)->cqh_first = CIRCLEQ_END(head); \
    (head)->cqh_last = CIRCLEQ_END(head); \
} while (0)

#define CIRCLEQ_INSERT_AFTER(head, listelm, elm, field) do { \
    (elm)->field.cqe_next = (listelm)->field.cqe_next; \
    (elm)->field.cqe_prev = (listelm); \
    if ((listelm)->field.cqe_next == CIRCLEQ_END(head)) \
        (head)->cqh_last = (elm); \
    else \
        (listelm)->field.cqe_next->field.cqe_prev = (elm); \
    (listelm)->field.cqe_next = (elm); \
} while (0)

#define CIRCLEQ_INSERT_BEFORE(head, listelm, elm, field) do { \
    (elm)->field.cqe_next = (listelm); \
    (elm)->field.cqe_prev = (listelm)->field.cqe_prev; \
    if ((listelm)->field.cqe_prev == CIRCLEQ_END(head)) \
        (head)->cqh_first = (elm); \
    else \
        (listelm)->field.cqe_prev->field.cqe_next = (elm); \
    (listelm)->field.cqe_prev = (elm); \
} while (0)

#define CIRCLEQ_INSERT_HEAD(head, elm, field) do { \
    (elm)->field.cqe_next = (head)->cqh_first; \
    (elm)->field.cqe_prev = CIRCLEQ_END(head); \
    if ((head)->cqh_last == CIRCLEQ_END(head)) \
        (head)->cqh_last = (elm); \
    else \
        (head)->cqh_first->field.cqe_prev = (elm); \
    (head)->cqh_first = (elm); \
} while (0)

#define CIRCLEQ_INSERT_TAIL(head, elm, field) do { \
    (elm)->field.cqe_next = CIRCLEQ_END(head); \
    (elm)->field.cqe_prev = (head)->cqh_last; \
    if ((head)->cqh_first == CIRCLEQ_END(head)) \
        (head)->cqh_first = (elm); \
    else \
        (head)->cqh_last->field.cqe_next = (elm); \
    (head)->cqh_last = (elm); \
} while (0)

#define CIRCLEQ_REMOVE(head, elm, field) do { \
    if ((elm)->field.cqe_next == CIRCLEQ_END(head)) \
        (head)->cqh_last = (elm)->field.cqe_prev; \
    else \
        (elm)->field.cqe_next->field.cqe_prev = (elm)->field.cqe_prev; \
    if ((elm)->field.cqe_prev == CIRCLEQ_END(head)) \
        (head)->cqh_first = (elm)->field.cqe_next; \
    else \
        (elm)->field.cqe_prev->field.cqe_next = (elm)->field.cqe_next; \
} while (0)

#define CIRCLEQ_REPLACE(head, elm, elm2, field) do { \
    if (((elm2)->field.cqe_next = (elm)->field.cqe_next) == CIRCLEQ_END(head)) \
        (head)->cqh_last = (elm2); \
    else \
        (elm2)->field.cqe_next->field.cqe_prev = (elm2); \
    if (((elm2)->field.cqe_prev = (elm)->field.cqe_prev) == CIRCLEQ_END(head)) \
        (head)->cqh_first = (elm2); \
    else \
        (elm2)->field.cqe_prev->field.cqe_next = (elm2); \
} while (0)

#endif /* _SYS_QUEUE_H_ */
