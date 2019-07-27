#include <avr/sleep.h>
#include <avr/power.h>
#include <EEPROM.h>

#define goto_menu Page_show = Page_menu; Timer_active = false; Alert_active = false; AP_running = false; mel_play(mel_done); checkWarning;
#define checkWarning if(Batt_warning){Battery_warning_show = 5;}
#define casovac_ap ((Timer_auto_default+1)-(((millis()-AP_start)/1000)+1))
#define now_active AO_lastActivity = millis(); Serial.print(".")

#include <Adafruit_NeoPixel.h>
#define pin_led_data  8 // LED -data
Adafruit_NeoPixel strip(2, pin_led_data, NEO_GRB + NEO_KHZ800);

#include "U8glib.h"
U8GLIB_SSD1306_128X64 display(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_FAST);
#define sfa display.setFont(u8g_font_unifontr); display.drawStr
#define sfb display.setFont(u8g_font_ncenB24r); display.drawStr
//U8GLIB_SSD1306_128X64 display(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);  // I2C / TWI
//U8GLIB_SSD1306_128X64 display(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_NO_ACK|U8G_I2C_OPT_FAST); // Fast I2C / TWI
//U8GLIB_SSD1306_128X64 display(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);  // I2C / TWI
//U8GLIB_SSD1306_128X64 display(U8G_I2C_OPT_NO_ACK);  // Display which does not send AC

#define Timer_val_default  180 // odpočet 3 minuty // value_default=180
#define Timer_auto_default 120 // první dvě minuty // value_default=120
#define Auto_timer_steps    30 // jednotlivé kroky // value_default=30

#define at_menu Page_show == Page_menu
#define at_auto Page_show == Page_auto
#define at_timr Page_show == Page_timr
#define at_exit Page_show == Page_exit
#define at_alrt Page_show == Page_alrt
#define at_blnk Page_show == Page_blnk
#define at_welc Page_show == Page_welc
#define at_sett Page_show == Page_sett

#define pin_led_powr 7 // LED -power
#define pin_piezo    6 // Piezo

#define bA_pin 4 // Button A pin
#define bB_pin 3 // Button B pin
#define bX_pin 2 // Button Back pin

#define Page_welc 1
#define Page_timr 2
#define Page_alrt 3
#define Page_menu 4
#define Page_exit 5
#define Page_auto 6
#define Page_blnk 7
#define Page_sett 8

#define Sett_page_mn 0
#define Sett_page_bt 1
#define Sett_page_ao 2
#define Sett_page_vl 3

#define Prom_aotimeout 50
#define Prom_battlow   51

#define mel_welc 1
#define mel_hihi 2
#define mel_nope 3
#define mel_done 4
#define mel_byby 5
#define mel_auto 6


bool
    AP_running, Batt_warning, LED_enable,
    Timer_active, Alert_active, Timer_alert_active;

uint8_t
    Batt_percent, Battery_warning_show, Batt_low,
    _oldAD, LED_saturation, LED_intensity,
    Sett_page_now, Sett_page_count = 3, AO_timeout,
    AP_step, AP_step_last, Page_show = Page_exit;

uint16_t
    LED_hue, VCC_lastUpdated, VCC_lastValue, Batt_mv, Batt_v;

int32_t
    Alert_counting, Alert_ring_last, AO_lastActivity,
    AP_start, AP_pause_start, Batt_last,
    Batt_warning_last,
    Alert_duration = 30, Alert_every = 1000,
    Timer_seconds, Timer_minutes, sett_start,
    Timer_remain, Timer_value, Timer_start;


