/*  linkedlist.c
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

#include "linkedlist.h"
#include "libtc/libtc.h"
// #include <stdlib.h>
#include <string.h>


#ifndef NEW
#define NEW(type,cnt) (type*)tc_malloc(sizeof(type)*cnt) 
#define NEWBYTES(cnt) tc_malloc(cnt) 
/* #define NEW(type,cnt) (type*)malloc(sizeof(type)*cnt) 
   #define NEWBYTES(cnt) malloc(cnt) */
#endif

#ifndef FREE
#define FREE(ptr) tc_free(ptr) 
/* #define FREE(ptr) free(ptr) */
#endif

/* creates a new linked list with the given element (t is copied) */
LinkedList* linked_list_new(const void* t, unsigned int size){
  void* t_new = NEWBYTES(size);
  memcpy(t_new, t, size);
  return linked_list_new_insitu(t_new);
}

/* creates a new linked list with the given element (t is directly used) */
LinkedList* linked_list_new_insitu(void* t){
  LinkedList* list = NEW(LinkedList, 1);
  list->next=0;
  list->item = t;
  return list;
}


/* adds(inserts) a new element to the linked list just after the given 
   entry. t is copied and the new entry is returned
 */
LinkedList* linked_list_add(LinkedList* list, const void* t, unsigned int size){
  LinkedList* next = list->next;
  LinkedList* new  = linked_list_new(t, size);
  new->next  = next;
  list->next = new;
  return new;
}
/* adds a new element to the linked list (t is directly used) and the new 
 * entry is returned
 */
LinkedList* linked_list_add_insitu(LinkedList* list, void* t){
  LinkedList* next = list->next;
  LinkedList* new  = linked_list_new_insitu(t);
  new->next  = next;
  list->next = new;
  return new;
}

/* returns last element of the list*/
LinkedList* linked_list_last(LinkedList* list){
  LinkedList* p = list;
  while(p && p->next){
    p=p->next;
  }
  return p;
}

/* removes and deletes the given linked list element with data 
 * and returns the next pointer.
 * possible usage:
 * list->next=list_list_remove(list->next);
 */
LinkedList* linked_list_remove(LinkedList* list){
  LinkedList* next = list->next;
  if(list->item) FREE(list->item);
  FREE(list);
  return next;
}

/* removes and deletes the following element with data
 * in the given linked list and bends the next pointer.
 */
void linked_list_remove_next(LinkedList* list){
  LinkedList* next  = list->next;
  if(next){
    LinkedList* nnext = next->next;
    if(next->item) FREE(next->item);
    FREE(next);
    list->next = nnext;
  }
}

/* deletes the entire linked list (as many elements as there are!)*/
void linked_list_delete(LinkedList* list){
  do{
    LinkedList* p = list->next;
    if(list->item) FREE(list->item);
    FREE(list);
    list = p;
  }while(list);
}
