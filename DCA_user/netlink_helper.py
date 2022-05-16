import os
import socket
import struct


# types
NLMSG_NOOP = 1
NLMSG_ERROR = 2
NLMSG_DONE = 3
NLMSG_OVERRUN = 4
MSG_SETCFG = 11
MSG_GETCFG = 12
NLMSG_MIN_TYPE = 0x10

# flags
NLM_F_REQUEST = 1
NLM_F_MULTI = 2
NLM_F_ACK = 4
NLM_F_ECHO = 8


class Message:
    def __init__(self, msg_type, flags=0, seq=-1, payload=None):
        self.type = msg_type
        self.flags = flags
        self.seq = seq
        self.pid = 1
        payload = payload or []
        if isinstance(payload, list):
            contents = []
            for attr in payload:
                contents.append(attr._dump())
            self.payload = b''.join(contents)
        else:
            self.payload = payload

    def send(self, conn):
        if self.seq == -1:
            self.seq = conn.seq()

        self.pid = conn.pid
        length = len(self.payload)

        hdr = struct.pack("IHHII", length + 4 * 4, self.type,
                          self.flags, self.seq, self.pid)
        conn.send(hdr + bytes(self.payload, 'utf-8'))


class Connection(object):
    """
    Object representing Netlink socket connection to the kernel.
    """

    def __init__(self, nlservice=25, groups=0):
        # nlservice = Netlink IP service
        self.fd = socket.socket(socket.AF_NETLINK, socket.SOCK_RAW, nlservice)
        self.fd.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)
        self.fd.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
        self.fd.bind((0, groups))  # pid=0 lets kernel assign socket PID
        self.pid, self.groups = self.fd.getsockname()
        self.pid = os.getpid()
        self._seq = 0

    def send(self, msg):
        if isinstance(msg, Message):
            if msg.seq == -1:
                msg.seq = self.seq()
            #msg.seq = 1
            msg.pid = self.pid
            length = len(msg.payload)
            hdr = struct.pack("IHHII", length + 4 * 4, msg.type,
                              msg.flags, msg.seq, msg.pid)
            msg = hdr + msg.payload.encode('utf-8')
            return self.fd.send(msg)

    def recve(self):
        #data, (nlpid, nlgrps) =  self.fd.recvfrom(16384)
        data = self.fd.recv(16384)
        msglen, msg_type, flags, seq, pid = struct.unpack("IHHII", data[:16])
        msg = Message(msg_type, flags, seq, data[16:])
        msg.pid = pid
        # if msg_type == NLMSG_DONE:
        # print("payload :", msg.payload)
        # print("msg.pid :", msg.pid)
        # print("msg.seq :", msg.seq)
        if msg.type == NLMSG_ERROR:
            errno = -struct.unpack("i", msg.payload[:4])[0]
            if errno != 0:
                err = OSError("Netlink error: %s (%d)" %
                              (os.strerror(errno), errno))
                err.errno = errno
                # print("err :",err)
                raise err

        # return msg.payload
        return msg

    def seq(self):
        self._seq += 1
        return self._seq
