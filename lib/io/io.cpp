#include "../../src/runtime.hpp"

#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <string>
#include <memory>
#include <exception>
#include <thread>

// lets hope all this C stuff can once be gone
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * In C++, copy constructors for streams are usually undefined
 * which makes a simple solution of just assigning or passing
 * iostream derived classes unworkable.
 *
 * These channel classes flatten C++ i/o streams hierarchy into
 * one channel class with all operations and derived classes 
 * which implement them.
 **/

class Unsupported : public std::exception {
public:
    Unsupported() :
        _message("")  {
    }

    Unsupported(const UnicodeString &m) :
        _message(m)  {
    }

    ~Unsupported() {
    }

    UnicodeString message() const {
        return _message;
    }

    friend std::ostream & operator<<(std::ostream &os, const Unsupported &e) {
        os << "unsupported(" << e.message() << ")";
        return os;
    }

private:
    UnicodeString   _message;
};

typedef enum {
    CHANNEL_STREAM_IN,
    CHANNEL_STREAM_OUT,
    CHANNEL_STREAM_ERR,
    CHANNEL_FILE,
    CHANNEL_FD,
} channel_tag_t;

class Channel;
typedef std::shared_ptr<Channel> ChannelPtr;

class Channel {
public:
    Channel(channel_tag_t t): _tag(t) {
    }

    channel_tag_t tag() {
        return _tag;
    }

    virtual UnicodeString read() {
        throw Unsupported();
    }

    virtual UChar32 read_char() { // XXX: wait for the libicu fix?
        throw Unsupported();
    }

    virtual UnicodeString read_line() { // implemented otherwise
        throw Unsupported();
    }

    virtual void write(const UnicodeString& n) {
        throw Unsupported();
    }

    virtual void write_char(const UChar32& n) {
        throw Unsupported();
    }

    virtual void write_line(const UnicodeString& n) {
        throw Unsupported();
    }

    virtual void close() {
    }

    virtual void flush() {
    }

    virtual bool eof() {
        return false;
    }

protected:
    channel_tag_t   _tag;
};

class ChannelStreamIn: public Channel {
public:
    ChannelStreamIn(): Channel(CHANNEL_STREAM_IN) {
    }

    static ChannelPtr create() {
        return ChannelPtr(new ChannelStreamIn());
    }

    virtual UnicodeString read() override {
        UnicodeString s;
        std::cin >> s;
        return s;
    }

    virtual UnicodeString read_line() override {
        std::string str;
        std::getline(std::cin, str);
        return UnicodeString(str.c_str());
    }

    virtual bool eof() override {
        return std::cin.eof();
    }
};

class ChannelStreamOut: public Channel {
public:
    ChannelStreamOut(): Channel(CHANNEL_STREAM_OUT) {

    }

    static ChannelPtr create() {
        return ChannelPtr(new ChannelStreamOut());
    }

    virtual void write(const UnicodeString& s) override {
        std::cout << s;
    }

    virtual void write_line(const UnicodeString& s) override {
        std::cout << s << std::endl;
    }

    virtual void flush() override {
        std::cout.flush();
    }
};

class ChannelStreamErr: public Channel {
public:
    ChannelStreamErr(): Channel(CHANNEL_STREAM_ERR) {
    }

    static ChannelPtr create() {
        return ChannelPtr(new ChannelStreamErr());
    }

    virtual void write(const UnicodeString& s) override {
        std::cerr << s;
    }

    virtual void write_line(const UnicodeString& s) override {
        std::cerr << s << std::endl;
    }

    virtual void flush() override {
        std::cerr.flush();
    }
};

class ChannelFile: public Channel {
public:
    ChannelFile(const UnicodeString& fn): Channel(CHANNEL_FILE), _fn(fn) {
        std::string utf8;
        fn.toUTF8String(utf8);
        _stream = std::fstream(utf8.c_str()); // unsafe due to NUL
    }

    static ChannelPtr create(const UnicodeString& fn) {
        return ChannelPtr(new ChannelFile(fn));
    }

    virtual UnicodeString read() override {
        UnicodeString str;
        _stream >> str;
        return str;
    }

    virtual UnicodeString read_line() override {
        std::string str;
        std::getline(_stream, str);
        return UnicodeString(str.c_str());
    }

    virtual void write(const UnicodeString& s) override {
        _stream << s;
    }

    virtual void write_line(const UnicodeString& s) override {
        _stream << s << std::endl;
    }

