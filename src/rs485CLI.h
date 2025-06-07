#pragma once
#include <stdio.h>
#include <driver/uart.h>

void CLI_RS485_Init(void);
void CLI_RS485_Send_data(const char* data);
// void RS485_Send(uart_port_t uart_port,uint8_t* buf,uint16_t size);
