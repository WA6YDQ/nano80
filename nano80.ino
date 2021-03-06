/*
 * This is an 8080 simulator specifically for the
 * Arduino Nano. It uses an I2C 32K byte FRAM chip for main memory,
 * a front panel to program memory includinga 1x8 LCD, 8 toggle switches,
 * and 6 momentary buttons. The memory (non-volatile) is from 0000h thru 7fffh.
 * There is boot loader code to remotely program memory.
 * This makes a very good 8080 software development tool and play toy.
 * 
 * (C) k theis 11/2019 MIT license applies.
 * 
 * version 1.05  11/30/2019 DAA passes. All op codes passed the diag.asm check.
 * version 1.04e 11/29/2019 All instructions passed op code checker xcpt DAA.
 * version 1.04d 11/26/2019 Set parity flag (P) based on parity test, fix RLC,RRC,RAL
 * version 1.04c 11/26/2019 Change docs, removed unused vars. No functional changes.
 * version 1.04b 11/25/2019 fix things in debug() 
 * version 1.04a 11/25/2019 SBB: fix it right this time
 * version 1.04  11/25/2019 SBB: a=a-n-C change, added intel HEX bootloader. Uses custom backend
 * uploader w/20msec delays between HEX lines. Needed for FRAM.
 * version 1.03  11/22/2019 removed all bootloader code. fram access is too slow for it
 * version 1.02c 11/21/2019 fix issue with conditional return, shorten code
 * version 1.02b 11/21/2019 max baud 1200, fixed boot loader & serial read()
 * version 1.02a 11/20/2019 fixed bootloader byte sequence
 * version 1.02 11/20/2019 wrote inline bootloader, load/reset starts it
 * version 1.01 11/20/2019 minor change to reset()
 * version 1.0  11/18/2019 released to wild
 */

#include <string.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include "Adafruit_FRAM_I2C.h"

#define MAXMEM 32768     // maximum RAM size
#define SERIALSPEED 9600 // Serial.write works till the buffer is filled.
                         // Because of slowwwww FRAM memory access, don't expect much. 
#define DEBUG 0          // 0: no debugging serial output (except at halt), 
                         // 1: show address and opcode on serial port while running

#define HALTLED 10       // D10 Halt LED
#define RUNLED 13        // D13 Run LED
#define RUNHALT 11       // D11 runhalt switch run=0, halt=1 (toggle switch)
#define RESET A1         // D13 reset switch (momentary switch)
#define LDAH A0          // load address high (momentary switch)
#define LDAL 12          // load address lo (momentary switch)
#define STEP 3           // increment address, show data (momentary switch)
#define LOAD 2           // load data into memory, step address (momentary switch)
#define STEPMINUS A2     // decrement address, show data (momentary switch)

#define e 9              // LCD display (1x8) control lines
#define rs 8
#define d7 7
#define d6 6
#define d5 5
#define d4 4

#define DEBOUNCE 80     // push button switch debounce time

uint16_t    PC, A, BC, DE, HL, StackP, temp, carry, hi, lo;
uint8_t     OP;
bool        C, Z, P, AC, S, INTE;


LiquidCrystal lcd(rs,e,d4,d5,d6,d7);
Adafruit_FRAM_I2C fram = Adafruit_FRAM_I2C();

/* send debugging info to the serial port */
void debug() {
    char crnt[strlen("PC:%4.4X  OP:%2.2X")+1];
    sprintf(crnt,"PC:%4.4X  OP:%2.2X",PC,OP);
    Serial.println(crnt);
    char regs[strlen("A:%2.2X  BC:%4.4X  DE:%4.4X  HL:%4.4X  SP:%4.4X")+1];
    sprintf(regs,"A:%2.2X  BC:%4.4X  DE:%4.4X  HL:%4.4X  SP:%4.4X",A,BC,DE,HL,StackP);
    Serial.println(regs);
    char flags[strlen("Z:%d  C:%d  S:%d  AC:%d  P:%d")+1];
    sprintf(flags,"Z:%d  C:%d  S:%d  AC:%d  P:%d",Z,C,S,AC,P);
    Serial.println(flags);
    Serial.println();
    return;
}

