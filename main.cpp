#include "mbed.h"
#include "INA3221.h"

BufferedSerial pc( USBTX, USBRX, 9600 ) ;
AnalogIn battery_voltage( A0, 4.2 ) ; 
INA3221 INA( PB_9 , PB_8 );
DigitalOut myled( LED1 );

int main(void)
{  
    if(INA.CheckConnection()) {
        printf("Connection OK \n");
    }
    else {
        printf("Error no Connection");
        while(1);
    }
        
    INA.Rest();
    INA.SetMode(INA3221_MODE_SHUNT_BUS_CONTINUOUS);
    INA.SetShuntConversionTime(INA3221_1_1_MS);                          
    INA.SetBusConversionTime(INA3221_1_1_MS);                            
    INA.SetAveragingMode(INA3221_AVERAGE_64);                                        
    
    for( uint8_t i = 1 ; i < 4 ; i++ )
    {
        INA.EnableChannel(i); 
        INA.EnableChannelSummation(i);
    }

    volatile float volts ;
    volatile uint16_t batt_measure ; // raw state of charge measure as propotion of system voltage (3.3v)
    uint8_t* bytes ; // output buffer
    pc.set_format( 8, mbed::SerialBase::None, 1 ) ;
    
    while( 1 )
    {
        // Voltage in
        volts = battery_voltage.read_voltage() ;
        uint8_t v_i = volts + '0' ;                             // intger part of volt measure
        uint8_t v_f = ( volts + '0' - v_i ) * 10 + '0';         // mantissa of volt measure
        bytes = new uint8_t[7]{'V', 'o', 'l', 't', 's', ':', '\t'};
        pc.write( bytes, 7 ) ;
        ThisThread::sleep_for( 30ms ) ;
        bytes = new uint8_t[5]{ v_i, '.', v_f, 0x0A } ;
        pc.write( bytes, 4 ) ;
        ThisThread::sleep_for( 30ms ) ;

        // Battery voltage as percent of system reference voltage (~3.3v / 3.4v?)
        batt_measure = battery_voltage.read() * 100 ;
        if ( batt_measure == 0 )
            bytes = new uint8_t[4]{ 0x30, 0x30, 0x30, 0x0A } ;  // 000\n
        else if ( batt_measure % 100 == 0 ) 
            bytes = new uint8_t[4]{ 0x31, 0x30, 0x30, 0x0A } ;  // 100\n
        else
        {
            volatile uint8_t LSD = batt_measure % 10 + '0' ;
            volatile uint8_t MSD = ( batt_measure + '0' - LSD ) / 10 + '0' ;
            bytes = new uint8_t[3]{ 0x30, MSD, LSD } ;
        }

        pc.write( bytes, 3 ) ;   
        ThisThread::sleep_for( 30ms ) ;
        bytes = new uint8_t[10]{ '%', ' ', 'o', 'f', ' ', 'V', 'R', 'E', 'F', '\n' } ;
        pc.write( bytes, 10 ) ;
        //printf("% of VREF\n") ;
        ThisThread::sleep_for( 30ms ) ;

        while(INA.ConversionReady()==0); // wait for sensor
        
        printf("Ch1: %d mA\n", int(INA.GetCurrent(1)*1000) );
        printf("Ch1: %d V (10x)\n\n", int(INA.GetBusVoltage(1)*10) ); // voltage is multiplied by 10 for easy display
        /*
        printf("Ch2: %d mA\n", int(INA.GetCurrent(1)*1000) );
        printf("Ch2: %d V (10x)\n\n", int(INA.GetBusVoltage(1)*10) ); 
        */
        
        myled = !myled;

        
        
        ThisThread::sleep_for( 3s ) ;

    }

}