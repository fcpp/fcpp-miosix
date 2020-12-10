// Copyright © 2020 Giorgio Audrito and Federico Terraneo. All Rights Reserved.

#include "driver.hpp"

using namespace miosix;

namespace fcpp {

namespace os {

#ifdef DBG_TRANSCEIVER_ACTIVITY_LED
/*
 * Driver to blink the wandstem board green LED every time activity() is called.
 */

struct ActivityThread
{
    ActivityThread()
    {
        threadPtr = Thread::create(&ActivityThread::run, STACK_MIN);
    }
    
    static void run(void*)
    {
        greenLed::mode(Mode::OUTPUT);
        for(;;)
        {
            Thread::wait();
            greenLed::high();
            Thread::sleep(100);
            greenLed::low();
        }
    }
    
    Thread *threadPtr = nullptr;
};

static ActivityThread activityThread;
    
void activity()
{
    if(activityThread.threadPtr) activityThread.threadPtr->wakeup();
}
#else
void activity() {}    
#endif

const char transceiver::panHeader[transceiver::panHeaderSize] = {
    0x41, //frame type 0b001 (data), intra pan
    0x08, //no source addressing, short destination addressing
    0x00, //seq no
    0xaa, 0xbb, //pan ID (hardcoded)
    0xff, 0xff, //destination addr (broadcast)
};

}

}
