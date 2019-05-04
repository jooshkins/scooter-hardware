//********** accel
    #include "Particle.h"
    #include <LIS3DH.h>
    
    SYSTEM_THREAD(ENABLED);

    void movementInterruptHandler();
    
    LIS3DHSPI accel(SPI, A2, WKP);
    
    bool sensorsInitialized;
    unsigned long lastPrintSample = 0;
    
    volatile bool movementInterrupt = false;
    uint8_t lastPos = 0;
//*********** END accel

//********** GPS
    #include "gp20u7_particle.h"
    GP20U7 gps = GP20U7(Serial1);
    Geolocation currentLocation;
    String lat;
    String lon;
    String coords;
//********** END GPS

//********** google maps
    #include <google-maps-device-locator.h>
    GoogleMapsDeviceLocator locator;
//********** END google maps

//********** scooter
    int lock = C0;
    int alarm = C2;
    bool lockState;
    bool alarmState;
    bool permaLock;
    FuelGauge batteryMonitor;
    double stateOfCharge;
    double cellVoltage;
    unsigned long unlockTime;
//********** END scooter


// SYSTEM_MODE(MANUAL); // turn off cell to save data

void setup() {
    
    Particle.function("lock",lockToggle);
    Particle.function("soundAlarm",alarmToggle);
    Particle.function("PermaLock", permaLockToggle);
    Particle.variable("GPS", coords);
    Particle.variable("BatterySOC", stateOfCharge);
    Particle.variable("BatteryVoltage", cellVoltage);
    Particle.variable("UnlockTime", unlockTime);

    //********** scooter
    pinMode(lock, OUTPUT);
    pinMode(alarm, OUTPUT);

    digitalWrite(lock, LOW);
    digitalWrite(alarm, LOW);
    
    lockState = false;
    alarmState = false;
    permaLock= false;
    unlockTime = 0;
    
    Serial.println("setting lock!");
    lockToggle("on");
    //********** END scooter
    
    //********** GPS
    gps.begin();
    //********** END GPS
    
    //********** google maps -- publish every 5min
    locator.withLocatePeriodic(300); 
    //********** END google maps
    
    //********** for accel
    Serial.begin(9600);
    attachInterrupt(WKP, movementInterruptHandler, RISING);

    delay(5000);
    
    // Initialize sensors
    LIS3DHConfig config;
    config.setLowPowerWakeMode(16);
    
    sensorsInitialized = accel.setup(config);
    Serial.printlnf("sensorsInitialized=%d", sensorsInitialized);
    //********** END accel
}


void loop() {
    cellVoltage = batteryMonitor.getVCell();
	stateOfCharge = batteryMonitor.getSoC();
	
    if (stateOfCharge < 10) {
        permaLockToggle("on");
    }
    
    if (permaLock) {
        unlockTime = 0;
    }
    
    if ((Time.now() - unlockTime) >= 900) {
        if (!lockState) {
            Serial.println("Scooter has been unlocked for over 15mins, locking now!");
            lockToggle("on");
        }
    }
    
    //********** GPS
     if (gps.read()) {
        currentLocation = gps.getGeolocation();
        
        lat = String(currentLocation.latitude,5);
        lon = String(currentLocation.longitude,5);
        coords = String(lat + " " + lon);
        
        Serial.println("Your current location is:");
        Serial.println(coords);
        
        Serial.println("Cell Voltage:");
        Serial.println(cellVoltage);
        Serial.println("State of Charge:");
        Serial.println(stateOfCharge);
        
        Serial.println("Time since unlock: ");
        Serial.println(Time.now() - unlockTime);
    }
    //********** END GPS

	if (movementInterrupt) {
    	accel.clearInterrupt();
    	Serial.println("movementInterrupt");
    	
    	if (lockState) {
    	    alarmToggle("on");
    	    Serial.println("sounding alarm!");
    	} else {
    	    Serial.println("not locked, all good!");
    	}
    	// Recalibrate the accelerometer for possibly being in a new orientation.
    	// Wait up to 15 seconds for it to be stationary for 2 seconds.
    	bool ready = accel.calibrateFilter(2000, 15000);
    	Serial.printlnf("calibrateFilter ready=%d", ready);
    	movementInterrupt = false;
    	alarmToggle("off");
    	Serial.println("no more movement, shutting down alarm.");
	}
	
	// google maps
	locator.loop();
}

int lockToggle(String command) {

    if (command=="on") {
        digitalWrite(lock,LOW);
        lockState = true;
        return 1;
    }
    else if (command=="off") {
        digitalWrite(lock,HIGH);
        lockState = false;
        unlockTime = Time.now();
        return 0;
    }
    else {
        return -1;
    }
}

int alarmToggle(String command) {

    if (command=="on") {
        digitalWrite(alarm,HIGH);
        alarmState = true;
        return 1;
    }
    else if (command=="off") {
        digitalWrite(alarm,LOW);
        alarmState = false;
        return 0;
    }
    else {
        return -1;
    }
}

int permaLockToggle(String command) {

    if (command=="on") {
        permaLock = true;
        return 1;
    }
    else if (command=="off") {
        permaLock = false;
        return 0;
    }
    else {
        return -1;
    }
}


void movementInterruptHandler() {
	movementInterrupt = true;
}
