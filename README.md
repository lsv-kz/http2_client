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

Edit configuration file config.txt  
./http2_client  

### Configuration file "config.txt":
<pre>RequestsPath            ../requests</pre>  
<pre>LogPath                 ../logs</pre>  

<pre>MaxConcurrentStreams    128</pre>  
<pre>Timeout                 ? #  second</pre>  
<pre>TimeoutPoll             ? #  millisecond</pre>  
<pre>UserAgent               anonymous</pre>  

<pre>MinWindowSize           ? #  octets</pre>  
<pre>MaxWindowSize           ? #  octets</pre>  

\# SETTINGS_HEADER_TABLE_SIZE, if -1 then not included in frame SETTINGS  
<pre>SettingsHeaderTableSize  ? # octets</pre>  

\# SETTINGS_INITIAL_WINDOW_SIZE, if -1 then not included in frame SETTINGS  
<pre>InitialWindowSize        ? # octets</pre>  

\# SETTINGS_MAX_FRAME_SIZE, if -1 then not included in frame SETTINGS  
<pre>SettingsMaxFrameSize     ? # octets</pre>  
