#!/usr/bin/env python

import sys
import socket
import argparse
from CatchDB import CatchDB

commands = { 'zset' : 'zset name key score',
             'zget' : 'zget name key',
             'zsize' : 'zsize name',
             'zdel' : 'zdel name key',
             'ztopn' : 'ztopn name num\nDESCRIPTION: get top n key-score pairs with highest score',
             'zgetall' : 'zgetall name\nDESCRIPTION: get all key-score pairs of zset "name"',
}

def help():
    print 'help cmd\t\tshow usage of the cmd'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', action="store", dest="host", 
                        default='0.0.0.0', required=False)
    parser.add_argument('--port', action="store", dest="port", 
                        type=int, default=7777, required=False)

    given_args = parser.parse_args()
    host = given_args.host
    port = given_args.port

    db = CatchDB(host, port)

    while True:
        msg = raw_input('> ').strip()
        if (len(msg) == 0):
            continue
        elif (msg == 'quit' or msg == 'q'):
            break
        elif msg.startswith('help') or msg.startswith('?'):
            words = msg.split(' ')
            size = len(words)
            if size > 2:
                print 'Unrecognized Command. Perhaps you mean "help cmd" or "? cmd"'
            elif size == 1:
                help()
            else:
                print commands[words[1]]
        else:
            result = db.process(msg.strip())
            if not result:
                print 'connection cloesd by server'
                break
            print(result)

if __name__ == '__main__':
    main()
