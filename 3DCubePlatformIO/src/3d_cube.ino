
/* Cubes_3D Main File
**
** Author: kwal@itu.dk (WITH ASSISTANCE FROM lukeawalker@hotmail.co.uk)
** Date:   11/14/2022
**
** Coding standards:
**    braces {
**        Indent - 4 spaces
**    }
**    variable_names
**    CONSTANT_NAMES (including #defines)
**    functionNames
**    ClassNames
**    Line length - max 100 characters
*/

/***************************************************************************************************
** INCLUDES
***************************************************************************************************/

/* First put c/c++ standard headers */
#include <stdint.h>
#include "driver/ledc.h"
#include <WiFi.h>
#include <HTTPClient.h>
// #include <ESPmDNS.h>
// #include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
// #include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
//#include <HTTPClient.h>
#include <StreamUtils.h>
#include <ESPmDNS.h>
#include <Preferences.h>

/* Then library headers */
#include "Arduino.h"

/* lastly our own headers */
#include "neural_network.h"
//#include "fake_nn.h"
#include "neural_damage.h" //DAMAGE DEBUG

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/* Program selector */
#define ALL
// #define PULSE_TEST

/* Timer variables */
#define TIMER_0 0
#define PRESCALER_1MHZ 80
#define COUNT_UP true,

#define MAX_MESSAGE_FLOAT_LENGTH 28
#define MAX_MESSAGE_BYTE_LENGTH MAX_MESSAGE_FLOAT_LENGTH*4
#define MESSAGE_GAP_US 1000
#define MESSAGE_TIMESCALER 41 // 118 AT 30HZ
#define MESSAGE_DUTYSCALER 806
#define MESSAGE_OFFSET 403 // MESSAGE_DUTYSCALER/2
#define MESSAGE_FREQUENCY 2400 // HZ
#define MESSAGE_REPETITIONS 3

#define HEADER_MESSAGE 6
//#define INTERRUPT_MESSAGE 9
#define MAX_MESSAGE_DUTY 10
#define ONE_LENGTH 4
#define ZERO_LENGTH 2

#define OFFSET_FACTOR 127
#define SCALE_FACTOR 2//10
#define MAX_MESSAGE_RAW 255.0


#define UPDATE_TIMEOUT_MS 3000
#define MAX_UPDATES 90

#define LED_PIN 45
#define LED_COUNT 1

#define FORMAT_SPIFFS_IF_FAILED true

/***************************************************************************************************
** TYPEDEFS
***************************************************************************************************/

typedef enum
{
    PWM_READY = 0,
    PWM_STARTED,
    PWM_ENDED,
    HEADER_READY,
    HEADER_STARTED,
    HEADER_ENDED,
} pwm_send_state_t;

/***************************************************************************************************
** ENUMS
***************************************************************************************************/

enum
{
    EAST = 0,
    WEST,
    NORTH,
    SOUTH,
    FRONT,
    BACK,
    NUM_DIRECTIONS,
};

/***************************************************************************************************
** CLASSES
***************************************************************************************************/
class Neighborhood
{
public:
    uint16_t east0 = 0;
    uint16_t east1 = 0;
    uint16_t east2 = 0;
    uint16_t east3 = 0;
    uint16_t east4 = 0;
    uint16_t east5 = 0;
    uint16_t east6 = 0;
    uint16_t east7 = 0;

    uint16_t west0 = 0;
    uint16_t west1 = 0;
    uint16_t west2 = 0;
    uint16_t west3 = 0;
    uint16_t west4 = 0;
    uint16_t west5 = 0;
    uint16_t west6 = 0;
    uint16_t west7 = 0;

    uint16_t north0 = 0;
    uint16_t north1 = 0;
    uint16_t north2 = 0;
    uint16_t north3 = 0;
    uint16_t north4 = 0;
    uint16_t north5 = 0;
    uint16_t north6 = 0;
    uint16_t north7 = 0;

    uint16_t south0 = 0;
    uint16_t south1 = 0;
    uint16_t south2 = 0;
    uint16_t south3 = 0;
    uint16_t south4 = 0;
    uint16_t south5 = 0;
    uint16_t south6 = 0;
    uint16_t south7 = 0;

    uint16_t front0 = 0;
    uint16_t front1 = 0;
    uint16_t front2 = 0;
    uint16_t front3 = 0;
    uint16_t front4 = 0;
    uint16_t front5 = 0;
    uint16_t front6 = 0;
    uint16_t front7 = 0;

    uint16_t back0 = 0;
    uint16_t back1 = 0;
    uint16_t back2 = 0;
    uint16_t back3 = 0;
    uint16_t back4 = 0;
    uint16_t back5 = 0;
    uint16_t back6 = 0;
    uint16_t back7 = 0;
};

class FloatData
{
public:
    /*******************************************************************************************
    ** METHODS
    *******************************************************************************************/
    FloatData()
    {
    }

    union ByteToFloat
   {
    uint8_t byteData[4];     
    float floatData; 
   };

    FloatData(float *new_message)
    {
        for (int i = 0; i < MAX_MESSAGE_FLOAT_LENGTH; i++)
        {
            message[i] = new_message[i];
        }
    }

    void fromRaw(uint16_t *raw_data)
    {
        for (int i = 0; i < MAX_MESSAGE_FLOAT_LENGTH; i++)
        {
            ByteToFloat converter;
            converter.byteData[0] = raw_data[i*4];
            converter.byteData[1] = raw_data[i*4+1];
            converter.byteData[2] = raw_data[i*4+2];
            converter.byteData[3] = raw_data[i*4+3];
            message[i] = converter.floatData;
        }
    }

    void toRaw(uint8_t *raw_data)
    {
        for (int i = 0; i < MAX_MESSAGE_FLOAT_LENGTH; i++)
        {   
            ByteToFloat converter;
            converter.floatData = message[i];
            raw_data[i*4] = converter.byteData[0];
            raw_data[i*4+1] = converter.byteData[1];
            raw_data[i*4+2] = converter.byteData[2];
            raw_data[i*4+3] = converter.byteData[3];
        }
    }

    /*******************************************************************************************
    ** MEMBERS
    *******************************************************************************************/
    float message[MAX_MESSAGE_FLOAT_LENGTH] = {0.0};
};

class PwmRxData
{
public:
    /*******************************************************************************************
    ** METHODS
    *******************************************************************************************/
    PwmRxData(int rx_pin)
    {
        pin = rx_pin;
    }

    bool complete()
    {
        return message_pos >= MAX_MESSAGE_BYTE_LENGTH;
    }

    /* Set once when the message completes. Reset by a read to this function (or a reset) */
    bool hasCompleted()
    {
        bool temp = has_completed;
        has_completed = false;
        return temp;
    }

    void reset()
    {
        message_pos = 0;
        start_time = 0;
        end_time = 0;
        has_completed = false;
        started = false;
        header_received = false;
        raw_pos = 0;
        bit_pos = 0;
        //interrupt_received = false;
    }

    uint8_t modifyBit(uint8_t inputByte,uint8_t position, uint8_t bitValue){
        uint8_t outputByte = 0;
        if(bitValue>=1){
            outputByte = inputByte | (1 << position);  // for setting the bit
        }else{      
            outputByte = inputByte & ~(1 << position); // for clearing the bit
        }
        return outputByte;
    }

    void handleRxInterrupt()
    {
        /* Only increment the message if we've not finished yet. This prevents old data being
        ** overwritten before we have a chance to read it */
        if (digitalRead(pin) == HIGH)
        {
            start_time = micros();
            started = true;
        }
        if (digitalRead(pin) == LOW and started == true)
        {
            //if (interrupt_received)
            //{
            //    started = false;
            //    return;
            //}
            //else
            //{
                // Serial.println("Interrupted LOW");
                end_time = micros();
                long elapsedTime = (end_time - start_time);

                //if ((elapsedTime > floor((INTERRUPT_MESSAGE - 0.5) * MESSAGE_TIMESCALER) && (elapsedTime < (MAX_MESSAGE_DUTY)*MESSAGE_TIMESCALER)))
                //{
                //    interrupt_received = true;
                //}

                if (!complete())
                {
                    if (!header_received)
                    {
                        // Serial.print("Header lenght ");
                        // Serial.println(header);
                        if (elapsedTime > floor((HEADER_MESSAGE - 0.5) * MESSAGE_TIMESCALER))
                        {
                            header_received = true;
                            rawMicroseconds[raw_pos++] = elapsedTime;
                        }
                    }
                    else
                    {
                        rawMicroseconds[raw_pos++] = elapsedTime;
                        // message[message_pos++] = elapsedTime / MESSAGE_TIMESCALER;

                        if (elapsedTime > floor((HEADER_MESSAGE - 0.5) * MESSAGE_TIMESCALER))
                        {
                            reset();
                        }
                        else
                        {
                            uint8_t receivedValue = 0;
                            if (elapsedTime > floor(MESSAGE_TIMESCALER * (ONE_LENGTH - 0.5)))
                            {
                                receivedValue = 1;
                            }

                            messageByteBuffer = modifyBit(messageByteBuffer, bit_pos, receivedValue);

                            if (bit_pos >= 7)
                            {
                                message[message_pos++] = messageByteBuffer;
                                bit_pos = 0;
                                messageByteBuffer = 0;
                            }
                            else
                            {
                                bit_pos++;
                            }
                        }
                    }

                    if (complete())
                    {
                        has_completed = true;
                        // header_received = false;
                    }
                }
                started = false;
            //}
        }
    }

    /*******************************************************************************************
    ** MEMBERS
    *******************************************************************************************/
    int pin;

    uint32_t start_time = 0;
    uint32_t end_time = 0;
    uint8_t bit_pos = 0;
    uint8_t message_pos = 0;
    uint16_t header = 0;
    uint16_t raw_pos = 0;

    bool has_completed = false;
    bool started = false;
    bool header_received = false;
    //bool interrupt_received = false;

    /* Message information limited to 8-bits in micros. Used 16-bits to account for potential
    ** noise */
    uint8_t messageByteBuffer = 0;
    uint16_t message[MAX_MESSAGE_BYTE_LENGTH] = {0};
    uint16_t rawMicroseconds[MAX_MESSAGE_BYTE_LENGTH*8 + 1] = {0};
};

class PwmTxData
{
public:
    /*******************************************************************************************
    ** METHODS
    *******************************************************************************************/
    // PwmTxData(int tx_pin)
    // {
    //     pin = tx_pin;
    // }

    PwmTxData(int pwm_channel)
    {
        channel = pwm_channel;
    }

    bool complete()
    {
        return message_pos >= MAX_MESSAGE_BYTE_LENGTH;
    }

    void reset()
    {
        message_pos = 0;
        bit_pos = 0;
        start_time = 0;
        end_time = 0;
        pulse_state = PWM_READY;
        header_sent = false;
        pulse_sent = true;
        message_repeat = 0;
    }

    void setMessage(uint8_t *new_message)
    {
        for (int i = 0; i < MAX_MESSAGE_BYTE_LENGTH; i++)
        {
            message[i] = new_message[i];
        }
    }

