#include "EventQueue.h"

void mdaq::EventQueue::append(EventNode * thisNode) {
  // if the head is null, this will be the first element and both its neighbors are null
  if(head == NULL) {
    thisNode->npx = NULL;
  }
  // otherwise, the new head's neighbors are the old head and null
  // and the old head's neighbors will become its original neighbor and the new head
  else {
    thisNode->npx = XOR(NULL, head);
    head->npx = XOR(thisNode, XOR(NULL, head->npx));
  }

  // lastly we update the head to be this new head
  head = thisNode;
}

// append several EventNodes in their own queue
void mdaq::EventQueue::append(EventQueue * thisQueue) {

  // if the head is null, this will become the entire queue and that's all
  if(head == NULL) {
    head = thisQueue;
    return;
  }

  // otherwise
  else {
    // first find the tail of the new queue by going backwards through thisQueue
    EventNode * current = thisQueue;
    EventNode * previous = NULL;
    EventNode * next;
    while(current != NULL) {
      next = XOR(previous, current->npx);
      previous = current;
      current = next;
    }


  }

}

// remove the head; delete the head's data and move the head down one place
void mdaq::EventQueue::pop() {
  // if the queue is empty, we're done
  if(head == NULL) return;

  // Grab the current head
  EventNode * thisNode = head;

  // Grab head's next, which is the XOR of nothing and head
  EventNode * next = XOR(NULL, head->npx);

  // if there was a next element, make that the new head and make its neighbors its old neighbor and null
  if(next != NULL) next->npx = XOR(head, XOR(NULL, next->npx));
  head = next;
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
