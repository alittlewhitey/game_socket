#ifndef GAME_SOCK_SERVER
#define GAME_SOCK_SERVER
#include<map>
#include<chrono>
#include<thread>
#include<atomic>
#include<execution>
#include<algorithm>
#include<boost/asio.hpp>

#include "error.hpp"
#include "packet.hpp"

constexpr int packet_head = 0x79364A8F;
extern boost::asio::io_context sock_ioc;


enum class packet_type: int{
    // begin with 4 is send by client
    // begin with 5 is send by server


    /*
        All packet start with


        head: int
        type: int
        size: int
    */


    /*
        timestamp: uint64_t (milisecond)
    */
    heart_beat = 5001,

    /*
        timestamp: uint64_t (milisecond)
    */
    heart_beat_back = 4001,
    
    /*
        I dont know what struct it should be.
        so it is void
    */
    register_connect = 4002,

    /*
            // means client get signal
        void
    */
    register_connect_back = 5002,

    /*
        errno: int
    */
    disconnect = 3001,

    /*
        data_size: int
        world_data: byte[data_size]
    */
    send_binary = 5101,

    /*
            // means client get signal
        void
    */
    received_binary = 4101
};
typedef int sock_id;

class Server_Connect{
    std::atomic<bool> blocking = 0;
    std::atomic<bool> alive = 1;
    const std::chrono::system_clock::time_point process_start_time;
    const std::chrono::system_clock::time_point connect_start_time;
public:
    Server_Connect(sock_id id,boost::asio::ip::tcp::socket& client,const std::chrono::system_clock::time_point process_start_time):id(id),client(client)
    ,process_start_time(process_start_time),connect_start_time(std::chrono::system_clock::now()){
        
    }

    sock_id id;
    boost::asio::ip::tcp::socket& client;

    //sub of time(0)
    std::chrono::milliseconds delay = std::chrono::milliseconds(0);

    int heart_beat_wait_times = 0;
    const int heart_broken_line = 10;
    const std::chrono::seconds timeout = std::chrono::seconds(5);
    std::jthread heart_beat_thread;
    bool heart_beating = 0;
    std::jthread start_check;
    bool start_checked = 0;

    std::function<void(char*,int)> data_proc = [](char*,int){};

    bool is_blocking(){
        return blocking;
    }
    void end(){
        alive = 0;
    }
    ~Server_Connect(){
        if(heart_beating)
            heart_beat_thread.request_stop();
        if(!start_checked)
            start_check.request_stop();
    }

    void heart_beat_proc(const std::stop_token token){
        while(!token.stop_requested()&&alive){
            if(heart_beat_wait_times >= heart_broken_line){
                OPacket op;
                op.head = packet_head;
                op.type = (int)packet_type::disconnect;
                op.write<int>((int)sock_errno::heart_broken);
                client << op;
                alive = 0;
                break;
            }
            OPacket op;
            op.head = packet_head;
            op.type = (int)packet_type::heart_beat;
            op.write<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()-process_start_time).count());
            std::cout << "send heart beat" << std::endl;
            client << op;
            ++heart_beat_wait_times;
            std::this_thread::sleep_for(timeout);
        }
    }


    void clipack(){
        OPacket op;
        op.head = packet_head;
        op.type = (int)packet_type::disconnect;
        op.write<int>((int)sock_errno::clipack);
        client << op;
        alive = 0;
    }
    bool check_head(int head){
        if(head == packet_head)
            return 1;
        alive = 0;
        return 0;
    }
    void send_binary(std::string data){
        OPacket op;
        op.head = packet_head;
        op.type = (int)packet_type::send_binary;
        op.write_data(data);
        client << op;
    }

    void handle_packet(IPacket packet){
        if(!check_head(packet.head))
            return;
        std::cout << "get packet :" << (int)packet.type << std::endl;
        switch(packet.type){
        case (int)packet_type::heart_beat:{
            clipack();
        }break;
        case (int)packet_type::heart_beat_back:{
            uint64_t t = packet.read<uint64_t>();
            delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - (process_start_time+std::chrono::milliseconds(t)));
            heart_beat_wait_times = 0;
        }break;
        case (int)packet_type::register_connect:{
            heart_beat_thread = std::jthread([this](const std::stop_token token){
                std::cout << "Heart beat start" << std::endl;
                heart_beating = 1;
                this->heart_beat_proc(token);
                heart_beating = 0;
            });
            OPacket op;
            op.head = packet_head;
            op.type = (int)packet_type::register_connect_back;
            client << op;
        }break;
        case (int)packet_type::register_connect_back:{
            clipack();
        }break;
        case (int)packet_type::disconnect:{
            int code = packet.read<int>();
            logger << "========\n";
            logger << client.remote_endpoint().address() << " request to disconnect." << std::endl;
            logger << "Code: "<< code << std::endl;
            sock_error(logger,(sock_errno)code);
            alive = 0;
        }break;
        case (int)packet_type::send_binary:{
            std::string data = packet.read_data();
            std::cout << data << std::endl;
            data_proc((char*)data.c_str(),data.size());
            std::cout << "bbb" << std::endl;
            OPacket op;
            op.head = packet_head;
            op.type = (int)packet_type::received_binary;
            client << op;
        }break;
        case (int)packet_type::received_binary:{

        }break;
        }
    }
    void run(const std::stop_token token){
        std::cout << "Run" << std::endl;
        start_check = std::jthread([this](std::stop_token token){
            std::this_thread::sleep_for(2*timeout);
            if(token.stop_requested()){
                start_checked = 1;
                return;
            }
            if(!heart_beat_thread.joinable()){
                end();
            }
            start_checked = 1;
        });
        while(!token.stop_requested()&&alive){
            IPacket p;
            try{
                client >> p;
            }catch(std::exception& e){
                logger << e.what() << std::endl;
            }
            handle_packet(std::move(p));
        }
    }
};
class Client_Connect{
    std::atomic<bool> blocking = 0;
    std::atomic<bool> alive = 1;
    const std::chrono::system_clock::time_point connect_start_time;
public:
    Client_Connect(boost::asio::ip::tcp::socket& server):server(server),connect_start_time(std::chrono::system_clock::now()){}
    bool heart_beating = 0;
    const std::chrono::seconds timeout = std::chrono::seconds(10);
    boost::asio::ip::tcp::socket& server;
    std::jthread start_check;

