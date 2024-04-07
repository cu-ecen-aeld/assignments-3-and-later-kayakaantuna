#! /bin/sh
# Description simple start stop deamon that starts or stops simpleserver example.
# Author Kaya Kaan Tuna <tunakayakaan@gmail.com>
# Ref: Coursera Week 4 Linux System Initialization Video

case "$1" in
    start)
        echo "Starting the aesdsocket server"
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
    ;;
    stop)
        echo "Stopping the aesdsocket server"
        start-stop-daemon -K -n aesdsocket
    ;;
    *)
        echo "Usage: $0 (start|stop)"
    exit 1
esac

exit 0