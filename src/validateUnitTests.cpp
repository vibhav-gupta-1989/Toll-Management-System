#include<iostream>
#include "mongoDbHandler.h"
#include "mongocxx/instance.hpp"

int main(){
    mongocxx::instance instance;
    TollMgmtSystem::MongoDbHandler mHandler; 

    // Ticket Valid Test (Vehicle driver is at source toll station)
    assert(true == mHandler.ValidateTest1());

    // Ticket Valid Test (Vehicle driver is in the middle of their path to destination)
    assert(true == mHandler.ValidateTest2());

    // Ticket Valid Test (Vehicle driver hasn't paid for the ticket online. So they
    // pay for it upon validation. But start time is okay i.e. within 24 hours. Hence
    // ticket is valid.)
    assert(true == mHandler.ValidateTest3());

    // Ticket Invalid Test (Ticket Start time is before 24 hours)
    assert(false == mHandler.ValidateTest4());

    // Ticket Invalid Test (Vehicle driver hasn't paid for the ticket online. So they
    // pay for it upon validation. But start is also not okay i.e. not within 24 hours.
    // Hence ticket is invalid.)
    assert(false == mHandler.ValidateTest5());

    // Ticket Invalid Test (toll from source station to destination station is less than toll 
    // from current station to destination station)
    assert(false == mHandler.ValidateTest6());

    return 0;
}