void draw() {

         if (at_welc) { sfb(00, 50, "Welcome");
                        if (millis() > 1500) {goto_menu; } }

    else if (at_exit) { // message welcome/exitting
                        if (millis() < 1000) {sfb(00, 50, "Welcome"); }
                        else                 {sfb(00, 50, "Exiting"); }
                      }

    else if (at_alrt) { sfb(00, 50, "Alertin"); }

    else if (at_blnk) { }

    else if (at_timr) { casomira(32, 44, Timer_remain); }

    else if (at_menu) { sfb(00, 32, "A  3min"); display.drawLine(33, 5, 33, 64);
                                        sfb( 0, 64, "B  Auto"); display.drawLine(0, 35, 128, 35); }

    else if (at_auto) { switch (AP_step) {
            case 13:   sfa(48, 30, "Brou noc");   sfb(15, 50, "Konec");  break;
            case  1:   sfa(57, 37, "na stisk");   sfb(05, 55, "Cekam");  break;
            case  2: sfb(55, 50, "1"); break; case  3: sfb(55, 50, "2"); break;
            case  4: sfb(55, 50, "3"); break; case  5: sfb(55, 50, "4"); break;
            case  6: sfb(55, 50, "5"); break; case  7: sfb(55, 50, "6"); break;
            case  8: sfb(55, 50, "5"); break; case  9: sfb(55, 50, "4"); break;
            case 10: sfb(55, 50, "3"); break; case 11: sfb(55, 50, "2"); break;
            case 12: sfb(55, 50, "1"); break; case  0: casomira(32, 44, casovac_ap); break; }}

    else if (at_sett) { switch (Sett_page_now) {
            case Sett_page_mn:
                sfa(30, 12, "Svetluska :)");
                if(!LED_enable or LED_intensity < 250) {sfa(30, 30, "Rozsvitit ->"); }
                else {sfa(30, 30, "Barvicky -->"); }
                sfa(30, 62, "Nastaveni ->");
            break;
            case Sett_page_ao:
                char _secOF[4]; itoa(AO_timeout, _secOF, 10);
                sfa(02, 12, "Automaticky");
                sfa(02, 24, "vypnout po:");

                if(AO_timeout) {sfb(32, 60, _secOF); }
                else {sfb(24, 60, "Off"); }
            break;
            case Sett_page_vl:
                char _low[4]; itoa(Batt_low, _low, 10);
                sfa(02, 12, "Baterry LOW");
                sfa(02, 24, "warning: [%]");
                if (!Batt_low){ sfb(02, 60, "Nikdy"); }
                else if (Batt_low > 50){ sfb(10, 60, "Furt"); }
                else {sfb(24, 60, _low); }
            break;
            case Sett_page_bt:
                
                char _volts[4]; itoa(readVcc(), _volts, 10);
                char _percent[4]; itoa(Batt_percent, _percent, 10);

                sfa(02, 12, "Batt:"); sfa(50, 10, _volts); sfa(80, 10, "[mV]"); 
                sfb(24, 60, _percent); sfb(63, 60, "%");
            break;
            case 4: sfa(02, 12, "d"); break;
            case 5: sfa(02, 12, "e"); break;
            case 6: sfa(02, 12, "f"); break;
        }}
}

void casomira(byte _x, byte _y, int _time) {

        int _minutes = (  (  _time  )  /  60   );
        int _seconds = ((_time) - (_minutes*60));

        char _mins[4]; itoa(_minutes, _mins, 10);
        char _secs[4]; itoa(_seconds, _secs, 10);

        if (_minutes<10){ sfb( _x- 0, _y, _mins); }
        else            { sfb( _x-21, _y, _mins); }
                          sfb( _x+23, _y-3, ":");
        if (_seconds<10){ sfb( _x+37, _y,   "0");
                          sfb( _x+56, _y, _secs); }
        else            { sfb( _x+37, _y, _secs); }
}

void apHandler() {
    if (at_auto) {

        if (AP_running and AP_step == 0 and millis()/1000 - AP_start/1000 > (Timer_auto_default)) {
            AP_running = false; AP_step = 1; mel_play(mel_auto);
        }
        if (AP_step > 1) { byte _secs = Auto_timer_steps;

            int _ap_dur = (millis() - AP_start) / 1000;
            if (_secs)

            if      (_ap_dur <  1*_secs+1) {AP_step =  2; } else if (_ap_dur <  2*_secs+1) {AP_step =  3; }
            else if (_ap_dur <  3*_secs+1) {AP_step =  4; } else if (_ap_dur <  4*_secs+1) {AP_step =  5; }
            else if (_ap_dur <  5*_secs+1) {AP_step =  6; } else if (_ap_dur <  6*_secs+1) {AP_step =  7; }
            else if (_ap_dur <  7*_secs+1) {AP_step =  8; } else if (_ap_dur <  8*_secs+1) {AP_step =  9; }
            else if (_ap_dur <  9*_secs+1) {AP_step = 10; } else if (_ap_dur < 10*_secs+1) {AP_step = 11; }
            else if (_ap_dur < 11*_secs+1) {AP_step = 12; } else if (_ap_dur < 11*_secs+5) {AP_step = 13; }
            else {Page_show = Page_exit; }

            if (AP_step != AP_step_last) {
                AP_step_last = AP_step;
                if (AP_step == 13) tone(pin_piezo, 5000, 500); // ton na konec
                else tone(pin_piezo, 6000, 100); // ton po 30sec
                now_active;
}   }   }   }