    virtual void close() override {
        _stream.close();
    }

    virtual void flush() override {
        _stream.flush();
    }

    virtual bool eof() override {
        return _stream.eof();
    }
protected:
    UnicodeString   _fn;
    std::fstream    _stream;
};

// Convenience class for file descriptors from _sockets_.
// should be removed as soon as C++ adds stream io on them.
class ChannelFD: public Channel {
public:
    ChannelFD(const int fd): Channel(CHANNEL_FD), _fd(fd) {
    }

    static ChannelPtr create(const int fd) {
        return ChannelPtr(new ChannelFD(fd));
    }

    UnicodeString read_line() override {
        const int CHUNK_SIZE = 1024;
        char* str = (char*) malloc(CHUNK_SIZE);
        int allocated = CHUNK_SIZE; 
        int count = 0;

        _eof = false;

        // XXX: replace this code once with buffered writes
        // read to a char* one byte at a time until '\n'
        int n = 0;
        char ch;
        do {
            n = ::read(_fd, (void*) &ch, (size_t) 1);
            if (n < 0) { // this signals an error
                throw "error in read";
            } else if (n == 0) { // this signals EOF
                _eof = true;
            } else { // n == 1
                if (ch == '\n') {
                } else {
                    str[count] = ch;
                    count++;
                    if (count >= allocated) {
                        str = (char*) realloc(str, allocated + CHUNK_SIZE);
                        allocated += CHUNK_SIZE;
                    }
                }
            }
        } while ((n > 0) && (ch != '\n'));

        auto s = UnicodeString::fromUTF8(StringPiece(str, count));
        delete str;
        return s;
    }

    void write_byte(const char c) {
        char ch;
        ch = c;
        int n = 0;
        do {
            n = ::write(_fd, &ch, 1); // always remember write can fail, folks
            if (n == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50)); // XXX
            }
        } while (n == 0); 
        if (n < 0) {
            throw "error in write";
        }
    }

    void write_line(const UnicodeString& s) override {
        std::string utf8;
        s.toUTF8String(utf8);

        auto len = utf8.length();
        auto str = utf8.c_str();

        for (long unsigned int i = 0; i < len; i++) {
            write_byte(str[i]);
        }

        write_byte('\n');
    }

    void close() override {
        ::close(_fd);
    }

    void flush() override {
    }

    bool eof() override {
        return _eof;
    }
    
protected:
    int  _fd;
    bool _eof;
};


/**
 * Egel's primitive input/output combinators.
 *
 * Unstable and not all combinators fully provide what their 
 * specification promises. Neither are all combinators implemented
 * according to spec.
 **/

// IO.channel
// Values which are input/output channels
class ChannelValue: public Opaque {
public:
    OPAQUE_PREAMBLE(ChannelValue, "IO", "channel");

    ChannelValue(const ChannelValue& chan): Opaque(chan.machine(), chan.symbol()) {
        _value = chan.value();
    }

    VMObjectPtr clone() const override {
        return VMObjectPtr(new ChannelValue(*this));
    }

    int compare(const VMObjectPtr& o) override {
        auto v = (std::static_pointer_cast<ChannelValue>(o))->value();
        if (_value < v) return -1;
        else if (v < _value) return 1;
        else return 0;
    }

    void set_value(ChannelPtr cp) {
        _value = cp;
    }

    ChannelPtr value() const {
        return _value;
    }

protected:
    ChannelPtr _value;
};

#define CHANNEL_TEST(o, sym) \
    ((o->tag() == VM_OBJECT_OPAQUE) && \
     (VM_OBJECT_OPAQUE_SYMBOL(o) == sym))
#define CHANNEL_VALUE(o) \
    ((std::static_pointer_cast<ChannelValue>(o))->value())

// IO.cin
// Standard input.
class Stdin: public Medadic {
public:
    MEDADIC_PREAMBLE(Stdin, "IO", "stdin");

    VMObjectPtr apply() const override {
        auto cin = ChannelStreamIn::create();
        auto in  = ChannelValue(machine());
        in.set_value(cin);
        return in.clone();
    }
};

// IO.stdout
// Standard output.
class Stdout: public Medadic {
public:
    MEDADIC_PREAMBLE(Stdout, "IO", "stdout");

    VMObjectPtr apply() const override {
        auto cout = ChannelStreamOut::create();
        auto out  = ChannelValue(machine());
        out.set_value(cout);
        return out.clone();
    }
};