/* clear FRAM, then load the boot loader in hi memory */
void clrmem() {     // called from reset()
    uint16_t adr;
    uint8_t dat;
    lcd.home();
    lcd.print("Clr Mem ");
    /* first clear FRAM */
    for (adr=0; adr < 32768; adr++) 
        fram.write8(adr,0);
    lcd.clear();
    return;
}

/* read intel .hex records from the serial port  
 *  This needs a custom uploader (see upload.c in repository) to properly
 *  deal with the FRAM wait states. The uploader sends an intel HEX file
 *  line by line with a 20msec delay at the end of each line. This allows 
 *  time to send the data to the FRAM memory as well as dealing with the 
 *  limited ram buffer for the serial port.
 */
void bootload () {

    int cnt, bytenum, addr, recordtype, dat, cd = 0;
    uint8_t record[80];
    uint8_t tdat[4] = {0,0,0,0};
    lcd.home();
    lcd.print("booting ");
    /* main read & store loop */

    while (Serial.available() == 0) continue;   // wait for data  
dlloop:
    for (cnt=0;cnt<79;cnt++) record[cnt]=0;
    cd = 0;
    cnt = 0;
    if (Serial.available()==0) goto dlloop;
    while (1) {
        dat = Serial.read();
        if (dat == -1) continue;    // wait till data arrives
        if (dat == 0x3a) continue;  // start char
        if (dat == '\n') break;
        tdat[cd]=dat & 0xff;
        cd++;
        if (cd<2) continue;         // combine bytes
        cd = 0;
         if (tdat[0] > 0x39) tdat[0]-=7;        // convert from ascii
         tdat[0]-=0x30;
         if (tdat[1] > 0x39) tdat[1]-=7;
         tdat[1]-=0x30;
        record[cnt] = ((16*tdat[0]) + tdat[1]);
        output(record[cnt],0);      // display byte on LED's (eye candy)
        if (cnt==0) bytenum = record[0];    // 0 because we skip the ':'
        /* count=0  address=1-2   record type=3  bytenum+1=checksum */
        if (cnt==3) recordtype = record[3];
        cnt++;
    }
    if (recordtype == 0) {      // load data from DATA record type
        // now store the record
        addr = (256*record[1])+record[2];
        for (cd=0; cd<bytenum;cd++) {
            fram.write8(addr,record[cd+4]);
            addr++;
        }  
        goto dlloop;
    }
     // ignore record type other than 0 for now (will need for 8086 etc)

    output(0,0);        // clear the LED's
    lcd.home();
    lcd.print(addr,HEX);
    lcd.print("        ");
    delay(1000);
    lcd.home();
    lcd.print("Done    ");
    delay(1000);
    
    lcd.clear();
    return;
}


/* reset the processor */
void reset(void) {
    digitalWrite(RUNLED,0);     // turn OFF run led

    if (digitalRead(STEPMINUS) == 0) {  // press STEPMINUS and reset will
        clrmem();                       // clear memory (FRAM)
        while (digitalRead(STEPMINUS) == 0) { 
            delay(DEBOUNCE);
        }
        delay(DEBOUNCE);
    }

    if (digitalRead(LOAD) == 0) {
        bootload();
        while (digitalRead(LOAD) == 0) {
            delay(DEBOUNCE);
        }
        delay(DEBOUNCE);
    }


    // normal reset
    PC = A = BC = DE = HL = StackP = 0; // The real 8080 doesn't clear these
    Z = C = S = AC = P = INTE = 0;      // on reset. Remove this if you want.
                                        // (The Z80 DOES set some to FFFFh, ie. SP)

    digitalWrite(HALTLED, 0);   // turn OFF HALT led
    while (digitalRead(RESET) == 0) {
        delay(DEBOUNCE);
    }
    delay(DEBOUNCE);
    return;
}


/* show address/data (based on PC/OP) on a 1x8 LCD */
void updatelcd() {
    char lcd_addr[5];
    char lcd_data[3];
    lcd.clear();
    sprintf(lcd_addr,"%4.4X",PC);
    if (PC >= MAXMEM)   // account for missing memory
            OP=0x00;
        else
            OP = fram.read8(PC);
    sprintf(lcd_data,"%2.2X",OP);
    lcd.setCursor(0,0);
    lcd.print(lcd_addr);
    lcd.setCursor(6,0);
    lcd.print(lcd_data);
    return;
}

