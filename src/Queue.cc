#include "Queue.h"

void mdaq::Queue::SetEvent(CAEN_DGTZ_X743_EVENT_t * evt, 
			   CAEN_DGTZ_EventInfo_t * info, 
			   mdaq::V1743Event& data) {

  // loop over groups in this digitizer
  for(unsigned int iGroup = 0; iGroup < nGroupsPerDigitizer; iGroup++) {

    // loop over channels in this group
    for(unsigned iChannel = 0; iChannel < nChannelsPerGroup; iChannel++) {
      data.TriggerCount[nChannelsPerGroup * iGroup + iChannel] = evt->DataGroup[iGroup].TriggerCount[iChannel];
      data.TimeCount[nChannelsPerGroup * iGroup + iChannel]    = evt->DataGroup[iGroup].TimeCount[iChannel];

      for(unsigned int iSample = 0; iSample < evt->DataGroup[iGroup].ChSize; iSample++) {
        data.waveform[nChannelsPerGroup * iGroup + iChannel][iSample] = (evt->DataGroup[iGroup].DataChannel[iChannel])[iSample] * ADCTOVOLTS;
      }
    } // end loop channels

    data.EventId[iGroup] = evt->DataGroup[iGroup].EventId;
    data.TDC[iGroup]     = evt->DataGroup[iGroup].TDC;

#ifdef SAVE_OPTIONAL_VALUES
    data.PosEdgeTimeStamp[iGroup] = evt->DataGroup[iGroup].PosEdgeTimeStamp;
    data.NegEdgeTimeStamp[iGroup] = evt->DataGroup[iGroup].NegEdgeTimeStamp;
    data.PeakIndex[iGroup]        = evt->DataGroup[iGroup].PeakIndex;
    data.Peak[iGroup]             = evt->DataGroup[iGroup].Peak;
    data.Baseline[iGroup]         = evt->DataGroup[iGroup].Baseline;

    // "charge" is meaningless in normal mode, so just set event 0 by groups
    data.StartIndexCell[iGroup][0]   = evt->DataGroup[iGroup].StartIndexCell;
    data.Charge[iGroup][0]           = evt->DataGroup[iGroup].Charge;
#endif

  } // end loop groups

  // EventInfo

  data.EventSize      = info->EventSize;
  data.BoardId        = info->BoardId;
  data.Pattern        = info->Pattern;
  data.ChannelMask    = info->ChannelMask;
  data.EventCounter   = info->EventCounter;
  data.TriggerTimeTag = info->TriggerTimeTag;

  if(data.TriggerTimeTag < previousTTT) {
    data.TriggerTimeTagRollovers++;
    previousTTT = data.TriggerTimeTag;
  }

  for(int iGroup = 0; iGroup < nGroupsPerDigitizer; iGroup++) {
    if(data.TDC[iGroup] < previousTDC[iGroup]) {
      data.TDCRollovers[iGroup]++;
      previousTDC[iGroup] = data.TDC[iGroup];
    }
  }

  data.DataPresent = true;

  data.tdcClockTick = tdcClockTick;
  data.nanosecondsPerSample = nanosecondsPerSample;

  // data.DAQEventNumber is ordered as events are written to disk, so EventFactory will set this
  // data.SetTimestamp is the system time when the event was created, so this should be done here

  data.SetTimestamp(std::chrono::system_clock::now());

}

void mdaq::Queue::SetEvent(mdaq::V1743Event * evt, mdaq::V1743Event& data) {

  // loop over groups in this digitizer
  for(unsigned int iGroup = 0; iGroup < nGroupsPerDigitizer; iGroup++) {

    // loop over channels in this group
    for(unsigned iChannel = 0; iChannel < nChannelsPerGroup; iChannel++) {
      data.TriggerCount[nChannelsPerGroup * iGroup + iChannel] = evt->TriggerCount[nChannelsPerGroup * iGroup + iChannel];
      data.TimeCount[nChannelsPerGroup * iGroup + iChannel] = evt->TimeCount[nChannelsPerGroup * iGroup + iChannel];

      for(unsigned int iSample = 0; iSample < maxSamples; iSample++) {
        data.waveform[nChannelsPerGroup * iGroup + iChannel][iSample] = evt->waveform[nChannelsPerGroup * iGroup + iChannel][iSample];
      }
    } // end loop channels

    data.EventId[iGroup] = evt->EventId[iGroup];
    data.TDC[iGroup]     = evt->TDC[iGroup];

#ifdef SAVE_OPTIONAL_VALUES
    data.PosEdgeTimeStamp[iGroup] = evt->PosEdgeTimeStamp[iGroup];
    data.NegEdgeTimeStamp[iGroup] = evt->NegEdgeTimeStamp[iGroup];
    data.PeakIndex[iGroup]        = evt->PeakIndex[iGroup];
    data.Peak[iGroup]             = evt->Peak[iGroup];
    data.Baseline[iGroup]         = evt->Baseline[iGroup];

    // "charge" is meaningless in normal mode, so just set event 0 by groups
    data.StartIndexCell[iGroup][0]   = evt->StartIndexCell[iGroup][0];
    data.Charge[iGroup][0]           = evt->Charge[iGroup][0];
#endif

  } // end loop groups

  // EventInfo

  data.EventSize      = evt->EventSize;
  data.BoardId        = evt->BoardId;
  data.Pattern        = evt->Pattern;
  data.ChannelMask    = evt->ChannelMask;
  data.EventCounter   = evt->EventCounter;
  data.TriggerTimeTag = evt->TriggerTimeTag;

  data.TriggerTimeTagRollovers = evt->TriggerTimeTagRollovers;

  for(int iGroup = 0; iGroup < nGroupsPerDigitizer; iGroup++) {
    data.TDCRollovers[iGroup] = evt->TDCRollovers[iGroup];
  }

  data.DataPresent = evt->DataPresent;

  data.tdcClockTick = evt->tdcClockTick;
  data.nanosecondsPerSample = evt->nanosecondsPerSample;

  // data.DAQEventNumber is ordered as events are written to disk, so EventFactory will set this
  // data.SetTimestamp is the system time when the event was created, so this should be done here

  data.DAQTimeStamp = evt->DAQTimeStamp;

}
