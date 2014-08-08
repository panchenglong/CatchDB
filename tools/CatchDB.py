import sys
import socket

class CatchDB:
    def __init__(self, host, port):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        except socket.error, e:
            print "Error creating socket: %s" % e
            sys.exit(1)

        try:
            self.sock.connect((host, port))
        except socket.gaierror, e:
            print "Error connecting host: %s" % e
            self.sock.close()
            sys.exit(1)
            
    def __del__(self):
        self.sock.close()

    def process(self, msg):
        blocks = msg.split(' ')
        # cmd = blocks[0]
        # args = tuple(blocks[1:-1])
        # if hasattr(self, cmd):
        #     func = getattr(self, cmd)
        #     try:
        #         res = apply(func, args)
        #     except TypeError, e:
        #         return e
        #     return res
        # else:
        #     return 'unknown command'
        query = self._encode(blocks)

        try:
            # Send data
            sent = self.sock.sendall(query)
            # Look for the response

            data = self.sock.recv(4096)

        except socket.errno, e:
            print "Socket error: %s" % str(e)
        except Exception, e:
            print "Other exception: %s" % str(e)

        return '\n'.join(data.rstrip('\n').split('\n')[1::2])

    def qsize(self, name):
        pass

    def qfront(self, name):
        pass

    def qback(self, name):
        pass

    def qpush(self, name, item):
        return qpush_front(self, name, item)

    def qpush_front(self, name, item):
        pass

    def qpush_back(self, name, item):
        pass

    def qpop(self, name):
        return qpop_back(self, name)

    def qpop_front(self, name):
        pass

    def qpop_back(self, name):
        pass

    def qclear(self, name):
        pass

    def qlist(self, name):
        pass

    def qslice(self, name, begin, end):
        pass

    def qget(self, name, index):
        pass

    def _encode(self, blocks):
        query = []
        for b in blocks:
            query.append(''.join([str(len(b)), '\n', b, '\n']))
        query.append('\n')

        return ''.join(query)
