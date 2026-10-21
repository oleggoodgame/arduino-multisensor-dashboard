#include <LiquidCrystal.h>
#include <IRremote.h>

#define LED_RED 12
#define LED_GREEN 8
#define LED_YELLOW 10
#define PHOTORESYSTOR A3
#define BUTTON 11
#define TRIGGER 3
#define ECHO_PIN 2
#define CRITICAL_LEVEL 20

#define DB7 4
#define DB6 5
#define DB5 6
#define DB4 7

#define LCD_ENABLE A4
#define LCD_RG A5

#define IR_SENSOR 9
#define IR_BUTTON_STOP 0
#define IR_BUTTON_NEXT 6
#define IR_BUTTON_PLAY 5
#define IR_BUTTON_BACK 4
#define IR_BUTTON_EQ 13

// LED functions
void lightLed();
void ledLEDwithPR();
// ---
// Button variables and functions
bool buttonStateStop = false;   // true = system is stopped
bool fifi = false;
unsigned long lastTimeDebounce = millis();
unsigned long timeDebounce = 40;

void changeButtonState();
// ------
// Ultrasonic sensor variables and functions
unsigned long lastTimeTriggered = millis();
unsigned long timeTrigger = 400;
double previousDistance = 400;
unsigned long timeMeasureVal = 0; 
volatile unsigned long startedMeasuring;
volatile unsigned long endMeasuring;
volatile bool newDistance = false;

double distance();
void triggerUltraSonicSensor();
void interruptEcho();
// ---------------
// LCD setup
LiquidCrystal lcd(LCD_RG, LCD_ENABLE, DB4, DB5, DB6, DB7);

void writeInLCD(double distance);
void writeInLCD(int measuredPR);
void writeInLCD();

// Using bool since there are only two options, but better to use int or byte 
bool currentStateMeasurement = true; // true = centimeters, false = inches
bool currentStateToShow = true;      // true = UltraSonicSensor, false = PhotoResistor
// ---
// Photoresistor variables and functions
unsigned long lastTimeMeasuredPR = millis();
unsigned long timeMeasurePR = 200;
int currentDataInPR();

// -----------
// IR Sensor functions
void triggerIRSensor();
void setStopped(bool stopped);
void setScreen();
void setEQ();
// --------

void setup()
{
    Serial.begin(9600);
    lcd.begin(16, 2);
    IrReceiver.begin(IR_SENSOR);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(BUTTON, INPUT);

    pinMode(TRIGGER, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(ECHO_PIN), interruptEcho, CHANGE);
}

void loop()
{
    unsigned long timeNow = millis();
    triggerIRSensor();
    if (timeNow - lastTimeDebounce > timeDebounce)
    {
        lastTimeDebounce = timeNow;
        if (digitalRead(BUTTON) == 1 && fifi == false)
        {
            fifi = true;
            changeButtonState();
        }
        if (digitalRead(BUTTON) == 0)
        {
            fifi = false;
        }
    }

    if (buttonStateStop == false)
    {
        if (timeNow - lastTimeTriggered > timeTrigger)
        {
            lastTimeTriggered += timeTrigger;
            triggerUltraSonicSensor();
        }

        if (currentStateToShow == true)
        {
            if (newDistance)
            {
                double distanceC = distance();
                if (!buttonStateStop) 
                {
                    writeInLCD(distanceC);
                }
                newDistance = false;
            }
        }
        else
        {
            writeInLCD(currentDataInPR());
        }
    }
    else
    {
        writeInLCD();
    }

    lightLed();
    ledLEDwithPR();
}

void lightLed()
{
    if (buttonStateStop == false)
    {
        digitalWrite(LED_GREEN, 1);
        digitalWrite(LED_RED, 0);
    }
    else
    {
        digitalWrite(LED_GREEN, 0);
        digitalWrite(LED_RED, 1);
    }
}

void changeButtonState()
{
    buttonStateStop = !buttonStateStop;
    lcd.clear();
}

double distance()
{
    unsigned long timeMeasure = endMeasuring - startedMeasuring;
    timeMeasureVal = timeMeasure;
    double cm = timeMeasure * 0.0343 / 2;    // centimeters
    
    if (cm < CRITICAL_LEVEL)
    {
        lcd.clear();
        buttonStateStop = true;
        Serial.println("STOP");
    }

    if (currentStateMeasurement == true)
    {
        return cm;
    }
  
    return cm * 0.3937;                    // inches
}

void triggerUltraSonicSensor()
{
    digitalWrite(TRIGGER, 0);
    delayMicroseconds(2);
    digitalWrite(TRIGGER, 1);
    delayMicroseconds(10);
    digitalWrite(TRIGGER, 0);
}

void interruptEcho()
{
    if (digitalRead(ECHO_PIN) == 1)
    {
        startedMeasuring = micros();
    }
    else
    {
        endMeasuring = micros();
        newDistance = true;
    }
}

int currentDataInPR()
{
    return analogRead(PHOTORESYSTOR);
}

void writeInLCD(double distanceVal)
{
    lcd.setCursor(0, 0);
    lcd.print("Distance:");
    lcd.setCursor(9, 0);
    lcd.print(distanceVal);
    
    if (currentStateMeasurement == true)
    {
        lcd.print("cm    ");
    }
    else
    {
        lcd.print("inch  ");
    }
    
    lcd.setCursor(0, 1);
    lcd.print("Time:");
    lcd.setCursor(5, 1);
    lcd.print(timeMeasureVal);
    lcd.print("us    ");
}

void writeInLCD(int measuredPR)
{
    lcd.setCursor(0, 0);
    lcd.print("PhotoResistor:");
    lcd.setCursor(0, 1);
    lcd.print(measuredPR);
    lcd.print("    ");
}

void writeInLCD()
{
    lcd.setCursor(0, 0);
    lcd.print("STOPPED       ");
    lcd.setCursor(0, 1);
    lcd.print("PRESS BUTTON  ");
}

void setStopped(bool stopped)
{
    if (buttonStateStop != stopped)
    {
        buttonStateStop = stopped;
        lcd.clear();
    }
}

void setScreen()
{
    currentStateToShow = !currentStateToShow;
    lcd.clear();
}

void setEQ()
{
    currentStateMeasurement = !currentStateMeasurement;
    lcd.clear();
}

void triggerIRSensor()
{
    if (IrReceiver.decode())
    {
        int command = IrReceiver.decodedIRData.command;
        bool isRepeat = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
        IrReceiver.resume();
        Serial.println(command);
        
        if (isRepeat) return;

        if (command == IR_BUTTON_STOP)        setStopped(true);
        else if (command == IR_BUTTON_PLAY)   setStopped(false);
        else if (command == IR_BUTTON_BACK)   setScreen();
        else if (command == IR_BUTTON_NEXT)   setScreen();
        else if (command == IR_BUTTON_EQ)     setEQ();
    }
}

void ledLEDwithPR()
{
    int data = 256 - currentDataInPR() * 0.374;
    Serial.println(data); 

    if (buttonStateStop == true)
    {
        analogWrite(LED_YELLOW, 0);
    }
    else
    {
        analogWrite(LED_YELLOW, data);
    }
}
