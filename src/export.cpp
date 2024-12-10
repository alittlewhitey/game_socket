#include"export.hpp"

Server* server = nullptr;
Client* client = nullptr;
std::jthread accept_thread;
extern "C"{
    void server_create(int port){
        server = new Server(port);
    }
    void server_accept(){
        accept_thread = std::jthread([](std::stop_token token){
            server->accept(token);
        });
        accept_thread.detach();
    }
    int server_connect_count(){
        return server->connect_count();
    }
    void server_send_binary(char* buf,int size){
        std::string data;
        data.reserve(size);
        for(int i = 0;i<size;++i){
            data.push_back(buf[i]);
        }
        server->each_sock([&data](boost::asio::ip::tcp::socket& a){
            OPacket op;
            op.head = packet_head;
            op.type = (int)packet_type::send_binary;
            op.write_data(data);
            a << op;
        });
    }
    void server_data_proc_register(sock_id id,data_handler handler){
        std::cout << "Register" << std::endl;
        server->use_connect(id,[handler](Server_Connect& connect){
            connect.data_proc = handler;
        });
    }
    void server_delete(){
        accept_thread.request_stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        delete server;
    }
    void client_create(){
        client = new Client();
    }
    bool client_connect(const char* ip,int port){
        if(client->connect(ip,port)){
            client->run();
            return 1;
        }
        return 0;
    }
    void client_send_binary(char* buf,int size){
        std::string data;
        data.reserve(size);
        for(int i = 0;i<size;++i){
            data.push_back(buf[i]);
        }
        OPacket op;
        op.head = packet_head;
        op.type = (int)packet_type::send_binary;
        op.write_data(data);
        client->use_connect([&op](Client_Connect& connect){
            connect.server << op;
        });
    }
    void client_data_proc_register(data_handler handler){
        std::cout << "Register" << std::endl;
        client->use_connect([handler](Client_Connect& connect){
            connect.data_proc = handler;
        });
    }
    void client_delete(){
        delete client;
    }
}