    // void sendInterruptMessage(){

    //     if (!complete())
    //         {
    //             /* only send one message at a time - wait for the neural network to update
    //             ** before sending another one */
    //             if (pulse_sent)
    //             {
    //                 if (!header_sent)
    //                 {
    //                     ledcWrite(channel, INTERRUPT_MESSAGE * MESSAGE_DUTYSCALER); // 2900 = 5900 us = 100 units
    //                 }else{
    //                     message_pos = MAX_MESSAGE_BYTE_LENGTH;
    //                 }
    //                 pulse_sent = false;
    //             }
    //         }
    // }

    uint8_t getBit(uint8_t byteNumber, uint8_t position){
        return (byteNumber >> position)  & 0x01;
    }

    void sendMessageLedc()
    {

        if (message_repeat < MESSAGE_REPETITIONS)
        {
            ////////////////// send message!!!! //////////////////////////////////
            if (!complete())
            {
                /* only send one message at a time - wait for the neural network to update
                ** before sending another one */
                if (pulse_sent)
                {
                    if (!header_sent)
                    {
                        ledcWrite(channel, HEADER_MESSAGE * MESSAGE_DUTYSCALER + MESSAGE_OFFSET); // 2900 = 5900 us = 100 units
                    }
                    else
                    {   
                        if (getBit(message[message_pos],bit_pos) == 0)
                        {
                            //ledcWrite(channel, (MESSAGE_DUTYSCALER >> 2) & 0xFF); // 2900 = 5900 us = 100 units
                            ledcWrite(channel, MESSAGE_DUTYSCALER * ZERO_LENGTH); // 2900 = 5900 us = 100 units
                        }
                        else
                        {
                            ledcWrite(channel, (MESSAGE_DUTYSCALER *ONE_LENGTH) + MESSAGE_OFFSET); // 2900 = 5900 us = 100 units
                        }

                        if(bit_pos >= 7)
                        {
                            bit_pos = 0;
                            message_pos++;
                        }
                        else
                        {
                            bit_pos++;
                        }
                        
                    }
                    pulse_sent = false;
                }
            }
            else
            {
                /* Soft reset for the repetition*/
                // Serial.println(message_pos);
                if (pulse_sent)
                {
                    message_repeat++;
                    message_pos = 0;
                    header_sent = false;
                    pulse_sent = true;
                }
            }
        }
    }

    

    /*******************************************************************************************
    ** MEMBERS
    *******************************************************************************************/
    uint8_t message[MAX_MESSAGE_BYTE_LENGTH] = {0};
    uint8_t message_pos = 0;
    uint8_t bit_pos = 0;
    uint8_t message_repeat = 0;
    uint32_t start_time = 0;
    uint32_t end_time = 0;
    pwm_send_state_t pulse_state = PWM_READY;
    int pin;
    int channel;
    bool header_sent = false;
    bool pulse_sent = true;
};

class RgbWriter
{
public:
    /*******************************************************************************************
    ** METHODS
    *******************************************************************************************/
    RgbWriter(int r_pin, int g_pin, int b_pin)
    {
        r = r_pin;
        g = g_pin;
        b = b_pin;
    }

    void rgbWrite(uint8_t r_col, uint8_t g_col = 0, uint8_t b_col = 0)
    {
        analogWrite(r, r_col);
        analogWrite(g, g_col);
        analogWrite(b, b_col);
    }

    /*******************************************************************************************
    ** MEMBERS
    *******************************************************************************************/
    int r;
    int g;
    int b;
};

/***************************************************************************************************
** ARDUINO FUNCTION PROTOTYPES
***************************************************************************************************/

void IRAM_ATTR buttonInterrupt();

void IRAM_ATTR westInterrupt();
void IRAM_ATTR eastInterrupt();
void IRAM_ATTR northInterrupt();
void IRAM_ATTR southInterrupt();
void IRAM_ATTR frontInterrupt();
void IRAM_ATTR backInterrupt();

void IRAM_ATTR westPwmstop();
void IRAM_ATTR eastPwmstop();
void IRAM_ATTR northPwmstop();
void IRAM_ATTR southPwmstop();
void IRAM_ATTR frontPwmstop();
void IRAM_ATTR backPwmstop();

/***************************************************************************************************
** LOCAL FUNCTION PROTOTYPES
***************************************************************************************************/

static void receiveMessage(PwmRxData *rx_data, const char *direction);
static void updateNeuralNet();

void saveNeighborsMem();
void loadNeighborsMem();
void clearNeighborsMem();

/***************************************************************************************************
** SERVER ENDPOINTS FUNCTION PROTOTYPES
***************************************************************************************************/

void createServerEndpoints(AsyncWebServer &server);

/***************************************************************************************************
** LOCAL VARIABLES
***************************************************************************************************/

/* OVER THE AIR VARIABLES*/
/* TODO: Obviously these might need to changed depending on where we are upload from */
// const char *ssid = "Tiles";
// const char *password = "Tiles2022";

const char *ssid = "sensors";
const char *password = "vFBaDdtH"; // Dynamic IP
// const char *password = "sKPMBWsg"; //Static IP

// the IP address for the shield:
// IPAddress ip(192, 168, 74, 28);
// IPAddress gateway(192, 168, 74, 1);
// IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);
WiFiClient client;  // or WiFiClientSecure for HTTPS
HTTPClient http;
//HTTPClient http;

/* Pin definitions */
/* TODO: CHANGE PIN DEFS DEPENDING ON  BOARD DESIGN */
static const int BOOT_BTN = 0;

static const int OUTPUT_PIN_E = 15;
static const int OUTPUT_PIN_W = 10;

static const int RECEIVE_PIN_E = 16;
static const int RECEIVE_PIN_W = 9;

static const int OUTPUT_PIN_N = 7;
static const int OUTPUT_PIN_S = 14;

static const int RECEIVE_PIN_N = 8;
static const int RECEIVE_PIN_S = 13;

static const int OUTPUT_PIN_F = 5;
static const int OUTPUT_PIN_B = 11;

static const int RECEIVE_PIN_F = 6;
static const int RECEIVE_PIN_B = 12;

/* Messages */
static PwmRxData pwm_rx_data[NUM_DIRECTIONS] = {
    PwmRxData(RECEIVE_PIN_E), /* East */
    PwmRxData(RECEIVE_PIN_W), /* West */
    PwmRxData(RECEIVE_PIN_N), /* North */
    PwmRxData(RECEIVE_PIN_S), /* South */
    PwmRxData(RECEIVE_PIN_F), /* Front */
    PwmRxData(RECEIVE_PIN_B), /* Back */
};
static PwmTxData pwm_tx_data[NUM_DIRECTIONS] = {
    PwmTxData(LEDC_CHANNEL_5), /* East */
    PwmTxData(LEDC_CHANNEL_4), /* West */
    PwmTxData(LEDC_CHANNEL_7), /* North */
    PwmTxData(LEDC_CHANNEL_3), /* South */
    PwmTxData(LEDC_CHANNEL_6), /* Front */
    PwmTxData(LEDC_CHANNEL_2), /* Back */

};

/* TODO: Double check, this should fill with 0.0 after the 1.0 */
static float cell_state[MAX_MESSAGE_FLOAT_LENGTH] = {1.0, 0.0};

int state_guess = -1;

static const int NEURAL_NETWORK_PARAMETER = 84;

int UPDATE_NUM = 0;

float channel_updates[MAX_UPDATES][MAX_MESSAGE_FLOAT_LENGTH];
uint8_t guesses[MAX_UPDATES];
uint32_t times[MAX_UPDATES];

uint64_t macId;
uint8_t macChunk0;
uint8_t macChunk1;
uint8_t macChunk2;
uint8_t macChunk3;
uint8_t macChunk4;
uint8_t macChunk5;
uint8_t macChunk6;
uint8_t macChunk7;

bool getNeighbors = false;
bool fail = false;
bool resetBuffers = false;
bool idle_state = false;
bool connected = false;
bool data_sent = false;
unsigned long const timeoutLength = 15000;
unsigned long expStartTime;
IPAddress database_ip = IPAddress(192,168,72,22);
uint8_t send_attempts = 0;
bool send_backup = false;
bool button_pressed = false;
bool interrupt_message_received = false;
unsigned long neighborsElapsedTime;
uint8_t neighborResponses = 0;
bool damage_mode = false;
int8_t damage_class = 0; //DAMAGE DEBUG


String const firmware_version = "4.98";

Neighborhood neighbors;

Preferences preferences;



Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

/***************************************************************************************************
** ARDUINO PRIMARY FUNCTION (No declaration necessary. Not local)
***************************************************************************************************/