    bool is_blocking(){
        return blocking;
    }
    void end(){
        alive = 0;
    }
    void serpack(){
        OPacket op;
        op.head = packet_head;
        op.type = (int)packet_type::disconnect;
        op.write<int>((int)sock_errno::serpack);
        server << op;
        alive = 0;
    }
    bool check_head(int head){
        if(head == packet_head)
            return 1;
        alive = 0;
        return 0;
    }
    void register_connect(){
        OPacket op;
        op.head = packet_head;
        op.type = (int)packet_type::register_connect;
        server << op;
        start_check = std::jthread([this](std::stop_token token){
            std::this_thread::sleep_for(timeout);
            if(token.stop_requested())
                return;
            if(!heart_beating){
                end();
            }
        });
    }
    std::function<void(char*,int)> data_proc = [](char*,int){};
    void handle_packet(IPacket packet){
        if(!check_head(packet.head))
            return;
        switch(packet.type){
        case (int)packet_type::heart_beat:{
            std::cout << "Heart Beat" << std::endl;
            heart_beating = 1;
            uint64_t a = packet.read<uint64_t>();
            OPacket op;
            op.head = packet_head;
            op.type = (int)packet_type::heart_beat_back;
            op.write<uint64_t>(a);
            server << op;
        }break;
        case (int)packet_type::heart_beat_back:{
            serpack();
        }break;
        case (int)packet_type::register_connect:{
            serpack();
        }break;
        case (int)packet_type::register_connect_back:{

        }break;
        case (int)packet_type::disconnect:{
            int code = packet.read<int>();
            logger << "========\n";
            logger << server.remote_endpoint().address() << " request to disconnect." << std::endl;
            logger << "Code: "<< code << std::endl;
            sock_error(logger,(sock_errno)code);
            alive = 0;
        }break;
        case (int)packet_type::send_binary:{
            std::string data = packet.read_data();
            std::cout << data << std::endl;
            data_proc((char*)data.c_str(),data.size());
            std::cout << "ccc" << std::endl;
            OPacket op;
            op.head = packet_head;
            op.type = (int)packet_type::received_binary;
            server << op;
        }break;
        case (int)packet_type::received_binary:{
            
        }break;
        }
    }
    void run(const std::stop_token token){
        while(!token.stop_requested()&&alive){
            IPacket p;
            try{
                server >> p;
            }catch(std::exception& e){
                logger << e.what() << std::endl;
            }
            handle_packet(std::move(p));
        }
    }
};
class Server;
class p_connect_server{
    sock_id id = -1;
    boost::asio::ip::tcp::socket* sock = nullptr;
    Server_Connect* connect = nullptr;
    std::jthread thread;
    const std::chrono::system_clock::time_point start_time;
    bool running = 0;
public:
    p_connect_server(const p_connect_server& p) = delete;
    p_connect_server(p_connect_server&& p):thread(std::move(p.thread)),start_time(start_time){
        id = p.id;
        sock = p.sock;
        connect = p.connect;
        running = p.running;
        p.id = -1;
        p.sock = nullptr;
        p.connect = nullptr;
        p.running = 0;
    }
    p_connect_server(sock_id id,const std::chrono::system_clock::time_point start_time):id(id),sock(new boost::asio::ip::tcp::socket(sock_ioc)),start_time(start_time){
        connect = new Server_Connect(id,*sock,start_time);
    }
    boost::asio::ip::tcp::socket& get_sock(){
        return *sock;
    }
    Server_Connect& get_connect(){
        return *connect;
    }
    void run(std::function<void(sock_id)> freeRes){
        running = 1;
        thread = std::jthread([this,freeRes](const std::stop_token token){
            connect->run(token);
            freeRes(id);
        });
    }
    ~p_connect_server(){
        if(id == -1)
            return;
        if(!running){
            goto DELETE_START;
        }

        using namespace std::chrono_literals;
        connect->end();
        thread.request_stop();
        thread.detach();
        std::this_thread::sleep_for(1ms);
    DELETE_START:
        delete connect;
        try{
            sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            sock->cancel();
            sock->close();
        }catch(std::exception& e){
            sock->close();
            logger << e.what() << std::endl;
        }
        delete sock;
    }
};
class Server{

