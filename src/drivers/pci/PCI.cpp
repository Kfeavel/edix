/**
 * @file PCI.cpp
 * @author Keeton Feavel (keetonfeavel@cedarville.edu)
 * @brief 
 * @version 0.1
 * @date 2019-09-26
 * 
 * @copyright Copyright Keeton Feavel (c) 2019
 * 
 */
#include <drivers/pci/PCI.hpp>

PeripheralComponentInterconnectDeviceDescriptor::PeripheralComponentInterconnectDeviceDescriptor() {
    // Stubbed
}

PeripheralComponentInterconnectDeviceDescriptor::~PeripheralComponentInterconnectDeviceDescriptor() {
    // Stubbed
}

PeripheralComponentInterconnectController::PeripheralComponentInterconnectController() {
    // Stubbed
}

PeripheralComponentInterconnectController::~PeripheralComponentInterconnectController() {
    // Stubbed
}

uint32_t PeripheralComponentInterconnectController::read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset) {
    uint32_t id =
        0x1 << 31
        | ((bus & 0xFF) << 16)
        | ((device & 0x1F) << 11)
        | ((function & 0x07) << 8)
        | (registeroffset & 0xFC);
    writeLong(PCI_COMMAND_PORT, id);
    uint32_t result = readLong(PCI_DATA_PORT);
    return result >> (8* (registeroffset % 4));
}

void PeripheralComponentInterconnectController::write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value) {
    uint32_t id =
        0x1 << 31
        | ((bus & 0xFF) << 16)
        | ((device & 0x1F) << 11)
        | ((function & 0x07) << 8)
        | (registeroffset & 0xFC);
    writeLong(PCI_COMMAND_PORT, id);
    writeLong(PCI_DATA_PORT, value); 
}

bool PeripheralComponentInterconnectController::deviceHasFunctions(uint16_t bus, uint16_t device) {
    return read(bus, device, 0, 0x0E) & (1<<7);
}

void PeripheralComponentInterconnectController::selectDrivers(DriverManager* driverManager, InterruptManager* interrupts)
{
    for (int bus = 0; bus < 8; bus++) {
        //
        for (int device = 0; device < 32; device++) {
            //
            int numFunctions = deviceHasFunctions(bus, device) ? 8 : 1;
            //
            for (int function = 0; function < numFunctions; function++) {
                PeripheralComponentInterconnectDeviceDescriptor dev = getDeviceDescriptor(bus, device, function);
                //
                if (dev.vendor_id == 0x0000 || dev.vendor_id == 0xFFFF) {
                    continue;
                }
                //
                for (int barNum = 0; barNum < 6; barNum++) {
                    BaseAddressRegister bar = getBaseAddressRegister(bus, device, function, barNum);
                    //
                    if (bar.address && (bar.type == InputOutput)) {
                        dev.portBase = (uint32_t)bar.address;
                    }
                    //
                    Driver* driver = getDriver(dev, interrupts);
                    if (driver != 0) {
                        driverManager->addDriver(driver);
                    }
                }
                // Print the bus address
                kprint("PCI BUS ");
                kprintHex(bus & 0xFF);
                // Print the device name
                kprint(", DEVICE ");
                kprintHex(device & 0xFF);
                // Print the device function
                kprint(", FUNCTION ");
                kprintHex(function & 0xFF);
                // Print vendor info
                kprint(" = VENDOR ");
                kprintHex((dev.vendor_id & 0xFF00) >> 8);
                kprintHex(dev.vendor_id & 0xFF);
                kprint(", DEVICE ");
                kprintHex((dev.device_id & 0xFF00) >> 8);
                kprintHex(dev.device_id & 0xFF);
                kprint("\n");
            }
        }
    }
}

BaseAddressRegister PeripheralComponentInterconnectController::getBaseAddressRegister(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar) {
    // Define the BaseAddressRegister to be set
    BaseAddressRegister result;
    uint32_t headertype = read(bus, device, function, 0x0E) & 0x7F;
    //
    int maxBARs = 6 - (4*headertype);
    if(bar >= maxBARs) {
        return result;
    }
    //
    uint32_t bar_value = read(bus, device, function, 0x10 + 4*bar);
    result.type = (bar_value & 0x1) ? InputOutput : MemoryMapping;
    uint32_t temp;
    //
    if(result.type == MemoryMapping) {
        switch((bar_value >> 1) & 0x3) {    
            case 0: // 32 Bit Mode
                break;
            case 1: // 20 Bit Mode
                break;
            case 2: // 64 Bit Mode
                break;
        }
    //
    } else { // Input / Output
        result.address = (uint8_t*)(bar_value & ~0x3);
        result.prefetchable = false;
    }
    // Return the base address register
    return result;
}

Driver* PeripheralComponentInterconnectController::getDriver(PeripheralComponentInterconnectDeviceDescriptor dev, InterruptManager* interrupts) {
    // See https://wiki.osdev.org/Pci#Class_Codes for a list of codes on many known devices.
    // Each code can be entered here and used to identify a device and get its driver.
    Driver *driver = 0;
    switch(dev.vendor_id) {
        case 0x1022: // AMD
            switch(dev.device_id) {
                case 0x2000: // am79c973
                    /*
                    // Coming soon with the networking push
                    driver = (amd_am79c973*)MemoryManager::activeMemoryManager->malloc(sizeof(amd_am79c973));
                    if (driver != 0) {
                        new (driver) amd_am79c973(...);
                    }
                     */
                    kprint("AMD am79c973 ");
                    break;
            }
            break;
        case 0x8086: // Intel
            //kprint("Intel x86 (8086) ");
            break;
        default:
            // NOTE: If we print something here then we basically fill the screen, so don't.
            //kprint("[ERROR] Unknown Vendor - Can't load driver.\n");
            break;
    }
    // Get the PCI device function
    switch(dev.class_id) {
        case 0x03: // graphics
            switch(dev.subclass_id) {
                case 0x00: // VGA
                    kprint("VGA ");
                    break;
            }
            break;
        default:
            // NOTE: If we print something here then we basically fill the screen, so don't.
            //kprint("[ERROR] Unknown Device - Can't load driver.\n");
            break;
    }
    
    //return driver;
    return 0;
}

PeripheralComponentInterconnectDeviceDescriptor PeripheralComponentInterconnectController::getDeviceDescriptor(uint16_t bus, uint16_t device, uint16_t function) {
    PeripheralComponentInterconnectDeviceDescriptor result;
    
    result.bus = bus;
    result.device = device;
    result.function = function;
    
    result.vendor_id = read(bus, device, function, 0x00);
    result.device_id = read(bus, device, function, 0x02);

    result.class_id = read(bus, device, function, 0x0b);
    result.subclass_id = read(bus, device, function, 0x0a);
    result.interface_id = read(bus, device, function, 0x09);

    result.revision = read(bus, device, function, 0x08);
    result.interrupt = read(bus, device, function, 0x3c);
    
    return result;
}