void setup()
{

    /* THIS CODE RELATES TO THE OVER THE AIR SETUP*/
    Serial.begin(115200);
    //while (!Serial);
    // delay(3000);
    // Serial.println("Starting");
    
    // Serial.println("Clearing neighbors");
    // clearNeighborsMem();
    // Serial.println("Cleared");
    // while(true);


    //preferences.begin("cube", false);

    //preferences.clear();

    //preferences.end();



    // connectToWifi();

    macId = ESP.getEfuseMac();
    macChunk0 = ESP.getEfuseMac() & 0xFF;
    macChunk1 = (ESP.getEfuseMac() >> 8) & 0xFF;
    macChunk2 = (ESP.getEfuseMac() >> 8 * 2) & 0xFF;
    macChunk3 = (ESP.getEfuseMac() >> 8 * 3) & 0xFF;
    macChunk4 = (ESP.getEfuseMac() >> 8 * 4) & 0xFF;
    macChunk5 = (ESP.getEfuseMac() >> 8 * 5) & 0xFF;
    macChunk6 = (ESP.getEfuseMac() >> 8 * 6) & 0xFF;
    macChunk7 = (ESP.getEfuseMac() >> 8 * 7) & 0xFF;

    pinMode(BOOT_BTN, INPUT_PULLUP);

    pinMode(RECEIVE_PIN_E, INPUT_PULLUP);
    pinMode(RECEIVE_PIN_W, INPUT_PULLUP);
    pinMode(RECEIVE_PIN_N, INPUT_PULLUP);
    pinMode(RECEIVE_PIN_S, INPUT_PULLUP);
    pinMode(RECEIVE_PIN_F, INPUT_PULLUP);
    pinMode(RECEIVE_PIN_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BOOT_BTN), buttonInterrupt, CHANGE);
 
    attachInterrupt(digitalPinToInterrupt(RECEIVE_PIN_W), westInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RECEIVE_PIN_E), eastInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RECEIVE_PIN_N), northInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RECEIVE_PIN_S), southInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RECEIVE_PIN_F), frontInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RECEIVE_PIN_B), backInterrupt, CHANGE);

    /**************************************************************************************************/
    /* LEDC SETUP*/

    ledcAttachPin(OUTPUT_PIN_E, LEDC_CHANNEL_5);
    ledcAttachPin(OUTPUT_PIN_W, LEDC_CHANNEL_4);
    ledcAttachPin(OUTPUT_PIN_N, LEDC_CHANNEL_7);
    ledcAttachPin(OUTPUT_PIN_S, LEDC_CHANNEL_3);
    ledcAttachPin(OUTPUT_PIN_F, LEDC_CHANNEL_6);
    ledcAttachPin(OUTPUT_PIN_B, LEDC_CHANNEL_2);

    ledcSetup(LEDC_CHANNEL_7, MESSAGE_FREQUENCY, 13); // channel 7, 60 Hz PWM (16ms), 13 bit resolution
    ledcSetup(LEDC_CHANNEL_6, MESSAGE_FREQUENCY, 13); // channel 6, 60 Hz PWM (16ms), 13 bit resolution
    ledcSetup(LEDC_CHANNEL_5, MESSAGE_FREQUENCY, 13); // channel 5, 60 Hz PWM (16ms), 13 bit resolution
    ledcSetup(LEDC_CHANNEL_4, MESSAGE_FREQUENCY, 13); // channel 4, 60 Hz PWM (16ms), 13 bit resolution
    ledcSetup(LEDC_CHANNEL_3, MESSAGE_FREQUENCY, 13); // channel 3, 60 Hz PWM (16ms), 13 bit resolution
    ledcSetup(LEDC_CHANNEL_2, MESSAGE_FREQUENCY, 13); // channel 2, 60 Hz PWM (16ms), 13 bit resolution

    attachInterrupt(digitalPinToInterrupt(OUTPUT_PIN_E), eastPwmstop, FALLING);
    attachInterrupt(digitalPinToInterrupt(OUTPUT_PIN_W), westPwmstop, FALLING);
    attachInterrupt(digitalPinToInterrupt(OUTPUT_PIN_N), northPwmstop, FALLING);
    attachInterrupt(digitalPinToInterrupt(OUTPUT_PIN_S), southPwmstop, FALLING);
    attachInterrupt(digitalPinToInterrupt(OUTPUT_PIN_F), frontPwmstop, FALLING);
    attachInterrupt(digitalPinToInterrupt(OUTPUT_PIN_B), backPwmstop, FALLING);

    /**************************************************************************************************/

    /* Cell state is default because this is the setup function */
    FloatData default_floats(cell_state);

    // /* Ready to send! */
    for (int i = 0; i < NUM_DIRECTIONS; i++)
    {
        default_floats.toRaw(pwm_tx_data[i].message);
        pwm_tx_data[i].reset();
    }

    /* LED colour set up */
    strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show();            // Turn OFF all pixels ASAP
    strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

    Serial.println("Reading Memory");

    preferences.begin("cube", true);

    getNeighbors = preferences.getBool("neighbors",false);
    

    if (!getNeighbors)
    {
        Serial.println("Not in getNeighbors");
        send_backup = preferences.getBool("backup", false);
        

        if (send_backup)
        {
            Serial.println("Backup found");
            idle_state = true;

            preferences.getBytes("times", times, preferences.getBytesLength("times"));
            preferences.getBytes("guesses", guesses, preferences.getBytesLength("guesses"));

            for (int i = 0; i < MAX_UPDATES; i++)
            {
                float update_buffer[MAX_MESSAGE_FLOAT_LENGTH];
                preferences.getBytes("updates" + i, update_buffer, preferences.getBytesLength("updates" + i));
                for (int j = 0; j < MAX_MESSAGE_FLOAT_LENGTH; j++)
                {
                    channel_updates[i][j] = update_buffer[j];
                }
            }

        }
            fail = preferences.getBool("fail", false);
            if(fail){
                Serial.println("In fail state");
            }
            damage_mode = preferences.getBool("damage_mode",false);
            if(damage_mode){
                Serial.println("In damage mode");
                damage_class = preferences.getChar("damage_class",0);
                Serial.print("Damage class: ");
                Serial.println(damage_class);
            }
    }else{
        Serial.println("getNeighbors Found"); 
    }

    preferences.end();

    if(getNeighbors){
        Serial.println("neighbors Loading");
        loadNeighborsMem();
        
        Serial.println("neighbors Loaded");
    }
    
    //COMM DEBUG
    // resetBuffers = true;
    // getNeighbors = true;
    // UPDATE_NUM = MAX_UPDATES + 1;

    

    while (millis() < 5000);

    // Flash Green
    colorWipe(strip.Color(0, 255 / 2, 0));
    delay(1000);
    colorWipe(strip.Color(0, 0, 0));
    delay(1000);
    colorWipe(strip.Color(0, 255 / 2, 0));
    delay(1000);
    colorWipe(strip.Color(0, 0, 0));
    delay(1000);
    colorWipe(strip.Color(0, 255 / 2, 0));
    delay(1000);
    colorWipe(strip.Color(0, 0, 0));
    delay(1000);
    colorWipe(strip.Color(0, 255 / 2, 0));
    delay(1000);
}