void prepareTime() {

    if (at_timr) {

        Timer_remain = Timer_value - ( (millis()/1000) - Timer_start );
        if (!Timer_active) Timer_remain = Timer_value;
        if (Timer_active and Timer_remain < 0) {
            Page_show = Page_alrt;
            Timer_active = false;
            Alert_active = true;
            now_active;
        }

        Timer_minutes = (Timer_remain) / 60;
        Timer_seconds = (Timer_remain) - (Timer_minutes*60);
}   }

void alertHandling() {
    if (at_alrt) {
        if (Alert_active and millis() - Alert_ring_last > Alert_every) {
            Alert_ring_last = millis();
            if (Alert_counting < 50) Alert_counting++;
            Alert_every    = map(Alert_counting, 0, 50, 900, 200);
            Alert_duration = map(Alert_counting, 0, 50,  10, 150);
            tone(pin_piezo, 4000-Alert_counting*20, Alert_duration);
        }
    } else Alert_counting = 1;
}

bool flag;
bool ledSpeed;

#define bA_now (!digitalRead(bA_pin))
#define bX_now (!digitalRead(bX_pin))
#define bB_now (!digitalRead(bB_pin))

void buttonWatch() {

    if (bA_now or bX_now or bB_now) {
        now_active;  // activity loged
        if (!flag) delay(40);  // debounce
    }

    if (!flag and bA_now and !bX_now and !bB_now) { flag = true;

        if      (at_menu) {start_timer(Timer_val_default); mel_play(mel_done); checkWarning; }
        else if (at_alrt) {goto_menu; }
        else if (at_auto and AP_step == 1) {AP_step = 2; AP_start = millis(); mel_play(mel_done); return; }
        else if (at_sett) {
            switch (Sett_page_now) {
                case Sett_page_ao:
                    if(AO_timeout < 60) {
                        if(AO_timeout < 10) {AO_timeout += 5; }
                        else {AO_timeout += 10; }
                    } else {AO_timeout = 0; }
                    EEPROM.write(Prom_aotimeout, AO_timeout);
                break;
                case Sett_page_vl:
                    if(Batt_low < 50) {
                        if(Batt_low < 20) {Batt_low += 5; }
                        else {Batt_low += 10; }
                    } else if (Batt_low == 50){Batt_low++; }
                    else {Batt_low = 0;}
                    EEPROM.write(Prom_battlow, Batt_low);
                break;
                case Sett_page_mn:
                    //ledSpeed = !ledSpeed;
                    LED_enable = true;
                break;
                case Sett_page_bt:
                    //ledSpeed = !ledSpeed;
                    LED_enable = true; LED_saturation = 0; LED_intensity = 255;
                break;
            }
        }
        else    {mel_play(mel_nope); }
    }
    if (bA_now and at_sett and Sett_page_now == Sett_page_mn) {
        LED_enable = true;
        if(ledSpeed)LED_hue += 60;
        else LED_hue += 25;
        if (LED_saturation < 255) LED_saturation++;
    }



    if (!flag and !bA_now and bX_now and !bB_now) { flag = true;

        if      (at_menu or at_alrt)       {Page_show = Page_exit; return; }
        else if (at_auto and AP_step == 1) {AP_step = 2; AP_start = millis(); return; }
        else if (at_sett) {Page_show = Page_exit; LED_enable = false; return; }
        goto_menu;
    }



    if (!flag and !bA_now and !bX_now and bB_now) { flag = true;

        if      (at_menu)                  {start_ap(); mel_play(mel_done); checkWarning; }
        else if (at_auto and AP_step == 1) {AP_step = 2; AP_start = millis(); return; }
        else if (at_alrt)                  {goto_menu; }
        else if (at_sett) {
            LED_enable = false;
            if(Sett_page_now < Sett_page_count){
                Sett_page_now++;
            }
            else Sett_page_now = 0;
        }
        else {mel_play(mel_nope); }
    }

    if(!bA_now and !bX_now and !bB_now){flag = false; }
}

