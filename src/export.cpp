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
    void server_world_sync(char* buf,int size){
        std::string data;
        data.reserve(size);
        for(int i = 0;i<size;++i){
            data.push_back(buf[i]);
        }
        server->each_connect([&data](p_connect_server& a){
            OPacket op;
            op.head = packet_head;
            op.type = (int)packet_type::world_sync;
            op.write_data(data);
            a.get_sock() << op;
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
    void client_world_sync_proc_register(client_world_sync_handler handler){
        std::cout << "Register" << std::endl;
        client->use_p_connect([handler](Client_Connect& connect){
            connect.world_sync_proc = handler;
        });
    }
    void client_delete(){
        delete client;
    }
}