#include "Monitor/Monitor.h"

//Monitor object
Monitor mon = Monitor();

//Demo variables
int rondo = 0;
char* roboto = "BEEEEPPP";

/* This function is called once at start up ----------------------------------*/
void setup()
{
    //Initialize the Monitor
    mon.begin();
    
    //Set up the variables to be monitored
    mon.variable("robotSays",roboto,STRING);
    mon.variable("random",&rondo,INT);
    
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    //Updates the Monitor; run at least once per loop for good resolution
    mon.report();
    
    rondo = random(100);
    delay(20);
}