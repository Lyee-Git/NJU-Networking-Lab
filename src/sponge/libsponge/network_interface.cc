#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

void NetworkInterface::send_datagram_handler(const InternetDatagram dgram, const EthernetAddress next_hop_MAC)
{
    EthernetFrame frame;
    frame.header().type = EthernetHeader::TYPE_IPv4;
    frame.header().src = _ethernet_address;
    frame.header().dst = next_hop_MAC;
    frame.payload() = move(BufferList(dgram.serialize()));
    _frames_out.push(frame);
}

void NetworkInterface::send_ARP(const size_t ARP_TYPE, const EthernetAddress next_hop_MAC, const uint32_t next_hop_ip)
{
    EthernetFrame frame;
    frame.header().type = EthernetHeader::TYPE_ARP;
    frame.header().src = _ethernet_address;
    frame.header().dst = next_hop_MAC;
    ARPMessage arp;
    arp.opcode = ARP_TYPE == NetworkInterface::TYPE_REQUEST? ARPMessage::OPCODE_REQUEST : ARPMessage::OPCODE_REPLY;
    arp.sender_ip_address = _ip_address.ipv4_numeric();
    arp.sender_ethernet_address = _ethernet_address;
    arp.target_ip_address = next_hop_ip;
    if(ARP_TYPE == NetworkInterface::TYPE_REPLY)
        arp.target_ethernet_address = next_hop_MAC;
    frame.payload() = move(BufferList(arp.serialize()));
    _frames_out.push(frame);
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    optional<EthernetAddress> next_hop_MAC = nullopt;
    map<uint32_t, EthernetAddress_Entry>::iterator MAC_iter;
    MAC_iter = _cache.find(next_hop_ip);
    bool need_send_ARP_request = false;
    if(MAC_iter != _cache.end())
        next_hop_MAC = MAC_iter->second.MAC_address;
    if(next_hop_MAC.has_value())
        //send datagrams
        send_datagram_handler(dgram, next_hop_MAC.value());
    else
    {
        //queue datagrams
        optional<Queuing_List> QueuingList= nullopt;
        map<uint32_t, Queuing_List>::iterator Queuing_List_iter = Queuing_List_Map.find(next_hop_ip);
        if(Queuing_List_iter != Queuing_List_Map.end())
            QueuingList = Queuing_List_iter->second;
        //If the network interface already sent an ARP request about the same IP address in the last ﬁve seconds, don’t send a second request
        if(QueuingList.has_value())
        {
            QueuingList.value().queuing_datagrams.push(dgram);
            if(QueuingList.value().time_since_last_ARP_req >= 5000)
                need_send_ARP_request = true;
        }
        //Send ARP request when first handle a datagram with unkown ethernet_address
        else
        {
            Queuing_List new_queuing_list;
            new_queuing_list.queuing_datagrams.push(dgram);
            Queuing_List_Map[next_hop_ip] = new_queuing_list;
            need_send_ARP_request = true;
        }
    }
    if(need_send_ARP_request)
        send_ARP(NetworkInterface::TYPE_REQUEST, ETHERNET_BROADCAST, next_hop_ip);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    optional<InternetDatagram> res = nullopt;

    //check: ignore any frames not destined for the network interface
    EthernetAddress dst_MAC = frame.header().dst;
    if(dst_MAC != _ethernet_address && dst_MAC != ETHERNET_BROADCAST)
        return res;

    if(frame.header().type == EthernetHeader::TYPE_ARP)
    {
        ARPMessage arp;
        if(arp.parse(Buffer(frame.payload())) == ParseResult::NoError)
        {
            // create a mapping for source ip & MAC
            map<uint32_t, EthernetAddress_Entry>::iterator iter_cache;
            iter_cache = _cache.find(arp.sender_ip_address);
            if(iter_cache != _cache.end())
            {
                // receive frame from ip_addr which is already cached
                // update variables
                iter_cache->second.time_caching = 0;
                iter_cache->second.MAC_address = arp.sender_ethernet_address;
            }
            else
            {
                //create a new cache entry
                EthernetAddress_Entry new_entry;
                new_entry.MAC_address = arp.sender_ethernet_address;
                new_entry.time_caching = 0;
                _cache[arp.sender_ip_address] = new_entry;
            }

            // now clear correspondent queuing list with known dst MAC addr
            map<uint32_t, Queuing_List>::iterator iter_queuingList;
            iter_queuingList = Queuing_List_Map.find(arp.sender_ip_address);
            if(iter_queuingList != Queuing_List_Map.end())
            {
                // send all queuing InternetDatagrams in queue:queuing_datagrams
                while (!iter_queuingList->second.queuing_datagrams.empty())
                {
                    InternetDatagram dgram = iter_queuingList->second.queuing_datagrams.front();
                    send_datagram_handler(dgram, arp.sender_ethernet_address);
                    iter_queuingList->second.queuing_datagrams.pop();
                }
                Queuing_List_Map.erase(arp.sender_ip_address);
            }

            if(arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == _ip_address.ipv4_numeric())
                send_ARP(NetworkInterface::TYPE_REPLY, arp.sender_ethernet_address, arp.sender_ip_address);
        }
    }
    else if(frame.header().type == EthernetHeader::TYPE_IPv4)
    {
        InternetDatagram datagram;
        if(datagram.parse(Buffer(frame.payload())) == ParseResult::NoError)
            res = datagram;
    }
    return res;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick)
{
    // check and update variable concerning time
    map<uint32_t, EthernetAddress_Entry>::iterator iter_cache;
    iter_cache = _cache.begin();
    while(iter_cache != _cache.end())
    {
        iter_cache->second.time_caching += ms_since_last_tick;
        if(iter_cache->second.time_caching >= 30000)
            _cache.erase(iter_cache++);
        else
            iter_cache++;
    }
    map<uint32_t, Queuing_List>::iterator iter_queuingList;
    for (iter_queuingList = Queuing_List_Map.begin(); iter_queuingList != Queuing_List_Map.end(); iter_queuingList++)
        iter_queuingList->second.time_since_last_ARP_req += ms_since_last_tick;
}
