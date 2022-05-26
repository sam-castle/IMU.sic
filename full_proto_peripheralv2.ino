#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>
#include "MadgwickAHRS.h"
 
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

#include "gesture_model.h" //tensorflow data

BLEByteCharacteristic* gestureChar = nullptr;
BLEByteCharacteristic* buttonChar = nullptr;
BLEIntCharacteristic* yawChar = nullptr;
BLEIntCharacteristic* pitchChar = nullptr;
BLEIntCharacteristic* rollChar = nullptr;
BLEService* yscheulService = nullptr;


const float accelerationThreshold = 2.8; // threshold of significant in G's
const int numSamples = 119;

int samplesRead = numSamples;

// global variables used for TensorFlow Lite (Micro)
tflite::MicroErrorReporter tflErrorReporter;

// pull in all the TFLM ops, you can remove this line and
// only pull in the TFLM ops you need, if would like to reduce
// the compiled size of the sketch.
tflite::AllOpsResolver tflOpsResolver;

const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

// Create a static memory buffer for TFLM, the size may need to
// be adjusted based on the model you are using
constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize];

// array to map gesture index to a name
const char* GESTURES[] = {
  "double_tap",
  "pulse",
  "swipe",
  "twist",
  "wave"  
};

#define NUM_GESTURES (sizeof(GESTURES) / sizeof(GESTURES[0]))

float aX, aY, aZ, gX, gY, gZ;
boolean active = false;

// values for acceleration and rotation:
float xAcc, yAcc, zAcc;
float xGyro, yGyro, zGyro;

// values for orientation:
int roll, pitch, yaw;

/////////button setup/////////

// constants
const int buttonPin = 2; // push button
const int led1 = LEDR; // led1 = red
const int led2 = LEDG; // led2 = green

//variables
int buttonState = 0; // current state of button

Madgwick filter;  // initialize a Madgwick filter
const float sensorRate = 104.00;  // sensor's sample rate is fixed at 104 Hz


