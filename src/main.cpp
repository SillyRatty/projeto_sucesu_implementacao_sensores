#include <Arduino.h>
#include <ESP_Google_Sheet_Client.h>
#include <WiFi.h>
#include <bluetooth_handler.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS     1000

#define DEBOUNCE_TIME_MS 100
#define CONNECTION_TIMEOUT_MS 60000
#define SPREADSHEET_ID "1IOMHVhPE2Gnn6MTJu0-jPUQnvRpsMLtBdbBi0PXbuv4"
#define WIFI_SSID "Moto LuisV"
#define WIFI_PASSWORD "morcegoo"
#define PROJECT_ID "projeto-sucesu"
#define CLIENT_EMAIL "projeto-sucesu@projeto-sucesu.iam.gserviceaccount.com"
#define USER_EMAIL "brassilomonitoramento@gmail.com"
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQCnCevBIop6lN6i\ngpV3z01m3v+E3Gq1wvuJSasrYS9sa4wLE1ETb85nUd/BSSZ4/DVrHrR1V/mpntnh\nNNZubXmP+FRNG7Bpu3vmj4LkUf7xXVixr7VCAVDnpQ7hyQTNIyzj7Sf/rTExOhUE\neC9YJS+/4w6C8bnX9jVyUyCYvGaNFhQcifNzDY9oZkTCTh5qzNCy2NHabKZj6D/S\nEXcmmXO5QwuwIH9oomJMSzRSJ27FRDC6gePcXcv7OCjNILcSd6UF3jTQsB4uBclW\nNTBkdL3ZE/woJM6P3PcKQdd7FLMvISfA0v3iOgyG3ScPR7YCRVC9Gpi0cSGw2pVt\ngE9OA6hdAgMBAAECggEAAwJS8YJXLxW6Qpn9nVBQgaQbKeAnbMM1cTSwu7Hr5WHo\nMbuuwARJZv0UZDcwIglqrbvtqxhJn0k4SmFMSsyRqwrhlReeZvLORmObC25N6Thl\nETzFrbzrIZeMnBUMXBgThlhoWkFbDufDJyoPGEebNs0dUgIoXRnKi31J3taBNm6b\ntt4eVTha6/bpZLyuKJ0DenTZuyWHba//4oMRBpNLBNRWNAarfC/FfhUx/LS5StZv\neZsI1MzNMdaFvmDs2cde9tSLQjqKufJVTRtK5Gc9J9+jPlOjC5ThK7zoD7iIFECG\nd+RGQnzYgSkGdhK2X7WXNJvV0ecsELY801O2vFdw1QKBgQDfzJcjrhcsuikYG4yC\nxnHROwT2OB1vwlEzQT8ZiUC823CpPYNvg5BmCmX1opRMrHyHtQNWL1X+P4wKEONS\n34aMWhYDPZtVpHETBnmrA7RkUQG8QJvhbgAN6QudvcxspgINyLZ0kMfNt5BCMXS6\nL8bh/IGy9EfJ3FRVeQzyP3Hb6wKBgQC/Ep6MQlHRPhUnsmf9STPRIteXHRIR+1ry\nhVb/dEBi4UHrXivkNhGqJoLqOhE/P5D+DojS1wv31aro5NxI2tBSnDCEabVMGJhN\nIs4KEAP4D0sfbgA8tUlMDD+X129DX5L/VnLkZhEEN/ZIrYSmut266AJ95e/B7tOL\nd0LSRJxi1wKBgAalMo+yqamAVZwLpV50ugh2ci1FeUpKQNL+yvrdInj5Ix1DYFW9\nWFpPULu8eh7UuFXnQBQdD7ae+G+MFNPvIzWjryeleqlOWrUv6gjz8TVRWnXWjUpi\nU8oV82xZqGpJe6IhJZnMesJXAweUJF0q8gLf1pITwtXxnudy0rmWvLDFAoGAJkWN\nelyiVL4RsLTcfXBXL7HB6yPIhc+KBP/OUDax2txxvrpLIlfuK92w5UeiQr/xw+GX\naFvoqsiihimpMfKaPaHJWgxuYwKeJv0stflFQj+V9YKsC3zoZJIjfr+JKr6PDBP9\nyV/kUqr+6snOwzK0zQZB0YPoBTlYKBxIRyO1C1sCgYBTdRb+v6V68ydK183TlJ5r\nRrwa7AMSGfvAUp+UEKw68Ph57DKwKflmYmoZvwMO+IplEe9fuILcacZb7ESe16fe\neG9lUpDGbSb4O1c6/vVBjMSbw6uch64xFmyUr6yz/vUn+KEkqx/nk/MHI4x61KdZ\npgRwlWVVKbBkfCj9bHJutw==\n-----END PRIVATE KEY-----\n";

