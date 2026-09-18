#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace inxi {

struct PciDevice {
    std::string slot;  // 00:02.0
    std::string sysfs;
    std::uint32_t vendor_id = 0;
    std::uint32_t device_id = 0;
    std::uint32_t class_id = 0;
    std::string vendor;
    std::string device;
    std::string class_name;
    std::string driver;
    std::string subsystem;
};

struct UsbDevice {
    std::string bus_port;
    std::uint32_t vendor_id = 0;
    std::uint32_t product_id = 0;
    std::string vendor;
    std::string product;
    std::string driver;
    std::string cls;  // hex class
    std::string speed;
};

const std::vector<PciDevice>& pci_devices();
const std::vector<UsbDevice>& usb_devices();
std::string pci_id_string(const PciDevice& d);
std::string class_prefix(std::uint32_t class_id, int nibbles = 2);

}  // namespace inxi
