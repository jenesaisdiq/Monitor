#include "application.h"

#ifndef Monitor_h
#define Monitor_h



class Monitor {

 public:

  Monitor();
  
  void
    begin(void),
    variable(const char* name, void* var, Spark_Data_TypeDef userVarType),
    readPins(void),
    readVars(void),
    ipMaker(void),
    tcpClientMaintainer(void),
    ledOff(void),
    publishMode(bool),
    report(void);
  
 private:

};

#endif