// IO.stderr
// Standard error.
class Stderr: public Medadic {
public:
    MEDADIC_PREAMBLE(Stderr, "IO", "stderr");

    VMObjectPtr apply() const override {
        auto cerr = ChannelStreamErr::create();
        auto err  = ChannelValue(machine());
        err.set_value(cerr);
        return err.clone();
    }
};


// IO.open s
// Open the named file, and return a new channel on
// that file, positionned at the beginning of the file. The file is
// truncated to zero length if it already exists. It is created if it
// does not already exists. 

class Open: public Monadic {
public:
    MONADIC_PREAMBLE(Open, "IO", "open");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {
        if (arg0->tag() == VM_OBJECT_TEXT) {
            auto fn = VM_OBJECT_TEXT_VALUE(arg0);
            auto stream = ChannelFile::create(fn);
            auto channel  = ChannelValue(machine());
            channel.set_value(stream);
            return channel.clone();
        } else {
            return nullptr;
        }
    }
};

// IO.close channel
// Close the given channel. Anything can happen if any of the
// functions above is called on a closed channel.

class Close: public Monadic {
public:
    MONADIC_PREAMBLE(Close, "IO", "close");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {
        static symbol_t sym = 0;
        if (sym == 0) sym = machine()->enter_symbol("IO", "channel");

        if (CHANNEL_TEST(arg0, sym)) {
            auto chan = CHANNEL_VALUE(arg0);
            chan->close();
            return create_nop();
        } else {
            return nullptr;
        }
    }
};

// IO.read channel
// Read a string from a channel.

class Read: public Monadic {
public:
    MONADIC_PREAMBLE(Read, "IO", "read");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {
        static symbol_t sym = 0;
        if (sym == 0) sym = machine()->enter_symbol("IO", "channel");

        if (CHANNEL_TEST(arg0, sym)) {
            auto chan = CHANNEL_VALUE(arg0);
            UnicodeString str = chan->read();
            return create_text(str);
        } else {
            return nullptr;
        }
    }
};

// IO.read_line channel
// Read a string from a channel.

class ReadLine: public Monadic {
public:
    MONADIC_PREAMBLE(ReadLine, "IO", "read_line");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {
        static symbol_t sym = 0;
        if (sym == 0) sym = machine()->enter_symbol("IO", "channel");

        if (CHANNEL_TEST(arg0, sym)) {
            auto chan = CHANNEL_VALUE(arg0);
            UnicodeString str = chan->read_line();
            return create_text(str);
        } else {
            return nullptr;
        }
    }
};

// IO.write chan o
// Write the string s to channel chan.

class Write: public Dyadic {
public:
    DYADIC_PREAMBLE(Write, "IO", "write");

