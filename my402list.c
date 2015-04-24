#include <stdio.h>
#include <stdlib.h>
#include "my402list.h"
#include "cs402.h"

int  My402ListLength(My402List* list){
	My402ListElem *elem=NULL;
	int length=0;
	for(elem=My402ListFirst(list);elem!=NULL;elem=My402ListNext(list, elem)){
	length+=1;	
	}
	return length;
}

int  My402ListEmpty(My402List* list){
	if(My402ListLength(list)==0)
	return TRUE;
	else 
	return FALSE;
}

int  My402ListAppend(My402List* list, void* object){
	if(My402ListEmpty(list))
	{
		My402ListElem *new_elem=(My402ListElem *)malloc(sizeof(My402ListElem));
		if(new_elem==NULL)
		return FALSE;
		new_elem->next=&(list->anchor);
		new_elem->prev=&(list->anchor);
		new_elem->obj=object;
		(list->anchor).next=new_elem;
		(list->anchor).prev=new_elem;
		list->num_members +=1;
		return TRUE;
		}
	else
	{
		My402ListElem *last_elem=My402ListLast(list);
		My402ListElem *new_elem=(My402ListElem *)malloc(sizeof(My402ListElem));
		if(new_elem==NULL)
		return FALSE;
		new_elem->next=&(list->anchor);
		new_elem->prev=last_elem;
		new_elem->obj=object;
		last_elem->next=new_elem;
		(list->anchor).prev=new_elem;
		list->num_members+=1;
		return TRUE;
		}
	return FALSE;
}

int  My402ListPrepend(My402List* list, void* object){
	if(My402ListEmpty(list))
	{
		My402ListElem *new_elem=(My402ListElem *)malloc(sizeof(My402ListElem));
		if(new_elem==NULL)
		return FALSE;
		new_elem->next=&(list->anchor);
		new_elem->prev=&(list->anchor);
		new_elem->obj=object;
		(list->anchor).next=new_elem;
		(list->anchor).prev=new_elem;
		list->num_members +=1;
		return TRUE;
		}
	else
	{
		My402ListElem *first_elem=My402ListFirst(list);
		My402ListElem *new_elem=(My402ListElem *)malloc(sizeof(My402ListElem));
		if(new_elem==NULL)
		return FALSE;
		new_elem->next=first_elem;
		new_elem->prev=&(list->anchor);
		new_elem->obj=object;
		first_elem->prev=new_elem;
		(list->anchor).next=new_elem;
		list->num_members+=1;
		return TRUE;
		}
	return FALSE;
}

void My402ListUnlink(My402List* list, My402ListElem* elem){
	if(elem!=&(list->anchor)){
	My402ListElem *temp_elem_prev=elem->prev;
	My402ListElem *temp_elem_next=elem->next;
	temp_elem_prev->next=temp_elem_next;
	temp_elem_next->prev=temp_elem_prev;
	free(elem);
	list->num_members-=1;
	}
	else{
	/*check how to throw error*/
	fprintf(stderr,"Error:@Line:%d:: Trying to unlink an anchor\n",__LINE__);
	}
}

void My402ListUnlinkAll(My402List* list){
	My402ListElem *elem=NULL, *temp_elem=NULL;
	for(elem=My402ListFirst(list);elem!=NULL;elem=temp_elem){
		temp_elem=My402ListNext(list, elem);
		free(elem);
	}
	(list->anchor).next=&(list->anchor);
	(list->anchor).prev=&(list->anchor);
	list->num_members=0;
}

int  My402ListInsertAfter(My402List* list, void* object, My402ListElem* elem){
	if(elem==NULL){
		return My402ListAppend(list,object);
	}
	else{
		My402ListElem *new_elem=(My402ListElem *)malloc(sizeof(My402ListElem));
		if(new_elem==NULL)
		return FALSE;
		new_elem->next=elem->next;
		new_elem->prev=elem;
		elem->next->prev=new_elem;
		elem->next=new_elem;
		new_elem->obj=object;
		list->num_members +=1;
		return TRUE;
		}
	return FALSE;
}

int  My402ListInsertBefore(My402List* list, void* object, My402ListElem* elem){
	if(elem==NULL){
		return My402ListPrepend(list,object);
	}
	else{
		My402ListElem *new_elem=(My402ListElem *)malloc(sizeof(My402ListElem));
		if(new_elem==NULL)
		return FALSE;
		new_elem->next=elem;
		new_elem->prev=elem->prev;
		elem->prev->next=new_elem;
		elem->prev=new_elem;
		new_elem->obj=object;
		list->num_members +=1;
		return TRUE;
		}
	return FALSE;
}

My402ListElem *My402ListFirst(My402List* list){
	if((list->anchor).next!=&(list->anchor))
	return (list->anchor).next;
	else
	return NULL;
}

My402ListElem *My402ListLast(My402List* list){
	if((list->anchor).next!=&(list->anchor))
	return (list->anchor).prev;
	else
	return NULL;
}

My402ListElem *My402ListNext(My402List* list, My402ListElem* elem){
	return (elem->next==&(list->anchor) ? NULL: elem->next);
}

My402ListElem *My402ListPrev(My402List* list, My402ListElem* elem){
	return (elem->prev==&(list->anchor) ? NULL: elem->prev);
}

My402ListElem *My402ListFind(My402List* list, void* object){
	My402ListElem *elem=NULL;
	for(elem=My402ListFirst(list);elem!=NULL;elem=My402ListNext(list, elem)){
		if(elem->obj==object)
		return elem;
	}
	return NULL;
}

int My402ListInit(My402List* list){
	if(My402ListLength(list)!=0){
	My402ListUnlinkAll(list);
	}
	list->num_members=0;
	(list->anchor).obj=NULL;

	(list->anchor).next=&(list->anchor);
	(list->anchor).prev=&(list->anchor);
	return TRUE;
}
