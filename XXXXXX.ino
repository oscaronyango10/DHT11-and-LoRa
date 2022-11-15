#include <lmic.h>
#include <hal/hal.h>
#include <DHT.h>


#define DHTTYPE    DHT11     // DHT 22 (AM2302)
#define DHTPIN 5

DHT dht(DHTPIN, DHTTYPE);


// LoRaWAN NwkSKey, network session **** DO NOT USE AS IS. NEEDS TO BE CHANGED AS PER TUTORIAL ******
static const PROGMEM u1_t NWKSKEY[16] = { 0xD9, 0x7D, 0x42, 0x7A, 0x10, 0x5A, 0xF6, 0x03, 0x7C, 0x3E, 0x27, 0x77, 0x18, 0x8F, 0x6D, 0xDD };

// LoRaWAN AppSKey, application session key **** DO NOT USE AS IS. NEEDS TO BE CHANGED AS PER TUTORIAL ******
static const u1_t PROGMEM APPSKEY[16] = { 0x1F, 0x96, 0x43, 0x20, 0xFA, 0x32, 0x4E, 0x6C, 0xAC, 0xF6, 0x29, 0xB2, 0x7B, 0x00, 0x79, 0xC1 };

// LoRaWAN end-device address (DevAddr) **** DO NOT USE AS IS. NEEDS TO BE CHANGED AS PER TUTORIAL ******
static const u4_t DEVADDR = { 0x260BC634 };

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

// Schedule data trasmission in every this many seconds (might become longer due to duty
// cycle limitations).
// we set 10 seconds interval
const unsigned TX_INTERVAL = 10;

// Pin mapping according to Cytron LoRa Shield RFM
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 4,
  .dio = {2, 3, LMIC_UNUSED_PIN},
};

void onEvent (ev_t ev) {

  switch(ev) {
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
      break;  
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j){
  int Temperature;
  int Humidity;
  
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
  // Get Temperature
  Temperature=int(dht.readTemperature()*10);
  // Get humidity 
  Humidity=int(dht.readHumidity()*10);

    // prepare and schedule data for transmission 
    LMIC.frame[0] = Temperature >> 8; 
    LMIC.frame[1] = Temperature;
    LMIC.frame[2] = Humidity >> 8;
    LMIC.frame[3] = Humidity; 
    LMIC_setTxData2(1, LMIC.frame, 4, 0); // (port 1, 4 bytes, unconfirmed)

    Serial.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting"));

  // Initialize Temp sensor device.
  dht.begin();

  // LMIC init
  os_init();

  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // Disable ADR
  LMIC_setAdrMode(false);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7,14);

  // Start job
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();

}