    VMObjectPtr apply(const VMObjectPtr& arg0, const VMObjectPtr& arg1) const override {
        static symbol_t sym = 0;
        if (sym == 0) sym = machine()->enter_symbol("IO", "channel");

        if (CHANNEL_TEST(arg0, sym)) {
            auto chan = CHANNEL_VALUE(arg0);
            if (arg1->tag() == VM_OBJECT_TEXT) {
                auto s = VM_OBJECT_TEXT_VALUE(arg1);
                chan->write(s);
                return create_nop();
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }
};

class WriteLine: public Dyadic {
public:
    DYADIC_PREAMBLE(WriteLine, "IO", "write_line");

    VMObjectPtr apply(const VMObjectPtr& arg0, const VMObjectPtr& arg1) const override {
        static symbol_t sym = 0;
        if (sym == 0) sym = machine()->enter_symbol("IO", "channel");

        if (CHANNEL_TEST(arg0, sym)) {
            auto chan = CHANNEL_VALUE(arg0);
            if (arg1->tag() == VM_OBJECT_TEXT) {
                auto s = VM_OBJECT_TEXT_VALUE(arg1);
                chan->write_line(s);
                return create_nop();
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }
};

// IO.flush channel
// Flush the buffer associated with the given output channel, 
// performing all pending writes on that channel. Interactive programs
// must be careful about flushing std_out and std_err at the right time.

class Flush: public Monadic {
public:
    MONADIC_PREAMBLE(Flush, "IO", "flush");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {
        static symbol_t sym = 0;
        if (sym == 0) sym = machine()->enter_symbol("IO", "channel");

        if (CHANNEL_TEST(arg0, sym)) {
            auto chan = CHANNEL_VALUE(arg0);
            chan->flush();
            return create_nop();
        } else {
            return nullptr;
        }
    }
};

// IO.eof channel
// True if there is no more input, false otherwise.

class Eof: public Monadic {
public:
    MONADIC_PREAMBLE(Eof, "IO", "eof");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {
        static symbol_t sym = 0;
        if (sym == 0) sym = machine()->enter_symbol("IO", "channel");

        if (CHANNEL_TEST(arg0, sym)) {
            auto chan = CHANNEL_VALUE(arg0);
            return create_bool(chan->eof());
        } else {
            return nullptr;
        }
    }
};

// IO.print o0 .. on
// Print objects on standard output; don't escape characters or 
// strings when they are the argument. May recursively print large
// objects leading to stack explosion.

class Print: public Variadic {
public:
    VARIADIC_PREAMBLE(Print, "IO", "print");

    VMObjectPtr apply(const VMObjectPtrs& args) const override {

        UnicodeString s;
        for (auto& arg:args) {
            if (arg->tag() == VM_OBJECT_INTEGER) {
                s += arg->to_text();
            } else if (arg->tag() == VM_OBJECT_FLOAT) {
                s += arg->to_text();
            } else if (arg->tag() == VM_OBJECT_CHAR) {
                s += VM_OBJECT_CHAR_VALUE(arg);
            } else if (arg->tag() == VM_OBJECT_TEXT) {
                s += VM_OBJECT_TEXT_VALUE(arg);
            } else {
                return nullptr;
            }
        }
        std::cout << s;

        return create_nop();
    }
};

// IO.getline
// Read characters from standard input
// until a newline character is encountered. Return the string of all
// characters read, without the newline character at the end.

class Getline: public Medadic {
public:
    MEDADIC_PREAMBLE(Getline, "IO", "getline");

    VMObjectPtr apply() const override {
        std::string line;
        std::getline(std::cin, line);
        UnicodeString str(line.c_str());
        return create_text(str);
    }
};


// IO.exit n
// Flush all pending writes on stdout and stderr, and terminate the 
// process and return the status code to the operating system.
// (0 to indicate no errors, a small positive integer for failure.)

class Exit: public Monadic {
public:
    MONADIC_PREAMBLE(Exit, "IO", "exit");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {
        if (arg0->tag() == VM_OBJECT_INTEGER) {
            auto i = VM_OBJECT_INTEGER_VALUE(arg0);
            // XXX: uh.. flushall, or something?
            exit(i);
            // play nice
            return create_nop();
        } else {
            return nullptr;
        }
    }
};

//////////////////////////////////////////////////////////////////////
// Highly unstable and experimental client/server code.

class ServerObject: public Opaque {
public:
    OPAQUE_PREAMBLE(ServerObject, "IO", "serverobject");

    ServerObject(const ServerObject& so): Opaque(so.machine(), so.symbol()) {
        memcpy( (char*) &_server_address,  (char *) &so._server_address, sizeof(_server_address));
        _portno = so._portno;
        _queue = so._queue;
        _sockfd = so._sockfd;
    }

    VMObjectPtr clone() const override {
        return VMObjectPtr(new ServerObject(*this));
    }

    int compare(const VMObjectPtr& o) override {
        // XXX: not the foggiest idea whether this words.
        // I assume file descriptors are unique.
        auto v = (std::static_pointer_cast<ServerObject>(o));
        if (_sockfd < v->_sockfd) {
            return -1;
        } else if (_sockfd > v->_sockfd) {
            return 1;
        } else {
            return 0;
        }
    }

    void bind(int port, int in) {
        _portno = port;
        _queue = in;
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0) {
            throw VMObjectText::create("error opening socket");
        }

        bzero((char *) &_server_address, sizeof(_server_address));
        _server_address.sin_family = AF_INET;
        _server_address.sin_addr.s_addr = INADDR_ANY;
        _server_address.sin_port = htons(_portno);
        if (::bind(_sockfd, (struct sockaddr *) &_server_address,
                 sizeof(_server_address)) < 0) {
            throw VMObjectText::create("error on bind");
        }
        listen(_sockfd,_queue);
    }

    VMObjectPtr accept() {
        struct sockaddr_in address;
        socklen_t n = sizeof(address); // XXX: What is the supposed lifetime of this?
        int fd = ::accept(_sockfd,
                     (struct sockaddr *) &address,
                     &n);
        if (fd < 0) {
            throw VMObjectText::create("error opening socket");
        }
        auto cn = ChannelFD::create(fd);
        auto c  = ChannelValue(machine());
        c.set_value(cn);
        return c.clone();
    }

protected:
    struct sockaddr_in _server_address = {0};
    int _portno = 0;
    int _queue = 0;
    int _sockfd = 0;
};

#define SERVER_OBJECT_TEST(o, sym) \
    ((o->tag() == VM_OBJECT_OPAQUE) && \
     (VM_OBJECT_OPAQUE_SYMBOL(o) == sym))
#define SERVER_OBJECT_CAST(o) \
    (std::static_pointer_cast<ServerObject>(o))

// IO.accept serverobject
class Accept: public Monadic {
public:
    MONADIC_PREAMBLE(Accept, "IO", "accept");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {
        static symbol_t sym = 0;
        if (sym == 0) sym = machine()->enter_symbol("IO", "serverobject");

        if (SERVER_OBJECT_TEST(arg0, sym)) {
            auto so = SERVER_OBJECT_CAST(arg0);
            auto chan = so->accept();
            return chan;
        } else {
            return nullptr;
        }
    }
};

// IO.server port in
class Server: public Dyadic {
public:
    DYADIC_PREAMBLE(Server, "IO", "server");

    VMObjectPtr apply(const VMObjectPtr& arg0, const VMObjectPtr& arg1) const override {
        if ( (arg0->tag() == VM_OBJECT_INTEGER) && (arg1->tag() == VM_OBJECT_INTEGER) ) {
            auto port = VM_OBJECT_INTEGER_VALUE(arg0);
            auto in   = VM_OBJECT_INTEGER_VALUE(arg1);

            auto so = ServerObject(machine());
            so.bind(port, in);

            return so.clone();
        } else {
            return nullptr;
        }
    }
};

// IO.client host port
class Client: public Dyadic {
public:
    DYADIC_PREAMBLE(Client, "IO", "client");

    VMObjectPtr apply(const VMObjectPtr& arg0, const VMObjectPtr& arg1) const override {
        if ( (arg0->tag() == VM_OBJECT_TEXT) && (arg1->tag() == VM_OBJECT_INTEGER) ) {
            auto host = VM_OBJECT_TEXT_VALUE(arg0);
            auto port = VM_OBJECT_INTEGER_VALUE(arg1);

            struct sockaddr_in server_address;
            int sockfd;

            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                throw VMObjectText::create("error opening socket");
            }

            bzero((char *) &server_address, sizeof(server_address));
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(port);

            std::string utf8;
            host.toUTF8String(utf8);
            // Convert IPv4 and IPv6 addresses from text to binary form
            if(::inet_pton(AF_INET, utf8.c_str(), &server_address.sin_addr)<=0) {
                throw VMObjectText::create("invalid address");
            }

            if (::connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
                throw VMObjectText::create("connection failed");
            }

            auto cn = ChannelFD::create(sockfd);
            auto c  = ChannelValue(machine());
            c.set_value(cn);
            return c.clone();

        } else {
            return nullptr;
        }
    }

};

extern "C" std::vector<UnicodeString> egel_imports() {
    return std::vector<UnicodeString>();
}

extern "C" std::vector<VMObjectPtr> egel_exports(VM* vm) {
    std::vector<VMObjectPtr> oo;

    oo.push_back(VMObjectData(vm, "IO", "channel").clone());

    oo.push_back(ChannelValue(vm).clone());
    oo.push_back(Stdin(vm).clone());
    oo.push_back(Stdout(vm).clone());
    oo.push_back(Stderr(vm).clone());
    oo.push_back(Open(vm).clone());
    oo.push_back(Close(vm).clone());
    oo.push_back(Read(vm).clone());
    oo.push_back(ReadLine(vm).clone());
    oo.push_back(Write(vm).clone());
    oo.push_back(WriteLine(vm).clone());
    oo.push_back(Flush(vm).clone());
    oo.push_back(Eof(vm).clone());
    oo.push_back(Print(vm).clone());
    oo.push_back(Exit(vm).clone());

// hacked TCP protocol
    oo.push_back(ServerObject(vm).clone());
    oo.push_back(Accept(vm).clone());
    oo.push_back(Server(vm).clone());
    oo.push_back(Client(vm).clone());

    return oo;
}
