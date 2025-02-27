
/*
// Simple Arduino trasmisster
// Arduino Nano
// ELRS 2.4G TX moduel
// Custom PCB from JLCPCB
// const float codeVersion = 0.9.1; // Software revision
// https://github.com/kkbin505/Arduino-Transmitter-for-ELRS

 * This file is part of Simple TX
 *
 * Simple TX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Simple TX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */
/* 
 =======================================================================================================
 BUILD OPTIONS (comment out unneeded options)
 =======================================================================================================
 */
 //#define DEBUG // if not commented out, Serial.print() is active! For debugging only!!

 #include "config.h"
 #include "crsf.c"
 #include "led.h"
 #include "battery.h"
 

int Aileron_value = 0;        // values read from the pot 
int Elevator_value = 0; 
int Throttle_value=0;
int Rudder_value = 0; 

int loopCount=0;

int Arm = 0;        // switch values read from the digital pin
int AUX2_value = 0; 
int AUX3_value = 0; 
int AUX4_value = 0; 

float batteryVoltage;

int currentPktRate = 0;
int currentPower = 0;
int currentSetting = 0;

uint8_t crsfPacket[CRSF_PACKET_SIZE];
int rcChannels[CRSF_MAX_CHANNEL];
uint32_t crsfTime = 0;


void setup() {
    for (uint8_t i = 0; i < CRSF_MAX_CHANNEL; i++) {
        rcChannels[i] = RC_CHANNEL_MIN;
    }
   analogReference(EXTERNAL);
   pinMode(DIGITAL_PIN_SWITCH_ARM, INPUT_PULLUP);
   pinMode(DIGITAL_PIN_SWITCH_AUX2, INPUT_PULLUP);
   //pinMode(DIGITAL_PIN_SWITCH_AUX3, INPUT_PULLUP);
   pinMode(DIGITAL_PIN_SWITCH_AUX4, INPUT_PULLUP);
   pinMode(DIGITAL_PIN_LED, OUTPUT);//LED
   pinMode(DIGITAL_PIN_BUZZER, OUTPUT);//LED
   //digitalWrite(DIGITAL_PIN_BUZZER, LOW);
   batteryVoltage=7.0; 
   
   delay(1000); //Give enough time for uploda firmware
   Serial.begin(SERIAL_BAUDRATE);
   // Serial.write(crsfPacket1, CRSF_PACKET_SIZE);

   digitalWrite(DIGITAL_PIN_LED, HIGH); //LED ON

   

}

void loop() {
    uint32_t currentMicros = micros();

    //Read Voltage
    batteryVoltage=analogRead(VOLTAGE_READ_PIN)/103.0;

    if (batteryVoltage<WARNING_VOLTAGE){
       fastBlinkLED(DIGITAL_PIN_LED);
    }
 // fastBlinkLED(DIGITAL_PIN_LED);
    /*
     * Here you can modify values of rcChannels
     */
    Aileron_value = constrain(analogRead(analogInPinAileron),223,893); //My gimbal do not center, this function constrain end.
    Elevator_value= constrain(analogRead(analogInPinElevator),130,778); 
    Throttle_value=analogRead(analogInPinThrottle); 
    Rudder_value = constrain(analogRead(analogInPinRudder),209,853);  //My gimbal do not center, this function constrain end.
    Arm = digitalRead(DIGITAL_PIN_SWITCH_ARM);
    AUX2_value = digitalRead(DIGITAL_PIN_SWITCH_AUX2);
    //AUX3 = digitalRead(DIGITAL_PIN_SWITCH_AUX3);
    AUX4_value = digitalRead(DIGITAL_PIN_SWITCH_AUX4);
    rcChannels[AILERON] = map(Aileron_value,893,223,RC_CHANNEL_MIN,RC_CHANNEL_MAX); //reverse
    rcChannels[ELEVATOR] = map(Elevator_value,130,778,RC_CHANNEL_MIN,RC_CHANNEL_MAX); 
    rcChannels[THROTTLE] = map(Throttle_value,855,183,RC_CHANNEL_MIN,RC_CHANNEL_MAX);//reverse
    rcChannels[RUDDER] = map(Rudder_value ,209,853,RC_CHANNEL_MIN,RC_CHANNEL_MAX);

    
	//Aux 1 Arm Channel
    if(Arm==0){
      rcChannels[AUX1] =RC_CHANNEL_MIN;
    }else if(Arm==1){
      rcChannels[AUX1] =RC_CHANNEL_MAX;
    }

	//Aux 2 Channel
    if(AUX2_value==1){
      rcChannels[AUX2] =RC_CHANNEL_MIN;
    }else if(AUX2_value==0){
      rcChannels[AUX2] =RC_CHANNEL_MAX;
    }

    //Aux 4
    if(AUX4_value==1){
      rcChannels[AUX4] =RC_CHANNEL_MIN;
    }else if(AUX4_value==0){
      rcChannels[AUX4] =RC_CHANNEL_MAX;
    }

	//Additional switch add here.
//  transmit_enable=!digitalRead(transmit_pin);

   selectProtocol();
  // currentSetting=1;
    if (currentMicros > crsfTime) {

      if (loopCount<=200){
        //Build commond packet
        if (currentSetting >0){
          buildElrsPacket(crsfCmdPacket,ELRS_PKT_RATE_COMMAND,currentPktRate);
          Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
        }
        loopCount++;
      }
      else if (loopCount > 200 && loopCount < 400){
        if (currentSetting <0){
          buildElrsPacket(crsfCmdPacket,ELRS_POWER_COMMAND,currentPower);
          Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
        }
        loopCount++;
      }
      else{
        crsfPreparePacket(crsfPacket, rcChannels);
      //For gimal calibation only
	  #ifdef DEBUG
        Serial.print("A_"); 
        Serial.print(Aileron_value); 
        Serial.print("_"); 
        Serial.print(rcChannels[0]); 
        Serial.print(";E_"); 
        Serial.print(Elevator_value);
        Serial.print("_"); 
        Serial.print(rcChannels[1]);  
        Serial.print(";T_"); 
        Serial.print(Throttle_value);
        Serial.print("_"); 
        Serial.print(rcChannels[2]); 
        Serial.print("_R_");  
        Serial.print(Rudder_value);
        Serial.print("_"); 
        Serial.print(rcChannels[3]); 
        Serial.print(";Arm_");  
        Serial.print(Arm);
        Serial.print(";Mode_");  
        Serial.print(AUX2);
        Serial.print("_BatteryVoltage:");  
        Serial.print(batteryVoltage);
        Serial.print("_CurrentSetting:");  
        Serial.print(currentSetting);
        Serial.println();  
        delay(500);
       #else
         Serial.write(crsfPacket, CRSF_PACKET_SIZE);
       #endif
      }
        crsfTime = currentMicros + CRSF_TIME_BETWEEN_FRAMES_US;
    }
}

void selectProtocol(){

    currentSetting =0;
    // startup stick commands (protocol selection / renew transmitter ID)
    
    if(rcChannels[AILERON] < RC_MIN_COMMAND && rcChannels[ELEVATOR] < RC_MIN_COMMAND){ // Elevator down + aileron left
      currentPktRate = SETTING_1_PktRate;
      currentPower = SETTING_1_Power;
      currentSetting =1;
    }
    else if (rcChannels[AILERON] > RC_MAX_COMMAND && rcChannels[ELEVATOR] > RC_MAX_COMMAND){ // Elevator up + aileron right
      currentPktRate = SETTING_2_PktRate;
      currentPower = SETTING_2_Power;
      currentSetting =2;
    }
    //reture Setting;
}
