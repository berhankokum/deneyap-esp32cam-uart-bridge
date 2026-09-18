#include <stdint.h>
#include <stddef.h>

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PC_UART             UART_NUM_0
#define CAM_UART            UART_NUM_1

#define CAM_UART_TX_PIN     GPIO_NUM_23
#define CAM_UART_RX_PIN     GPIO_NUM_22

#define UART_BAUD_RATE      115200
#define UART_BUFFER_SIZE    512


static int uart_read_chunk(
    uart_port_t uart_num,
    uint8_t *buffer,
    size_t buffer_size)
{
    /*
     * Önce sadece 1 byte bekle.
     *
     * Böylece 512 byte dolmasını beklemek yerine
     * herhangi bir veri gelir gelmez uyanıyoruz.
     */
    int length = uart_read_bytes(
        uart_num,
        buffer,
        1,
        portMAX_DELAY
    );

    if (length <= 0)
    {
        return length;
    }

    /*
     * İlk byte geldikten sonra UART buffer'ında
     * başka byte'lar da varsa onları da hemen al.
     */
    size_t buffered_length = 0;

    ESP_ERROR_CHECK(
        uart_get_buffered_data_len(
            uart_num,
            &buffered_length
        )
    );

    size_t remaining_space =
        buffer_size - (size_t)length;

    size_t bytes_to_read =
        (buffered_length < remaining_space)
            ? buffered_length
            : remaining_space;

    if (bytes_to_read > 0U)
    {
        int extra_length = uart_read_bytes(
            uart_num,
            buffer + length,
            (uint32_t)bytes_to_read,
            0
        );

        if (extra_length > 0)
        {
            length += extra_length;
        }
    }

    return length;
}


static void pc_to_cam_task(void *arg)
{
    uint8_t buffer[UART_BUFFER_SIZE];

    while (1)
    {
        int length = uart_read_chunk(
            PC_UART,
            buffer,
            sizeof(buffer)
        );

        if (length > 0)
        {
            uart_write_bytes(
                CAM_UART,
                buffer,
                length
            );
        }
    }
}


static void cam_to_pc_task(void *arg)
{
    uint8_t buffer[UART_BUFFER_SIZE];

    while (1)
    {
        int length = uart_read_chunk(
            CAM_UART,
            buffer,
            sizeof(buffer)
        );

        if (length > 0)
        {
            uart_write_bytes(
                PC_UART,
                buffer,
                length
            );
        }
    }
}


void app_main(void)
{
    const uart_config_t uart_config =
    {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };


    /*
     * PC <-> Deneyap
     *
     * Deneyap'ın dahili USB-UART dönüştürücüsü
     * UART0'a bağlı.
     */
    ESP_ERROR_CHECK(
        uart_driver_install(
            PC_UART,
            2048,
            2048,
            0,
            NULL,
            0
        )
    );

    ESP_ERROR_CHECK(
        uart_param_config(
            PC_UART,
            &uart_config
        )
    );


    /*
     * Deneyap <-> ESP32-CAM
     */
    ESP_ERROR_CHECK(
        uart_driver_install(
            CAM_UART,
            2048,
            2048,
            0,
            NULL,
            0
        )
    );

    ESP_ERROR_CHECK(
        uart_param_config(
            CAM_UART,
            &uart_config
        )
    );

    ESP_ERROR_CHECK(
        uart_set_pin(
            CAM_UART,
            CAM_UART_TX_PIN,
            CAM_UART_RX_PIN,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        )
    );


    xTaskCreate(
        pc_to_cam_task,
        "pc_to_cam",
        3072,
        NULL,
        10,
        NULL
    );

    xTaskCreate(
        cam_to_pc_task,
        "cam_to_pc",
        3072,
        NULL,
        10,
        NULL
    );
}