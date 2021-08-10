#include <FreeRTOS.h>
#include <pico/stdlib.h>
#include <task.h>
#include <stdio.h>

void vTaskBlinky(void *pvParameters) {
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
}

void vTaskHello(void *param) {
    stdio_init_all();
    while (true) {
        vTaskDelay(1000);
        printf("Hello, world multitasking!\n");
    }
}

int main() {
    BaseType_t taskErr;
    TaskHandle_t taskHandle = NULL;
    /* Create the task, storing the handle. */
    taskErr = xTaskCreate(vTaskBlinky, "Blinky task", 512, NULL,
                            tskIDLE_PRIORITY, &taskHandle);
    configASSERT(pdPASS == taskErr);

    taskErr = xTaskCreate(vTaskHello, "Hello task", 512, NULL,
                            tskIDLE_PRIORITY, NULL);
    configASSERT(pdPASS == taskErr);

    vTaskStartScheduler();
    while (1) {
        configASSERT(0); /* We should never get here */
    }
}
