#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

// merge node b to node a and return the length of overlapping part between a&b.
// If they don't overlap, return value would be -1.
long StreamReassembler::merge_to(node &a, const node &b) {
    node dst1(0, ""), dst2(0, "");
    if (a.idx <= b.idx) {
        dst1 = a;
        dst2 = b;
    } else {
        dst1 = b;
        dst2 = a;
    }
    size_t end1 = dst1.idx + dst1.len, end2 = dst2.idx + dst2.len;
    if (end1 >= end2)  // dst2 within dst1
    {
        a = dst1;
        return dst2.len;
    } else if (end1 < dst2.idx)  // no intersection
    {
        return -1;
    } else  // intermediate case
    {
        a.idx = dst1.idx;
        a.data = dst1.data + dst2.data.substr(end1 - dst2.idx);
        a.len = a.data.length();
        return end1 - dst2.idx;
    }
}

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // handle boundary cases
    if(eof)
        _input_end_idx = index + data.length();
    size_t max_idx = _cur_idx - _output.buffer_size() + _capacity;
    if (index > max_idx)  // trespass limit
        return;
    if (index + data.length() <= _cur_idx) {
        if (_cur_idx >= _input_end_idx && empty())
            _output.end_input();
        return;
    }

    // remove offshore data from both left and right sides
    node block(index, data);
    if (block.idx + block.len > max_idx) {
        block.data = block.data.substr(0, max_idx - block.idx);
        block.len = block.data.length();
    }
    if (block.idx < _cur_idx) {
        size_t offset = _cur_idx - block.idx;
        block.idx = index + offset;
        block.data.assign(block.data.begin() + offset, block.data.end());
        block.len = block.data.length();
    }
    _unassembled_bytes += block.len;

    // merge substrings
    do {
        long merge_bytes = 0;
        //handle succ
        set<node>::iterator it = _data_buffer.lower_bound(block);
        for (; it != _data_buffer.end() && (merge_bytes = merge_to(block, *it)) >= 0;
             it = _data_buffer.lower_bound(block)) {
            _unassembled_bytes -= merge_bytes;
            _data_buffer.erase(it);
        }
        //handle prev
        if (it == _data_buffer.begin())
            break;
        it--;
        while ((merge_bytes = merge_to(block, *it)) >= 0)
        {
            _unassembled_bytes -= merge_bytes;
            _data_buffer.erase(it);
            it = _data_buffer.lower_bound(block);
            if(it == _data_buffer.begin())
                break;//break whenever a iteration out-of-range is possible
            it--;
        }
    } while (false);
    _data_buffer.insert(block);

    //write to stream ASAP & in order
    if(!_data_buffer.empty() && _data_buffer.begin()->idx == _cur_idx)
    {
        const node to_write = *_data_buffer.begin();
        size_t written_bytes = _output.write(to_write.data);
        _unassembled_bytes -= written_bytes;
        _cur_idx += written_bytes;
        _data_buffer.erase(to_write);
    }
    if (_cur_idx >= _input_end_idx && empty())
            _output.end_input();
    if (eof && empty())
            _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }

size_t StreamReassembler::cur_idx() const { return _cur_idx; }

bool StreamReassembler::input_ended() const { return stream_out().input_ended();}