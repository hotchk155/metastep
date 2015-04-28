
// The NOTECV is used to define a note to be output by the DAC.
// Internally it is defined as top byte being a MIDI note and 
// lower byte as a fraction of a note. For example 0x30AA would be
// between MIDI notes 0x30 and 0x31
typedef unsigned int NOTEVAL
class CSequencerOutput 
{
  public: 
    // called when sequencer starts a note.  
    void onStart(int channel, NOTEVAL note);
    
    // called when sequencer finishes a note before start of next one
    void onStop(int channel);
}