void loop()
{

#if defined(ALL)
    /* Real program */

    for (int i = 0; i < NUM_DIRECTIONS; i++)
    {
        if (pwm_rx_data[i].complete() && !getNeighbors)
        {

            // Check message values to determine if a neighbors message is being received
            bool check100s = true;

            for (int j = MAX_MESSAGE_BYTE_LENGTH - 1; j > MAX_MESSAGE_BYTE_LENGTH - 11; j--)
            {
                if (pwm_rx_data[i].message[j] != 100)
                {
                    check100s = false;
                }
            }

            if (check100s)
            {
                interrupt_message_received = true;
            }
        }
    }

    //External button pressed  or interrupt header received
    if(button_pressed || interrupt_message_received){
        // if (!getNeighbors)
        // {
        //     for (int i = 0; i < NUM_DIRECTIONS; i++)
        //     {
        //         pwm_tx_data[i].reset();
        //     }

        //     // Send interrrupt header
        //     long delayLapse = 5000;
        //     long startTime = millis();
        //     long elapsed = millis() - startTime;
        //     bool blink = true;
        //     long interval = startTime;
        //     while (elapsed < delayLapse)
        //     {
        //         // for (int i = 0; i < NUM_DIRECTIONS; i++)
        //         // {
        //         //     pwm_tx_data[i].sendInterruptMessage();
        //         // }
        //         if ((millis() - interval) > 100)
        //         {
        //             for (int i = 0; i < NUM_DIRECTIONS; i++)
        //             {
        //                 pwm_tx_data[i].reset();
        //             }
        //             if (blink)
        //             {
        //                 colorWipe(strip.Color(225 / 2, 225 / 2, 0)); // yellow
        //                 blink = false;
        //             }
        //             else
        //             {
        //                 colorWipe(strip.Color(0, 0, 0));
        //                 blink = true;
        //             }
        //             interval = millis();
        //         }
        //         elapsed = millis() - startTime;
        //     }
        // }
        // for (int i = 0; i < NUM_DIRECTIONS; i++)
        //     {
        //         pwm_rx_data[i].reset();
        //     }
        if (button_pressed)
        {
            if (!getNeighbors)
            {
                // Set resetBuffers, idle_state and getNeighbors flags
                resetBuffers = true;
                getNeighbors = true;
                data_sent = true;
            }
            else
            {
                resetBuffers = false;
                getNeighbors = false;
                idle_state = true;
            }
        }else if(interrupt_message_received){
                resetBuffers = true;
                getNeighbors = true;
                data_sent = true;
        }
        //If it was button pressed, toggle getNeighbors in flash
        Serial.println("Interrupted");
        button_pressed=false;
        interrupt_message_received = false;
    }
    

    if (fail)
    {
        UPDATE_NUM = MAX_UPDATES + 1;
        //getNeighbors = false;
        if (!getNeighbors)
        {
            for (int i = 0; i < NUM_DIRECTIONS; i++)
            {
                pwm_rx_data[i].reset();
            }
        }

        colorWipe(strip.Color(255 / 2, 0, 0)); // red
        delay(300);
        if (millis() > 410000)
        { // Just for showing it is failing in the videos
            idle_state = true;
        }      
    }

    if (idle_state)
    {
        UPDATE_NUM = MAX_UPDATES + 1;
        //fail = false;
        colorWipe(strip.Color(0, 0, 255 / 2)); // blue
        delay(300);
        colorWipe(strip.Color(0, 0, 0));
        delay(300);

        if (!connected)
        {
            long randInit = random(0, 5000);
            delay(randInit);

            WiFi.begin(ssid, password);
            // WiFi.config(ip,gateway,subnet);

            Serial.println("");
            uint8_t connect_try_counter = 0;

            // Wait for connection;
            unsigned long startConnection = millis();
            while (WiFi.status() != WL_CONNECTED)
            {
                randInit = random(0, 500);
                delay(500 + randInit);
                colorWipe(strip.Color(255 / 2, 0, 0));
                Serial.print(".");
                delay(500 + randInit);
                colorWipe(strip.Color(0, 0, 0));
                Serial.print(".");
                if (WiFi.status() != WL_CONNECTED && (millis() - startConnection) > timeoutLength)
                {
                    //Flash
                    startConnection = millis();
                    delay(500);
                    colorWipe(strip.Color(0, 0, 255 / 2));
                    Serial.print(".");
                    delay(500);
                    colorWipe(strip.Color(0, 0, 0));
                    Serial.print(".");
                    if(connect_try_counter>=1){
                        //Save in flash and restart
                        preferences.begin("cube", false);
                        
                        if(getNeighbors){
                            bool neighborsFlagInMemory = preferences.getBool("neighbors", false);
                            if(!neighborsFlagInMemory){
                                preferences.putBool("neighbors", true);
                            }
                        }else if (!send_backup)
                        {
                            Serial.println("Creating backup");

                            //preferences.begin("cube", false);

                            preferences.putBytes("times", times, sizeof(times));
                            preferences.putBytes("guesses", guesses, sizeof(guesses));

                            for (int i = 0; i < MAX_UPDATES; i++)
                            {
                                preferences.putBytes("updates" + i, channel_updates[i], sizeof(channel_updates[i]));
                            }

                            Serial.println("Setting backup flag");
                            preferences.putBool("backup", true);

                        }

                          preferences.end();

                          delay(1000);

                          ESP.restart();
                    }
                    connect_try_counter +=1;
                    //Connection failed retrying
                    WiFi.reconnect();
                }
            }

            WiFi.setSleep(false);
            Serial.println("");
            Serial.print("Connected to ");
            Serial.println(ssid);
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            createServerEndpoints(server);
            AsyncElegantOTA.begin(&server); // Start ElegantOTA
            server.begin();
            Serial.println("HTTP server started");
            connected = true;
            

        }

        if((!data_sent && connected) && !getNeighbors){
            
            Serial.print("Service at ");
            Serial.println(database_ip);

            client.setTimeout(5);

            //if (!client.connect(database_ip.toString().c_str(), 8000)) {
            if(!http.begin(client, "http://"+database_ip.toString()+":8000/status_result")) {
                data_sent= false;
                Serial.println(F("Connection to status server failed"));
                send_attempts +=1;
            }else{

                DynamicJsonDocument doc(49152); //49152
                // StaticJsonDocument<49152> doc;

                doc["macId"] = macId;
                doc["fail_state"] = fail;
                doc["firmware"] = firmware_version;
                doc["damage_mode"] = damage_mode;
                doc["damage_class"] = damage_class;

                JsonArray update_times = doc.createNestedArray("update_times");
                for (int i = 0; i < MAX_UPDATES; i++)
                {
                    update_times.add(times[i]);
                }

                JsonArray state_guesses = doc.createNestedArray("state_guesses");
                for (int i = 0; i < MAX_UPDATES; i++)
                {
                    state_guesses.add(guesses[i]);
                }

                JsonArray cell_updates = doc.createNestedArray("cell_updates");
                for (int i = 0; i < MAX_UPDATES; i++)
                {
                    JsonArray cell_updates_row = cell_updates.createNestedArray();
                    for (int j = 0; j < MAX_MESSAGE_FLOAT_LENGTH; j++)
                    {
                        cell_updates_row.add(channel_updates[i][j]);
                    }
                }

                Serial.println("Sending POST");

                // client.println("POST /status_result HTTP/1.1");
                // // Send the HTTP headers
                // client.println("Host: " + database_ip.toString() + ":8000");
                // client.println("Connection: close");
                // client.print("Content-Length: ");
                // client.println(measureJson(doc));
                // client.println("Content-Type: application/json");

                // Terminate headers with a blank line
                // client.println();

                // WriteBufferingStream bufferedWifiClient(client, 64);
                // serializeJson(doc, bufferedWifiClient);
                // bufferedWifiClient.flush();

                char status[32] = {0};
                // client.readBytesUntil('\r', status, sizeof(status));
                
                String json;
                serializeJson(doc, json);

                //http.begin(client, "http://"+database_ip.toString()+"/status_result");
                int code = http.POST(json);

                Serial.println(http.getString());
                Serial.println(code);
                

                //if (strcmp(status, "201 Created") == 0)
                if(code==201)
                {
                    Serial.println("POST successful");
                    data_sent = true;
                    if(send_backup){
                        preferences.begin("cube", false);


                        preferences.putBool("backup", false);
                       

                        preferences.end();
                    }

                    // Put into get neighbors mode
                    //resetBuffers = true;
                }
                else
                {
                    Serial.print(F("Unexpected status: "));
                    Serial.println(status);
                    data_sent = false;
                    send_attempts += 1;
                }
               
                
            

            // Close the connection
            Serial.println("Closing connection");
            //client.stop();
            http.end();

            }

            if(send_attempts>=20 && !data_sent){
                if (!send_backup)
                {   
                    Serial.println("Creating backup");        
                    
                    preferences.begin("cube", false);

                    preferences.putBytes("times", times,sizeof(times));
                    preferences.putBytes("guesses", guesses, sizeof(guesses));  

                    for(int i=0;i<MAX_UPDATES;i++){
                        preferences.putBytes("updates"+i, channel_updates[i], sizeof(channel_updates[i]));
                    }
                    
                    Serial.println("Setting backup flag");
                    preferences.putBool("backup", true);

                    preferences.end();

                    delay(1000);
                }

                ESP.restart();
            }
      
            // Disconnect
            //http.end();
        }


    }

    expStartTime = millis();

    while (UPDATE_NUM < MAX_UPDATES)
    {

        /* Sending */
        for (int i = 0; i < NUM_DIRECTIONS; i++)
        {
            pwm_tx_data[i].sendMessageLedc();
        }

        /* Receiving */
        receiveMessage(&pwm_rx_data[EAST], "East");
        receiveMessage(&pwm_rx_data[WEST], "West");
        receiveMessage(&pwm_rx_data[NORTH], "North");
        receiveMessage(&pwm_rx_data[SOUTH], "South");
        receiveMessage(&pwm_rx_data[FRONT], "Front");
        receiveMessage(&pwm_rx_data[BACK], "Back");

        for (int i = 0; i < NUM_DIRECTIONS; i++)
        {
            if (pwm_rx_data[i].complete() && !getNeighbors)
            {

                // Check message values to determine if a neighbors message is being received
                bool check100s = true;

                for (int j = MAX_MESSAGE_BYTE_LENGTH - 1; j > MAX_MESSAGE_BYTE_LENGTH - 11; j--)
                {
                    if (pwm_rx_data[i].message[j] != 100)
                    {
                        check100s = false;
                    }
                }

                if (check100s)
                {
                    interrupt_message_received = true;
                }
            }
        }

        /* Updating */
        uint32_t now = millis();
        static uint32_t prev = 0;
        if ((now - prev) > UPDATE_TIMEOUT_MS)
        {
            if (UPDATE_NUM > 0)
            {
                updateNeuralNet();
            }
            
            /* Copy cell state into send messages ready for new send */
            FloatData floats(cell_state);

            /* Ready to send! */
            for (int i = 0; i < NUM_DIRECTIONS; i++)
            {
                floats.toRaw(pwm_tx_data[i].message);
                // for (int j = 0; j < MAX_MESSAGE_LENGTH; j++){
                //     Serial.print(pwm_tx_data[i].message[j]);
                //     Serial.print(", ");
                // }
                // Serial.println();
                pwm_tx_data[i].reset();
            }
            //floats.toRaw(channel_updates[UPDATE_NUM]);
            for(int i=0;i<MAX_MESSAGE_FLOAT_LENGTH;i++){
                channel_updates[UPDATE_NUM][i] = cell_state[i];
            }
            guesses[UPDATE_NUM]=state_guess;
            times[UPDATE_NUM] = millis()-expStartTime;

            UPDATE_NUM = UPDATE_NUM + 1;
            prev = now;
        }
        if ((getNeighbors || fail) || (button_pressed || interrupt_message_received))
        {
            break;
        }
        if(UPDATE_NUM ==MAX_UPDATES){
            Serial.println("Exiting NCA going idle");
            idle_state = true;
        }
    }
      
    

    if (resetBuffers)
    {
        UPDATE_NUM = MAX_UPDATES + 1;
        //fail = false;
        getNeighbors = true;
        resetBuffers = false;

        // /* Clearing all receive buffers */
        pwm_rx_data[EAST].reset();
        pwm_rx_data[WEST].reset();
        pwm_rx_data[NORTH].reset();
        pwm_rx_data[SOUTH].reset();
        pwm_rx_data[FRONT].reset();
        pwm_rx_data[BACK].reset();

        // Serial.println("Waiting for all transmit communications to cease");
        /* Waiting for all transmit communications to cease */
        int delayLapse = 2000;
        long startTime = millis();
        long elapsed = millis() - startTime;
        while (elapsed < delayLapse)
        {
            elapsed = millis() - startTime;
        }
        neighborsElapsedTime = millis();
    }

    if (getNeighbors)
    {
        UPDATE_NUM = MAX_UPDATES+1;
        colorWipe(strip.Color(225 / 2, 225 / 2, 0)); // yellow
        delay(300);
        colorWipe(strip.Color(0, 0, 0));
        delay(300);
        Serial.println("Halting updates");
        Serial.println("Sending neighbor information");
        Serial.println("Clearing all receive buffers");

        if(millis() - neighborsElapsedTime > 2000){//COMM DEBUG

            receiveMessage(&pwm_rx_data[EAST], "East");
            receiveMessage(&pwm_rx_data[WEST], "West");
            receiveMessage(&pwm_rx_data[NORTH], "North");
            receiveMessage(&pwm_rx_data[SOUTH], "South");
            receiveMessage(&pwm_rx_data[FRONT], "Front");
            receiveMessage(&pwm_rx_data[BACK], "Back");

            // Serial.println("Unpacking");

            // Serial.println("Saving");

            if (pwm_rx_data[WEST].complete())
            {
                neighbors.west0 = pwm_rx_data[WEST].message[0];
                neighbors.west1 = pwm_rx_data[WEST].message[1];
                neighbors.west2 = pwm_rx_data[WEST].message[2];
                neighbors.west3 = pwm_rx_data[WEST].message[3];
                neighbors.west4 = pwm_rx_data[WEST].message[4];
                neighbors.west5 = pwm_rx_data[WEST].message[5];
                neighbors.west6 = pwm_rx_data[WEST].message[6];
                neighbors.west7 = pwm_rx_data[WEST].message[7];
            }
            if (pwm_rx_data[EAST].complete())
            {
                neighbors.east0 = pwm_rx_data[EAST].message[0];
                neighbors.east1 = pwm_rx_data[EAST].message[1];
                neighbors.east2 = pwm_rx_data[EAST].message[2];
                neighbors.east3 = pwm_rx_data[EAST].message[3];
                neighbors.east4 = pwm_rx_data[EAST].message[4];
                neighbors.east5 = pwm_rx_data[EAST].message[5];
                neighbors.east6 = pwm_rx_data[EAST].message[6];
                neighbors.east7 = pwm_rx_data[EAST].message[7];
            }
            if (pwm_rx_data[NORTH].complete())
            {
                neighbors.north0 = pwm_rx_data[NORTH].message[0];
                neighbors.north1 = pwm_rx_data[NORTH].message[1];
                neighbors.north2 = pwm_rx_data[NORTH].message[2];
                neighbors.north3 = pwm_rx_data[NORTH].message[3];
                neighbors.north4 = pwm_rx_data[NORTH].message[4];
                neighbors.north5 = pwm_rx_data[NORTH].message[5];
                neighbors.north6 = pwm_rx_data[NORTH].message[6];
                neighbors.north7 = pwm_rx_data[NORTH].message[7];
            }
            if (pwm_rx_data[SOUTH].complete())
            {
                neighbors.south0 = pwm_rx_data[SOUTH].message[0];
                neighbors.south1 = pwm_rx_data[SOUTH].message[1];
                neighbors.south2 = pwm_rx_data[SOUTH].message[2];
                neighbors.south3 = pwm_rx_data[SOUTH].message[3];
                neighbors.south4 = pwm_rx_data[SOUTH].message[4];
                neighbors.south5 = pwm_rx_data[SOUTH].message[5];
                neighbors.south6 = pwm_rx_data[SOUTH].message[6];
                neighbors.south7 = pwm_rx_data[SOUTH].message[7];
            }
            if (pwm_rx_data[FRONT].complete())
            {
                neighbors.front0 = pwm_rx_data[FRONT].message[0];
                neighbors.front1 = pwm_rx_data[FRONT].message[1];
                neighbors.front2 = pwm_rx_data[FRONT].message[2];
                neighbors.front3 = pwm_rx_data[FRONT].message[3];
                neighbors.front4 = pwm_rx_data[FRONT].message[4];
                neighbors.front5 = pwm_rx_data[FRONT].message[5];
                neighbors.front6 = pwm_rx_data[FRONT].message[6];
                neighbors.front7 = pwm_rx_data[FRONT].message[7];
            }
            if (pwm_rx_data[BACK].complete())
            {
                neighbors.back0 = pwm_rx_data[BACK].message[0];
                neighbors.back1 = pwm_rx_data[BACK].message[1];
                neighbors.back2 = pwm_rx_data[BACK].message[2];
                neighbors.back3 = pwm_rx_data[BACK].message[3];
                neighbors.back4 = pwm_rx_data[BACK].message[4];
                neighbors.back5 = pwm_rx_data[BACK].message[5];
                neighbors.back6 = pwm_rx_data[BACK].message[6];
                neighbors.back7 = pwm_rx_data[BACK].message[7];
            }

            for (int i = 0; i < NUM_DIRECTIONS; i++)
            {
                pwm_rx_data[i].reset();
            }
            if (neighborResponses == 1 && !connected)
            {
                long randInit = random(0, 5000);
                delay(randInit);

                WiFi.begin(ssid, password);
                // WiFi.config(ip,gateway,subnet);

                Serial.println("");
                uint8_t connect_try_counter = 0;

                // Wait for connection;
                unsigned long startConnection = millis();
                while (WiFi.status() != WL_CONNECTED)
                {
                    randInit = random(0, 500);
                    delay(500 + randInit);
                    colorWipe(strip.Color(255 / 2, 0, 0));
                    Serial.print(".");
                    delay(500 + randInit);
                    colorWipe(strip.Color(0, 0, 0));
                    Serial.print(".");
                    if (WiFi.status() != WL_CONNECTED && (millis() - startConnection) > timeoutLength)
                    {
                        // Flash
                        startConnection = millis();
                        delay(500);
                        colorWipe(strip.Color(0, 0, 255 / 2));
                        Serial.print(".");
                        delay(500);
                        colorWipe(strip.Color(0, 0, 0));
                        Serial.print(".");
                        if (connect_try_counter >= 1)
                        {
                            // Save in flash and restart
                            saveNeighborsMem();
                            delay(1000);

                            ESP.restart();
                        }
                        connect_try_counter += 1;
                        // Connection failed retrying
                        WiFi.reconnect();
                    }
                }

                WiFi.setSleep(false);
                Serial.println("");
                Serial.print("Connected to ");
                Serial.println(ssid);
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());

                createServerEndpoints(server);
                AsyncElegantOTA.begin(&server); // Start ElegantOTA
                server.begin();
                Serial.println("HTTP server started");

                connected = true;
            }
            neighborResponses++;
            neighborsElapsedTime = millis();
        }

        /* Ready to send! */
        for (int i = 0; i < NUM_DIRECTIONS; i++)
        {
            pwm_tx_data[i].message[0] = macChunk0;// COMM DEBUG
            pwm_tx_data[i].message[1] = macChunk1;
            pwm_tx_data[i].message[2] = macChunk2;
            pwm_tx_data[i].message[3] = macChunk3;
            pwm_tx_data[i].message[4] = macChunk4;
            pwm_tx_data[i].message[5] = macChunk5;
            pwm_tx_data[i].message[6] = macChunk6;
            pwm_tx_data[i].message[7] = macChunk7;
            for (int j = 8; j < MAX_MESSAGE_BYTE_LENGTH; j++)
            {
                pwm_tx_data[i].message[j] = 100;
            }
            // floatsID.toRaw(pwm_tx_data[i].message);
            pwm_tx_data[i].reset();
        }

        Serial.println("Sending");
        // startTime = millis();
        /* Sending */

        Serial.println("Waiting for all cubes to send");
        /* Waiting for all transmit communications to happen */
        long delayLapse = 1500;
        long startTime = millis();
        long elapsed = millis() - startTime;
        colorWipe(strip.Color(225 / 2, 225 / 2, 0)); // yellow
        while (elapsed < delayLapse)
        {
            elapsed = millis() - startTime;
            for (int i = 0; i < NUM_DIRECTIONS; i++)
            {
                pwm_tx_data[i].sendMessageLedc();
            }
        }

        colorWipe(strip.Color(0, 0, 0));
        delay(300);

        // receiveMessage(&pwm_rx_data[EAST], "East"); //COMM DEBUG
        // receiveMessage(&pwm_rx_data[WEST], "West");
        // receiveMessage(&pwm_rx_data[NORTH], "North");
        // receiveMessage(&pwm_rx_data[SOUTH], "South");
        // receiveMessage(&pwm_rx_data[FRONT], "Front");
        // receiveMessage(&pwm_rx_data[BACK], "Back");

    //     for (int i = 0; i < NUM_DIRECTIONS; i++) //COMM DEBUG
    // {
    //     if (pwm_rx_data[i].complete())
    //     {
    //         pwm_rx_data[i].reset();
    //     }
    // }

        if ((WiFi.status() != WL_CONNECTED) && connected){
            saveNeighborsMem();
            delay(1000);
            ESP.restart();
        }

        // getNeighbors = false;
    }

