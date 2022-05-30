import netP5.*;
import oscP5.*;
import processing.serial.*;   //includes serial object library


Serial mySerial; //creates local serial object from serial library

String myString = null; //a variable to collect serial data
int nl = 10; //ASCII code for carage return in serial
String gestures[] = {"NO EFFECTS", "DISTORTION", "PITCH SHIFT", "DELAY & REVERB", "ALL EFFECTS"};
int gestureSelect = 0;
float yaw_deg = 0;
float pitch_deg = 0;
float roll_deg = 0;
float yaw_rad = 0;
float pitch_rad = 0;
float roll_rad = 0;

OscP5 oscP5;
NetAddress myRemoteLocation;

void setup(){
  size(600, 500, P3D);
  ortho();
  frameRate(60);
  String myPort = Serial.list() [2]; //find correct serial port
  mySerial = new Serial(this, myPort, 9600);
  
  oscP5 = new OscP5(this,9000);  //start oscP5
  myRemoteLocation = new NetAddress("127.0.0.1",9000);
}

void draw(){
    
  while (mySerial.available() > 0){
    myString = mySerial.readStringUntil(nl); //strips data of serial port
    
    if (myString != null){
      
      float[] values = float(split(myString, ' '));  
      
      gestureSelect = (int)(values[0]);
      yaw_deg = (float)(values[1]);
      pitch_deg = (float)(values[2]);
      roll_deg = (float)(values[3]);
               
     
      println(gestureSelect, pitch_deg, yaw_deg, roll_deg);
      
      OscMessage gesture_sel = new OscMessage("/Gesture");
      OscMessage yawOrientation = new OscMessage("/Yaw");
      OscMessage pitchOrientation = new OscMessage("/Pitch");
      OscMessage rollOrientation = new OscMessage("/Roll");
      
      gesture_sel.add(gestureSelect);  //add values to osc messages
      yawOrientation.add(yaw_deg); 
      pitchOrientation.add(pitch_deg); 
      rollOrientation.add(roll_deg);
      
      oscP5.send(gesture_sel, myRemoteLocation);   //send the messages
      oscP5.send(pitchOrientation, myRemoteLocation); 
      oscP5.send(yawOrientation, myRemoteLocation); 
      oscP5.send(rollOrientation, myRemoteLocation); 
          
      yaw_rad = (float)(yaw_deg / 180 * Math.PI);
      pitch_rad = (float)(pitch_deg / 180 * Math.PI);
      roll_rad = (float)(roll_deg / 180 * Math.PI);
      
      background(240);
      fill(0);
      
      if(gestureSelect == 1000){
        translate(width/2, height/2);
        stroke(0);
        strokeWeight(3);
        fill(127);
        rotateX(pitch_rad);
        rotateY(yaw_rad);
        rotateZ(roll_rad);
        box(200, 100, 150);
      }else{
        textFont(createFont("Helvetica", 40));
        textSize(60);
        textAlign(CENTER, CENTER);
        text(gestures[gestureSelect], width/2, height/2);
        fill(0);
      }
    }
  }
}
