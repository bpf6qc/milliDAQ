#include "EventQueue.h"

// append an event at the tail
void mdaq::EventQueue::append(EventNode * thisNode) {

  // if head doesn't exist, thisNode will be the only element so head=tail=thisNode
  if(head == NULL) head = thisNode;

  // if tail exists, we update the links between old tail and this new tail
  if(tail != NULL) {
    tail->nxt = thisNode;
    thisNode->prv = tail;
  }

  // lastly we update the tail to be this new tail
  tail = thisNode;
}

// append several EventNodes in their own queue
void mdaq::EventQueue::append(EventQueue * thisQueue) {

  // if head doesn't exist, we just become thisQueue
  if(head == NULL) {
    head = thisQueue->head;
    tail = thisQueue->tail;
  }

  // otherwise
  else {
    // make our tail's nxt point to thisQueue's head
    tail->nxt = thisQueue->head;

    // make thisQueue's head's prv point to our tail
    thisQueue->head->prv = tail;

    // make our new tail be thisQueue's tail
    tail = thisQueue->tail;
  }

  // lastly thisQueue is already covered by head..tail so clear it out
  thisQueue->head = NULL;
  thisQueue->tail = NULL;
}

// remove the head; delete its data and move the tail down one place
// perhaps pop() is confusing as for std::vector it removes the most recent element,
// but we want to deal with the oldest (thus the head) data first
void mdaq::EventQueue::pop() {
  // if the head is null, the queue is empty and we're done
  if(head == NULL) return;

  // Grab the current head
  EventNode * thisNode = head;

  // if there was a next element, make that the new head
  if(thisNode->nxt != NULL) head = thisNode->nxt;
  else head = NULL;

  // null out the new head's previous, and delete the old head
  head->prv = NULL;
  delete thisNode;
}

unsigned int mdaq::EventQueue::depth() {

  // To count the size of the queue, we know that head points to something with a NULL neighbor.
  // We also know that the other end of the queue has a NULL neighbor.
  // We just need to start at the head, and continue until we get to the NULL neighbor of the first.

  unsigned int nEvents = 0;

  EventNode * current = head;
  EventNode * previous = NULL;
  EventNode * next;

  while(current != NULL) {
    nEvents++;
    next = XOR(previous, current->npx);
    previous = current;
    current = next;
  }

  return nEvents;
}