void setup() {

 yscheulService = new BLEService("19B10000-E8F2-537E-4F6C-D104768A1214");
 gestureChar = new BLEByteCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
 buttonChar = new BLEByteCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
 yawChar = new BLEIntCharacteristic("19B10003-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
 pitchChar = new BLEIntCharacteristic("19B10004-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
 rollChar = new BLEIntCharacteristic("19B10005-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

 pinMode(buttonPin,INPUT_PULLUP);
 pinMode(led1,OUTPUT);
 pinMode(led2, OUTPUT);
 
 Serial.begin(9600);
 //while (!Serial);
 
  // initialize the IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // start the filter to run at the sample rate:
  filter.begin(sensorRate);

  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setAdvertisedService(*yscheulService); // add the service UUID
  yscheulService->addCharacteristic(*gestureChar);
  yscheulService->addCharacteristic(*buttonChar);
  yscheulService->addCharacteristic(*yawChar);
  yscheulService->addCharacteristic(*pitchChar);
  yscheulService->addCharacteristic(*rollChar);
  BLE.addService(*yscheulService);
  // start advertising
  BLE.setLocalName("Nano 33 (Peripheral)");
  // set the initial value for the characeristic:
  gestureChar->writeValue(-1); //here
  buttonChar->writeValue(0);
  yawChar->writeValue(0);
  pitchChar->writeValue(0);
  rollChar->writeValue(0);
  // start advertising
  BLE.advertise();
  Serial.println("BLE Peripheral");
 
  // get the TFL representation of the model byte array
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (1);
  }
  // Create an interpreter to run the model
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);
 
  // Allocate memory for the model's input and output tensors
  tflInterpreter->AllocateTensors();
 
  // Get pointers for the model's input and output tensors
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
}




void loop() {

  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  
  // if a central is connected to peripheral:
  if (central){
    
    
    while (central.connected()){
      // bluetooth device is connected

      // check if the IMU is ready to read:
      if (IMU.accelerationAvailable() &&
      IMU.gyroscopeAvailable()) {
        // read accelerometer &and gyrometer:
        IMU.readAcceleration(xAcc, yAcc, zAcc);
        IMU.readGyroscope(xGyro, yGyro, zGyro);
    
        // update the filter, which computes orientation:
        filter.updateIMU(xGyro, yGyro, zGyro, xAcc, yAcc, zAcc);
    
        // print the yaw, pitch and roll
        roll = filter.getRoll();
        pitch = filter.getPitch();
        yaw = filter.getYaw();
      
      // wait for significant motion
      if(active == false && samplesRead == numSamples) {
        buttonState = digitalRead(buttonPin);

        if(buttonState == LOW) {
          digitalWrite(led1, LOW);
          digitalWrite(led2, HIGH);
          
          buttonChar->writeValue(1);  //send to bluetooth
                   
          orientation(central);
          return;
        }

        if(buttonState == HIGH) {
          digitalWrite(led1, HIGH);
          digitalWrite(led2, LOW);

          buttonChar->writeValue(0);        
   
          // sum up the absolutes
          float aSum = fabs(xAcc) + fabs(yAcc) + fabs(zAcc);
    
          // check if it's above the threshold
          if (aSum >= accelerationThreshold) {
            // reset the sample read count
            samplesRead = 0;
            active = true;
          }
        }
      }
      
      // check if the all the required samples have been read since
      // the last time the significant motion was detected
      if (active == true && samplesRead < numSamples) {
    
          // normalize the IMU data between 0 to 1 and store in the model's
          // input tensor
          tflInputTensor->data.f[samplesRead * 6 + 0] = (xAcc + 4.0) / 8.0;
          tflInputTensor->data.f[samplesRead * 6 + 1] = (yAcc + 4.0) / 8.0;
          tflInputTensor->data.f[samplesRead * 6 + 2] = (zAcc + 4.0) / 8.0;
          tflInputTensor->data.f[samplesRead * 6 + 3] = (xGyro + 2000.0) / 4000.0;
          tflInputTensor->data.f[samplesRead * 6 + 4] = (yGyro + 2000.0) / 4000.0;
          tflInputTensor->data.f[samplesRead * 6 + 5] = (zGyro + 2000.0) / 4000.0;
    
          samplesRead++;
    
          if (samplesRead == numSamples) {
            active = false;
            // Run inferencing
            TfLiteStatus invokeStatus = tflInterpreter->Invoke();
            if (invokeStatus != kTfLiteOk) {
              Serial.println("Invoke failed!");
              while (1);
              return;
            }

           for (int i = 0; i < NUM_GESTURES; i++) {
             if(tflOutputTensor->data.f[i] >= 0.93){
               //Serial.println(GESTURES[i]); 
               
               if(central.connected()){
                 // write to bluetooth
                 if(GESTURES[i] == "double_tap"){
                   gestureChar->writeValue(0);
                 }

                 if(GESTURES[i] == "pulse"){
                   gestureChar->writeValue(1);
                 }

                 if(GESTURES[i] == "swipe"){
                   gestureChar->writeValue(2);
                 }

                 if(GESTURES[i] == "twist"){
                   gestureChar->writeValue(3);
                 }

                 if(GESTURES[i] == "wave"){
                   gestureChar->writeValue(4);
                 }
                 
               }
             }
           }
          }
        }
      }     
    }
    //Serial.println("Bluetooth disconnected");
  }
}


void orientation(BLEDevice central){

//    Serial.print(yaw);
//    Serial.print(" ");
//    Serial.print(pitch);
//    Serial.print(" ");
//    Serial.println(roll);

    if(central.connected()){
      // write to bluetooth
      yawChar->writeValue(yaw);
      pitchChar->writeValue(pitch);
      rollChar->writeValue(roll);  
      
    }
  }