#define BUTTON_PIN 32

#define BLUETOOTH_UPDATE_TIME_MS 1000

String pacient_id = "12345";
uint8_t oxigen = 0;
float heart_rate = 0;
double temperature = 0;

void tokenStatusCallback(TokenInfo info){
    if (info.status == esp_signer_token_status_error){
        Serial.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        Serial.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else{
        Serial.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}
char *numberArray = (char*) malloc(20*sizeof(char));
uint16_t counter = 0;
FirebaseJson response;
void update_sheet(String pacient_id, double temperature, uint8_t oxigen, float heart_rate){
    bool success = false;
    bool ready = false;
    while(!ready) ready = GSheet.ready();
    if (ready){
        struct tm timeinfo;
        char *timeStringBuff;
        timeStringBuff = (char*) malloc(50*sizeof(char));
        String asString;
        char *buffer;
        buffer = (char*) malloc(40*sizeof(char));


        FirebaseJson valueRange;
        // while(true){
        //     bool button_state = digitalRead(BUTTON_PIN);
        //     // Serial.println(button_state);
        //     if(!button_state || last_button_state){
        //         last_button_state = button_state;
        //         continue;
        //     }
        //     // Serial.println("in");
        // // for (int counter = 0; counter < 10; counter++){
            Serial.println("\nUpdate spreadsheet values...");
            Serial.println("------------------------------");
            unsigned long connection_begin_ms = millis();
            while(WiFi.status() != WL_CONNECTED && millis() - connection_begin_ms < CONNECTION_TIMEOUT_MS){
                Serial.print(".");
                delay(300);
            }
            if (!getLocalTime(&timeinfo)) {
                Serial.println("Failed to obtain time");
                return;
            }
            strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
            asString = timeStringBuff;
            asString.replace(" ", "-");
            // Lugar de ler o valor do sensor
            // SensValue = analogRead(LM35_PIN);
            // SensValue = map(SensValue, 0, 1023, 2, 150);
            itoa(temperature, numberArray, 10);

            Serial.print(counter);
            sprintf(buffer, "values/[0]/[4]");
            valueRange.set(buffer, String(roundf(heart_rate*100)/100));
            sprintf(buffer, "values/[0]/[3]");
            valueRange.set(buffer, String(oxigen));
            sprintf(buffer, "values/[0]/[2]");
            valueRange.set(buffer, pacient_id);
            sprintf(buffer, "values/[0]/[1]");
            valueRange.set(buffer, String(round(temperature*100)/100));
            sprintf(buffer, "values/[0]/[0]");
            valueRange.set(buffer, asString);
            sprintf(buffer, "Temperatura!A%d:F%d", 2 + counter, 2 + counter);

            Serial.println(valueRange.raw());

            do{
                success = GSheet.values.update(&response /* returned response */, SPREADSHEET_ID /* spreadsheet Id to update */, buffer /* range to update */, &valueRange /* data to update */);
            } while(!success);
            response.toString(Serial, true);
            Serial.println();
            valueRange.clear();
            counter++;
    }
}

// bool pressed_button = false;
// unsigned long last_button_press_time = 0;
// void IRAM_ATTR button_interrupt(){
//     if(millis() - last_button_press_time > DEBOUNCE_TIME_MS){
//         pressed_button = true;
//         last_button_press_time = millis();
//     }
// }

BluetoothHandler bluetooth_handler;
void handle_message(String message){
    Serial.println(message);
    if(message == "update_sheet") update_sheet(pacient_id, temperature, oxigen, heart_rate);
    else pacient_id = message;
}

// char *typeName[]={"Object","Ambient"};
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
// float getTemp(char type){
//     float value;
//     float tempObjec = mlx.readObjectTempC();//in C object
//     if(type =='C'){
//         value = tempObjec;
//     }
//     return value;
// }
// void printTemp(char type){
//     float tmp = getTemp(type);
//     if(type =='C'){
//         Serial.print(typeName[0]);
//         Serial.print(" ");    
//         Serial.print(tmp);
//         Serial.print("°");      
//         Serial.println("C");
//     }
// }

// Create a PulseOximeter object
PulseOximeter pox;
// Time at which the last beat occurred
uint32_t tsLastReport = 0;
// Callback routine is executed when a pulse is detected
void onBeatDetected(){
    Serial.println("Beat!");
}

void update_bluetooth_data(){
    String message_buffer;

    // uint32_t pacient_id = esp_random();
    message_buffer = "*I" + pacient_id + "*";
    bluetooth_handler.print(message_buffer);

    // uint32_t temperature = esp_random();
    message_buffer = "*T" + String(round(temperature*100)/100) + " °C*";
    bluetooth_handler.print(message_buffer);

    // uint32_t oxigen = esp_random();
    message_buffer = "*O" + String(oxigen) + "%*";
    bluetooth_handler.print(message_buffer);

    // uint32_t heart_rate = esp_random();
    message_buffer = "*H" + String(roundf(heart_rate*100)/100) + "*";
    bluetooth_handler.print(message_buffer);
}

void setup() {
    // pinMode(BUTTON_PIN, INPUT_PULLUP);
    // attachInterrupt(BUTTON_PIN, button_interrupt, FALLING);

    Serial.begin(9600);

    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connection to WiFi");
    unsigned long connection_begin_ms = millis();
    while(WiFi.status() != WL_CONNECTED && millis() - connection_begin_ms < CONNECTION_TIMEOUT_MS){
        Serial.print(".");
        delay(300);
    }
    Serial.println("");
    if(WiFi.status() != WL_CONNECTED){
        Serial.print("[ERROR] Connection failed!");
        return;
    }
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());

    GSheet.setTokenCallback(tokenStatusCallback);
    // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
    GSheet.setPrerefreshSeconds(10 * 60);
    
    GSheet.setSystemTime(time(0));

    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

    bluetooth_handler.setup("Projeto Sucesu");
    Serial.println("bluetooth setup");

    // Initialize sensor
    if (!pox.begin()){
        Serial.println("FAILED");
        for(;;);
    }
    else{
        Serial.println("SUCCESS");
    } 
    pox.setIRLedCurrent(MAX30100_LED_CURR_46_8MA);
    // Register a callback routine
    pox.setOnBeatDetectedCallback(onBeatDetected);

    mlx.begin();
}

unsigned long last_bluetooth_update = 0;
void loop() {
    // Serial.println("loop");
    if(bluetooth_handler.available()){
        // Serial.println("message received");
        String message = "";
        while(bluetooth_handler.available()){
            char incoming_char = bluetooth_handler.read();
            if(!isPrintable(incoming_char)) break;
            message += incoming_char;
        }
        // Serial.println("handling message");
        handle_message(message);
        // Serial.println("message handled");
        bluetooth_handler.clear_buffer();
    }

    if(millis() - last_bluetooth_update > BLUETOOTH_UPDATE_TIME_MS){
        // Serial.println("sending data");
        update_bluetooth_data();
        last_bluetooth_update = millis();
    }

    // delay(2000);

    // if(pressed_button){
    //     update_sheet(String(esp_random()), esp_random());
    //     pressed_button = false;
    // }

    // Read from the sensor 
    pox.update();
    oxigen = pox.getSpO2();
    heart_rate = pox.getHeartRate();
    temperature = mlx.readObjectTempC();

    // Grab the updated heart rate and SpO2 levels
    if (millis() - tsLastReport > REPORTING_PERIOD_MS){
        Serial.print("Heart rate: ");
        Serial.print(heart_rate);
        Serial.print("bpm / SpO2: ");
        Serial.print("%");
        Serial.print(oxigen);
        Serial.print(" / Temperature: ");
        Serial.println(temperature);

        tsLastReport = millis();
    }
}
