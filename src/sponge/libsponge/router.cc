#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 7, please replace with a real implementation that passes the
// automated checks run by `make check_lab7`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    _routing_table.push_back(RouteEntry(route_prefix, prefix_length, next_hop, interface_num));
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    if (dgram.header().ttl <= 1)
        return;
    // longest prefix match
    int match_num = -1, match_len = -1;
    for (size_t i = 0; i < _routing_table.size(); i++)
    {
        uint32_t mask = _routing_table[i]._prefix_length == 0 ? 0 : 0xffffffff << (32 - _routing_table[i]._prefix_length);
        if ((dgram.header().dst & mask) == _routing_table[i]._route_prefix && match_len < _routing_table[i]._prefix_length)
        {
            match_num = i;
            match_len = _routing_table[i]._prefix_length;
        }
    }
    // If no routes matched, the router drops the datagram
    if (match_num == -1)
        return;
    dgram.header().ttl -= 1;
    RouteEntry match_entry = _routing_table[match_num];
    // else the next_hop will contain the IP address of the next router along the path
    if (match_entry._next_hop.has_value())
        _interfaces[match_entry._interface_num].send_datagram(dgram, match_entry._next_hop.value());
    // If the router is directly attached to the network in question, the next_hop will be an empty optional
    else
        _interfaces[match_entry._interface_num].send_datagram(dgram, Address::from_ipv4_numeric(dgram.header().dst));
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