void mel_play(byte _type) {
    now_active;
    pinMode(pin_piezo, OUTPUT);
    switch (_type) {
        case mel_welc:
            tone(pin_piezo, 4000, 100); delay(110);
            tone(pin_piezo, 5000, 100); delay(110);
            tone(pin_piezo, 6000, 100); delay(110);
        break;
        case mel_hihi:
            tone(pin_piezo, 4000, 100); delay(110);
            tone(pin_piezo, 6000, 100); delay(110);
        break;
        case mel_done:
            tone(pin_piezo, 5000,  20); delay( 20);
        break;
        case mel_nope:
            tone(pin_piezo, 700,   20); delay( 90);
            tone(pin_piezo, 800,   20); delay( 90);
            tone(pin_piezo, 600,   20); delay( 90);
        break;
        case mel_byby:
            tone(pin_piezo, 6000, 100); delay(110);
            //tone(pin_piezo, 5000, 100); delay(110);
            tone(pin_piezo, 4000, 100); delay(110);
        break;
        case mel_auto:
            tone(pin_piezo, 7000, 500); delay(500);
        break;
    }
    pinMode(pin_piezo, INPUT);
}


void setup() {

    pinMode(2, INPUT_PULLUP);
    pinMode(3, INPUT_PULLUP);
    pinMode(4, INPUT_PULLUP);

    pinMode(pin_led_powr, OUTPUT); digitalWrite(pin_led_powr, HIGH);

    Serial.begin(250000); Serial.print("start");

    display.setColorIndex(1); // Pixels ON
    //display.setRot180();

    if (EEPROM.read(Prom_aotimeout) > 60){AO_timeout = 0; }
    else {AO_timeout = EEPROM.read(Prom_aotimeout); }
    if (EEPROM.read(Prom_battlow) > 50){Batt_low = 51; }
    else {Batt_low = EEPROM.read(Prom_battlow); }

    strip.clear(); now_active;
}
void loop() {
    display.firstPage();
    do {
        rutine();
        if(Battery_warning_show) {
            sfb(02, 50, "DOBIJ!");
        } else draw();
    }
    while(display.nextPage());
    if(Battery_warning_show){
        delay(100);
        Battery_warning_show--;
    }

    exitHandler();
    timeout();
    rutine();
}
void rutine() {
    buttonWatch();
    prepareTime();
    apHandler();
    alertHandling();
    baterryWatch();
    ledHandler();
}
void ledHandler() {
    if(LED_enable) {
        pinMode(pin_led_powr, OUTPUT); digitalWrite(pin_led_powr, HIGH);
        if (LED_intensity < 255) {LED_intensity++; delay(map(LED_intensity, 0, 255, 25, 0)); }
        uint32_t rgbcolor = strip.ColorHSV(LED_hue, LED_saturation, LED_intensity);
        strip.fill(rgbcolor); strip.show();
    } else {
        if (LED_intensity) {
            LED_intensity--; delay(map(LED_intensity, 0, 255, 10, 0));
            uint32_t rgbcolor = strip.ColorHSV(LED_hue, LED_saturation, LED_intensity);
            strip.fill(rgbcolor); strip.show();
    }
        else {pinMode(7,  INPUT); digitalWrite(pin_led_powr,  LOW); LED_intensity = 0; }
    }
}
bool wakeupPin;
void exitHandler() {

    if (at_exit) {
        
        long _exitingStarted = millis();
        if (millis() < 1000) {mel_play(mel_welc); }
        else {mel_play(mel_byby); }
        display.firstPage(); do {draw(); } while(display.nextPage());
        while (LED_intensity) {
            LED_intensity--; delay(map(LED_intensity, 0, 255, 10, 0));
            uint32_t rgbcolor = strip.ColorHSV(LED_hue, LED_saturation, LED_intensity);
            strip.fill(rgbcolor); strip.show();
        }
        while(millis() - _exitingStarted < 300);
        pinMode(7,  INPUT); digitalWrite(pin_led_powr,  LOW); LED_intensity = 0;
        delay(100); Page_show = Page_blnk; display.firstPage(); do {draw(); } while(display.nextPage());

        display.sleepOn(); sleep_enable(); // wait, sleep display
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        _oldAD = ADCSRA;
        ADCSRA = 0; // set sleep mode, disable ADC
        MCUCR = bit (BODS) | bit (BODSE); MCUCR = bit (BODS); // vypne brown-out detection
        attachInterrupt(0, wakeUp, LOW); attachInterrupt(1, wakeUpPlay, LOW); 
        sleep_cpu(); // power_all_disable(); //!todo zkusit taky tohle

        // --------------------------------
        // waiting for wakeUp button
        // --------------------------------

        sleep_disable(); detachInterrupt(0); detachInterrupt(1);
        ADCSRA = _oldAD; display.sleepOff(); mel_play(mel_hihi);
        flag = true; delay(40);
        if(wakeupPin) {
            unsigned long b_button_pressed = millis();
            while(bX_now) {if(millis() - b_button_pressed > 400) {
                LED_hue = 0; LED_saturation = 0;
                Page_show = Page_sett; Sett_page_now = 0; LED_enable = true; return;
            }}
            goto_menu;
        } else {start_ap(); checkWarning; }
        
    }
}
void wakeUp() {
    wakeupPin = true; sleep_disable();
}
void wakeUpPlay() {
    wakeupPin = false; sleep_disable();
}
void timeout() {
    
    if (!AO_timeout) return;
    uint32_t _tout = AO_timeout * 1000;

    if      (Page_show == Page_auto) {_tout = 300000; }
    else if      (Alert_active)      {_tout = 120000; }
    else if       (LED_enable)       {_tout = 180000; }

    Serial.print("\n_tout=");
    Serial.print(_tout/1000);
    Serial.print("\t");
    Serial.print(millis() - AO_lastActivity);

    if(millis() - AO_lastActivity > _tout) {
        Serial.print(" -> turning off!");
        if (AP_running or Timer_active) {return; }
        goto_menu; Page_show = Page_exit;
    }
}


