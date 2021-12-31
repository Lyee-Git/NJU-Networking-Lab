#include "tcp_sender.hh"
#include <iostream>
#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check_lab4`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _RTO{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window()
{
    //handle border cases
    //send seg with syn to make handshake
    if(!_syn_sent)
    {
        TCPSegment seg;
        _syn_sent = true;
        seg.header().syn = true;
        _segment_handler(seg);
        return;
    }
    //wait for syn acked
    if(!_outstanding_segments.empty() && _outstanding_segments.front().header().syn)
        return;
    //another border case
    if(_fin_sent)
        return;
    //if zero, act like the window size is one
    _receiver_window_size = _receiver_window_size == 0 ? 1 : _receiver_window_size;
    unsigned int _win_right_edge = _rcv_base + _receiver_window_size;
    if (_stream.eof() && _win_right_edge > _next_seqno)
    {
	    TCPSegment seg;
	    seg.header().fin = true;
	    _fin_sent = true;
	    _segment_handler(seg);
    }
    while((_free_bytes = _win_right_edge - _next_seqno) != 0 && !_fin_sent && !_stream.buffer_empty())
    {
        unsigned int payload_size = _free_bytes > TCPConfig::MAX_PAYLOAD_SIZE ? TCPConfig::MAX_PAYLOAD_SIZE : _free_bytes;
        TCPSegment seg;
        seg.payload() = _stream.read(payload_size);
        //handle fin sending if free space permitted
        if(_stream.eof() && seg.length_in_sequence_space() < _receiver_window_size)
        {
            seg.header().fin = true;
            _fin_sent = true;
        }
        // size_t length_of_seg = seg.length_in_sequence_space();
        // cout << length_of_seg;
        _segment_handler(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size)
{
    uint64_t seqno_abs = unwrap(ackno, _isn, _rcv_base);
    //handle two border cases & update state variables
    if(seqno_abs > _next_seqno)
        return;
    _receiver_window_size = window_size;
    _back_off = _receiver_window_size ? true : false;
    if(seqno_abs <= _rcv_base)
        return;
    _rcv_base = seqno_abs;
    // check if a NEW SEG is received
    bool new_data_rcv = false;
    while (!_outstanding_segments.empty())
    {
        TCPSegment front = _outstanding_segments.front();
        unsigned int end_of_front = unwrap(front.header().seqno, _isn, _rcv_base) + front.length_in_sequence_space();
        //when a full segment is received
        if(seqno_abs >= end_of_front)
        {
            _bytes_in_flight -= front.length_in_sequence_space();
            _outstanding_segments.pop();
            new_data_rcv = true;
        }
        else
            break;
    }
    //When the receiver gives the sender an ackno that acknowledges the successful receipt of new data
    if(new_data_rcv)
    {
        //Set the RTO back to its “initial value”
        _RTO = _initial_retransmission_timeout;
        //If the sender has any outstanding data, restart the retransmission timer
        if(!_outstanding_segments.empty())
            _time_elipsed = 0;
        //Reset the count of “consecutive retransmissions”
        _consecutive_retransmission = 0;
    }
    //When all outstanding data has been acknowledged
    if(_outstanding_segments.empty())
    {
        _timer_running = false;
        _time_elipsed = 0;
    }
    //we could send new segs now
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick)
{
    if(_timer_running)
    {
        _time_elipsed += ms_since_last_tick;
        if(_time_elipsed >= _RTO)
        {
            _segments_out.push(_outstanding_segments.front());
            // If the window size is nonzero
            if(_back_off)
            {
                _RTO *= 2;
                _consecutive_retransmission++;
            }
            _time_elipsed = 0;
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission; }

void TCPSender::send_empty_segment()
{
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    //no need to be kept track of as “outstanding” and won’t ever be retransmitted
    _segments_out.push(seg);
}

void TCPSender::_segment_handler(TCPSegment &seg)
{
    seg.header().seqno = wrap(_next_seqno, _isn);
    _bytes_in_flight += seg.length_in_sequence_space();
    _next_seqno += seg.length_in_sequence_space();
    _outstanding_segments.push(seg);
    _segments_out.push(seg);
    if(!_timer_running)
    {
        _timer_running = true;
        _time_elipsed = 0;
    }
}
