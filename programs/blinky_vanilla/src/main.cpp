#include <FreeRTOS.h>
#include <functional>
#include <pico/stdlib.h>
#include <stdio.h>
#include <task.h>

void blink(void *pvParameters) {
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

struct HelloCountData {
    const char *msg;
    uint32_t counter;
};

void helloCount(HelloCountData *data) {
    stdio_init_all();
    while (true) {
        vTaskDelay(1000);
        printf("%04d %s\n", ++data->counter, data->msg);
    }
}

int main() {
    BaseType_t taskErr;
    TaskHandle_t blinky, talky;
    /* Create the task, storing the handle. */
    taskErr =
        xTaskCreate(blink, "Blinky task", 128, NULL, tskIDLE_PRIORITY, &blinky);
    configASSERT(pdPASS == taskErr);

    auto data = HelloCountData{"Hello, world multitasking!", 0};
    taskErr   = xTaskCreate((TaskFunction_t)(helloCount), "Hello task", 512,
                          &data, tskIDLE_PRIORITY, &talky);
    configASSERT(pdPASS == taskErr);

    vTaskStartScheduler();
    while (1) {
        configASSERT(0); /* We should never get here */
    }
}