void start_timer(int _val) {Page_show=Page_timr, Timer_active=true; Timer_value=_val; Timer_start=millis()/1000;}
void start_ap() {Page_show = Page_auto; AP_step = 0; AP_running = true; AP_start = millis(); }
long readVcc() {

    if(millis() - VCC_lastUpdated > 300) {
        VCC_lastUpdated = millis();

        #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
        ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
        #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
        ADMUX = _BV(MUX5) | _BV(MUX0);
        #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
        ADMUX = _BV(MUX3) | _BV(MUX2);
        #else
        ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
        #endif  

        delay(5); // Wait for Vref to settle
        ADCSRA |= _BV(ADSC); // Start conversion
        while (bit_is_set(ADCSRA,ADSC)); // measuring

        uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
        uint8_t high = ADCH; // unlocks both

        long result = (high<<8) | low;

        result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000

        VCC_lastValue = result;
    }
    return VCC_lastValue;
}
void baterryWatch() {
    //----------------------------
    //     Whem ||  min ||  max  |
    //----------------------------
    //  baterry || 3572 || 4000  |
    // charging || 4160 || 4600  |
    //----------------------------
    
    if (millis() - Batt_last > 1000) {
        Batt_last = millis();
        Batt_mv = readVcc();
        Batt_v  = Batt_mv / 1000;
        
        if(Batt_mv < 3572) {
            // baterry is discharged
            Batt_mv = 3572;
        }

        if (Batt_mv < 4200) {
            // baterry is discharging
        }

        if (Batt_mv > 4000) {
            Batt_mv = 4000;
        }

        Batt_percent = map(Batt_mv, 3572, 4000, 0, 99);
        Batt_warning = Batt_percent < Batt_low;
        if (Batt_low > 50) Batt_warning = true;
    }
}