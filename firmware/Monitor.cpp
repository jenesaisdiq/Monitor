#include "Monitor.h"

#define userMaxVars 4
#define tcpDelayMillis 50
#define pubDelayMillis 900

char ip[15];
char pinState[63];
char varState[63];
char tcpPinState[200];
char tcpVarState[200];


long lastPub = 0;
long lastUpdate = 0;
long currTime = 0;
int varCount = 0;                          // <--------------------------------
uint8_t pinVarSwitch = 0; 

TCPServer server = TCPServer(7073);
TCPClient client;

bool readDone = false;
bool clientState = false;
bool tcpConnected = false;

bool ledFlag = true;
bool publishFlag = true;

void *userVar[4];
const char* userVarName[4];
char* usersVarType[4];

Monitor::Monitor(){}

void Monitor::begin(){
    server.begin();
    Spark.variable("ip", ip, STRING);
    if(ledFlag){pinMode(D7,OUTPUT);}
}

void Monitor::ledOff(){
    ledFlag = false;
}

void Monitor::publishMode(bool modeChoice){
    publishFlag = modeChoice;
}


void Monitor::report(){
    currTime = millis();
    
    if(client.connected()){ //-----------------   MODE FOR TCP CONNECTED
        if(currTime - lastUpdate > tcpDelayMillis){
            readPins();
            if(varCount > 0){
                readVars();
                server.print(tcpVarState);
            }
            readDone = true;
            server.print(tcpPinState);
            lastUpdate = currTime;
            
            if(!clientState){
                clientState = true;
                if(ledFlag){digitalWrite(D7, HIGH);}
            } else {
                clientState = false;
                if(ledFlag){digitalWrite(D7, LOW);}
            }
        }
    }
    else {
       tcpClientMaintainer();
       clientState = false;
       if(ledFlag){digitalWrite(D7, LOW);}
    }

    
    if(currTime-lastPub > pubDelayMillis){    //---------- Limited pub rate, still needs to run when publishing is off to update ipMaker()
        if(publishFlag){                      //---------- Allows publishing to be turned off
            if(varCount > 0 && pinVarSwitch == 0){
                if(!readDone){
                    readVars();
                }
                Spark.publish("vars",varState,60, PRIVATE);
                pinVarSwitch++;
            }
            else {
                if(!readDone){
                    readPins();
                }
                Spark.publish("pins",pinState,60, PRIVATE);
                pinVarSwitch++;
                if(pinVarSwitch > 3){
                    pinVarSwitch = 0;
                }
            }
            lastPub = currTime;
        }
        
        ipMaker();
    }
        
    readDone = false;
}

void Monitor::readVars(){
    char result[63] = "";
    sprintf(tcpVarState,"{\"type\":\"vars\",\"vars\":{");
    for(int i = 0; i < varCount-1; i++){
        if(strncmp(usersVarType[i],"i",1) == 0){    
            int *tempI = (int *) userVar[i];
            sprintf(result,"%s%s:%d,",result,userVarName[i],*tempI);
            sprintf(tcpVarState,"%s\"%s\":\"%d\",",tcpVarState,userVarName[i],*tempI);
        } else if(strncmp(usersVarType[i],"s",1) == 0){    
            char *tempS = (char *) userVar[i];
            sprintf(result,"%s%s:%s,",result,userVarName[i],tempS);
            sprintf(tcpVarState,"%s\"%s\":\"%s\",",tcpVarState,userVarName[i],tempS);
        }
    }
    
    //broken-out extra instance of these conditionals to cover the no-last-comma case
    if(strncmp(usersVarType[varCount-1],"i",1) == 0){    
        int *tempI = (int *) userVar[varCount-1];
        sprintf(result,"%s%s:%d",result,userVarName[varCount-1],*tempI);
        sprintf(tcpVarState,"%s\"%s\":\"%d\"",tcpVarState,userVarName[varCount-1],*tempI);
    } else if(strncmp(usersVarType[varCount-1],"s",1) == 0){    
        char *tempS = (char *) userVar[varCount-1];
        sprintf(result,"%s%s:%s\0",result,userVarName[varCount-1],tempS);
        sprintf(tcpVarState,"%s\"%s\":\"%s\"",tcpVarState,userVarName[varCount-1],tempS);
    }
    
    sprintf(tcpVarState,"%s}}~",tcpVarState);
    memcpy(varState, result, sizeof(result));
}

void Monitor::readPins(){
    
    char readVal[5] = "----";
    sprintf(tcpPinState,"{\"type\":\"pins\",\"pins\":{\"analog\":[");
    String pubResult = "";
    
    String t2 = "--";
    
    char t[2] = "-";
    for (int i = 10; i < 18; i++) {
        
        String(analogRead(i)).toCharArray(readVal,5);
        
        t2 = readVal;
        
        while(t2.length() < 4){         //Fills out length to 4 so analog reads are always 4 chars
            t2 = "0" + t2;
        }
        pubResult += t2 + ",";
        if(i < 17){ 
            sprintf(tcpPinState,"%s%s,",tcpPinState,readVal);
        }
        else{
            sprintf(tcpPinState,"%s%s",tcpPinState,readVal);
        }
    }
    
    sprintf(tcpPinState,"%s],\"digital\":[",tcpPinState);
    
    for (int i = 0; i < 8; i++) {
        
        String digiTemp = String(digitalRead(i));
        pubResult += digiTemp + ",";
        
        digiTemp.toCharArray(t,2);
        if(i < 7){ 
            sprintf(tcpPinState,"%s%s,",tcpPinState,t);
        }
        else {
            sprintf(tcpPinState,"%s%s",tcpPinState,t);
        }
        
    }
   
    sprintf(tcpPinState,"%s]}}~",tcpPinState);

    // Casting weirdness for publishes
    // See https://github.com/spark/core-firmware/issues/146
    String temp2 = pubResult.substring(0, (pubResult.length() - 1));
    pubResult = temp2;
    pubResult.toCharArray(pinState, 63);
}

void Monitor::tcpClientMaintainer(){
    client = server.available();

}

void Monitor::variable(const char* name, void* var, Spark_Data_TypeDef userVarType) {
    if(varCount < userMaxVars){
        Spark.variable(name, var, userVarType);
        userVar[varCount] = var;
        if(userVarType == INT){
            usersVarType[varCount] = "i";
        } else if(userVarType == STRING){
            usersVarType[varCount] = "s";
        }

        userVarName[varCount] = name;
        varCount++;
    }
}

void Monitor::ipMaker(){
    IPAddress myIP = WiFi.localIP();
    sprintf(ip,"%d.%d.%d.%d",myIP[0],myIP[1],myIP[2],myIP[3]);
}
