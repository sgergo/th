/*
 * hiddfu_descriptors.c
 *
 *  Created on: Jul 9, 2015
 *      Author: gsantha
 */

#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"

#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbddfu-rt.h"
#include "usblib/device/usbdcomp.h"

#include "compositehid.h"
#include "hiddfu_descriptors.h"

databuffer_t rxdata;
databuffer_t txdata;

#define NUM_STRING_DESCRIPTORS  (sizeof(string_descriptors) /           \
                                 sizeof(string_descriptors[0]))

#define NUM_HID_SECTIONS        (sizeof(hid_sections) /                    \
                                 sizeof(hid_sections[0]))

//#define USB_VID                   0x2047        // Vendor ID (VID)
//#define USB_PID                   0x0301        // Product ID (PID)

#define USB_VID                 0x1cbe        // Vendor ID (VID)
#define USB_PID                 0x000a        // Product ID (PID)

#define LogicalMaximum2(byteH, byteL)  0x26, (byteH), (byteL)

uint8_t const datapipereport_descriptor[] = {

    // Vendor specific value 0xff00
    // 0x06, 0xff, 0x00
    UsagePageVendor(0xff00),

    // Arbitrary number as we are vendor specific
    // 0x09, 0x01
    Usage(0x01),

    // 0xa1, 0x01
    Collection(USB_HID_APPLICATION),

        // 0x95, 0x01,
        // The number of items in the report
        ReportCount(IRQIN_PACKET_SIZE),

        // 0x75, 0x08,
        // As this is just generic data, 8 bits in a byte
        ReportSize(0x08),

        // 0x26, 0xff, 0x00,
        // max = 255
        LogicalMaximum2(0xff, 0x00),

        // 0x15, 0x00,
        // min = 0
        LogicalMinimum(0),

        // 0x09, 0x01,
        // Arbitrary number as we are vendor specific
        Usage(0x01),

        // 0x81, 0x02, Input (Data,Var,Abs)
        // This shows we are describing the report that goes to the host
        Input(USB_HID_INPUT_DATA | USB_HID_INPUT_VARIABLE | USB_HID_INPUT_ABS),
        
        // Second report, same except it's output
        ReportCount(CTRL_PACKET_SIZE),
        ReportSize(0x08),
//      LogicalMaximum(127),
        0x26, 0xff, 0x00,
        LogicalMinimum(0),
        Usage(0x01),
        Output(USB_HID_INPUT_DATA | USB_HID_INPUT_VARIABLE | USB_HID_INPUT_ABS),
    EndCollection
};

const uint8_t language_descriptor[] = {
        4,
        USB_DTYPE_STRING,
        USBShort(USB_LANG_EN_US)
};

const uint8_t manufacturer_string[] = {
    (10 + 1) * 2,
    USB_DTYPE_STRING,
    'I', 0, 'G', 0, 'Y', 0, 'A', 0, 'L', 0, 'S', 0, 'O', 0, 'R', 0, 'T', 0, '!', 0,
};


const uint8_t product_string[] = {
        (6 + 1) * 2,
    USB_DTYPE_STRING,
    'S', 0, 'N', 0, 'A', 0,'K', 0,'E', 0, '!', 0,
};

const uint8_t serialnumber_string[] = {
    (4 + 1) * 2,
    USB_DTYPE_STRING,
#ifdef VER1_0
    'v', 0, '1', 0, '.', 0, '0', 0
#elif defined VER1_1
    'v', 0, '1', 0, '.', 0, '1', 0
#endif
};

const uint8_t interface_string[] = {
        28,
    USB_DTYPE_STRING,
    'H',0x00,'I',0x00,'D',0x00,' ',0x00,'I',0x00,'n',0x00,
    't',0x00,'e',0x00,'r',0x00,'f',0x00,'a',0x00,'c',0x00,
    'e',0x00
};

