#include"export.hpp"
bool get = 0;
void f(char* buf,int size){
    get = 1;
    for(int i = 0;i!=size;++i){
        std::cout << (int)(buf[i]) << '\t';
    }
}
int main(){
    client_create();
    bool b = client_connect("127.0.0.1");
    if(!b)
        return -1;
    client_world_sync_proc_register(f);
    while(!get){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    client_delete();
}