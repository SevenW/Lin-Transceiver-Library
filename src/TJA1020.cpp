// Utilizes a TJA1020 Lin-Transceiver
// Provides Lin-Interface but considers the statemachin in all steps
//
// Copyright mestrode <ardlib@mestro.de>
// Original Source: "https://github.com/mestrode/Lin-Transceiver-Library"
//
// Requires class Lin_Interface: https://github.com/mestrode/Lin-Interface-Library
//
// Tested with ESP32

#include "TJA1020.hpp"

#include <Lin_Interface.hpp>
// #include <Arduino.h>

//-----------------------------------------------------------------------------
// Pin definitions, we need the TX Pin to control the TJA1020 statemachine
// See "HardwareSerial.cpp"
#ifdef ESP32
    #ifndef TX0
    #define TX0 1
    #endif

    #ifndef TX1
    #define TX1 10
    #endif

    #ifndef TX2
    #define TX2 17
    #endif
#else
    #error "Need Pin definitions for LIN Transceiver"
#endif

//-----------------------------------------------------------------------------
// constructor

/// Provides HW UART via TJA1020 Chip
Lin_TJA1020::Lin_TJA1020(int uart_nr, uint32_t _baud, int8_t nslpPin) : Lin_Interface(uart_nr)
{
    // Use typical pin configuration if interfaced by uart_nr
    //############
    // See "HardwareSerial.cpp"
    if(uart_nr == 0) {
        _tx_pin = TX0;
    }
    if(uart_nr == 1) {
        _tx_pin = TX1;
    }
    if(uart_nr == 2) {
        _tx_pin = TX2;
    }
    //############

    // use default baud rate, if not specified
    Lin_Interface::baud = _baud ? _baud : 19200;

    _nslp_pin = nslpPin;
}

//-----------------------------------------------------------------------------
// write / read  on bus

/// Requests a Lin 2.0 Frame
bool Lin_TJA1020::readFrame(uint8_t FrameID)
{
    setMode(_writingSlope);
    return Lin_Interface::readFrame(FrameID);
}

/// Sends a Lin 2.0 Frame
void Lin_TJA1020::writeFrame(uint8_t FrameID, size_t size)
{
    setMode(_writingSlope);
    return Lin_Interface::writeFrame(FrameID, size);
}

/// Sends a Lin 1.3 Frame (Legacy Checksum)
void Lin_TJA1020::writeFrameClassic(uint8_t FrameID, size_t size)
{
    setMode(_writingSlope);
    Lin_Interface::writeFrameClassic(FrameID, size);
}

/// does control sequences to switch from one to another operational mode of the chip
/// NormalSlope, LowSlope for writing operation;
/// Sleep will release INH and may disables Power-Supply
/// @param mode TJA1020 Mode to be the next
void Lin_TJA1020::setMode(TJA1020_Mode mode)
{
    // we don't need to act, if we're allready there
    // see "setMode(sleep)" in the switch below
    if (mode == _currentMode)
    {
        return;
    }

    pinMode(_tx_pin, OUTPUT);   //  TX  Signal to LIN Tranceiver
    pinMode(_nslp_pin, OUTPUT); // /SLP Signal to LIN Tranceiver

    // Statemachine des TJA1020 von [SLEEP] nach [NORMAL SLOPE MODE] ändern
    switch (mode)
    {
    case NormalSlope:
        if (_currentMode == LowSlope)
        {
            // no direct step from LowSlope to NormalSlope
            setMode(Sleep);
        }

        // rising edge on /SLP while TXD = 1
        digitalWrite(_tx_pin, HIGH);
        delayMicroseconds(10); // ensure signal to settle

        // create rising edge
        digitalWrite(_nslp_pin, LOW);
        delayMicroseconds(15); //  ensure t_gotosleep (min. 10us)
        digitalWrite(_nslp_pin, HIGH);
        delayMicroseconds(15); // ensure t_gotonorm (min. 10us)

        // [Normal Slope Mode] reached
        _currentMode = NormalSlope;
        break;

    case LowSlope:
        if (_currentMode == NormalSlope)
        {
            // no direct step from NormalSlope to LowSlope
            setMode(Sleep);
        }

        // rising edge on /SLP while TXD = 0
        digitalWrite(_tx_pin, LOW);
        delayMicroseconds(10); // ensure signal to settle

        // create rising edge
        digitalWrite(_nslp_pin, LOW);
        delayMicroseconds(15); //  ensure t_gotosleep (min. 10us)
        digitalWrite(_nslp_pin, HIGH);
        delayMicroseconds(15); // ensure t_gotonorm (min. 10us)

        // [Low Slope Mode] reached
        _currentMode = LowSlope;
        break;

    default: // = Sleep
        // no direct step from Standby to Sleep, but we don't know if we're in 
        // we're going over _writingSlope
        setMode(_writingSlope);

        // rising edge on /SLP while TXD = 1
        digitalWrite(_tx_pin, HIGH);
        delayMicroseconds(10); // ensure signal to settle

        // create falling edge
        digitalWrite(_nslp_pin, HIGH);
        delayMicroseconds(15); //  ensure t_gotosleep (min. 10us)
        digitalWrite(_nslp_pin, LOW);
        delayMicroseconds(15); // ensure t_gotonorm (min. 10us)
        break;

        // [Sleep] reached
        _currentMode = Sleep;
    }
} // void Lin_TJA1020::setMode(TJA1020_Mode newMode)

/// Defines standard slope, to be used, when writing to the bus
void Lin_TJA1020::setSlope(TJA1020_Mode slope) {
  _writingSlope = slope;
  if (_writingSlope == Sleep) {
      _writingSlope = NormalSlope;
  }
}
