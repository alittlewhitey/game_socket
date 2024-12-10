#include"packet.hpp"

bool IsLittleEndian(){
    int a = 0x1234;
    char c = *(char*)&a;
    if(c == 0x34)
        return true;
    return false;
}

Packet::PacketData::DataIterator Packet::PacketData::operator[](int index){
	return DataIterator(*this,index);
}
std::string IPacket::read(){
	std::string s;
	for(int i = index;i!=size;++i){
		char c = read<char>();
		if(!c)
			break;
		s.push_back(c);
	}
	return s;
}
std::string IPacket::read_data(){
	int size = read<int>();
	std::string s;
	s.reserve(size);
	for(int i = 0;i!=size;++i){
		s.push_back(this->read<char>());
	}
	return s;
}
void OPacket::write(const char* str){
	write(std::string(str));
}
void OPacket::write(const std::string& str){
	for(char a : str){
		if(!a)
			break;
		this->write<char>(a);
	}
	this->write<char>('\0');
}


//自带写入大小
void OPacket::write_data(const char* data,size_t size){
	write<int>(size);
	for(int i =0;i!=size;++i){
		this->write<char>(data[i]);
	}
}
void OPacket::write_data(const std::string& data){
	write((int)data.size());
	for(auto a : data){
		this->write<char>(a);
	}
}
void operator<<(boost::asio::ip::tcp::socket& sock,const OPacket& p){
	try{	
		OPacket p2;
		p2.write<int>(p.head);
		p2.write<int>(p.type);
		p2.write<int>(p.size);
		for(int i = 0;i!=p.size;++i){  
			p2.write<char> (p.pData.data[i]);
		}
		if(sock.is_open()){
			sock.send(boost::asio::buffer(p2.pData.data));
		}
	}catch(boost::system::system_error e){
		std::cerr << __FILE__ << '\t' << __LINE__ << '\t' << e.what() << std::endl;
	}catch(std::exception& e){
		std::cerr << e.what() << std::endl;
	}
}
void operator>>(boost::asio::ip::tcp::socket& sock,IPacket& p){
    try{
		int bufSize = p.get_bufSize();
		unsigned char buffer[bufSize];
		for(auto a : buffer){
			a = '\0';
		}
        if(sock.is_open())
			sock.receive(boost::asio::buffer(buffer,bufSize));
        int head = 0,type = 0,size = 0;
        if(IsLittleEndian()){
            for(int i = 0;i!=4;++i){
                head |= static_cast<unsigned int>(buffer[3-i]) << 8*i;
                type |= static_cast<unsigned int>(buffer[7-i]) << 8*i;
                size |= static_cast<unsigned int>(buffer[11-i]) << 8*i;
            }
        }else{
            for(int i = 0;i!=4;++i){
                head |= static_cast<unsigned int>(buffer[i]) << 8*i;
                type |= static_cast<unsigned int>(buffer[4+i]) << 8*i;
                size |= static_cast<unsigned int>(buffer[8+i]) << 8*i;
            }
        }
		std::string str;
		str.resize(size);
		if(size <= bufSize-12){
			for(int i = 0; i!=size;++i){
				str[i]=buffer[i+12];
			}
		}else{
			int index = 0;
			for(int i = 0; i!=bufSize-12;++i){
				str[i]=buffer[i+12];
				++index;
			}
			while((index < size)&&sock.is_open()){
				size_t sz = sock.receive(boost::asio::buffer(buffer,bufSize));
				for(int i = 0; i!=sz;++i){
					str[index]=buffer[i];
					++index;
				}
			}
		}
        p.pData.data = str;
        p.head = head;
        p.type = type;
        p.size = size;
    }catch(boost::system::system_error& e){
		std::cerr << __FILE__ << '\t' << __LINE__ << '\t' << e.what() << std::endl;
	}catch(std::exception& e){
		std::cerr << e.what() << std::endl;
	}
}