#endif
}

void IRAM_ATTR buttonInterrupt(){
    if(digitalRead(BOOT_BTN)==LOW){
        button_pressed = true;
    }
}

/*!
** @brief Calls message handling function with WEST location. TODO: does this need to be in
**        "ARDUINO" section?
*/
void IRAM_ATTR westInterrupt()
{
    pwm_rx_data[WEST].handleRxInterrupt();
}

/*!
** @brief Calls message handling function with EAST location. TODO: does this need to be in
**        "ARDUINO" section?
*/
void IRAM_ATTR eastInterrupt()
{
    pwm_rx_data[EAST].handleRxInterrupt();
}

void IRAM_ATTR northInterrupt()
{
    pwm_rx_data[NORTH].handleRxInterrupt();
}

void IRAM_ATTR southInterrupt()
{
    pwm_rx_data[SOUTH].handleRxInterrupt();
}

void IRAM_ATTR frontInterrupt()
{
    pwm_rx_data[FRONT].handleRxInterrupt();
}

void IRAM_ATTR backInterrupt()
{
    pwm_rx_data[BACK].handleRxInterrupt();
}

void IRAM_ATTR eastPwmstop()
{
    ledcWrite(LEDC_CHANNEL_5, 0);
    if (!pwm_tx_data[EAST].header_sent)
    {
        pwm_tx_data[EAST].header_sent = true;
    }
    pwm_tx_data[EAST].pulse_sent = true;
    // pwm_tx_data[EAST].sendMessageLedc();
}

void IRAM_ATTR westPwmstop()
{
    ledcWrite(LEDC_CHANNEL_4, 0);
    if (!pwm_tx_data[WEST].header_sent)
    {
        pwm_tx_data[WEST].header_sent = true;
    }
    pwm_tx_data[WEST].pulse_sent = true;
    // pwm_tx_data[WEST].sendMessageLedc();
}

void IRAM_ATTR northPwmstop()
{
    ledcWrite(LEDC_CHANNEL_7, 0);
    if (!pwm_tx_data[NORTH].header_sent)
    {
        pwm_tx_data[NORTH].header_sent = true;
    }
    pwm_tx_data[NORTH].pulse_sent = true;
    // pwm_tx_data[NORTH].sendMessageLedc();
}

void IRAM_ATTR southPwmstop()
{
    ledcWrite(LEDC_CHANNEL_3, 0);
    if (!pwm_tx_data[SOUTH].header_sent)
    {
        pwm_tx_data[SOUTH].header_sent = true;
    }
    pwm_tx_data[SOUTH].pulse_sent = true;
    // pwm_tx_data[SOUTH].sendMessageLedc();
}

void IRAM_ATTR frontPwmstop()
{
    ledcWrite(LEDC_CHANNEL_6, 0);
    if (!pwm_tx_data[FRONT].header_sent)
    {
        pwm_tx_data[FRONT].header_sent = true;
    }
    pwm_tx_data[FRONT].pulse_sent = true;
    // pwm_tx_data[FRONT].sendMessageLedc();
}

void IRAM_ATTR backPwmstop()
{
    ledcWrite(LEDC_CHANNEL_2, 0);
    if (!pwm_tx_data[BACK].header_sent)
    {
        pwm_tx_data[BACK].header_sent = true;
    }
    pwm_tx_data[BACK].pulse_sent = true;
    // pwm_tx_data[BACK].sendMessageLedc();
}

/***************************************************************************************************
** LOCAL FUNCTION DEFINITIONS
***************************************************************************************************/

static void receiveMessage(PwmRxData *rx_data, const char *direction)
{
    if (rx_data->hasCompleted())
    {
        /* Print the whole message... and reset ready for the next one! */
        Serial.print(direction);
        Serial.print(" received: ");
        for (int i = 0; i < MAX_MESSAGE_BYTE_LENGTH; i++)
        {
            Serial.print(rx_data->message[i]);
            Serial.print(", ");
        }
        Serial.println();
        Serial.print("microseconds: ");
        for (int i = 0; i < MAX_MESSAGE_BYTE_LENGTH*8 + 1; i++)
        {
            Serial.print(rx_data->rawMicroseconds[i]);
            Serial.print(", ");
        }
        Serial.println();
    }
}

static void sumProductNn(float *sumA1, const float *pk, float *b, int i_loop)
{
    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        for (int i = 1; i < i_loop; i++)
        {
            int k = i * NEURAL_NETWORK_PARAMETER + j;
            sumA1[j] += (pgm_read_float_near(pk + k) * b[i]);
        }
    }
}

static void printArray(const char *array_name, float *array)
{
    Serial.print(array_name);
    Serial.print(',');
    for (int j = 0; j < MAX_MESSAGE_FLOAT_LENGTH; j++)
    {
        Serial.print(array[j]);
        Serial.print(',');
    }
    Serial.println();
}

static void colorWipe(uint32_t color)
{
    for (int i = 0; i < strip.numPixels(); i++)
    {                                  // For each pixel in strip...
        strip.setPixelColor(i, color); //  Set pixel's color (in RAM)
        strip.show();                  //  Update strip to match
    }
}