byte readSwitchPort() {
    byte val;
    Wire.beginTransmission(0x24); // address of input port
        Wire.write(0x09);
        Wire.endTransmission();
        Wire.requestFrom(0x24,1);
        while (Wire.available()) {            
            val = Wire.read();
        }
    return val;
}

void frontpanel() {
    uint16_t ADR;
    byte DAT;
    digitalWrite(RUNLED,0);     // turn off run led
    updatelcd();
    while (digitalRead(RUNHALT)==1) {   // loop while in stop mode
        if (digitalRead(RESET) == 0) {   // reset pressed?
            reset();
            return;
        }
        if (digitalRead(LDAH)==0) {     // Load Address high
            ADR = readSwitchPort();
            PC = (PC &0xFF) + (ADR*256);
            updatelcd();
            while (digitalRead(LDAH)==0) {
                delay(DEBOUNCE);
                continue;
            }
            delay(DEBOUNCE);
            continue;
        }
        if (digitalRead(LDAL) == 0) {   // Load Address lo
            ADR = readSwitchPort();
            PC = (PC & 0xFF00) + ADR;
            updatelcd();
            while (digitalRead(LDAH)==0) {
                delay(DEBOUNCE);
                continue;
            }
            delay(DEBOUNCE);
            continue;
        }
        if (digitalRead(STEP)==0) {     // inc PC, show mem
            PC += 1;
            updatelcd();
            while (digitalRead(STEP)==0) { 
                delay(DEBOUNCE);
                continue;
            }
            delay(DEBOUNCE);
            continue;
        }
        if (digitalRead(STEPMINUS)==0) {    // dec PC, show mem
            PC -= 1;
            updatelcd();
            while (digitalRead(STEPMINUS)==0) { 
                delay(DEBOUNCE);
                continue;
            }
            delay(DEBOUNCE);
            continue;   
        }
        if (digitalRead(LOAD)==0) {     // load into (PC) switches, inc PC
            DAT = readSwitchPort();
            fram.write8(PC,DAT);
            updatelcd();    // show updated value while switch pressed
            while (digitalRead(LOAD)==0) {
                delay(DEBOUNCE);
                continue;
            }
            delay(DEBOUNCE);
            PC += 1;
            updatelcd();        // show new value when switch released
            continue;
        }
        //
        
        continue;
    }
    delay(DEBOUNCE);
    lcd.clear();
    digitalWrite(RUNLED,1);     // turn on run led
    return;
}



void halt(void) {
    digitalWrite(RUNLED,0);     // turn off RUN led

    digitalWrite(HALTLED, 1);   // turn on HALT led
    debug();
    while (true) {
        if (digitalRead(RUNHALT) == 1) {
            frontpanel();
            digitalWrite(RUNLED,0);
        }
        if (digitalRead(RESET) == 0) {
            reset();
            break;
        }
    }
    delay(DEBOUNCE);
    digitalWrite(HALTLED,0);    // clear HALT led
    while (digitalRead(RESET) == 0) continue;
    delay(DEBOUNCE);
    return;

}



/* Get an 8080 register and return the value */
uint16_t getreg(uint16_t reg) {
    switch (reg) {
        case 0:     // B
            return ((BC >>8) & 0x00ff);
        case 1:     // C
            return (BC & 0x00ff);
        case 2:     // D
            return ((DE >>8) & 0x00ff);
        case 3:     // E
            return (DE & 0x00ff);
        case 4:     // H
            return ((HL >>8) & 0x00ff);
        case 5:     // L
            return (HL & 0x00ff);
        case 6:     // (HL)
            return fram.read8(HL);
        case 7:
            return (A);
        default:
            break;
    }
    return 0;
}


/* Put a value into an 8080 register from memory */
void putreg(uint16_t reg, uint16_t val)
{
    switch (reg) {
        case 0:
            BC = BC & 0x00FF;
            BC = BC | (val <<8);
            break;
        case 1:
            BC = BC & 0xFF00;
            BC = BC | val;
            break;
        case 2:
            DE = DE & 0x00FF;
            DE = DE | (val <<8);
            break;
        case 3:
            DE = DE & 0xFF00;
            DE = DE | val;
            break;
        case 4:
            HL = HL & 0x00FF;
            HL = HL | (val <<8);
            break;
        case 5:
            HL = HL & 0xFF00;
            HL = HL | val;
            break;
        case 6:
            fram.write8(HL,val & 0xff);
            break;
        case 7:
            A = val & 0xff;
        default:
            break;
    }
}



