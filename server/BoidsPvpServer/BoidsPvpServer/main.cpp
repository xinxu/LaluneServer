//
//  main.cpp
//  BoidsPvpServer
//
//  Created by Yanjie Chen on 1/12/15.
//  Copyright (c) 2015 Shanghai Yuyu Network Technology Co. Ltd. All rights reserved.
//

#include <iostream>

#include "./Server/PvpServer.h"

int main(int argc, const char * argv[]) {
    // insert code here...
    boids::PvpServerContextPtr context = boids::PvpServerContextPtr( new boids::PvpServerContext() );
    boids::PvpServerPtr server = boids::PvpServerPtr( new boids::PvpServer( context ) );
    if( server->init() ) {
    }
    else {
        std::cout << "server start failed" << std::endl;
    }
    
    return 0;
}
