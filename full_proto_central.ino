#include <ArduinoBLE.h>

const char* yscheulService = "19B10000-E8F2-537E-4F6C-D104768A1214";

long gesture = -1;
long buttonPress = 0;
long yawValue = 0;
long pitchValue = 0;
long rollValue = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);
   
  if (!BLE.begin()) {
    Serial.println("* Starting Bluetooth® Low Energy module failed!");
    while (1);
  }
  
  BLE.setLocalName("Nano 33 BLE (Central)"); 
  BLE.advertise();

  Serial.println("Arduino Nano 33 BLE Sense (Central Device)");
  Serial.println(" ");
}

void loop() {
  BLEDevice peripheral;
  
  Serial.println("- Discovering peripheral device...");

  do{
    BLE.scanForUuid(yscheulService);
    peripheral = BLE.available();
  } while (!peripheral);
  
  if (peripheral) {
    Serial.println("* Peripheral device found!");
    Serial.print("* Device MAC address: ");
    Serial.println(peripheral.address());
    Serial.print("* Device name: ");
    Serial.println(peripheral.localName());
    Serial.print("* Advertised service UUID: ");
    Serial.println(peripheral.advertisedServiceUuid());
    Serial.println(" ");
    BLE.stopScan();
    controlPeripheral(peripheral);
  }
}

void controlPeripheral(BLEDevice peripheral){
  Serial.println("- Connecting to peripheral device...");

  if (peripheral.connect()) {
    Serial.println("* Connected to peripheral device!");
    Serial.println(" ");
  } else {
    Serial.println("* Connection to peripheral device failed!");
    Serial.println(" ");
    return;
  }

  Serial.println("- Discovering peripheral device attributes...");
  if (peripheral.discoverAttributes()) {
    Serial.println("* Peripheral device attributes discovered!");
    Serial.println(" ");
  } else {
    Serial.println("* Peripheral device attributes discovery failed!");
    Serial.println(" ");
    peripheral.disconnect();
    return;
  }

  BLECharacteristic gestureChar = peripheral.characteristic("19B10001-E8F2-537E-4F6C-D104768A1214");
  BLECharacteristic buttonChar = peripheral.characteristic("19B10002-E8F2-537E-4F6C-D104768A1214");
  BLECharacteristic yawChar = peripheral.characteristic("19B10003-E8F2-537E-4F6C-D104768A1214");
  BLECharacteristic pitchChar = peripheral.characteristic("19B10004-E8F2-537E-4F6C-D104768A1214");
  BLECharacteristic rollChar = peripheral.characteristic("19B10005-E8F2-537E-4F6C-D104768A1214");
  
  if (!gestureChar) {
    Serial.println("Peripheral does not have gesture characteristic!");
    peripheral.disconnect();
    return;
  } else if (!gestureChar.canWrite()) {
    Serial.println("Peripheral does not have a writable gresture characteristic!");
    peripheral.disconnect();
    return;
  }  

  
  while (peripheral.connected()){
    gestureChar.readValue(gesture);
    buttonChar.readValue(buttonPress);  
    yawChar.readValue(yawValue);
    pitchChar.readValue(pitchValue);
    rollChar.readValue(rollValue);

    if(buttonPress == 1){
      Serial.print("1000");
      Serial.print(" ");
      Serial.print(yawValue);
      Serial.print(" ");
      Serial.print(pitchValue);
      Serial.print(" ");
      Serial.print(rollValue);
      Serial.println(" ");
    }

    if(buttonPress == 0){
      //nothing
    }

    for(int i = 0; i < 4; i++){
      if(gesture == i){
        Serial.print(i);
        Serial.print(" ");
        Serial.print("1000");
        Serial.print(" ");
        Serial.print("1000");
        Serial.print(" ");
        Serial.println("1000");
        gesture = -1;
        gestureChar.writeValue(gesture);
      }
    }   
  }
  
  Serial.println("- Peripheral device disconnected!");  
}
