#include"export.hpp"
int main(){
    server_create();
    server_accept();
    while(server_connect_count() <= 1)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    std::string s;
    s.reserve(2048);
    for(int i = 0;i!=2048;++i){
        s.push_back('A');
    }
    server_send_binary((char*)s.c_str(),2048);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    server_delete();
}