#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/queue.h"
#include "driver/gpio.h"

TaskHandle_t myTaskHandle = NULL;
QueueHandle_t queue;

#define LED_PIN 27
#define PUSH_BUTTON_PIN 33

void IRAM_ATTR button_isr_handler(void *arg)
{
    char cIn = '1';
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // ส่งข้อมูลจาก ISR ไปยัง queue
    xQueueSendFromISR(queue, &cIn, &xHigherPriorityTaskWoken);

    // ปลุก task ที่มีความสำคัญสูงขึ้นถ้าจำเป็น
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void Task(void *arg)
{
    char rxBuffer;

    while (1)
    {
        // รับข้อมูลจาก queue
        if (xQueueReceive(queue, &rxBuffer, (TickType_t) 5))
        {
            printf("Button pressed!\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

void app_main(void)
{
    // ตั้งค่าพิน GPIO
    esp_rom_gpio_pad_select_gpio(PUSH_BUTTON_PIN);
    esp_rom_gpio_pad_select_gpio(LED_PIN);

    gpio_set_direction(PUSH_BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    gpio_set_intr_type(PUSH_BUTTON_PIN, GPIO_INTR_POSEDGE);

    gpio_install_isr_service(0);

    // เพิ่ม ISR handler
    gpio_isr_handler_add(PUSH_BUTTON_PIN, button_isr_handler, NULL);

    // สร้าง queue ที่บรรจุตัวแปร char จำนวน 1 ตัว
    queue = xQueueCreate(1, sizeof(char));

    // ทดสอบว่าสร้าง queue สำเร็จ?
    if (queue == NULL)
    {
        printf("Failed to create queue\n");
        return; // หรือจัดการข้อผิดพลาดเพิ่มเติมที่นี่
    }

    // สร้าง Task
    if (xTaskCreatePinnedToCore(Task, "My_Task", 4096, NULL, 10, &myTaskHandle, 1) != pdPASS)
    {
        printf("Failed to create Task\n");
    }
}
