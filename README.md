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

# SETTINGS_HEADER_TABLE_SIZE, if -1 then not included in frame SETTINGS  
SettingsHeaderTableSize  ? # octets  

# SETTINGS_INITIAL_WINDOW_SIZE, if -1 then not included in frame SETTINGS  
InitialWindowSize        ? # octets  

# SETTINGS_MAX_FRAME_SIZE, if -1 then not included in frame SETTINGS  
SettingsMaxFrameSize     ? # octets  
