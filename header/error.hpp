#ifndef GAME_SOCK_ERROR
#define GAME_SOCK_ERROR
#include<cstring>
#include<iostream>
#include<fstream>
enum class sock_errno: int{
    success = 0,
    clipack = 1,
    serpack = 2,
    //QWQ
    heart_broken = 3,
    packet_format_err = 4
};
void sock_error(std::ostream& os,sock_errno err);
extern std::ofstream logger;
constexpr static int sock_errbuf_num = 256;
extern char sock_errbuf[sock_errbuf_num];

#endif // !GAME_SOCK_ERROR