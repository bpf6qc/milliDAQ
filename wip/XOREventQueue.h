#ifndef EVENT_NODE_H
#define EVENT_NODE_H

#include "Event.h"

// Define the structure for the queue of events
// Implemented as an XOR-linked list; each element has the event data and a single pointer npx,
// which is the XOR of pointers to its neighbors.
// Example queue state:
//
//     A        B        C        D
//   eventA   eventB   eventC   eventD
//   0^B=B     A^C      B^D      C^0=C
//
// We can easily pass around a single pointer to one end and move around with more XORs:
// Say we have a pointer to B above and want to move to C. Then if we know the address to A:
//     (pointer to A) ^ (B->npx) = A ^ (A^C) = C
// So if you know the address of your neighbor in the oppoiste direction, and the value of your own npx,
// you can move in any direction you want.
// We can abuse this by only caring about the last element,
// since we always know one of its neighbors is NULL. For D:
//     (NULL) ^ (D->npx) = NULL ^ (C^NULL) = C
// So we can work on the head element, remove it when we're done, and then move on until it's empty.
// This way we only need to pass around a single pointer to move many events around in memory.

namespace mdaq {

  typedef struct EventNode {
    Event * data;
    struct EventNode * npx; // XOR of the next and previous EventNodes
  } EventNode;

  class EventQueue {
  public:
    EventQueue() { head = NULL; };
    virtual ~EventQueue() {};

    EventNode * front() { return head; };

    void append(EventNode * thisNode);

    void append(const CAEN_DGTZ_X743_EVENT_t * evt, const CAEN_DGTZ_EventInfo_t * info) {
      EventNode * thisNode = new EventNode();
      thisNode->data->SetEvent(evt, info);
      append(thisNode);
    };
    void append(const CAEN_DGTZ_DPP_X743_Event_t ** evt, const CAEN_DGTZ_EventInfo_t * info, const uint32_t * NumEvents) {
      EventNode * thisNode = new EventNode();
      thisNode->data->SetEvent(DPPEvents, info, numEvents);
      append(thisNode);
    };

    void pop();

    unsigned int depth();

  private:
    // return the XOR of two addresses
    EventNode * XOR(EventNode * a, EventNode * b) {
      return (EventNode*) ((uintptr_t)(a) ^ (uintptr_t)(b));
    };

    EventNode * head;

  };

}

#endif
