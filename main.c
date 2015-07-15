#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_nvic.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"

// UART
#include "diagnostic/uartstdio.h"

// USB
#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbddfu-rt.h"
#include "usblib/device/usbdcomp.h"

// application
#include "diagnostic/uart.h"
#include "board/board.h"
#include "diagnostic/diagnostic.h"
#include "usb/compositehid.h"
#include "usb/hiddfu_descriptors.h"

extern databuffer_t rxdata;
extern databuffer_t txdata;
extern tUSBDHIDDevice hiddatapipe_device;
extern usbstate_t usbstate;
extern event_struct_t events;

#define DATA_INC_TIMEOUT 500

void systickhandler(void) {
	static int systickcnt = 0;
	int i = 0;

	if (events.history_updated == true && (systickcnt % EVENT_PRINT_TIMEOUT) == 0)
		events.print_history = true;
	else
		events.print_history = false;

	if ((systickcnt % DATA_INC_TIMEOUT) == 0) {

		for (i = CTRL_PACKET_SIZE-1; i > 0; i--)
			txdata.buffer[i] = txdata.buffer[i-1];
		txdata.buffer[0]++;
		usbstate.txdataupdated = 1;
		for (i = 0; i < CTRL_PACKET_SIZE; i++)
			UARTprintf("%3d", txdata.buffer[i]);
		UARTprintf("\n");
	}

	systickcnt++;
}

int main(void) {
	int i;
	uint8_t data[64];

    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    ROM_FPULazyStackingEnable();

    // Set the clocking to run from the PLL at 50MHz.
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    // Enable the GPIO peripheral used for USB, and configure the USB
    // pins.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 + GPIO_PIN_2 + GPIO_PIN_3);
	ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 + GPIO_PIN_2 + GPIO_PIN_3, 0);

#ifdef VER1_0
	ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3); //green
#elif defined VER1_1
	ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2); // blue
#endif
    // Init diagnostic
    diagnostic_clear_eventhistory();

    // Init UART
    ConfigureUART();

    // Print welcome message
    UARTprintf("Configuring USB\n");

    //------------------------------------------------------
//    // Set the USB stack mode to Device mode with VBUS monitoring.
//    USBStackModeSet(0, eUSBModeForceDevice, 0);
//
//    // Pass our device information to the USB library and place the device
//    // on the bus.
//    USBDHIDInit(0, &hiddatapipe_device);

    //------------------------------------------------------

    USBStackModeSet(0, eUSBModeForceDevice, 0);
    USBDHIDCompositeInit(0, &hiddatapipe_device, &(composite_device.psDevices[0]));
	USBDDFUCompositeInit(0, &dfu_device, &(composite_device.psDevices[1]));

	//
	// Pass the USB library our device information, initialize the USB
	// controller and connect the device to the bus.
	//
	USBDCompositeInit(0, &composite_device, DESCRIPTOR_BUFFER_SIZE, composite_descriptorbuffer);

    // Block until connected
    while (!usbstate.connected)
    	;

    // Configure SysTick
	ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 1000); // 1 ms
	ROM_SysTickEnable();
	ROM_SysTickIntEnable();

	// enable first IN report by clearing USB tx complete
	usbstate.txcomplete = 1;

    while(1) {

    	while (usbstate.connected && !usbstate.updatesignalled) {

    		//uncomment this if event history log is needed
	        if (events.print_history == true)
	        	diagnostic_print_eventhistory();

			// usbstate.txcomplete flag was set to 1 by txhandler after usbdhidreportwrite had succeeded
			if (usbstate.txcomplete  && usbstate.txdataupdated) {
				// clear usbstate.txcomplete flag to indicate that usbdhidreportwrite is busy
				usbstate.txcomplete = 0;
				usbstate.txdataupdated = 0;
				// put data (1 byte in this case) into the usb pipeline and the host will poll&read it
				// the polling rate is defined in the interrupt IN endpoint descriptor

				data[0] = txdata.buffer[0]; // temp buffer to avoid update change of txdata.buffer between the next 2 calls
				USBDHIDReportWrite(&hiddatapipe_device, data, 1, 0);
//				UARTprintf("Sending irq IN to host: %d\n", data[0]);

			}

			if (usbstate.hostsentreport) {
				// clear hostsentreport flag
				usbstate.hostsentreport = 0;

				for (i = 0; i < rxdata.size; i++) {
					UARTprintf("Received OUT report from host: %02x\n", rxdata.buffer[i]);
					rxdata.buffer[i] = 0;
				}
			}
    	}

    	// DFU requested

    	//
		// Terminate the USB device and detach from the bus.
		//
		USBDCompositeTerm(0);
		diagnostic_print_eventhistory();
		//
		// Disable all interrupts.
		//
		ROM_IntMasterDisable();

		//
		// Disable SysTick and its interrupt.
		//
		ROM_SysTickIntDisable();
		ROM_SysTickDisable();

		//
		// Disable all processor interrupts.  Instead of disabling them one at a
		// time, a direct write to NVIC is done to disable all peripheral
		// interrupts.
		//
		HWREG(NVIC_DIS0) = 0xffffffff;
		HWREG(NVIC_DIS1) = 0xffffffff;
		HWREG(NVIC_DIS2) = 0xffffffff;
		HWREG(NVIC_DIS3) = 0xffffffff;
		HWREG(NVIC_DIS4) = 0xffffffff;

		//
		// Enable and reset the USB peripheral.
		//
		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
		ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_USB0);
		USBClockEnable(USB0_BASE, 8, USB_CLOCK_INTERNAL);

		//
		// Wait for about a second.
		//
		ROM_SysCtlDelay(ROM_SysCtlClockGet() / 3);

		//
		// Re-enable interrupts at the NVIC level.
		//
		ROM_IntMasterEnable();

		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
		ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 + GPIO_PIN_2 + GPIO_PIN_3);
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 + GPIO_PIN_2 + GPIO_PIN_3, 0);
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);



		//
		// Call the USB boot loader.
		//
		ROM_UpdateUSB(0);

		//
		// Should never get here, but just in case.
		//
		while(1)
		{

		}
    }
}
