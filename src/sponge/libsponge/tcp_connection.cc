#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg)
{
    //if not active, refuse any segments
    if(!_active)
      return;
    _time_since_last_segment_received = 0;
    bool LISTEN = !_receiver.ackno().has_value() && _sender.next_seqno_absolute() == 0;
    //passively start a connection
    if(LISTEN)
    {
        if(!seg.header().syn)
            return;
        _receiver.segment_received(seg);
        // use connect() to send syn
        // 2nd of threeway-handshake
        connect();
        return;
    }
    bool SYN_SENT = _sender.next_seqno_absolute() > 0 && _sender.next_seqno_absolute() == _sender.bytes_in_flight();
    //in state SYN_SENT, expect for seg with ack and no payload
    //actively start a connection
    if(SYN_SENT && !_receiver.ackno().has_value())
    {
        //only accept empty segment with ack
        if(seg.payload().size() > 0 || !seg.header().ack)
        {
            //simultaneously open
            if(!seg.header().ack && seg.header().syn)
            {
                _receiver.segment_received(seg);
                //send empty seg with ack,syn to get further ack(established)
                _sender.send_empty_segment();
            }
            return;
        }
        //if rst, sets both the inbound and outbound streams to the error state and kills the connection permanently
        if(seg.header().rst)
        {
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            _active = false;
            return;
        }
    }
    //bool SYN_RECV = _receiver.ackno().has_value() && !_receiver.stream_out().input_ended();
    //gives the segment to the TCPReceiver
    _receiver.segment_received(seg);
    _sender.ack_received(seg.header().ackno, seg.header().win);
    if(seg.header().rst)
    {
        _sender.send_empty_segment();
        unclean_shutdown();
        return;
    }
    //client path: after receiving SYN/ACK from server
    //3rd of threeway-handshake: send ack(k = k + 1)
    if(_sender.segments_out().empty() && _receiver.ackno().has_value() && seg.length_in_sequence_space())
        _sender.send_empty_segment();
    send_segs_from_sender();
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    if(data.size() == 0)
       return 0;
    size_t written_size = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segs_from_sender();
    return written_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick)
{
    if(!_active)
      return;
    _time_since_last_segment_received += ms_since_last_tick;
    //tell the TCPSender about the passage of time
    _sender.tick(ms_since_last_tick);
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS)
    {
        //abort the connection, and send a empty reset segment to the peer
        unclean_shutdown();
    }
    send_segs_from_sender();
}

void TCPConnection::end_input_stream()
{
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segs_from_sender();
}

void TCPConnection::connect()
{
  _sender.fill_window();
    send_segs_from_sender();
}

void TCPConnection::set_seg(TCPSegment& seg)
{
    if(_receiver.ackno().has_value())
    {
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        seg.header().win = _receiver.window_size();
    }
}

void TCPConnection::send_segs_from_sender()
{
    //push it on to the _segments_out queue from sender
    while(!_sender.segments_out().empty())
    {
        TCPSegment seg(std::move(_sender.segments_out().front()));
        set_seg(seg);
        _sender.segments_out().pop();
        _segments_out.push(seg);
    }
    clean_shutdown();
}

void TCPConnection::clean_shutdown()
{
    //if the TCPConnection 's inbound stream ends before the TCPConnection has ever sent a fin segment
    //condition passive close
    if(!_sender.stream_in().eof() && _receiver.stream_out().input_ended())
        _linger_after_streams_finish = false;
    //Prerequisites #1 through #3 are true
    if(_sender.stream_in().eof() && _receiver.stream_out().input_ended() && !_sender.bytes_in_flight())
    {
        //passive close or linger after segments finish
        if(!_linger_after_streams_finish || _time_since_last_segment_received >= 10 * _cfg.rt_timeout)
            _active = false;
    }
}

void TCPConnection::unclean_shutdown()
{
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
    //send a segment with the rst flag set
    TCPSegment seg(std::move(_sender.segments_out().front()));
    set_seg(seg);
    seg.header().rst = true;
    _sender.segments_out().pop();
    _segments_out.push(seg);
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            _sender.send_empty_segment();
            unclean_shutdown();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