const uint8_t configuration_string[] =
{
    (28 + 1) * 2,
    USB_DTYPE_STRING,
    'C', 0, 'u', 0, 's', 0, 't', 0, 'o', 0, 'm', 0, ' ', 0, 'H', 0,
    'I', 0, 'D', 0, '&', 0, 'D', 0, 'F', 0, 'U', 0, ' ', 0, 'C', 0,
    'o', 0, 'n', 0, 'f', 0, 'i', 0, 'g', 0, 'u', 0, 'r', 0, 'a', 0,
    't', 0, 'i', 0, 'o', 0, 'n', 0

};

// The descriptor string table
const uint8_t * const string_descriptors[] = {
    language_descriptor,
    manufacturer_string,
    product_string,
    serialnumber_string,
    interface_string,
    configuration_string
};

static const uint8_t * const datapipeclass_descriptors[] = {
        datapipereport_descriptor
};

uint8_t datapipe_descriptor[] = {
    //
    // Configuration descriptor header.
    //
    9,                          // Size of the configuration descriptor.
    USB_DTYPE_CONFIGURATION,    // Type of this descriptor.
    USBShort(34),               // The total size of this full structure.
    1,                          // The number of interfaces in this
                                // configuration.
    1,                          // The unique value for this configuration.
    5,                          // The string identifier that describes this
                                // configuration.
    USB_CONF_ATTR_SELF_PWR,     // Bus Powered, Self Powered, remote wake up.
    250,                        // The maximum power in 2mA increments.
};

static const tHIDDescriptor datapipehid_descriptor = {
    9, // bLength
    USB_HID_DTYPE_HID, // bDescriptorType
    0x111, // bcdHID (version 1.11 compliant)
    0, // bCountryCode (not localized)
    1, // bNumDescriptors
    USB_HID_DTYPE_REPORT, // Report descriptor
    sizeof(datapipereport_descriptor) // Size of report descriptor
};

uint8_t hidinterface_descriptor[HIDINTERFACE_SIZE] =
{
    //
    // HID Device Class Interface Descriptor.
    //
    9,                          // Size of the interface descriptor.
    USB_DTYPE_INTERFACE,        // Type of this descriptor.
    0,                          // The index for this interface.
    0,                          // The alternate setting for this interface.
    1,                          // The number of endpoints used by this
                                // interface.
    USB_CLASS_HID,              // The interface class
    USB_HID_SCLASS_NONE,        // The interface sub-class.
    USB_HID_PROTOCOL_NONE,     // The interface protocol for the sub-class
                                // specified above.
    5,                          // The string index for this interface.
};

const uint8_t interruptINendpoint_descriptor[HIDINENDPOINT_SIZE] =
{
    //
    // Interrupt IN endpoint descriptor
    //
    7,                          // The size of the endpoint descriptor.
    USB_DTYPE_ENDPOINT,         // Descriptor type is an endpoint.
    USB_EP_DESC_IN | USBEPToIndex(USB_EP_1),
    USB_EP_ATTR_INT,            // Endpoint is an interrupt endpoint.
    USBShort(USBFIFOSizeToBytes(USB_FIFO_SZ_64)),
                                // The maximum packet size.
    250,                         // The polling interval for this endpoint.
                                //either 1ms for low/full speed devices and 125us for high speed devices
};

// Not needed until the out interrupt is enabled
// const uint8_t interruptOUTendpoint_descriptor[HIDOUTENDPOINT_SIZE] =
// {
//     //
//     // Interrupt Out endpoint descriptor
//     //
//     7,                          // The size of the endpoint descriptor.
//     USB_DTYPE_ENDPOINT,         // Descriptor type is an endpoint.
//     USB_EP_DESC_OUT | USBEPToIndex(USB_EP_1),
//     USB_EP_ATTR_INT,            // Endpoint is an interrupt endpoint.
//     USBShort(USBFIFOSizeToBytes(USB_FIFO_SZ_64)),
//                                 // The maximum packet size.
//     16,                         // The polling interval for this endpoint.
// };

