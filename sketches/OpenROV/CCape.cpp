#include "AConfig.h"
#if( HAS_STD_CAPE )

// Includes
#include "CCape.h"
#include "NDataManager.h"
#include "CTimer.h"
#include "NConfigManager.h"
#include "FreeMem.h"

namespace
{
    const int numReadings = 30;
    int readings[numReadings];      // the readings from the analog input
    CTimer time;
    CTimer statustime;
    int index = 0;                  // the index of the current reading
    int total = 0;                  // the running total
    int average = 0;                // the average
    // Define the number of samples to keep track of.  The higher the number,
    // the more the readings will be smoothed, but the slower the output will
    // respond to the input.  Using a constant rather than a normal variable lets
    // use this value to determine the size of the readings array.

    CPin vout( "vout", CAPE_VOLTAGE_PIN, vout.kAnalog, vout.in );
    CPin iout( "iout", CAPE_CURRENT_PIN, iout.kAnalog, iout.in );


    double GetTemp( void )
    {
        unsigned int wADC;
        double t;

        // The internal temperature has to be used
        // with the internal reference of 1.1V.
        // Channel 8 can not be selected with
        // the analogRead function yet.

        // Set the internal reference and mux.
        ADMUX = ( _BV( REFS1 ) | _BV( REFS0 ) | _BV( MUX3 ) );
        ADCSRA |= _BV( ADEN ); // enable the ADC

        delay( 20 );          // wait for voltages to become stable.

        ADCSRA |= _BV( ADSC ); // Start the ADC

        // Detect end-of-conversion
        while( bit_is_set( ADCSRA, ADSC ) );

        // Reading register "ADCW" takes care of how to read ADCL and ADCH.
        wADC = ADCW;

        // The offset of 324.31 could be wrong. It is just an indication.
        t = ( wADC - 324.31 ) / 1.22;

        // The returned temperature is in degrees Celcius.
        return ( t );
    }

    float mapf( long x, long in_min, long in_max, long out_min, long out_max )
    {
        return ( float )( x - in_min ) * ( out_max - out_min ) / ( float )( in_max - in_min ) + out_min;
    }
}

void CCape::Initialize()
{
    time.Reset();
    statustime.Reset();

    // initialize all the readings to 0:
    for( int thisReading = 0; thisReading < numReadings; thisReading++ )
    {
        readings[thisReading] = 0;
    }
}

void CCape::Update( CCommand& commandIn )
{

    if( time.HasElapsed( 100 ) )
    {
        // subtract the last reading:
        total = total - readings[index];
        // read from the sensor:
        readings[index] = iout.Read();
        delay( 1 );
        // add the reading to the total:
        total = total + readings[index];
        // advance to the next position in the array:
        index = index + 1;

        // if we're at the end of the array...
        if( index >= numReadings )
            // ...wrap around to the beginning:
        {
            index = 0;
        }

        // calculate the average:
        average = total / numReadings;
    }

    // send voltage and current
    if( statustime.HasElapsed( 100 ) )
    {
        NDataManager::m_capeData.VOUT = mapf( vout.Read(), 0, 1023, 0, 50 );
        NDataManager::m_capeData.IOUT = mapf( average, 0, 1023, 0, 5 ) + .4;
        NDataManager::m_capeData.FMEM = FreeMemory();
        NDataManager::m_capeData.ATMP = GetTemp();
        NDataManager::m_capeData.UTIM = millis();
    }
}
#endif