/* Return the value of a selected register pair */
int16_t getpair(int16_t reg)
{
    switch (reg) {
        case 0:
            return (BC);
        case 1:
            return (DE);
        case 2:
            return (HL);
        case 3:
            return (StackP);
        default:
            break;
    }
    return 0;
}



/* Put a value into an 8080 register pair */
void putpair(int16_t reg, uint16_t val) // was int16_t val
{
    switch (reg) {
        case 0:
            BC = val;
            break;
        case 1:
            DE = val;
            break;
        case 2:
            HL = val;
            break;
        case 3:
            StackP = val;
            break;
        default:
            break;
    }
}


/* Set flags based on val */
void setarith(int val) {
    if (val & 0x100)    // >= 256
        C = 1;
    else
        C = 0;
    if (val & 0x80) {   // negative
        S = 1;
    } else {
        S = 0;          // positive
    }
    if ((val & 0xff) == 0)
        Z = 1;
    else
        Z = 0;
    AC = 0;             // only true w/8080, not for Z80
    P = parity(val);

}

/* set flags after logical op */
void setlogical(int32_t reg)
{
    C = 0;      // always  (AND A opcode will clear CY)
    if (reg & 0x80) {
        S = 1;
    } else {
        S = 0;
    }
    if ((reg & 0xff) == 0)
        Z = 1;
      else
        Z = 0;
    AC = 0;
    P = parity(reg);
}

/* set flags after INR/DCR operation (8 bit only)*/
void setinc(int reg) {
    if (reg & 0x80) {
        S = 1;
    } else {
        S = 0;
    }
    if ((reg & 0xff) == 0)
        Z = 1;
      else
        Z = 0;
        
    P = parity(reg);

}



/* Test an 8080 flag condition and return 1 if true, 0 if false */
int cond(int con)
{
    switch (con) {
        case 0:
            if (Z == 0) return (1);
            break;
        case 1:
            if (Z != 0) return (1);
            break;
        case 2:
            if (C == 0) return (1);
            break;
        case 3:
            if (C != 0) return (1);
            break;
        case 4:
            if (P == 0) return (1);
            break;
        case 5:
            if (P != 0) return (1);
            break;
        case 6:
            if (S == 0) return (1);
            break;
        case 7:
            if (S != 0) return (1);
            break;
        default:
            break;
    }
    return (0);
}

/* get value of register pair */
int getpush(int reg) {

    int stat;

    switch (reg) {
        case 0:
            return (BC);
        case 1:
            return (DE);
        case 2:
            return (HL);
        case 3:
            stat = A << 8;
            if (S) stat |= 0x80;
            if (Z) stat |= 0x40;
            if (AC) stat |= 0x10;
            if (P) stat |= 0x04;
            stat |= 0x02;
            if (C) stat |= 0x01;
            return (stat);
        default:
            break;
    }
    return 0;
}

/* put value of register pair */
void putpush(int reg, int data) {

    switch (reg) {
        case 0:
            BC = data;
            break;
        case 1:
            DE = data;
            break;
        case 2:
            HL = data;
            break;
        case 3:
            A = (data >> 8) & 0xff;
            S = Z = AC = P = C = 0;
            if (data & 0x80) S  = 1;
            if (data & 0x40) Z  = 1;
            if (data & 0x10) AC = 1;
            if (data & 0x04) P  = 1;
            if (data & 0x01) C  = 1;
            break;
        default:
            break;
    }
}



/* test for parity */
int parity(unsigned char ptest) {    
    int p=0;

    if (ptest==0)   /* odd parity/no parity */
        return(0);
    
    while (ptest != 0) {
        p ^= ptest;
        ptest >>= 1;
    }
    if ((p & 0x1)==0)   /* 0=even parity */
        return(1);  /* parity set */
    else
        return(0);  /* parity not set */
}





/* take 8 bit value and addr, send to device 
 * Output Port 0: send a byte to LED's thru i2c device 
 * Output Port 1: send a byte to the serial port       
*/
void output(byte val, byte address) {
    if (address == 0) {     // send byte in A to LED's
        Wire.beginTransmission(0x20);   // set up output
        Wire.write(0x09);       // GPIO
        Wire.write(val);
        Wire.endTransmission();
        return;
    }
    
    if (address == 1) {     // serial output
        Serial.write(val);
        return;
    }


    return;     // all else
}


