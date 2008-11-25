/*
 *  linkedlist.h
 *
 *  Copyright (C) Georg Martius - September 2008
 *
 *  This file is part of transcode, a video stream processing tool
 *
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _LINKEDLIST_H
#define _LINKEDLIST_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Classical linked list structure */
typedef struct _linkedlist {
    void* item;
    struct _linkedlist* next;
}LinkedList;

/* creates a new linked list with the given element (t is copied) */
LinkedList* linked_list_new(const void* t, unsigned int size);
/* creates a new linked list with the given element (t is directly used) */
LinkedList* linked_list_new_insitu(void* t);

/* adds a new element to the linked list (t is copied) and the new 
 * entry is returned
 */
LinkedList* linked_list_add(LinkedList* list, const void* t, unsigned int size);
/* adds a new element to the linked list (t is directly used) and the new 
 * entry is returned
 */
LinkedList* linked_list_add_insitu(LinkedList* list, void* t);

/* returns last element of the list*/
LinkedList* linked_list_last(LinkedList* list);

/* removes and deletes the given linked list element with data 
 * and returns the next pointer.
 * possible usage:
 * list->next=list_list_remove(list->next);
 */
LinkedList* linked_list_remove(LinkedList* list);

/* removes and deletes the following element with data
 * in the given linked list and bends the next pointer.
 */
void linked_list_remove_next(LinkedList* list);

/* deletes the entire linked list (as many elements as there are!)*/
void linked_list_delete(LinkedList* list);


#endif // _LINKEDLIST_H
