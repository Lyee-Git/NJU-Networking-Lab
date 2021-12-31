#include "tcp_receiver.hh"

#include <optional>
#include <string>

// Dummy implementation of a TCP receiver

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // Listen: Wait for SYN
    if (!seg.header().syn && !_syn)
        return;
    else if (seg.header().syn) {
        if (_syn)
            return;  // already SYN_RECV, refuse other syn
        else         // syn: update _isn&abs_seqno
        {
            _syn = true;
            _isn = seg.header().seqno.raw_value();
        }
    }
    // handle fin
    if (seg.header().fin) {
        if (_fin)
            return;  // already FIN_RECV
        else
            _fin = true;
    }
    //use the index of the last reassembled byte as the checkpoint.
    //For the first segment, set stream_index 0.
    size_t abs_seqno = seg.header().syn ? 1 : unwrap(seg.header().seqno, WrappingInt32(_isn), _reassembler.cur_idx());
    size_t stream_index = abs_seqno - 1;
    std::string data = seg.payload().copy();
    _reassembler.push_substring(data, stream_index, seg.header().fin);
    _rcv_base = _reassembler.cur_idx() + 1;
    //handle fin
    if(_reassembler.input_ended() && _fin)
        _rcv_base++;
}

optional<WrappingInt32> TCPReceiver::ackno() const
{
    if(!_syn)
        return std::nullopt;
    else
    {
        return WrappingInt32(wrap(_rcv_base, WrappingInt32(_isn)));
    }
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
