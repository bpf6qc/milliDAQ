#ifndef EVENT_NODE_H
#define EVENT_NODE_H

#include "Event.h"

// Define the structure of the queue of events
// Implemented as a doubly-linked list
//
// EventQueue is simply a containter for two pointers,
// the head and the tail of the queue.
//
// Individual elements (EventNode) contain their data (the actual V1743 event)
// and pointers to the next and previous elements in the list.

namespace mdaq {

  typedef struct EventNode {
    Event * data;
    struct EventNode * prv;
    struct EventNode * nxt;
  } EventNode;

  class EventQueue {
  public:
    EventQueue() {
      head = NULL;
      tail = NULL;
    };
    virtual ~EventQueue() {};

    void append(EventNode * thisNode);

    void append(const CAEN_DGTZ_X743_EVENT_t * evt, const CAEN_DGTZ_EventInfo_t * info) {
      EventNode * thisNode = new EventNode();
      thisNode->data->SetEvent(evt, info);
      append(thisNode);
    };
    void append(CAEN_DGTZ_DPP_X743_Event_t ** evt, const CAEN_DGTZ_EventInfo_t * info, const uint32_t * NumEvents) {
      EventNode * thisNode = new EventNode();
      thisNode->data->SetEvent(evt, info, NumEvents);
      append(thisNode);
    };

    void pop();

    unsigned int depth();

    EventNode * head;
    EventNode * tail;
  };

}

#endif
