/*
*/

class CSequencer
{
  enum {
    F_RESET = 1,
    F_STEP= 2
  }
  byte m_flags;
  
  // current step
  char m_step;
  
  // total length of sequence
  char m_length;

  int *m_input;
  
  
  // Gate Length in milliseconds. Zero for tied.
  int m_gateLength;
    
  unsigned long m_endNote;
  
  char m_channel;
public:
  /////////////////////////////////////////////////////////////////////
  CSequencer() 
  {
    SequenceLength = MAX_SEQUENCE;
    SequenceBase = 0;
    GateLength = 0;
    reset();
  }

  /////////////////////////////////////////////////////////////////////
  void reset() 
  {
    Step = -1;
    m_endNote = 0;
  }
  
  /////////////////////////////////////////////////////////////////////
  // This function maps a 10 bit value from one of the pots to a NOTEVAL
  // in the range 0x0000 - 0x7fff
  NOTEVAL in2note(int a) 
  {
    return (a<<5);
  }
  
  /////////////////////////////////////////////////////////////////////
  void run(unsigned long milliseconds, int *in, SequencerOutput *out)
  {
    if(m_endNote && (milliseconds >= m_endNote)) {
      // end the previous note
      out->onStop(m_channel);
      m_endNote = 0;
    }
        
    if(m_flags & F_STEP) {
      // next sequencer step
      m_flags &= ~F_STEP;      
      if(++m_step >= m_length) {
        m_step = 0;
      }
      out->onStart(m_channel, in2note(in[m_base + m_step]);
      if(m_gateLength > 0) {
        m_endNote = milliseconds + GateLength;
      }
      else {
        m_endNote = 0;
      }
    }    
  }

  /////////////////////////////////////////////////////////////////////
  void next()
  {
      m_flags |= F_STEP;
  }
  
};