static void updateNeuralNet()
{

    float sumA1[NEURAL_NETWORK_PARAMETER] = {0};
    float sumB[NEURAL_NETWORK_PARAMETER] = {0};
    float sumC[MAX_MESSAGE_FLOAT_LENGTH - 1] = {0};
    float max_num = -10000;
    int max_position = -1;

    FloatData recv_messages[NUM_DIRECTIONS];
    for (int i = 0; i < NUM_DIRECTIONS; i++)
    {
        if (pwm_rx_data[i].complete())
        {
            recv_messages[i].fromRaw(pwm_rx_data[i].message);
            pwm_rx_data[i].reset();
        }
    }

    if (recv_messages[WEST].message[0] != 0)
    {
        recv_messages[WEST].message[0] = 1;
    }
    if (recv_messages[EAST].message[0] != 0)
    {
        recv_messages[EAST].message[0] = 1;
    }
    if (recv_messages[SOUTH].message[0] != 0)
    {
        recv_messages[SOUTH].message[0] = 1;
    }
    if (recv_messages[NORTH].message[0] != 0)
    {
        recv_messages[NORTH].message[0] = 1;
    }
    if (recv_messages[FRONT].message[0] != 0)
    {
        recv_messages[FRONT].message[0] = 1;
    }
    if (recv_messages[BACK].message[0] != 0)
    {
        recv_messages[BACK].message[0] = 1;
    }

    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        for (int i = 0; i < MAX_MESSAGE_FLOAT_LENGTH; i++)
        {
            int k = i * NEURAL_NETWORK_PARAMETER + j;
            if(!damage_mode){
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_self[0][0] + k) * cell_state[i]);
            }else{
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_self_damage[0][0] + k) * cell_state[i]);
            }
        }
    }
    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        for (int i = 0; i < MAX_MESSAGE_FLOAT_LENGTH; i++)
        {
            int k = i * NEURAL_NETWORK_PARAMETER + j;
            if(!damage_mode){
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_west[0][0] + k) * recv_messages[WEST].message[i]);
            }else{
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_west_damage[0][0] + k) * recv_messages[WEST].message[i]);
            }
        }
    }
    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        for (int i = 0; i < MAX_MESSAGE_FLOAT_LENGTH; i++)
        {
            int k = i * NEURAL_NETWORK_PARAMETER + j;
            if(!damage_mode){
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_east[0][0] + k) * recv_messages[EAST].message[i]);
            }else{
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_east_damage[0][0] + k) * recv_messages[EAST].message[i]);
            }
        }
    }
    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        for (int i = 0; i < MAX_MESSAGE_FLOAT_LENGTH; i++)
        {
            int k = i * NEURAL_NETWORK_PARAMETER + j;
            if(!damage_mode){
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_south[0][0] + k) * recv_messages[SOUTH].message[i]);
            }else{
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_south_damage[0][0] + k) * recv_messages[SOUTH].message[i]);
            }
        }
    }
    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        for (int i = 0; i < MAX_MESSAGE_FLOAT_LENGTH; i++)
        {
            int k = i * NEURAL_NETWORK_PARAMETER + j;
            if(!damage_mode){
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_north[0][0] + k) * recv_messages[NORTH].message[i]);
            }else{
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_north_damage[0][0] + k) * recv_messages[NORTH].message[i]);
            }
        }
    }
    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        for (int i = 0; i < MAX_MESSAGE_FLOAT_LENGTH; i++)
        {
            int k = i * NEURAL_NETWORK_PARAMETER + j;
            if(!damage_mode){
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_front[0][0] + k) * recv_messages[FRONT].message[i]);
            }else{
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_front_damage[0][0] + k) * recv_messages[FRONT].message[i]);
            }
        }
    }
    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        for (int i = 0; i < MAX_MESSAGE_FLOAT_LENGTH; i++)
        {
            int k = i * NEURAL_NETWORK_PARAMETER + j;
            if(!damage_mode){
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_back[0][0] + k) * recv_messages[BACK].message[i]);
            }else{
                sumA1[j] += (pgm_read_float_near(&percieve_kernel_back_damage[0][0] + k) * recv_messages[BACK].message[i]);
            }
        }
    }

    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        if(!damage_mode){
            sumA1[j] += (pgm_read_float_near(&percieve_bias[0] + j));
        }else{
            sumA1[j] += (pgm_read_float_near(&percieve_bias_damage[0] + j));
        }
    }

    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        if (sumA1[j] < 0)
        {
            sumA1[j] = 0;
        }
    }

    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {
        for (int i = 0; i < NEURAL_NETWORK_PARAMETER; i++)
        {
            int k = i * NEURAL_NETWORK_PARAMETER + j;
            if(!damage_mode){
                sumB[j] += sumA1[i] * (pgm_read_float_near(&dmodel_kernel_1[0][0] + k));
            }else{
                sumB[j] += sumA1[i] * (pgm_read_float_near(&dmodel_kernel_1_damage[0][0] + k));
            }
        }
    }

    for (int j = 0; j < NEURAL_NETWORK_PARAMETER; j++)
    {   
        if(!damage_mode){
            sumB[j] += (pgm_read_float_near(&dmodel_bias1[0] + j));
        }else{
            sumB[j] += (pgm_read_float_near(&dmodel_bias1_damage[0] + j));
        }
        if (sumB[j] < 0)
        {
            sumB[j] = 0;
        }
    }

    for (int j = 0; j < MAX_MESSAGE_FLOAT_LENGTH - 1; j++)
    {
        for (int i = 0; i < NEURAL_NETWORK_PARAMETER; i++)
        {
            int k = i * (MAX_MESSAGE_FLOAT_LENGTH - 1) + j;
            if(!damage_mode){
                sumC[j] += sumB[i] * (pgm_read_float_near(&dmodel_kernel_2[0][0] + k));
            }else{
                sumC[j] += sumB[i] * (pgm_read_float_near(&dmodel_kernel_2_damage[0][0] + k));
            }
        }
    }

    for (int j = 0; j < MAX_MESSAGE_FLOAT_LENGTH - 1; j++)
    {
        if(!damage_mode){
            sumC[j] += (pgm_read_float_near(&dmodel_bias2[0] + j));
        }else{
            sumC[j] += (pgm_read_float_near(&dmodel_bias2_damage[0] + j));
        }
    }

    for (int j = 1; j < MAX_MESSAGE_FLOAT_LENGTH; j++)
    {  
        if(!damage_mode){
            cell_state[j] += tanh(sumC[j - 1]);
        }else{
            int k = damage_class * (MAX_MESSAGE_FLOAT_LENGTH - 1) + (j-1); //DAMAGE DEBUG
            //Serial.printf("%.6f",pgm_read_float_near(&class_embeddings_damage[0][0] + k));
            cell_state[j] += sumC[j - 1] + (pgm_read_float_near(&class_embeddings_damage[0][0] + k));
        }
        
    }

    printArray("W", recv_messages[WEST].message);
    printArray("E", recv_messages[EAST].message);
    printArray("N", recv_messages[NORTH].message);
    printArray("S", recv_messages[SOUTH].message);
    printArray("F", recv_messages[FRONT].message);
    printArray("B", recv_messages[BACK].message);
    printArray("CELL", cell_state);

    // now we find out which number the cell thinks it is...
    // looks for the position of the maximum number of the last 10 digits..
    for (int i = (MAX_MESSAGE_FLOAT_LENGTH - 7); i < MAX_MESSAGE_FLOAT_LENGTH; i++)
    {
        // we can also print it for debugging...

        if (cell_state[i] > max_num)
        {
            max_num = cell_state[i];
            max_position = i - 21;
        }
    }
    Serial.println(UPDATE_NUM);
    Serial.println("my guess is..");
    Serial.println(max_position);

    state_guess = max_position;

    if (max_position == 3)
    {
        colorWipe(strip.Color(225 / 2, 225 / 2, 0));
    } // yellow
    if (max_position == 1)
    {
        colorWipe(strip.Color(0, 0, 255 / 2));
    } // blue
    if (max_position == 4)
    {
        colorWipe(strip.Color(255 / 2, 255/2, 255/2));
    } // white
    if (max_position == 0)
    {
        colorWipe(strip.Color(0, 255 / 2, 0));
    } // green
    if (max_position == 2)
    {
        colorWipe(strip.Color(255 / 2, 0, 255 / 2));
    } // purple
    if (max_position == 5)
    {
        colorWipe(strip.Color(255 / 2, 105 / 2, 180 / 2));
    } // pink
    if (max_position == 6)
    {
        colorWipe(strip.Color(0, 105 / 2, 105 / 2));
    } // purple
}

void saveNeighborsMem(){
    preferences.begin("cube", false);
    bool neighborsFlagInMemory = preferences.getBool("neighbors", false);
    if(!neighborsFlagInMemory){
        preferences.putBool("neighbors", true);
    }
    uint16_t bufferRAM[8];
    uint16_t bufferRead[8];

            bufferRAM[0] = neighbors.west0;
            bufferRAM[1] = neighbors.west1;
            bufferRAM[2] = neighbors.west2;
            bufferRAM[3] = neighbors.west3;
            bufferRAM[4] = neighbors.west4;
            bufferRAM[5] = neighbors.west5;
            bufferRAM[6] = neighbors.west6;
            bufferRAM[7] = neighbors.west7; 
            preferences.getBytes("nWEST", bufferRead , preferences.getBytesLength("nWEST"));
            bool different = false;
            for(int i=0;i<8;i++){
                if(bufferRead[i] != bufferRAM[i]){
                    different = true;
                }    
            }
            if(different){
                preferences.putBytes("nWEST", bufferRAM,sizeof(bufferRAM)); 
            }                 

            bufferRAM[0] = neighbors.east0;
            bufferRAM[1] = neighbors.east1;
            bufferRAM[2] = neighbors.east2;
            bufferRAM[3] = neighbors.east3;
            bufferRAM[4] = neighbors.east4;
            bufferRAM[5] = neighbors.east5;
            bufferRAM[6] = neighbors.east6;
            bufferRAM[7] = neighbors.east7;
            preferences.getBytes("nEAST", bufferRead , preferences.getBytesLength("nEAST"));
            different = false;
            for(int i=0;i<8;i++){
                if(bufferRead[i] != bufferRAM[i]){
                    different = true;
                }    
            }
            if(different){
                preferences.putBytes("nEAST", bufferRAM,sizeof(bufferRAM)); 
            }

            bufferRAM[0] = neighbors.north0;
            bufferRAM[1] = neighbors.north1;
            bufferRAM[2] = neighbors.north2;
            bufferRAM[3] = neighbors.north3;
            bufferRAM[4] = neighbors.north4;
            bufferRAM[5] = neighbors.north5;
            bufferRAM[6] = neighbors.north6;
            bufferRAM[7] = neighbors.north7;
            preferences.getBytes("nNORTH", bufferRead , preferences.getBytesLength("nNORTH"));
            different = false;
            for(int i=0;i<8;i++){
                if(bufferRead[i] != bufferRAM[i]){
                    different = true;
                }    
            }
            if(different){
                preferences.putBytes("nNORTH", bufferRAM,sizeof(bufferRAM)); 
            }

            bufferRAM[0] = neighbors.south0;
            bufferRAM[1] = neighbors.south1;
            bufferRAM[2] = neighbors.south2;
            bufferRAM[3] = neighbors.south3;
            bufferRAM[4] = neighbors.south4;
            bufferRAM[5] = neighbors.south5;
            bufferRAM[6] = neighbors.south6;
            bufferRAM[7] = neighbors.south7;
            preferences.getBytes("nSOUTH", bufferRead , preferences.getBytesLength("nSOUTH"));
            different = false;
            for(int i=0;i<8;i++){
                if(bufferRead[i] != bufferRAM[i]){
                    different = true;
                }    
            }
            if(different){
                preferences.putBytes("nSOUTH", bufferRAM,sizeof(bufferRAM)); 
            }

            bufferRAM[0] = neighbors.front0;
            bufferRAM[1] = neighbors.front1;
            bufferRAM[2] = neighbors.front2;
            bufferRAM[3] = neighbors.front3;
            bufferRAM[4] = neighbors.front4;
            bufferRAM[5] = neighbors.front5;
            bufferRAM[6] = neighbors.front6;
            bufferRAM[7] = neighbors.front7;
            preferences.getBytes("nFRONT", bufferRead , preferences.getBytesLength("nFRONT"));
            different = false;
            for(int i=0;i<8;i++){
                if(bufferRead[i] != bufferRAM[i]){
                    different = true;
                }    
            }
            if(different){
                preferences.putBytes("nFRONT", bufferRAM,sizeof(bufferRAM)); 
            }
            
            bufferRAM[0] = neighbors.back0;
            bufferRAM[1] = neighbors.back1;
            bufferRAM[2] = neighbors.back2;
            bufferRAM[3] = neighbors.back3;
            bufferRAM[4] = neighbors.back4;
            bufferRAM[5] = neighbors.back5;
            bufferRAM[6] = neighbors.back6;
            bufferRAM[7] = neighbors.back7;
            preferences.getBytes("nBACK", bufferRead , preferences.getBytesLength("nBACK"));
            different = false;
            for(int i=0;i<8;i++){
                if(bufferRead[i] != bufferRAM[i]){
                    different = true;
                }    
            }
            if(different){
                preferences.putBytes("nBACK", bufferRAM,sizeof(bufferRAM)); 
            }
    preferences.end();
}

