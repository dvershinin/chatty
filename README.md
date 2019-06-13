# chatty - a chatty tty.

chatty displays chatty messages in the title bar of your tty according to your input strings.

## Installation in CentOS 7

    yum install https://extras.getpagespeed.com/release-el7-latest.rpm
    yum install chatty
    
## Installation in other systems

    % make

or if your system is SVR4 system (Solaris etc.),

    % make CFLAGS=-DSVR4

## Usage

    % chatty /etc/chatty.conf (for CentOS 7)
    % chatty chatty-dict.ja (other systems)
    # (In the excuted shell, do whatever you want and exit)

Have fun!

## Authors

* Satoru Takabayashi <satoru@namazu.org>
