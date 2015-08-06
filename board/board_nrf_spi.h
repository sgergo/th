#ifndef BOARD_NRF_SPI.H
#define BOARD_NRF_SPI.H


void board_nrf_spi_chip_select(void);
void board_nrf_spi_chip_release(void);
void board_nrf_spi_transfer(void);
void board_nrf_spi_init(void);

#endif