#include "Utils.h"
#include "pb_encode.h"

std::vector<uint8_t> blockRead(int fd, size_t readSize)
{
	std::vector<uint8_t> buf(readSize);
	unsigned int idx = 0;
	ssize_t n;
    // printf("START READ\n");

	while (idx < readSize)
	{
		if ((n = recv(fd, &buf[idx], readSize - idx, 0)) <= 0)
		{
            printf("READ: 0 or something else %d\n ", n);
			return buf;
		}
		idx += n;
	}
    // printf("FINISH READ\n");
	return buf;
}
unsigned long long getCurrentTimestamp()
{
	unsigned long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	return now;
}

uint64_t hton64(uint64_t value) {
    int num = 42;
    if (*(char *)&num == 42) {
        uint32_t high_part = htonl((uint32_t)(value >> 32));
        uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));
        return (((uint64_t)low_part) << 32) | high_part;
    } else {
        return value;
    }
}

std::string bytesToHexString(std::vector<uint8_t>& v) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    std::vector<uint8_t>::const_iterator it;

    for (it = v.begin(); it != v.end(); it++) {
        ss << std::setw(2) << static_cast<unsigned>(*it);
    }

    return ss.str();
}

ssize_t blockWrite(int fd, std::vector<uint8_t> data)
{
	unsigned int idx = 0;
	ssize_t n;
    // printf("START WRITE\n");

	while (idx < data.size())
	{
		if ((n = send(fd, &data[idx], data.size() - idx < 64 ? data.size() - idx : 64, 0)) <= 0)
		{
            printf("WRITE: 0 or something else %d\n ", n);
			return data.size();
		}
		idx += n;
	}
    // printf("FINISH WRITE\n");

	return data.size();
}

unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}

std::string urlDecode(std::string str)
{
    std::string encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str[i];
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str[i];
        i++;
        code1=str[i];
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
    }
    
   return encodedString;
}