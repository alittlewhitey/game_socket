#ifndef GAME_SOCK_EXPORT
#define GAME_SOCK_EXPORT

#include "server.hpp"
#include "error.hpp"
#include "packet.hpp"

extern Server* server;
extern Client* client;
extern std::jthread accept_thread;
extern "C"{
    void server_create(int port = 7021);
    void server_accept();
    int server_connect_count();
    void server_world_sync(char* buf,int size);
    void server_delete();
    void client_create();
    bool client_connect(const char* ip,int port = 7021);
    typedef void ( client_world_sync_handler)(char* buf,int size);
    void client_world_sync_proc_register(client_world_sync_handler handler);
    void client_delete();
}

#endif // !GAME_SOCK_EXPORT
