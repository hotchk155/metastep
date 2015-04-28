class CStepper 
{
  
public:
  void render();
  
  // called when a button is pressed in the range assigned to the stepper
  void onButtonPress(int index);
  
  // called 
  void tick();
  
  void onSync();
  
};

