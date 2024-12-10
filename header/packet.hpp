#ifndef PACKET_
#define PACKET_
#include<iostream>
#include<fstream>
#include<string>
#include<boost/asio.hpp>

bool IsLittleEndian();
extern std::ofstream packet_logger;
bool RefreshLogFile();
struct Packet{
protected:
	int bufSize = 1024;
	const int minBufSize = sizeof(head)+sizeof(type)+sizeof(size);
public:
	bool set_bufSize(int size){
		if(size <= minBufSize)
			return 0;
		bufSize = size;
		return 1;
	}
	int get_bufSize(){
		return bufSize;
	}
	struct PacketData{
		std::string data;
		struct DataIterator{
			PacketData& pData;
			int index;
			DataIterator (PacketData& pData,int index):pData(pData),index(index){}
			template<typename T>
			explicit operator T(){
				//if(index + sizeof(T)>=pData.data.size()){
				if(index + sizeof(T)>pData.data.size()){
					packet_logger << "Packet: Out of range" << std::endl;
					packet_logger << "Expect index: " << index + sizeof(T) << std::endl;
					packet_logger << "Packet size: " << pData.data.size() << std::endl;
					throw std::runtime_error("Packet: Out of range");

				}
				T result;
				
				if(IsLittleEndian()){
					for(char i = 0;i!= sizeof(T);++i)
						*((char*)(&result)+sizeof(T)-1-i) = pData.data[index+i];
				}else{
					for(char i = 0;i!= sizeof(T);++i)
						*((char*)(&result)+sizeof(T)+i) = pData.data[index+i];
				}
				//index += sizeof(T);
				return result;
			}
		};
		template<typename T>
		void push_back(const T t){
			if(IsLittleEndian()){
				for(char i = 0;i!=sizeof(t);++i)
					data.push_back(*((char*)(&t)+sizeof(t)-1-i));
			}else{
				for(char i = 0;i!=sizeof(t);++i)
					data.push_back(*((char*)(&t)+sizeof(t)+i));
			}
			
		}
		DataIterator operator[](int index);
	};
	int index = 0;
	int type = 0;
	int size = 0;
    int head = 0;
	PacketData pData;
	
};
struct IPacket :public Packet{
	template<typename T>
	T read(){
		T t = static_cast<T>(pData[index]);
		index += sizeof(t);
		return t;
	}
	std::string read();
	std::string read_data();
	friend void operator>>(boost::asio::ip::tcp::socket& sock,IPacket& p);
	IPacket(const std::string& str){
		pData.data = str;
	}
    IPacket(){
        pData.data = "";
    }
};
struct OPacket :public Packet{
	template<typename T>
	void write(const T t){
		pData.push_back(t);
		index += sizeof(t);
		size += sizeof(t);
	}
	//自带写入字符串大小
	void write(const char* str);
	//自带写入字符串大小
	void write(const std::string& str);
	//void write_Gzip(const std::string& key,const OPacket& data);
	void write_data(const char*,size_t);
	void write_data(const std::string&);
	friend void operator<<(boost::asio::ip::tcp::socket& sock,const OPacket& p);
	OPacket(){};
};
#endif //!PACKET_