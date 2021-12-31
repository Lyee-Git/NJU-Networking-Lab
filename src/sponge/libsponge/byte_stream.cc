#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    size_t write_len = data.length();
    if(write_len > _capacity - _buffer.size())
        write_len = _capacity - _buffer.size();
    _buffer.append(BufferList(move(string().assign(data.begin(), data.begin() + write_len))));
    write_bytes += write_len;
    return write_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t peek_len = len;
    if(peek_len > _buffer.size())
        peek_len = _buffer.size();
    string res = _buffer.concatenate();
    return string().assign(res.begin(), res.begin() + peek_len);;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
     size_t pop_len = len;
     if(pop_len > _buffer.size())
        pop_len = _buffer.size();
    read_bytes += pop_len;
    _buffer.remove_prefix(pop_len);
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t read_len = len > _buffer.size() ? _buffer.size() : len;
    string res = peek_output(read_len);
    pop_output(read_len);//A combination of peek & pop
    return res;
}

void ByteStream::end_input() {_input_end = true; }

bool ByteStream::input_ended() const { return _input_end; }

size_t ByteStream::buffer_size() const { return _buffer.size(); }

bool ByteStream::buffer_empty() const { return _buffer.size() == 0; }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); } //"true" if the output has reached the ending

size_t ByteStream::bytes_written() const { return write_bytes; }

size_t ByteStream::bytes_read() const { return read_bytes; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
