#include<iostream>

#include "httpServer.h"
#include "served/multiplexer.hpp"



int main() {

    served::multiplexer multiplexer;
    TollMgmtSystem::HttpServer httpServer(multiplexer);

    httpServer.InitializeEndPoints();
    httpServer.StartServer();
    
}
