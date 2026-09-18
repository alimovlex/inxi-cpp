// network_item.h
// Collects network interface / NIC information.
// Perl: network / ip / ifconfig parsing subs
//
// Perl subs mapped:
//   get()                  → NetworkItem::get()
//   full_output()          → NetworkItem::full_output()
//   short_output()         → NetworkItem::short_output()
//   nic_data()             → NetworkItem::nic_data()
//   ip_data()              → NetworkItem::ip_data()
//   ifconfig_data()        → NetworkItem::ifconfig_data()
//   wireless_data()        → NetworkItem::wireless_data()
//   wan_data()             → NetworkItem::wan_data()
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

struct NicInfo {
    std::string if_name;      // "eth0"|"wlan0"|"enp3s0"
    std::string driver;
    std::string driver_version;
    std::string vendor;
    std::string model;
    std::string mac;
    std::string ip4;
    std::string ip6;
    std::string speed;        // link speed "1 Gbps"
    std::string duplex;       // "Full"|"Half"
    bool        up           = false;
    bool        wireless     = false;
    std::string ssid;
    std::string freq;         // "5.18 GHz"
    std::string signal_dbm;
    std::string tx_rate;
    std::string fw_version;
    std::string bus_id;
    std::string port;         // "Ethernet"|"USB"|"pcie"
};

class NetworkItem final : public InfoItem {
public:
    explicit NetworkItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Network"; }
    std::vector<OutputRow> get() override;

private:
    std::vector<OutputRow> full_output(const std::vector<NicInfo>& nics);
    std::vector<OutputRow> short_output(const std::vector<NicInfo>& nics);

    std::vector<NicInfo> nic_data();
    void                 ip_data(std::vector<NicInfo>& nics);
    void                 ifconfig_data(std::vector<NicInfo>& nics);
    void                 wireless_data(NicInfo& nic);
    std::string          wan_data();
};

}  // namespace inxi
