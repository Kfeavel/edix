/**
 * @file InterruptManager.cpp
 * @author Keeton Feavel (keetonfeavel@cedarville.edu)
 * @brief 
 * @version 0.1
 * @date 2019-09-26
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include <cpu/interrupts/InterruptManager.hpp>

InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256];
InterruptManager* InterruptManager::activeInterruptManager = nullptr;
Timer* InterruptManager::activeInterruptManagerTimer = nullptr;

void InterruptManager::setInterruptDescriptorTableEntry(
    uint8_t interrupt,
    uint16_t CodeSegment, void (*handler)(),
    uint8_t DescriptorPrivilegeLevel,
    uint8_t DescriptorType)
{
    // address of pointer to code segment (relative to global descriptor table)
    // and address of the handler (relative to segment)
    interruptDescriptorTable[interrupt].handlerAddressLowBits = ((uint32_t) handler) & 0xFFFF;
    interruptDescriptorTable[interrupt].handlerAddressHighBits = (((uint32_t) handler) >> 16) & 0xFFFF;
    interruptDescriptorTable[interrupt].gdt_codeSegmentSelector = CodeSegment;

    const uint8_t IDT_DESC_PRESENT = 0x80;
    interruptDescriptorTable[interrupt].access = IDT_DESC_PRESENT | ((DescriptorPrivilegeLevel & 3) << 5) | DescriptorType;
    interruptDescriptorTable[interrupt].reserved = 0;
}


InterruptManager::InterruptManager(uint16_t hardwareInterruptOffset, GlobalDescriptorTable* globalDescriptorTable)
    : programmableInterruptControllerMasterCommandPort(0x20),
      programmableInterruptControllerMasterDataPort(0x21),
      programmableInterruptControllerSlaveCommandPort(0xA0),
      programmableInterruptControllerSlaveDataPort(0xA1)
{
    this->hardwareInterruptOffset = hardwareInterruptOffset;
    uint32_t CodeSegment = globalDescriptorTable->CodeSegmentSelector();
    void (* handleExceptionsArray [20])() = {
        &handleException0x00, &handleException0x01, &handleException0x02, &handleException0x03,
        &handleException0x04, &handleException0x05, &handleException0x06, &handleException0x07,
        &handleException0x08, &handleException0x09, &handleException0x0A, &handleException0x0B, 
        &handleException0x0C, &handleException0x0D, &handleException0x0E, &handleException0x0F,
        &handleException0x10, &handleException0x11, &handleException0x12, &handleException0x13
    };

    int handleExceptionsCodeArray[20] = {
        0x00, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
        0x0F, 0x10, 0x11, 0x12, 0x13
    };

    void (* handleInterruptRequestArray [16])() = {
        &handleInterruptRequest0x00, &handleInterruptRequest0x01, &handleInterruptRequest0x02, &handleInterruptRequest0x03,
        &handleInterruptRequest0x04, &handleInterruptRequest0x05, &handleInterruptRequest0x06, &handleInterruptRequest0x07,
        &handleInterruptRequest0x08, &handleInterruptRequest0x09, &handleInterruptRequest0x0A, &handleInterruptRequest0x0B, 
        &handleInterruptRequest0x0C, &handleInterruptRequest0x0D, &handleInterruptRequest0x0E, &handleInterruptRequest0x0F
    };

    int handleInterruptCodeArray[16] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F
    };

    for(uint8_t i = 255; i > 0; --i) {
        setInterruptDescriptorTableEntry(i, CodeSegment, &interruptIgnore, 0, IDT_INTERRUPT_GATE);
    }
    // This line is necessary. Setting the i to >= 0 above won't work.
    setInterruptDescriptorTableEntry(0, CodeSegment, &interruptIgnore, 0, IDT_INTERRUPT_GATE);

    for (int i = 0; i < 19; i++) {
        setInterruptDescriptorTableEntry(handleExceptionsCodeArray[i], CodeSegment, handleExceptionsArray[i], 0, IDT_INTERRUPT_GATE);
    }

    for (int i = 0; i < 15; i++) {
        setInterruptDescriptorTableEntry(hardwareInterruptOffset + handleInterruptCodeArray[i], CodeSegment, handleInterruptRequestArray[i], 0, IDT_INTERRUPT_GATE);
    }

    programmableInterruptControllerMasterCommandPort.write(0x11);
    programmableInterruptControllerSlaveCommandPort.write(0x11);

    // remap
    programmableInterruptControllerMasterDataPort.write(hardwareInterruptOffset);
    programmableInterruptControllerSlaveDataPort.write(hardwareInterruptOffset + 8);

    programmableInterruptControllerMasterDataPort.write(0x04);
    programmableInterruptControllerSlaveDataPort.write(0x02);

    programmableInterruptControllerMasterDataPort.write(0x01);
    programmableInterruptControllerSlaveDataPort.write(0x01);

    programmableInterruptControllerMasterDataPort.write(0x00);
    programmableInterruptControllerSlaveDataPort.write(0x00);

    InterruptDescriptorTablePointer idt_pointer;
    idt_pointer.size  = 256 * sizeof(GateDescriptor) - 1;
    idt_pointer.base  = (uint32_t)interruptDescriptorTable;
    asm volatile("lidt %0" : : "m" (idt_pointer));
}

InterruptManager::~InterruptManager() {
    deactivate();
}

uint16_t InterruptManager::getHardwareInterruptOffset() {
    return hardwareInterruptOffset;
}

void InterruptManager::setInterruptManagerTimer(Timer* timer) {
    activeInterruptManagerTimer = timer;
}

Timer* InterruptManager::getInterruptManagerTimer() {
    return activeInterruptManagerTimer;
}

void InterruptManager::activate() {
    if(activeInterruptManager == nullptr) {
        activeInterruptManager = this;
        asm("sti");
    }
}

void InterruptManager::deactivate() {
    if (activeInterruptManager == this) {
        activeInterruptManager = nullptr;
        activeInterruptManagerTimer = nullptr;
        asm("cli");
    }
}

uint32_t InterruptManager::handleInterrupt(uint8_t interrupt, uint32_t esp) {
    if (activeInterruptManager != nullptr) {
        if (interrupt == 0x00 + activeInterruptManager->hardwareInterruptOffset) {
            if (activeInterruptManagerTimer != nullptr) {
                activeInterruptManagerTimer->callback();
            } else {
                kprint("CPU timer did not activate!\n");
            }
        }
        if (activeInterruptManager->handlers[interrupt] != 0) { 
            // This handleInterrupt function is located in the InterruptHandler.cpp file
            esp = activeInterruptManager->handlers[interrupt]->handleInterrupt(esp);
        // Panic because we don't know how to handle this. Also make an annoying sound
        } else if (interrupt != activeInterruptManager->hardwareInterruptOffset) {
            // Call the LibC panic() function defined in stdio.hpp
            panic(interrupt);
        }

        // hardware interrupts must be acknowledged
        if (activeInterruptManager->hardwareInterruptOffset <= interrupt 
        && interrupt < activeInterruptManager->hardwareInterruptOffset + 16) {
            activeInterruptManager->programmableInterruptControllerMasterCommandPort.write(0x20);
            if (activeInterruptManager->hardwareInterruptOffset + 8 <= interrupt) {
                activeInterruptManager->programmableInterruptControllerSlaveCommandPort.write(0x20);
            }
        }
    }
    
    return esp;
}
