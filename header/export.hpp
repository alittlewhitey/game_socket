#ifndef GAME_SOCK_EXPORT
#define GAME_SOCK_EXPORT

#include "server.hpp"
#include "error.hpp"
#include "packet.hpp"

extern Server* server;
extern Client* client;
extern std::jthread accept_thread;
extern "C"{
    typedef void (data_handler)(char* buf,int size);
    void server_create(int port = 7021);
    void server_accept();
    int server_connect_count();
    void server_send_binary(char* buf,int size);
    void server_data_proc_register(sock_id id,data_handler handler);
    void server_delete();
    void client_create();
    bool client_connect(const char* ip,int port = 7021);
    void client_send_binary(char* buf,int size);
    void client_data_proc_register(data_handler handler);
    void client_delete();
}

#endif // !GAME_SOCK_EXPORT
