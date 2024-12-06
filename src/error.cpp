#include"error.hpp"

void sock_error(std::ostream& os,sock_errno err){
    switch(err){
    case sock_errno::success:{
        os << "Success." <<std::endl;
    }break;
    case sock_errno::clipack:{
        os << "Receive a packet that should be sent to client." << std::endl;
    }break;
    case sock_errno::serpack:{
        os << "Receive a packet that should be sent to server." << std::endl;
    }break;
    case sock_errno::heart_broken:{
        os << "Timeout before receive a heart-beat packet." << std::endl;
    }break;
    case sock_errno::packet_format_err:{
        os << "The packet data format is not as expected." << std::endl;
    }break;
    }
}
std::ofstream logger("./logs.txt");
char sock_errbuf[sock_errbuf_num];