void loadNeighborsMem(){
    preferences.begin("cube", true);
    
    uint16_t buffer[8];
    Serial.println("Loading West");
    if(preferences.isKey("nWEST")){
        Serial.println("West Found");
    preferences.getBytes("nWEST", buffer , preferences.getBytesLength("nWEST"));
    neighbors.west0 = buffer[0];
    Serial.println("Pos 0 loaded");
    neighbors.west1 = buffer[1];
    Serial.println("Pos 1 loaded");
    neighbors.west2 = buffer[2];
    Serial.println("Pos 2 loaded");
    neighbors.west3 = buffer[3];
    Serial.println("Pos 3 loaded");
    neighbors.west4 = buffer[4];
    Serial.println("Pos 4 loaded");
    neighbors.west5 = buffer[5];
    Serial.println("Pos 5 loaded");
    neighbors.west6 = buffer[6];
    Serial.println("Pos 6 loaded");
    neighbors.west7 = buffer[7];
    Serial.println("Pos 7 loaded");
    }
    
    if(preferences.isKey("nEAST")){
        Serial.println("East Found");
    preferences.getBytes("nEAST", buffer , preferences.getBytesLength("nEAST"));
    neighbors.east0 = buffer[0];
    Serial.println("Pos 0 loaded");
    neighbors.east1 = buffer[1];
    Serial.println("Pos 1 loaded");
    neighbors.east2 = buffer[2];
    Serial.println("Pos 2 loaded");
    neighbors.east3 = buffer[3];
    Serial.println("Pos 3 loaded");
    neighbors.east4 = buffer[4];
    Serial.println("Pos 4 loaded");
    neighbors.east5 = buffer[5];
    Serial.println("Pos 5 loaded");
    neighbors.east6 = buffer[6];
    Serial.println("Pos 6 loaded");
    neighbors.east7 = buffer[7];
    Serial.println("Pos 7 loaded");
    }
    
    if(preferences.isKey("nNORTH")){
        Serial.println("North Found");
    preferences.getBytes("nNORTH", buffer , preferences.getBytesLength("nNORTH"));
    neighbors.north0 = buffer[0];
    Serial.println("Pos 0 loaded");
    neighbors.north1 = buffer[1];
    Serial.println("Pos 1 loaded");
    neighbors.north2 = buffer[2];
    Serial.println("Pos 2 loaded");
    neighbors.north3 = buffer[3];
    Serial.println("Pos 3 loaded");
    neighbors.north4 = buffer[4];
    Serial.println("Pos 4 loaded");
    neighbors.north5 = buffer[5];
    Serial.println("Pos 5 loaded");
    neighbors.north6 = buffer[6];
    Serial.println("Pos 6 loaded");
    neighbors.north7 = buffer[7];
    Serial.println("Pos 7 loaded");
    }
    
    if(preferences.isKey("nSOUTH")){
        Serial.println("South Found");
    preferences.getBytes("nSOUTH", buffer , preferences.getBytesLength("nSOUTH"));
    neighbors.south0 = buffer[0];
    Serial.println("Pos 0 loaded");
    neighbors.south1 = buffer[1];
    Serial.println("Pos 1 loaded");
    neighbors.south2 = buffer[2];
    Serial.println("Pos 2 loaded");
    neighbors.south3 = buffer[3];
    Serial.println("Pos 3 loaded");
    neighbors.south4 = buffer[4];
    Serial.println("Pos 4 loaded");
    neighbors.south5 = buffer[5];
    Serial.println("Pos 5 loaded");
    neighbors.south6 = buffer[6];
    Serial.println("Pos 6 loaded");
    neighbors.south7 = buffer[7];
    Serial.println("Pos 7 loaded");
    }

    if(preferences.isKey("nFRONT")){
        Serial.println("Front Found");
    preferences.getBytes("nFRONT", buffer , preferences.getBytesLength("nFRONT"));
    neighbors.front0 = buffer[0];
    Serial.println("Pos 0 loaded");
    neighbors.front1 = buffer[1];
    Serial.println("Pos 1 loaded");
    neighbors.front2 = buffer[2];
    Serial.println("Pos 2 loaded");
    neighbors.front3 = buffer[3];
    Serial.println("Pos 3 loaded");
    neighbors.front4 = buffer[4];
    Serial.println("Pos 4 loaded");
    neighbors.front5 = buffer[5];
    Serial.println("Pos 5 loaded");
    neighbors.front6 = buffer[6];
    Serial.println("Pos 6 loaded");
    neighbors.front7 = buffer[7];
    Serial.println("Pos 7 loaded");
    }
    
    if(preferences.isKey("nBACK")){
        Serial.println("Back Found");
    preferences.getBytes("nBACK", buffer , preferences.getBytesLength("nBACK"));
    neighbors.back0 = buffer[0];
    Serial.println("Pos 0 loaded");
    neighbors.back1 = buffer[1];
    Serial.println("Pos 1 loaded");
    neighbors.back2 = buffer[2];
    Serial.println("Pos 2 loaded");
    neighbors.back3 = buffer[3];
    Serial.println("Pos 3 loaded");
    neighbors.back4 = buffer[4];
    Serial.println("Pos 4 loaded");
    neighbors.back5 = buffer[5];
    Serial.println("Pos 5 loaded");
    neighbors.back6 = buffer[6];
    Serial.println("Pos 6 loaded");
    neighbors.back7 = buffer[7];
    Serial.println("Pos 7 loaded");
    }
    Serial.println("Loaded all neighbors info");
    preferences.end();
    Serial.println("Closing namespace");
}

void clearNeighborsMem(){
    uint16_t bufferRead[8];
    uint16_t bufferWrite[8] = {0,0,0,0,0,0,0,0};

    preferences.begin("cube", false);

    preferences.getBytes("nWEST", bufferRead , preferences.getBytesLength("nWEST"));
    bool different = false;
    for(int i=0;i<8;i++){
        if(bufferRead[i] != bufferWrite[i]){
            different = true;
        }    
    }
    if(different){
        preferences.putBytes("nWEST", bufferWrite,sizeof(bufferWrite)); 
    }       
            
    preferences.getBytes("nEAST", bufferRead , preferences.getBytesLength("nEAST"));
    different = false;
    for(int i=0;i<8;i++){
        if(bufferRead[i] != bufferWrite[i]){
            different = true;
        }    
    }
    if(different){
        preferences.putBytes("nEAST", bufferWrite,sizeof(bufferWrite)); 
    }

    preferences.getBytes("nNORTH", bufferRead , preferences.getBytesLength("nNORTH"));
    different = false;
    for(int i=0;i<8;i++){
        if(bufferRead[i] != bufferWrite[i]){
            different = true;
        }    
    }
    if(different){
        preferences.putBytes("nNORTH", bufferWrite,sizeof(bufferWrite)); 
    }

    preferences.getBytes("nSOUTH", bufferRead , preferences.getBytesLength("nSOUTH"));
    different = false;
    for(int i=0;i<8;i++){
        if(bufferRead[i] != bufferWrite[i]){
            different = true;
        }    
    }
    if(different){
        preferences.putBytes("nSOUTH", bufferWrite,sizeof(bufferWrite)); 
    }

    preferences.getBytes("nFRONT", bufferRead , preferences.getBytesLength("nFRONT"));
    different = false;
    for(int i=0;i<8;i++){
        if(bufferRead[i] != bufferWrite[i]){
            different = true;
        }    
    }
    if(different){
        preferences.putBytes("nFRONT", bufferWrite,sizeof(bufferWrite)); 
    }
    
    preferences.getBytes("nBACK", bufferRead , preferences.getBytesLength("nBACK"));
    different = false;
    for(int i=0;i<8;i++){
        if(bufferRead[i] != bufferWrite[i]){
            different = true;
        }    
    }
    if(different){
        preferences.putBytes("nBACK", bufferWrite,sizeof(bufferWrite)); 
    }

    preferences.end();
}

/***************************************************************************************************
** SERVER ENDPOINT FUNCTION DEFINITIONS
***************************************************************************************************/