/* read byte from input device, return as byte
 * Input Port 0: read a byte from the 12c port (switches)
 * Input Port 1: Read a byte from the serial port 
 * Input Port 2: Return Serial Port Status (>0 means byte ready)
*/
byte input(byte address) {

    if (address == 0) {         // return byte on 12c input port
        return readSwitchPort() & 0xff;
    }
    
    if (address == 1) {         // return next available serial byte
        int val;
        while (true) {
            val = Serial.read();
            if (val == -1) continue;
            return val;
        }
    }
    
    if (address == 2) {         // return number of bytes ready on the serial port
        return Serial.available();
    }
    
    return 0;

    
}

void setup(void) {

    Wire.setClock(400000L);     // tried, makes no difference - left in for reference
                                // (did NOT try to recompile arduino source)
    /* set up FRAM */
    fram.begin();

    /* LED's */
    pinMode(HALTLED, OUTPUT);
    digitalWrite(HALTLED, 0);
    pinMode(RUNLED, OUTPUT);
    digitalWrite(RUNLED, 0);
    
    /* control buttons (momentary) */
    pinMode(RESET, INPUT_PULLUP);
    pinMode(LOAD, INPUT_PULLUP);
    pinMode(STEP, INPUT_PULLUP);
    pinMode(LDAH, INPUT_PULLUP);
    pinMode(LDAL, INPUT_PULLUP);
    pinMode(STEPMINUS, INPUT_PULLUP);
    
    /* control switch */
    pinMode(RUNHALT, INPUT_PULLUP);
    
    lcd.begin(8,1);

    /* initialize output port (LED) */
    Wire.beginTransmission(0x20);   // address of LED output port
    Wire.write(0x00);               // IODIR
    Wire.write(0x00);               // output
    Wire.endTransmission();

    /* set all LED's to off */
    Wire.beginTransmission(0x20);   // set up output
    Wire.write(0x09);               // GPIO
    Wire.write(0x00);
    Wire.endTransmission();

    /* initialize input port (SWITCHES) */
    Wire.beginTransmission(0x24);   // address of input port
    Wire.write(0x00);               // IODIR
    Wire.write(0xff);               // set as inputs
    Wire.endTransmission();

    /* set pull-up resisters on switches */
    Wire.beginTransmission(0x24);   // address of input port
    Wire.write(0x06);               // GPPU
    Wire.write(0xff);               // enable pull-up resisters
    Wire.endTransmission();
  
    Serial.begin(SERIALSPEED);
    
    Serial.println("Restart");

}

