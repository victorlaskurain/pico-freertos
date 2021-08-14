#include <FreeRTOS.h>
#include <functional>
#include <pico/stdlib.h>
#include <stdio.h>
#include <task.h>

#include <vla/task.hpp>

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

void helloCount(const char *msg, uint32_t counter) {
    stdio_init_all();
    while (true) {
        vTaskDelay(1000);
        printf("%04d %s\n", ++counter, msg);
    }
}

int main() {
    auto blinky = vla::Task(blink, "Blinky task");
    configASSERT(blinky);
    auto talky =
        vla::Task(std::bind(helloCount, "Hello, world multitasking!", 0),
                  "Hello count task", 512);
    configASSERT(talky);

    vTaskStartScheduler();
    while (1) {
        configASSERT(0); /* We should never get here */
    }
}
