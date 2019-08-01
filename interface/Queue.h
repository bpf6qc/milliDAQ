#ifndef QUEUE_H
#define QUEUE_H

#include <atomic>
#include <cassert>
#include <memory>
#include <stdexcept>

#include "CAENDigitizer.h"

#include "V1743Event.h"

// Define the structure of the queue of events
// Implemented as a circular buffer with a pre-allocated pool of events
// Results in backpressure when the write index catches up the the read index
// So this needs to be a big enough pool (or fast enough writer)

#define POOL_SIZE 5000

namespace mdaq {

  class Queue {
  public:
    Queue(const Queue&) = delete;
    Queue& operator= (const Queue&) = delete;

    explicit Queue() :
    data(static_cast<V1743Event*>(std::malloc(POOL_SIZE * sizeof(V1743Event)))),
      previousTTT(0),
      readIndex(0),
      writeIndex(0)
      
	{
	  assert(POOL_SIZE >= 2);
	  if(!data) throw std::bad_alloc();

	  for(int i = 0; i < nGroupsPerDigitizer; i++) { previousTDC[i] = 0; };

	  for(int i = 0; i < POOL_SIZE; i++) { new (&data[i]) V1743Event(); };
	};

    // delete any remaining data
    ~Queue() {
      /*
        for(int i = 0; i < POOL_SIZE; i++) {
	data[i].~Event();
        }
      */
      std::free(data);
    };

    void SetTimeUnits(long SAMFrequency) {
      // 3.2 GS/s -- 0L
      // 1.6 GS/s -- 1L
      // 0.8 GS/s -- 2L
      // 0.4 GS/s -- 3L
      tdcClockTick = 5.0 * (1 << SAMFrequency);
      nanosecondsPerSample = (1.0 / 3.2) * (1 << SAMFrequency);
    };

    //////////////////////////////////////////////////
    // Method to convert CAEN classes to V1743Event
    // To make V1743 portable for analyzers, converting
    // these here before writing to disk removes
    // dependence on the CAEN library
    //////////////////////////////////////////////////

    void SetEvent(CAEN_DGTZ_X743_EVENT_t * evt, 
                  CAEN_DGTZ_EventInfo_t * info, 
                  V1743Event& data);

    void SetEvent(V1743Event * evt, V1743Event& data);

    //////////////////////////////////////////////////
    // Queue/dequeue methods
    //////////////////////////////////////////////////

    // set the next queue element's values to writeIndex, then update the writeIndex
    bool push(CAEN_DGTZ_X743_EVENT_t * evt, 
              CAEN_DGTZ_EventInfo_t * info) {
      auto const currentWrite = writeIndex.load(std::memory_order_relaxed);
      auto nextRecord = (currentWrite + 1) % POOL_SIZE;

      if(nextRecord != readIndex.load(std::memory_order_acquire)) {
	SetEvent(evt, info, data[currentWrite]);
	writeIndex.store(nextRecord, std::memory_order_release);
	return true;
      }

      return false; // queue is full
    };

    bool push(V1743Event * evt) {
      auto const currentWrite = writeIndex.load(std::memory_order_relaxed);
      auto nextRecord = (currentWrite + 1) % POOL_SIZE;

      if(nextRecord != readIndex.load(std::memory_order_acquire)) {
	SetEvent(evt, data[currentWrite]);
        writeIndex.store(nextRecord, std::memory_order_release);
        return true;
      }

      return false; // queue is full
    };

    // copy the data at readIndex to the given event, then update readIndex
    bool pull(V1743Event *& evt) {
      auto const currentRead = readIndex.load(std::memory_order_relaxed);
      if(currentRead == writeIndex.load(std::memory_order_acquire)) {
	// queue is empty
	return false;
      }

      auto nextRecord = (currentRead + 1) % POOL_SIZE;
      evt = &data[currentRead];
      readIndex.store(nextRecord, std::memory_order_release);
      return true;
    };

    // bfrancis -- have observed a bug in this function that can allow readIndex to get past
    // writeIndex, and was once to "pull" 5000-1 events from 1 single event
    // Need to investigate or ignore
    void pull(std::vector<V1743Event*> &evts) {
      while(true) {
        auto const currentRead = readIndex.load(std::memory_order_relaxed);
        if(currentRead == writeIndex.load(std::memory_order_acquire)) {
          // queue is empty
          break;
        }

        auto nextRecord = (currentRead + 1) % POOL_SIZE;
        evts.push_back(&data[currentRead]);
        readIndex.store(nextRecord, std::memory_order_release);
      }
    };

    // return a pointer to the data at readIndex (nullptr if queue is empty)
    V1743Event * front() {
      auto const currentRead = readIndex.load(std::memory_order_relaxed);
      if(currentRead == writeIndex.load(std::memory_order_acquire)) {
	// read == write so the queue is empty
	return nullptr;
      }
      return &data[currentRead];
    };

    // intentionally set the read index to the current write index, causing the queue to look empty
    // used at the ends of runs to be sure that no events are left over and stored on the next run
    void Destroy() {
      auto const currentWrite = writeIndex.load(std::memory_order_acquire);
      readIndex.store(currentWrite, std::memory_order_release);
    };

    //////////////////////////////////////////////////
    // Queue size methods
    //////////////////////////////////////////////////

    bool isEmpty() const {
      return (readIndex.load(std::memory_order_acquire) == writeIndex.load(std::memory_order_acquire));
    };

    bool isFull() const {
      auto nextRecord = (writeIndex.load(std::memory_order_acquire) + 1) % POOL_SIZE;
      return (nextRecord == readIndex.load(std::memory_order_acquire));
    };

    // This is really a guess because there are no locks on threads actively appending/getting
    // If the producer calls this, the size may be less since the consumer might be removing items
    // If the consumer calls this, the size may be more since the producer might be adding items
    size_t GuessSize() const {
      int value = writeIndex.load(std::memory_order_acquire) - readIndex.load(std::memory_order_acquire);
      if(value < 0) value += POOL_SIZE;
      return value;
    };

  private:
    char pad0[128];
    V1743Event * const data;

    uint32_t previousTTT;
    uint64_t previousTDC[nGroupsPerDigitizer];

    double tdcClockTick; // [ns]
    float nanosecondsPerSample;

    __attribute__((__aligned__(128))) std::atomic<unsigned int> readIndex;
    __attribute__((__aligned__(128))) std::atomic<unsigned int> writeIndex;

    char pad1[128 - sizeof(writeIndex)];
  };

}

#endif
