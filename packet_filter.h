/*
    This file is part of Kismet

    Kismet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kismet is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kismet; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"

#include "packetchain.h"
#include "packet.h"

#include "trackedcomponent.h"

// Common packet filter mechanism which can be used in multiple locations;
// implements basic default behavior, filtering by address, and REST endpoints.

class Packetfilter : public tracker_component {
public:
    Packetfilter(const std::string& in_id, const std::string& in_description,
            const std::string& in_type);

    virtual ~Packetfilter() {
        local_locker l(&mutex);
    }

    __ProxyGet(filter_id, std::string, std::string, filter_id);
    __ProxyGet(filter_description, std::string, std::string, filter_description);
    __ProxyGet(filter_type, std::string, std::string, filter_type);
    __Proxy(filter_default, uint8_t, bool, bool, filter_default);

    virtual bool filter_packet(kis_packet *packet) = 0;

protected:
    virtual void register_fields() override {
        tracker_component::register_fields();

        RegisterField("kismet.packetfilter.id", "Filter ID/Endpoint", &filter_id);
        RegisterField("kismet.packetfilter.description", "Filter description", &filter_description);
        RegisterField("kismet.packetfilter.type", "Filter mechanism", &filter_type);
        RegisterField("kismet.packetfilter.default", "Default filter (pass/reject)", &filter_default);
    }

    kis_recursive_timed_mutex mutex;

    std::shared_ptr<TrackerElementString> filter_id;
    std::shared_ptr<TrackerElementString> filter_description;
    std::shared_ptr<TrackerElementString> filter_type;
    std::shared_ptr<TrackerElementUInt8> filter_default;

    // Default behavior endpoint
    std::shared_ptr<Kis_Net_Httpd_Simple_Post_Endpoint> default_endp;
};

// Mac-address based filter.
// Filters can be applied to 'ANY', source, destination, network, or transmitter
// (in wifi terms, source, dest, bssid, or 4mac transmitter for wds).
// Filters are true (filter/reject packet), or false (pass packet).  Packets not matched
// by any filter are passed to the default filter term.
class PacketfilterMacaddr : public Packetfilter {
public:
    PacketfilterMacaddr(const std::string& in_id, const std::string& in_description);
    virtual ~PacketfilterMacaddr() {}


protected:
    virtual void register_fields() override {
        tracker_component::register_fields();

        RegisterField("kismet.packetfilter.macaddr.source", "Source addresses filtered", &filter_source);
        RegisterField("kismet.packetfilter.macaddr.destination", "Destination addresses filtered", &filter_dest);
        RegisterField("kismet.packetfilter.macaddr.network", "Network/BSSID addresses filtered", &filter_network);
        RegisterField("kismet.packetfilter.macaddr.transmit", "Transmit addresses filtered", &filter_transmit);

        RegisterField("kismet.packetfilter.macaddr.any", "Any matching address type", &filter_transmit);
    }

    // Source, dest, and BSSID (for wifi) or transmitter (for others)
    std::shared_ptr<TrackerElementMacMap> filter_source;
    std::shared_ptr<TrackerElementMacMap> filter_dest;
    std::shared_ptr<TrackerElementMacMap> filter_network;
    std::shared_ptr<TrackerElementMacMap> filter_transmit;

    // ANY address found in a packet
    std::shared_ptr<TrackerElementMacMap> filter_any;
};