void loop() {  
begin:
    reset();    /* initialize everything but memory */
    digitalWrite(RUNLED,1);

    while (true) {
        if (digitalRead(RESET) == 0) goto begin;    // calling loop() resets the arduino, so we use a goto
        
        if (digitalRead(RUNHALT)) {
            delay(DEBOUNCE);
            frontpanel();  // if halt sw enabled, go to loader. Else run.
        }
        
        if (PC >= MAXMEM) // account for non-existant memory
            OP=0x00;
        else
            OP = fram.read8(PC);

        /* debugging - show address/opcode on serial port */
        if (DEBUG) {
            char tmpout[strlen("PC:%4.4X  OP:%2.2X")+1];
            sprintf(tmpout,"PC:%4.4X  OP:%2.2X",PC,OP);
            Serial.println(tmpout);
        }


        /* This is the instruction decoder. Decoding style is lifted from
         * https://github.com/simh/simh/blob/master/ALTAIR/altair_cpu.c
         * since it's smaller and cleaner than my orig code. I cleaned
         * up some things for this build, and tightened some code. 
        */


        if (OP == 0x76) {    // HLT, stop until reset
            halt(); 
            goto begin;     // restart at 0
        }

        if ((OP & 0xC0)==0x40) {                    // MOV DEST,SRC
            temp = getreg(OP & 0x07);               // read from reg
            putreg((OP >> 3) & 0x07, temp);         // save to reg
            PC += 1;
            continue;
        }

        if ((OP & 0xC7)==0x06) {                  // MVI nn
            putreg((OP >> 3) & 0x07, fram.read8(PC+1));
            PC += 2;
            continue;
        }


        if ((OP & 0xCF) == 0x01) {                  // LXI nn
            temp = fram.read8(++PC);
            temp += (fram.read8(++PC) * 256);
            putpair(((OP >> 4) & 0x03), temp);
            PC += 1;
            continue;
        }

        if ((OP & 0xEF) == 0x0A) {                  // LDAX
            temp = getpair((OP >> 4) & 0x03);
            putreg(7, fram.read8(temp));
            PC += 1;
            continue;
        }

        if ((OP & 0xEF) == 0x02) {                  // STAX
            temp = getpair((OP >> 4) & 0x03);
            fram.write8(temp,getreg(7)); 
            PC += 1;
            continue;
        }

        /* opcodes with tests */
        
        if ((OP & 0xF8) == 0xB8) {                  // CMP
            temp = A & 0xFF;
            temp -= getreg(OP & 0x07);
            setarith(temp);
            PC += 1;
            continue;
        }

        if ((OP & 0xC7) == 0xC2) {                  // JMP <condition>
            if (cond((OP >> 3) & 0x07) == 1) {
                lo = fram.read8(++PC);
                hi = fram.read8(++PC);
                PC = (hi << 8) + lo;
            } else {
                PC += 3;
            }
            continue;
        }


        if ((OP & 0xC7) == 0xC4) {                  // CALL <condition>
            if (cond((OP >> 3) & 0x07) == 1) {
                lo = fram.read8(++PC);
                hi = fram.read8(++PC);
                PC++;
                fram.write8(--StackP,(PC & 0xff00) >> 8);
                fram.write8(--StackP,(PC & 0xff));
                //PC = (hi << 8) + lo;
                PC = (hi * 256) + lo;
            } else {
                PC += 3;
            }
            continue;
        }

        if ((OP & 0xC7) == 0xC0) {                  // RET <condition>
            if (cond((OP >> 3) & 0x07) == 1) {
                PC = fram.read8(StackP);
                StackP++;
                PC |= ((fram.read8(StackP) << 8) & 0xff00);
                StackP++;
            } else {
                PC += 1;
            }
            continue;
        }

        if ((OP & 0xC7) == 0xC7) {                  // RST
            StackP--;
            fram.write8(StackP,(PC >> 8) & 0xff);
            StackP--;
            fram.write8(StackP,PC & 0xff);
            PC = OP & 0x38;
            continue;
        }


        if ((OP & 0xCF) == 0xC5) {                  // PUSH
            temp = getpush((OP >> 4) & 0x03);
            StackP--;
            fram.write8(StackP,(temp >> 8) & 0xff);
            StackP--;
            fram.write8(StackP,temp & 0xff);
            PC += 1;
            continue;
        }


        if ((OP & 0xCF) == 0xC1) {                   // POP
            temp = fram.read8(StackP);
            StackP++;
            temp |= fram.read8(StackP) << 8;
            StackP++;
            putpush((OP >> 4) & 0x03, temp);
            PC += 1;
            continue;
        }

        if ((OP & 0xF8) == 0x80) {                  // ADD
            A += getreg(OP & 0x07);
            setarith(A);
            A = A & 0xFF;
            PC += 1;
            continue;
        }

        if ((OP & 0xF8) == 0x88) {                  // ADC
            carry = 0;
            if (C) carry = 1;
            A += getreg(OP & 0x07);
            A += carry;
            setarith(A);
            A = A & 0xFF;
            PC += 1;
            continue;
        }

        if ((OP & 0xF8) == 0x90) {                  // SUB
            A -= getreg(OP & 0x07);
            setarith(A);
            A = A & 0xFF;
            PC += 1;
            continue;
        }

        if ((OP & 0xF8) == 0x98) {                  // SBB
            carry = 0;
            if (C) carry = 1;
            A = A - getreg(OP & 0x07) - carry;
            setarith(A);
            A = A & 0xFF;
            PC += 1;
            continue;
        }

        if ((OP & 0xC7) == 0x04) {                  // INR
            temp = getreg((OP >> 3) & 0x07);
            temp++;
            setinc(temp);
            temp = temp & 0xFF;
            putreg((OP >> 3) & 0x07, temp);
            PC += 1;
            continue;
        }

        if ((OP & 0xC7) == 0x05) {                  // DCR
            temp = getreg((OP >> 3) & 0x07);
            temp--;
            setinc(temp);
            temp = temp & 0xFF;
            putreg((OP >> 3) & 0x07, temp);
            PC += 1;
            continue;
        }

        if ((OP & 0xCF) == 0x03) {                  // INX
            temp = getpair((OP >> 4) & 0x03);
            temp++;
            temp = temp & 0xFFFF;
            putpair((OP >> 4) & 0x03, temp);
            PC += 1;
            continue;
        }
        
        if ((OP & 0xCF) == 0x0B) {                  // DCX
            temp = getpair((OP >> 4) & 0x03);
            temp--;
            temp = temp & 0xFFFF;
            putpair((OP >> 4) & 0x03, temp);
            PC += 1;
            continue;
        }

        if ((OP & 0xCF) == 0x09) {                  // DAD
            C = 0;
            if (long(HL) + long(getpair((OP >> 4) & 0x03) > 0xffff))
                C = 1;
            HL += getpair((OP >> 4) & 0x03);
            PC += 1;
            continue;
        }

        if ((OP & 0xF8) == 0xA0) {                  // ANA
            A &= getreg(OP & 0x07);
            C = 0;
            setlogical(A);
            A &= 0xFF;
            PC += 1;
            continue;
        }

        if ((OP & 0xF8) == 0xB0) {                  // ORA
            A |= getreg(OP & 0x07);
            C = 0;
            setlogical(A);
            A &= 0xFF;
            PC += 1;
            continue;
        }
        
        
        if ((OP & 0xF8) == 0xA8) {                  // XRA
            A ^= getreg(OP & 0x07);
            C = 0;
            setlogical(A);
            A &= 0xFF;
            PC += 1;
            continue;
        }

        /* now do the rest of the instructions */

        switch(OP) {

            case 0xfe: {                        // CPI
                temp = A & 0xFF;
                temp -= fram.read8(++PC);
                PC += 1;
                setarith(temp);
                break;
            }

            case 0xe6: {                        // ANI
                A &= fram.read8(++PC);
                PC += 1;
                C = AC = 0;
                setlogical(A);
                A &= 0xFF;
                break;
            }

            case 0xee: {                        // XRI
                A ^= fram.read8(++PC);
                PC += 1;
                C = AC = 0;
                setlogical(A);
                A &= 0xFF;
                break;
            }

            case 0xf6: {                        // ORI
                A |= fram.read8(++PC);
                PC += 1;
                C = AC = 0;
                setlogical(A);
                A &= 0xFF;
                break;
            }

            /* Jump/Call instructions */
            
            case 0xc3: {                        // JMP
                lo = fram.read8(++PC);
                hi = fram.read8(++PC);
                PC = (hi << 8) + lo;
                break;
            }

            case 0xe9: {                        // PCHL
                PC = HL;
                break;
            }

            case 0xcd: {                        // CALL

                lo = fram.read8(++PC);
                hi = fram.read8(++PC);
                PC++;       // point to address after call
                --StackP;
                fram.write8(StackP,(PC & 0xff00) >> 8);
                --StackP;
                fram.write8(StackP,(PC & 0xff));
                PC = (hi * 256) + lo;
                break;
            }

            case 0xc9: {                        // RET
            PC = fram.read8(StackP);
            StackP++;
            PC |= ((fram.read8(StackP) << 8) & 0xff00);
            StackP++;
            break;
            }

            /* Data Transfer instructions */
            
            case 0x32: {                        // STA
                lo = fram.read8(++PC);
                hi = fram.read8(++PC);
                PC += 1;
                temp = (hi << 8) + lo;
                fram.write8(temp,A);
                break;
            }

            case 0x3a: {                        // LDA
                lo = fram.read8(++PC);
                hi = fram.read8(++PC);
                PC += 1;
                temp = (hi << 8) + lo;
                A = fram.read8(temp);
                break;
            }

            case 0x22: {                        // SHLD
                lo = fram.read8(++PC);
                hi = fram.read8(++PC);
                PC += 1;
                temp = (hi << 8) + lo;
                fram.write8(temp,HL);
                temp++;
                fram.write8(temp,(HL >>8) & 0x00ff);
                break;
            }

            case 0x2a: {                        // LHLD
                lo = fram.read8(++PC);
                hi = fram.read8(++PC);
                PC += 1;
                temp = (hi << 8) + lo;
                HL = fram.read8(temp);
                temp++;
                HL = HL | (fram.read8(temp) << 8);
                break;
            }


            case 0xeb: {                        // XCHG
                temp = HL;
                HL = DE;
                DE = temp;
                PC += 1;
                break;
            }

             /* Arithmetic Instructions */
            
            case 0xc6: {                        // ADI
                A += fram.read8(++PC);
                PC += 1;
                setarith(A);
                A = A & 0xFF;
                break;
            }

            case 0xce: {                        // ACI
                carry = 0;
                if (C) carry = 1;
                A += fram.read8(++PC);
                A += carry;
                PC += 1;
                setarith(A);
                A = A & 0xFF;
                break;
            }

            case 0xd6: {                        // SUI
                A -= fram.read8(++PC);
                PC += 1;
                setarith(A);
                A = A & 0xFF;
                break;
            }

            case 0xde: {                        // SBI
                carry = 0;
                if (C) carry = 1;
                A -= (fram.read8(++PC) + carry);
                PC += 1;
                setarith(A);
                A = A & 0xFF;
                break;
            }

            
            case 0x27: {                        // DAA
                uint8_t a_hi, a_lo;
                a_lo = A & 0x0F; 
                a_hi = (A >> 4) & 0x0F;
                
                if (a_lo > 9 || AC == 1) {
                    a_lo += 6;
                    if (a_lo > 0xf) {
                        C = 1;
                        a_hi += 1;
                        
                    }
                }
                a_lo &= 0x0f; 

                if (a_hi > 9 ) a_hi += 6;
                if (a_hi > 15) 
                    C = 1;
                else
                    C = 0;
                
                a_hi &= 0xf;
                    
                A = (a_hi << 4);
                A |= a_lo;
                A &= 0xFF;

                Z = 0;
                if (A == 0) Z = 1;
                P = parity(A);
                S = 0;
                if (A > 0x80) S = 1;
                PC += 1;
                break;    
            
            }

            case 0x07: {                        // RLC
                C = 0;
                A = (A << 1);
                if (A & 0x100) C = 1;
                A &= 0xFF;
                if (C)
                    A |= 0x01;
                PC += 1;
                break;
            }
            
            case 0x0f: {                        // RRC
                C = 0;
                if ((A & 0x01) == 1)
                    C = 1;
                A = (A >> 1) & 0xFF;
                if (C)
                    A |= 0x80;
                PC += 1;
                break;
            }

            case 0x17: {                        // RAL
                temp = C;
                C = 0;
                A = (A << 1);
                if (A & 0x100) C = 1;
                A &= 0xFF;
                if (temp)
                    A |= 1;
                else
                    A &= 0xFE;
                PC += 1;
                break;
            }

            case 0x1f: {                        // RAR
                temp = C;
                C = 0;
                if ((A & 0x01) == 1)
                    C = 1;
                A = (A >> 1) & 0xFF;
                if (temp)
                    A |= 0x80;
                else
                    A &= 0x7F;
                PC += 1;
                break;
            }


            case 0x2f: {                        // CMA
                A = ~ A;
                A &= 0xFF;
                PC += 1;
                break;
            }

            case 0x3f: {                        // CMC
                C = ~ C;
                PC += 1;
                break;
            }

            case 0x37: {                        // STC
                C = 1;
                PC += 1;
                break;
            }

            /* Stack and Control Group */
            
            case 0xe3: {                        // XTHL
                lo = fram.read8(StackP);
                hi = fram.read8(StackP + 1);
                fram.write8(StackP,HL & 0xFF);
                fram.write8(StackP+1,(HL >> 8) & 0xFF);
                HL = (hi << 8) + lo;
                PC += 1;
                break;
            }

            case 0xf9: {                        // SPHL
                StackP = HL;
                PC += 1;
                break;
            }

            case 0xfb: {                        // EI
                INTE = 1;
                PC += 1;
                break;
            }

            case 0xf3: {                        // DI
                INTE = 0;
                PC += 1;
                break;
            }

            case 0xd3:  {                       // OUT
                output(A,fram.read8(++PC));
                PC += 1;
                break;
            }
            
            case 0xdb:  {                       // IN
                A = input(fram.read8(++PC));
                A &= 0xff;
                PC += 1;
                break;
            }

         
            default:
                PC += 1;        // for NOP & unused opcodes
                break;
        }

        continue;        
    }

}
