#include"export.hpp"
int main(){
    server_create();
    server_accept();
    while(server_connect_count() <= 1)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    char buf[20] = "Hello World!";
    server_world_sync(buf,strlen(buf));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    server_delete();
}