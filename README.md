# Client uses only HTTP2(h2) protocol

Written on C++ (version C++11) with using C functions.

Tested on OS: Debian, OpenBSD, FreeBSD

### Features:
* ALPN
* Methods: GET, POST, HEAD
* Stream flow control
* Dynamic Table of Header Fields

### Not supported:
* Stream prioritization
* Frames CONTINUATION

### Compiling and run:
Required libraries: OpenSSL or LibreSSL.  
cd http2_client/src/  
make clean  
make  

Edit configuration file: config.txt  
./http2_client  

### Configuration file:
RequestsPath            ../requests  
LogPath                 ../logs  

MaxConcurrentStreams    128  
Timeout                 ? #  second  
TimeoutPoll             ? #  millisecond  
UserAgent               anonymous  

MinWindowSize           ? #  octets 
MaxWindowSize           ? #  octets  

SettingsHeaderTableSize  ? # octets, Dynamic Table Size: -1 param SETTINGS_HEADER_TABLE_SIZE not including to frame SETTINGS  
InitialWindowSize        ? # octets, -1 param SETTINGS_INITIAL_WINDOW_SIZE not including to frame SETTINGS  
SettingsMaxFrameSize     ? # octets, -1 param SETTINGS_MAX_FRAME_SIZE not including to frame SETTINGS  
