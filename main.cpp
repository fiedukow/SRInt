#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "zmq.hpp"
#include <string>
#include <iostream>
#include "zhelpers.hpp"
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

int main () {
    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5555");
    int answer_number = 0;
    std::cout << "Server is up and running!" << std::endl;
    while (true) {
        std::cout << "Received " << s_recv(socket) << std::endl;                                

        std::stringstream ss;
        ss << "World " << answer_number++;
        //  Send reply back to client
        std::cout << "Answering: \"" << ss.str() << "\""<< std::endl;
        s_send(socket, ss.str());
    }
    return 0;
}

