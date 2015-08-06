




void board_nrf_spi_chip_select(void) {

}

void board_nrf_spi_chip_release(void) {

}

void board_nrf_spi_transfer(void) {

}

void board_nrf_spi_init(void) {

	// chip select pin 
	SysCtlPeripheralEnable(SPI_NRF_PERIPH_CS_PIN);
	ROM_GPIOPinTypeGPIOOutput(SPI_NRF_CS_PIN);
	ROM_GPIOPinWrite(SPI_NRF_PERIPH_CS_PIN_PORT_BASE, SPI_NRF_CS_PIN, SPI_NRF_CS_PIN);

	// SSI pins
	SysCtlPeripheralEnable(SPI_NRF_PERIPH_SSI0);
	SysCtlPeripheralEnable(SPI_NRF_PERIPH_SSI_PIN);
	GPIOPinConfigure(SPI_NRF_CLK_PIN_TYPE);
    GPIOPinConfigure(SPI_NRF_SOMI_PIN_TYPE);
    GPIOPinConfigure(SPI_NRF_SIMO_PIN_TYPE);
    GPIOPinTypeSSI(SPI_NRF_PERIPH_SSI_PIN_PORT_BASE, SPI_NRF_CLK_PIN | SPI_NRF_SOMI_PIN | SPI_NRF_SIMO_PIN);
    SSIConfigSetExpClk(SPI_NRF_PERIPH_SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, 1000000, 8);

    SSIEnable(SPI_NRF_PERIPH_SSI0_BASE);
}
