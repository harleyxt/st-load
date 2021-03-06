#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <stdlib.h>

#include <string>
#include <sstream>
#include <vector>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>
#include <htl_app_http_client.hpp>

#include <htl_app_m3u8_parser.hpp>

class String
{
private:
    string value;
public:
    String(string str = ""){
        value = str;
    }
public:
    String& set_str(string str){
        value = str;
        return *this;
    }
    int length(){
        return (int)value.length();
    }
    bool startswith(string key, String* left = NULL){
        size_t pos = value.find(key);
        
        if(pos == 0 && left != NULL){
            left->set_str(value.substr(pos + key.length()));
            left->strip();
        }
        
        return pos == 0;
    }
    bool endswith(string key, String* left = NULL){
        size_t pos = value.rfind(key);
        
        if(pos == value.length() - key.length() && left != NULL){
            left->set_str(value.substr(pos));
            left->strip();
        }
        
        return pos == value.length() - key.length();
    }
    String& strip(){
        while(value.length() > 0){
            if(startswith("\n")){
                value = value.substr(1);
                continue;
            }
            
            if(startswith("\r")){
                value = value.substr(1);
                continue;
            }
            
            if(startswith(" ")){
                value = value.substr(1);
                continue;
            }
            
            if(endswith("\n")){
                value = value.substr(0, value.length() - 1);
                continue;
            }
            
            if(endswith("\r")){
                value = value.substr(0, value.length() - 1);
                continue;
            }
            
            if(endswith(" ")){
                value = value.substr(0, value.length() - 1);
                continue;
            }

            break;
        }
        
        return *this;
    }
    string getline(){
        size_t pos = string::npos;
        
        if((pos = value.find("\n")) != string::npos){
            return value.substr(0, pos);
        }
        
        return value;
    }
    String& remove(int size){
        if(size >= (int)value.length()){
            value = "";
            return *this;
        }
        
        value = value.substr(size);
        return *this;
    }
    const char* c_str(){
        return value.c_str();
    }
};

HlsM3u8Parser::HlsM3u8Parser(){
}

HlsM3u8Parser::~HlsM3u8Parser(){
}

int HlsM3u8Parser::ParseM3u8Data(HttpUrl* url, string m3u8, vector<M3u8TS>& ts_objects, int& target_duration){
    int ret = ERROR_SUCCESS;
    
    String data(m3u8);
    
    // http://tools.ietf.org/html/draft-pantos-http-live-streaming-08#section-3.3.1
    // An Extended M3U file is distinguished from a basic M3U file by its
    // first line, which MUST be the tag #EXTM3U.
    if(!data.startswith("#EXTM3U")){
        ret = ERROR_HLS_INVALID;
        Error("invalid hls, #EXTM3U not found. ret=%d", ret);
        return ret;
    }
    
    String value;
    
    M3u8TS ts_object;
    while(data.length() > 0){
        String line;
        data.remove(line.set_str(data.strip().getline()).strip().length()).strip();

        // http://tools.ietf.org/html/draft-pantos-http-live-streaming-08#section-3.4.2
        // #EXT-X-TARGETDURATION:<s>
        // where s is an integer indicating the target duration in seconds.
        if(line.startswith("#EXT-X-TARGETDURATION:", &value)){
            target_duration = atoi(value.c_str());
            ts_object.duration = target_duration;
            continue;
        }
        
        // http://tools.ietf.org/html/draft-pantos-http-live-streaming-08#section-3.3.2
        // #EXTINF:<duration>,<title>
        // "duration" is an integer or floating-point number in decimal
        // positional notation that specifies the duration of the media segment
        // in seconds.  Durations that are reported as integers SHOULD be
        // rounded to the nearest integer.  Durations MUST be integers if the
        // protocol version of the Playlist file is less than 3.  The remainder
        // of the line following the comma is an optional human-readable
        // informative title of the media segment.
        // ignore others util EXTINF
        if(line.startswith("#EXTINF:", &value)){
            ts_object.duration = atof(value.c_str());
            continue;
        }
        
        if(!line.startswith("#")){
            ts_object.ts_url = url->Resolve(line.c_str());
            ts_objects.push_back(ts_object);
            continue;
        }
    }
    
    return ret;
}