    static unsigned long long sock_id_counter;
    std::mutex connects_lock;
    std::map<sock_id,p_connect_server> p_connects;
    int port;
    bool run = 1;

    const std::chrono::system_clock::time_point start_time;
public:
    int connect_count(){
        return p_connects.size();
    }
    void each_connect(std::function<void(Server_Connect&)> handler){
        std::for_each(std::execution::par_unseq,p_connects.begin(),p_connects.end(),[handler](decltype(*p_connects.begin())& item){
            handler(item.second.get_connect());
        });
    }
    void use_connect(sock_id id,std::function<void(Server_Connect&)> handler){
        handler(p_connects.at(id).get_connect());
    }
    void each_sock(std::function<void(boost::asio::ip::tcp::socket&)> handler){
        std::for_each(std::execution::par_unseq,p_connects.begin(),p_connects.end(),[handler](decltype(*p_connects.begin())& item){
            handler(item.second.get_sock());
        });
    }
    void use_sock(sock_id id,std::function<void(boost::asio::ip::tcp::socket&)> handler){
        handler(p_connects.at(id).get_sock());
    }
    void close_connect(sock_id id){
        connects_lock.lock();
        p_connects.erase(id);
        connects_lock.unlock();
    }
    void close_all_connect(){
        connects_lock.lock();
        p_connects.clear();
        connects_lock.unlock();
    }
    Server(int port = 7021):port(port),start_time(std::chrono::system_clock::now()){}
    void accept(std::stop_token token){
        while(!token.stop_requested()){
            sock_id id = sock_id_counter;
            ++sock_id_counter;
            //auto p_sock = new boost::asio::ip::tcp::socket(sock_ioc);
            connects_lock.lock();
            p_connects.emplace(id,p_connect_server(id,start_time));//At there, p_connect_server must be build
            connects_lock.unlock();
            auto& connect = p_connects.at(id);
            //connect.get_sock().open(boost::asio::ip::tcp::v4());
            boost::asio::ip::tcp::acceptor acptr(sock_ioc,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port),1);
            try{
                acptr.accept(connect.get_sock());
            }catch(std::exception& e){
                logger << e.what() << std::endl;
            }
            if(!run)
                return;
            logger << "Connect: " << connect.get_sock().remote_endpoint().address() << "\nID: " << id << std::endl;
            connect.run([&](sock_id id){
                close_connect(id);
            });
        }
    }
    ~Server(){
        run = 0;
        // std::vector<sock_id> collect;
        // collect.reserve(p_connects.size());
        // for(auto& a : p_connects){
        //     collect.push_back(a.first);
        // }
        // std::for_each(std::execution::par_unseq,collect.begin(),collect.end(),[this](sock_id id){
        //     close_connect(id);
        // });
        close_all_connect();
        

        boost::asio::ip::tcp::socket sock(sock_ioc);
        boost::asio::ip::tcp::endpoint point(boost::asio::ip::address::from_string("127.0.0.1"),port);
        sock.connect(point);
        try{
            sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            sock.cancel();
            sock.close();
        }catch(std::exception& e){
            logger << e.what() << std::endl;
            sock.close();
            
        }
    }
};
class Client{
    bool valued = 0;
    std::unique_ptr<boost::asio::ip::tcp::socket> server;
    std::unique_ptr<Client_Connect> p_connect;
    std::string ip = "127.0.0.1";
    int port = 7021;
    std::jthread thread;
public:
    Client(){}
    bool connect(std::string ip,int port = 7021){
        server.reset(new boost::asio::ip::tcp::socket(sock_ioc));
        p_connect.reset(new Client_Connect(*server));
        valued = 1;
        this->ip = ip;
        this->port = port;
        try{
            server->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip),port));
            p_connect->register_connect();
        }catch(std::exception& e){
            logger << e.what() << std::endl;
            return 0;
        }
        return 1;
    }
    void use_connect(std::function<void(Client_Connect&)> handler){
        handler(*p_connect);
    }
    void close_connect(){
        using namespace std::chrono_literals;
        thread.request_stop();
        std::this_thread::sleep_for(1ms);
        try{
            server->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            server->cancel();
            server->close();
        }catch(std::exception& e){
            server->close();
            logger << e.what() << std::endl;
        }
        valued = 0;
    }
    void run(){
        thread = std::jthread([&](const std::stop_token token){
            p_connect->run(token);
        });
    }
};


#endif // !GAME_SOCK_SERVER