void createServerEndpoints(AsyncWebServer &server)
{

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Hi! I am a cube."); });

    // server.on("/start-nca", HTTP_GET, [](AsyncWebServerRequest *request)
    //           {

    //     runNCA = true;

    //     AsyncResponseStream *response = request->beginResponseStream("application/json");
    //     StaticJsonDocument<32> doc;

    //     doc["runNCA"] = runNCA;

    //     serializeJson(doc, *response);

    //     request->send(response);});

    // server.on("/reset-buffers", HTTP_GET, [](AsyncWebServerRequest *request)
    //           {

    //     resetBuffers = true;

    //     AsyncResponseStream *response = request->beginResponseStream("application/json");
    //     StaticJsonDocument<32> doc;

    //     doc["resetBuffers"] = resetBuffers;

    //     serializeJson(doc, *response);

    //     request->send(response);});

    server.on("/state_guess", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<256> doc;
        
        doc["macId"] = macId; 
        doc["mac0"] = macChunk0;
        doc["mac1"] = macChunk1;
        doc["mac2"] = macChunk2;
        doc["mac3"] = macChunk3;
        doc["mac4"] = macChunk4;
        doc["mac5"] = macChunk5;
        doc["mac6"] = macChunk6;
        doc["mac7"] = macChunk7;
        doc["state_guess"] = state_guess;
        doc["update_num"] = UPDATE_NUM;
        doc["fail_state"] = fail;
        doc["firmware"] = firmware_version;
        doc["damage_mode"] = damage_mode;
        doc["damage_class"] = damage_class;

        serializeJson(doc, *response);
        
        request->send(response); });

    server.on("/clear_damage", HTTP_GET, [](AsyncWebServerRequest *request)
    {
       
       Serial.println("Clearing damage mode");
       damage_mode = false;

       preferences.begin("cube", false);

       bool damage_mode_memory = preferences.getBool("damage_mode",false);
       if(damage_mode_memory){
            preferences.putBool("damage_mode", false);
        }

        preferences.end();

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<64> doc;

        doc["update_num"] = UPDATE_NUM;
        doc["fail_state"] = fail;
        doc["macId"] = macId;
        doc["damage_mode"] = damage_mode;

        serializeJson(doc, *response);

        request->send(response);


    });

    AsyncCallbackJsonWebHandler *handlerDamageMode = new AsyncCallbackJsonWebHandler("/damage_mode", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                {
        Serial.println("Receiving damage_mode command");
        Serial.println("damage class: ");

        JsonObject jsonObj = json.as<JsonObject>();
        damage_class = jsonObj["damage_class"].as<int8_t>();
        Serial.println(damage_class);

        damage_mode = true;
       
        preferences.begin("cube", false);
        
        bool damage_mode_memory = preferences.getBool("damage_mode",false);
        if(!damage_mode_memory){
            preferences.putBool("damage_mode", true);
        }

        int8_t damage_class_memory = preferences.getChar("damage_class",0);
        if(damage_class_memory != damage_class){
            preferences.putChar("damage_class", damage_class);
        }

        preferences.end();

        

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<32> doc;

        doc["damage_mode"] = damage_mode;
        doc["damage_class"] = damage_class;

        serializeJson(doc, *response);

        request->send(response); });

    server.addHandler(handlerDamageMode);

    server.on("/clear_fail", HTTP_GET, [](AsyncWebServerRequest *request)
              {

        fail = false;

        preferences.begin("cube", false);
        
        bool failFlagMemory = preferences.getBool("fail",false);
        if(failFlagMemory){
            preferences.putBool("fail", false);
        }

        preferences.end();

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<48> doc;

        doc["update_num"] = UPDATE_NUM;
        doc["fail_state"] = fail;
        doc["macId"] = macId;

        serializeJson(doc, *response);

        request->send(response);});

    server.on("/clear_get_neighbors", HTTP_GET, [](AsyncWebServerRequest *request)
              {

        if(getNeighbors){
        getNeighbors = false;

        preferences.begin("cube", false);
        

        bool neighborsFlagInMemory = preferences.getBool("neighbors", false);
        if(neighborsFlagInMemory){
            preferences.putBool("neighbors", false);
        }
        preferences.end();

        clearNeighborsMem();
        }

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<48> doc;

        doc["update_num"] = UPDATE_NUM;
        doc["fail_state"] = fail;
        doc["macId"] = macId;

        serializeJson(doc, *response);

        request->send(response); });

    AsyncCallbackJsonWebHandler *handlerMacFailSearch = new AsyncCallbackJsonWebHandler("/mac_search", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                {
        Serial.println("Receiving mac fail command");
        Serial.println("macs array: ");

        JsonObject jsonObj = json.as<JsonObject>();
        JsonArray array = jsonObj["macs"].as<JsonArray>();
        for(JsonVariant v : array) {
            Serial.println(v.as<String>());
            Serial.println(v.as<uint64_t>());
            if(v.as<uint64_t>() == macId){
                fail = true;
                
                preferences.begin("cube", false);
        
                bool failFlagMemory = preferences.getBool("fail",false);
                if(!failFlagMemory){
                    preferences.putBool("fail", true);
                }

                preferences.end();
            }
        }

        

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<32> doc;

        doc["update_num"] = UPDATE_NUM;
        doc["fail_state"] = fail;

        serializeJson(doc, *response);

        request->send(response); });

    server.addHandler(handlerMacFailSearch);

    server.on("/morphology", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      
    /* Serialize and send back as response */
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<1024> doc;
        
        doc["MyID"] = macId;
        doc["MyID0"] = macChunk0;
        doc["MyID1"] = macChunk1;
        doc["MyID2"] = macChunk2;
        doc["MyID3"] = macChunk3;
        doc["MyID4"] = macChunk4;
        doc["MyID5"] = macChunk5;
        doc["MyID6"] = macChunk6;
        doc["MyID7"] = macChunk7;


        doc["WEST0"] = neighbors.west0;
        doc["WEST1"] = neighbors.west1;
        doc["WEST2"] = neighbors.west2;
        doc["WEST3"] = neighbors.west3;
        doc["WEST4"] = neighbors.west4;
        doc["WEST5"] = neighbors.west5;
        doc["WEST6"] = neighbors.west6;
        doc["WEST7"] = neighbors.west7;

        doc["EAST0"] = neighbors.east0;
        doc["EAST1"] = neighbors.east1;
        doc["EAST2"] = neighbors.east2;
        doc["EAST3"] = neighbors.east3;
        doc["EAST4"] = neighbors.east4;
        doc["EAST5"] = neighbors.east5;
        doc["EAST6"] = neighbors.east6;
        doc["EAST7"] = neighbors.east7;

        doc["NORTH0"] = neighbors.north0;
        doc["NORTH1"] = neighbors.north1;
        doc["NORTH2"] = neighbors.north2;
        doc["NORTH3"] = neighbors.north3;
        doc["NORTH4"] = neighbors.north4;
        doc["NORTH5"] = neighbors.north5;
        doc["NORTH6"] = neighbors.north6;
        doc["NORTH7"] = neighbors.north7;

        doc["SOUTH0"] = neighbors.south0;
        doc["SOUTH1"] = neighbors.south1;
        doc["SOUTH2"] = neighbors.south2;
        doc["SOUTH3"] = neighbors.south3;
        doc["SOUTH4"] = neighbors.south4;
        doc["SOUTH5"] = neighbors.south5;
        doc["SOUTH6"] = neighbors.south6;
        doc["SOUTH7"] = neighbors.south7;

        doc["FRONT0"] = neighbors.front0;
        doc["FRONT1"] = neighbors.front1;
        doc["FRONT2"] = neighbors.front2;
        doc["FRONT3"] = neighbors.front3;
        doc["FRONT4"] = neighbors.front4;
        doc["FRONT5"] = neighbors.front5;
        doc["FRONT6"] = neighbors.front6;
        doc["FRONT7"] = neighbors.front7;

        doc["BACK0"] = neighbors.back0;
        doc["BACK1"] = neighbors.back1;
        doc["BACK2"] = neighbors.back2;
        doc["BACK3"] = neighbors.back3;
        doc["BACK4"] = neighbors.back4;
        doc["BACK5"] = neighbors.back5;
        doc["BACK6"] = neighbors.back6;
        doc["BACK7"] = neighbors.back7;

        serializeJson(doc, *response);
        
        request->send(response);
    

         Serial.println("Receiving");

        receiveMessage(&pwm_rx_data[EAST], "East");
        receiveMessage(&pwm_rx_data[WEST], "West");
        receiveMessage(&pwm_rx_data[NORTH], "North");
        receiveMessage(&pwm_rx_data[SOUTH], "South");
        receiveMessage(&pwm_rx_data[FRONT], "Front");
        receiveMessage(&pwm_rx_data[BACK], "Back");

        // Serial.println("Unpacking");

        // Serial.println("Saving");

        if (pwm_rx_data[WEST].complete())
        {
            neighbors.west0 = pwm_rx_data[WEST].message[0];
            neighbors.west1 = pwm_rx_data[WEST].message[1];
            neighbors.west2 = pwm_rx_data[WEST].message[2];
            neighbors.west3 = pwm_rx_data[WEST].message[3];
            neighbors.west4 = pwm_rx_data[WEST].message[4];
            neighbors.west5 = pwm_rx_data[WEST].message[5];
            neighbors.west6 = pwm_rx_data[WEST].message[6];
            neighbors.west7 = pwm_rx_data[WEST].message[7];
        }
        if (pwm_rx_data[EAST].complete())
        {
            neighbors.east0 = pwm_rx_data[EAST].message[0];
            neighbors.east1 = pwm_rx_data[EAST].message[1];
            neighbors.east2 = pwm_rx_data[EAST].message[2];
            neighbors.east3 = pwm_rx_data[EAST].message[3];
            neighbors.east4 = pwm_rx_data[EAST].message[4];
            neighbors.east5 = pwm_rx_data[EAST].message[5];
            neighbors.east6 = pwm_rx_data[EAST].message[6];
            neighbors.east7 = pwm_rx_data[EAST].message[7];
        }
        if (pwm_rx_data[NORTH].complete())
        {
            neighbors.north0 = pwm_rx_data[NORTH].message[0];
            neighbors.north1 = pwm_rx_data[NORTH].message[1];
            neighbors.north2 = pwm_rx_data[NORTH].message[2];
            neighbors.north3 = pwm_rx_data[NORTH].message[3];
            neighbors.north4 = pwm_rx_data[NORTH].message[4];
            neighbors.north5 = pwm_rx_data[NORTH].message[5];
            neighbors.north6 = pwm_rx_data[NORTH].message[6];
            neighbors.north7 = pwm_rx_data[NORTH].message[7];
        }
        if (pwm_rx_data[SOUTH].complete())
        {
            neighbors.south0 = pwm_rx_data[SOUTH].message[0];
            neighbors.south1 = pwm_rx_data[SOUTH].message[1];
            neighbors.south2 = pwm_rx_data[SOUTH].message[2];
            neighbors.south3 = pwm_rx_data[SOUTH].message[3];
            neighbors.south4 = pwm_rx_data[SOUTH].message[4];
            neighbors.south5 = pwm_rx_data[SOUTH].message[5];
            neighbors.south6 = pwm_rx_data[SOUTH].message[6];
            neighbors.south7 = pwm_rx_data[SOUTH].message[7];
        }
        if (pwm_rx_data[FRONT].complete())
        {
            neighbors.front0 = pwm_rx_data[FRONT].message[0];
            neighbors.front1 = pwm_rx_data[FRONT].message[1];
            neighbors.front2 = pwm_rx_data[FRONT].message[2];
            neighbors.front3 = pwm_rx_data[FRONT].message[3];
            neighbors.front4 = pwm_rx_data[FRONT].message[4];
            neighbors.front5 = pwm_rx_data[FRONT].message[5];
            neighbors.front6 = pwm_rx_data[FRONT].message[6];
            neighbors.front7 = pwm_rx_data[FRONT].message[7];
        }
        if (pwm_rx_data[BACK].complete())
        {
            neighbors.back0 = pwm_rx_data[BACK].message[0];
            neighbors.back1 = pwm_rx_data[BACK].message[1];
            neighbors.back2 = pwm_rx_data[BACK].message[2];
            neighbors.back3 = pwm_rx_data[BACK].message[3];
            neighbors.back4 = pwm_rx_data[BACK].message[4];
            neighbors.back5 = pwm_rx_data[BACK].message[5];
            neighbors.back6 = pwm_rx_data[BACK].message[6];
            neighbors.back7 = pwm_rx_data[BACK].message[7];
        }
        
        for (int i = 0; i < NUM_DIRECTIONS; i++)
        {
            pwm_rx_data[i].reset();
        }
        
        });
}