//-------------------------------------------------------------------------------------------------------------------
const tConfigSection hidconfig_section = {
    sizeof(datapipe_descriptor),
    datapipe_descriptor
};

const tConfigSection hidinterface_section = {
    sizeof(hidinterface_descriptor),
    hidinterface_descriptor
};

const tConfigSection hidINendpoint_section = {
    sizeof(interruptINendpoint_descriptor),
    interruptINendpoint_descriptor
};

// Not needed until the out interrupt is enabled
// const tConfigSection hidOUTendpoint_section = {
//     sizeof(interruptOUTendpoint_descriptor),
//     interruptOUTendpoint_descriptor
// };

tConfigSection hiddescriptor_section = {
   sizeof(datapipehid_descriptor),
   (const uint8_t *)&datapipehid_descriptor
};
//-------------------------------------------------------------------------------------------------------------------

const tConfigSection *hid_sections[] = {
    &hidconfig_section,
    &hidinterface_section,
    &hiddescriptor_section,
    &hidINendpoint_section,
//    &hidOUTendpoint_section,
};

tConfigHeader hidconfig_header = {
    NUM_HID_SECTIONS,
    hid_sections
};

const tConfigHeader * const hidconfig_descriptors[] = {
    &hidconfig_header
};


tHIDReportIdle report_idle[1] = {
        { 125, 0, 0, 0 } // Report 0 polled every 500 ms (4*125)
//        { 0, 0, 0, 0 } // Report 0 never polled
};

tUSBDHIDDevice hiddatapipe_device = {

    USB_VID,
    USB_PID,
    250,
    USB_CONF_ATTR_BUS_PWR,
    USB_HID_SCLASS_NONE,
    USB_HID_PROTOCOL_NONE,
    // The device has a single input report.
    1,
    // A pointer to our array of tHIDReportIdle structures. For this device,
    // the array must have 1 element (matching the value of the previous field).
    report_idle,
    // A pointer to your receive callback event handler.
    rxhandler,
    // A value that you want passed to the receive callback alongside every
    // event.
    (void *)&rxdata,
    // A pointer to your transmit callback event handler.
    txhandler,
    // A value that you want passed to the transmit callback alongside every
    // event.
    (void *)&txdata,
    // This device does not want to use a dedicated interrupt OUT endpoint
    // since there are no output or feature reports required.
    false,
    // A pointer to the HID descriptor for the device.
    &datapipehid_descriptor,
    // A pointer to the array of HID class descriptor pointers for this device.
    // The number of elements in this array and their order must match the
    // information in the HID descriptor provided above.
    datapipeclass_descriptors,
    // A pointer to your string table.
    string_descriptors,
    // The number of entries in your string table. This must equal
    // (1 + (5 + (num HID strings)) * (num languages)).
    NUM_STRING_DESCRIPTORS,
    // Config section
    hidconfig_descriptors
};

// Number of devices
#define NUM_DEVICES             2

// The array of devices supported by this composite device.
tCompositeEntry composite_devices[NUM_DEVICES];

tUSBDDFUDevice dfu_device = {
    dfudetach_callback,
    (void *)&dfu_device
};

// Additional workspace required by the composite driver to hold a lookup
// table allowing mapping of composite interface and endpoint numbers to
// individual device class instances.
uint32_t composite_workspace[NUM_DEVICES];

// The instance data for this composite device.
tCompositeInstance composite_instance;

//****************************************************************************
//
// Allocate the Device Data for the top level composite device class.
//
//****************************************************************************
tUSBDCompositeDevice composite_device = {

    USB_VID,
    USB_PID,
    500,
    USB_CONF_ATTR_BUS_PWR,
    // Device event handler function pointer (receives connect, disconnect
    // and other device-level notifications).
    compositehandler,
    string_descriptors,
    NUM_STRING_DESCRIPTORS,
    NUM_DEVICES,
    composite_devices,
};

// A buffer into which the composite device can write the combined config
// descriptor.
uint8_t composite_descriptorbuffer[DESCRIPTOR_BUFFER_SIZE];
