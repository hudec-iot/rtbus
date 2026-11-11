/*
 * SPDX-License-Identifier: MPL-2.0
 */

#include "native_board.h"

#include <zephyr/device.h>
#include <zephyr/irq.h>

#include <rtbus/runtime_serial.h>

static int native_api_rtbus_post(uint8_t task_id, uint32_t ctx_id,
                                 const void *payload, uint32_t payload_len)
{
    struct native_rtbus_post_msg msg = {
        .task_id = task_id,
        .ctx_id = ctx_id,
        .payload_len = payload_len,
    };

    int ret;

    /*
     * Keep the source payload valid until it has been copied into the runtime
     * queue. The upgrade handoff checks this counter before force-aborting the
     * native application thread.
     */
    atomic_inc(&native_api_busy);

    if (payload_len > NATIVE_RTBUS_POST_PAYLOAD_MAX) {
        ret = -RTBUS_EMSGSIZE;
        goto out;
    }

    if (payload_len != 0 && payload == NULL) {
        ret = -RTBUS_EINVAL;
        goto out;
    }

    if (payload_len != 0) {
        memcpy(msg.payload, payload, payload_len);
    }

    ret = k_msgq_put(&native_rtbus_post_msgq, &msg, K_NO_WAIT);

    if (ret == 0) {
        /* Wake the runtime loop; EMOS will drain this msgq from its context. */
        runtime_event_signal();
    }

out:
    atomic_dec(&native_api_busy);
    if (ret == 0) {
        /* Do not hold the upgrade guard while blocked. Only runtime-owned
         * data remains queued, and stopping the image resets this handshake.
         * No application callbacks are dispatched during admission wait.
         */
        ret = k_sem_take(&native_post_sem, K_FOREVER);
        if (ret == 0) {
            ret = native_post_status;
        }
    }
    return ret;
}

static int native_api_result_wait(uint32_t timeout_ms)
{
    bool forever = timeout_ms == RTBUS_WAIT_FOREVER;
    int64_t deadline = k_uptime_get() + timeout_ms;

    while (k_sem_take(&native_result_sem, K_NO_WAIT) != 0) {
        int64_t remaining_ms = deadline - k_uptime_get();
        uint32_t events;

        if (atomic_get(&native_patch_handoff) != 0) {
            return -EINTR;
        }
        if (!forever && remaining_ms <= 0) {
            return -EAGAIN;
        }

        events = k_event_wait(&native_wake_event, NATIVE_WAKE_ALL, false,
                             forever ? K_FOREVER : K_MSEC(remaining_ms));
        (void)k_event_clear(&native_wake_event, events);
        native_service_dispatch_events();
    }

    return 0;
}

static int native_api_on_event(runtime_event_callback_t callback)
{
    native_event_callback = callback;
    return 0;
}

static int32_t native_api_k_delay(uint32_t delay_ms)
{
    int64_t deadline = k_uptime_get() + delay_ms;

    native_service_dispatch_events();

    while (true) {
        int64_t remaining_ms = deadline - k_uptime_get();
        uint32_t events;

        if (atomic_get(&native_patch_handoff) != 0) {
            return -EINTR;
        }

        if (remaining_ms <= 0) {
            break;
        }

        events = k_event_wait(&native_wake_event,
                              NATIVE_WAKE_ALL,
                              false,
                              K_MSEC(remaining_ms));
        (void)k_event_clear(&native_wake_event, events);
        if ((events & NATIVE_WAKE_IRQ) != 0U) {
            native_service_dispatch_events();
        }
    }

    native_service_dispatch_events();
    return 0;
}

static uint32_t native_api_millis(void)
{
    return k_uptime_get_32();
}

/* Application interrupt control only gates deferred callback dispatch.
 * Kernel, BLE, USB and GPIO capture remain interruptible. Calls are idempotent.
 */
static void native_api_irq_disable(void)
{
    native_irqs_disabled = true;
}

static void native_api_irq_restore(void)
{
    if (native_irqs_disabled) {
        native_irqs_disabled = false;
        native_service_dispatch_events();
    }
}

static void native_api_delay_us(uint32_t us)
{
    k_busy_wait(us);
}

/* Sample the 32-bit hardware clock at least once per second so its wrap
 * can be extended without consuming application RAM. Current board clocks
 * wrap more slowly than this interval. The accumulator survives app reloads.
 */
static uint32_t native_clock_last;
static uint64_t native_clock_cycles;
static uint64_t native_clock_origin_us;
static bool native_clock_started;
static void native_clock_sample(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    unsigned int key = irq_lock();
    uint32_t now = k_cycle_get_32();
    native_clock_cycles += (uint32_t)(now - native_clock_last);
    native_clock_last = now;
    irq_unlock(key);
}
K_TIMER_DEFINE(native_clock_timer, native_clock_sample, NULL);

static uint32_t native_api_micros(void)
{
    unsigned int key = irq_lock();
    if (!native_clock_started) {
        native_clock_last = k_cycle_get_32();
        native_clock_origin_us = k_ticks_to_us_floor64(k_uptime_ticks());
        native_clock_started = true;
        k_timer_start(&native_clock_timer, K_SECONDS(1), K_SECONDS(1));
    }
    uint32_t now = k_cycle_get_32();
    native_clock_cycles += (uint32_t)(now - native_clock_last);
    native_clock_last = now;
    uint64_t cycles = native_clock_cycles;
    uint64_t origin = native_clock_origin_us;
    irq_unlock(key);
    return (uint32_t)(origin + k_cyc_to_us_floor64(cycles));
}

static void native_api_yield(void)
{
    native_service_dispatch_events();
    if (atomic_get(&native_patch_handoff) != 0) { k_sleep(K_FOREVER); }
    k_yield();
}

static int native_api_serial_control(uint32_t port, uint32_t operation,
                                      uint32_t baud, uint32_t format)
{
    atomic_inc(&native_api_busy);
    int result = rtbus_runtime_serial_control(port, operation, baud, format);
    atomic_dec(&native_api_busy);
    return result;
}

static const struct gpio_dt_spec *native_gpio_physical_resolve(uint32_t pin)
{
#if defined(CONFIG_SOC_NRF52840) && \
    DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay) && \
    DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
    /* Stable addresses are also used by interrupt registrations. Resolving
     * a pin does not configure it; gpio_configure performs that operation.
     */
    static struct gpio_dt_spec physical_pins[48];
    uint32_t port = (pin >> 8U) & 0xffU;
    uint32_t number = pin & 0xffU;

    if ((pin & UINT32_C(0xffff0000)) != RTBUS_PIN_PHYSICAL_FLAG ||
        port > 1U || number >= (port == 0U ? 32U : 16U)) {
        return NULL;
    }

    struct gpio_dt_spec *entry = &physical_pins[port * 32U + number];
    entry->port = port == 0U ? DEVICE_DT_GET(DT_NODELABEL(gpio0)) :
                              DEVICE_DT_GET(DT_NODELABEL(gpio1));
    entry->pin = number;
    entry->dt_flags = GPIO_ACTIVE_HIGH;
    return entry;
#else
    ARG_UNUSED(pin);
    return NULL;
#endif
}

static int native_gpio_resolve(uint32_t pin,
                               const struct gpio_dt_spec **entry)
{
    if ((pin & RTBUS_PIN_PHYSICAL_FLAG) != 0U) {
        *entry = native_gpio_physical_resolve(pin);
        if (*entry == NULL) {
            return -EINVAL;
        }
        return gpio_is_ready_dt(*entry) ? 0 : -ENODEV;
    }

    size_t count;
    const struct native_board_gpio_pin *pins =
        native_board_gpio_pins(&count);

    for (size_t i = 0U; i < count; i++) {
        if (pins[i].app_pin == pin) {
            *entry = &pins[i].gpio;
            if (!gpio_is_ready_dt(*entry)) {
                return -ENODEV;
            }

            return 0;
        }
    }

    return -EINVAL;
}

static int native_api_gpio_configure(uint32_t pin, uint32_t mode)
{
    const struct gpio_dt_spec *entry;
    gpio_flags_t flags;
    int ret;

    ret = native_gpio_resolve(pin, &entry);
    if (ret != 0) {
        return ret;
    }

    switch (mode) {
    case RUNTIME_GPIO_MODE_INPUT:
        flags = GPIO_INPUT;
        break;
    case RUNTIME_GPIO_MODE_OUTPUT:
        flags = GPIO_OUTPUT;
        break;
    case RUNTIME_GPIO_MODE_INPUT_PULLUP:
        flags = GPIO_INPUT | GPIO_PULL_UP;
        break;
    case RUNTIME_GPIO_MODE_INPUT_PULLDOWN:
        flags = GPIO_INPUT | GPIO_PULL_DOWN;
        break;
    case RUNTIME_GPIO_MODE_OUTPUT_OPENDRAIN:
        flags = GPIO_OUTPUT | GPIO_OPEN_DRAIN;
        break;
    default:
        return -EINVAL;
    }

    return gpio_pin_configure_dt(entry, flags);
}

static int native_api_gpio_write(uint32_t pin, uint32_t value)
{
    const struct gpio_dt_spec *entry;
    int ret;

    ret = native_gpio_resolve(pin, &entry);
    if (ret != 0) {
        return ret;
    }

    return gpio_pin_set_dt(entry, value != 0U);
}

static int native_api_gpio_read(uint32_t pin)
{
    const struct gpio_dt_spec *entry;
    int ret;

    ret = native_gpio_resolve(pin, &entry);
    if (ret != 0) {
        return ret;
    }

    return gpio_pin_get_dt(entry);
}

#define NATIVE_GPIO_IRQ_COUNT CONFIG_RTBUS_NATIVE_GPIO_IRQ_COUNT
struct native_gpio_irq {
    struct gpio_callback gpio_callback;
    const struct gpio_dt_spec *pin;
    rtbus_gpio_irq_callback_t callback;
    void *argument;
    atomic_t pending;
};
static struct native_gpio_irq native_gpio_irqs[NATIVE_GPIO_IRQ_COUNT];

static void native_gpio_irq_handler(const struct device *dev,
                                     struct gpio_callback *callback,
                                     gpio_port_pins_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(pins);
    struct native_gpio_irq *entry = CONTAINER_OF(callback, struct native_gpio_irq, gpio_callback);
    if (entry->callback != NULL && atomic_get(&native_patch_handoff) == 0) {
        /* Coalesce repeated edges until the native thread dispatches them.
         * Never execute application code in the hardware ISR.
         */
        atomic_set(&entry->pending, 1);
        (void)k_event_post(&native_wake_event, NATIVE_WAKE_IRQ);
    }
}

static void native_gpio_irq_dispatch(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(native_gpio_irqs); ++i) {
        struct native_gpio_irq *entry = &native_gpio_irqs[i];

        if (native_irqs_disabled || atomic_get(&native_patch_handoff) != 0) {
            break;
        }
        if (atomic_cas(&entry->pending, 1, 0) && entry->callback != NULL) {
            entry->callback(entry->argument);
        }
    }
}

static int native_gpio_irq_remove(struct native_gpio_irq *entry)
{
    if (entry->pin == NULL) { return 0; }
    entry->callback = NULL;
    int result = gpio_pin_interrupt_configure_dt(entry->pin, GPIO_INT_DISABLE);
    int removed = gpio_remove_callback(entry->pin->port, &entry->gpio_callback);
    entry->pin = NULL;
    entry->argument = NULL;
    atomic_clear(&entry->pending);
    return result != 0 ? result : removed;
}

static void native_gpio_irq_reset(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(native_gpio_irqs); ++i) {
        (void)native_gpio_irq_remove(&native_gpio_irqs[i]);
    }
}

static int native_api_gpio_interrupt(uint32_t pin, uint32_t mode,
                                      rtbus_gpio_irq_callback_t callback, void *argument)
{
    const struct gpio_dt_spec *spec;
    int result = native_gpio_resolve(pin, &spec);
    if (result != 0) { return result; }
    gpio_flags_t flags;
    switch (mode) {
    case RTBUS_GPIO_IRQ_DETACH: flags = GPIO_INT_DISABLE; break;
    case RTBUS_GPIO_IRQ_LOW: flags = GPIO_INT_LEVEL_LOW; break;
    case RTBUS_GPIO_IRQ_HIGH: flags = GPIO_INT_LEVEL_HIGH; break;
    case RTBUS_GPIO_IRQ_CHANGE: flags = GPIO_INT_EDGE_BOTH; break;
    case RTBUS_GPIO_IRQ_FALLING: flags = GPIO_INT_EDGE_FALLING; break;
    case RTBUS_GPIO_IRQ_RISING: flags = GPIO_INT_EDGE_RISING; break;
    default: return -EINVAL;
    }
    if (mode != RTBUS_GPIO_IRQ_DETACH && callback == NULL) { return -EINVAL; }
    if (atomic_get(&native_patch_handoff) != 0 && mode != RTBUS_GPIO_IRQ_DETACH) { return -EBUSY; }
    struct native_gpio_irq *entry = NULL;
    for (size_t i = 0; i < ARRAY_SIZE(native_gpio_irqs); ++i) {
        if (native_gpio_irqs[i].pin != NULL &&
            native_gpio_irqs[i].pin->port == spec->port &&
            native_gpio_irqs[i].pin->pin == spec->pin) {
            entry = &native_gpio_irqs[i];
            result = native_gpio_irq_remove(entry);
            if (result != 0) { return result; }
            break;
        }
        if (entry == NULL && native_gpio_irqs[i].pin == NULL) { entry = &native_gpio_irqs[i]; }
    }
    if (mode == RTBUS_GPIO_IRQ_DETACH) { return 0; }
    if (entry == NULL) { return -ENOMEM; }
    entry->pin = spec;
    entry->callback = callback;
    entry->argument = argument;
    gpio_init_callback(&entry->gpio_callback, native_gpio_irq_handler, BIT(spec->pin));
    result = gpio_add_callback(spec->port, &entry->gpio_callback);
    if (result == 0) { result = gpio_pin_interrupt_configure_dt(spec, flags); }
    if (result != 0) { (void)native_gpio_irq_remove(entry); }
    return result;
}

static void native_api_rtt_vprintf(const char *fmt, va_list args)
{
    if (atomic_get(&native_console_suppressed) != 0) {
        return;
    }

    vprintk(fmt, args);
}

static size_t native_api_serial_write(uint32_t port, const uint8_t *data,
                                      size_t size)
{
    size_t written = 0U;

    if (data == NULL || size == 0U ||
        atomic_get(&native_console_suppressed) != 0) {
        return 0U;
    }

    /*
     * The source buffer may live in native application RAM. Keep upgrade
     * handoff from clearing shared RAM while runtime is draining it.
     */
    atomic_inc(&native_api_busy);

    written = rtbus_runtime_serial_write(port, data, size);

    atomic_dec(&native_api_busy);
    return written;
}

static size_t native_api_serial_vprintf(uint32_t port, const char *fmt,
                                        va_list args)
{
    char buffer[160];
    int length;

    if (fmt == NULL || atomic_get(&native_console_suppressed) != 0) {
        return 0U;
    }

    /*
     * The format string and varargs belong to native application RAM. Format
     * into runtime-owned memory before handing bytes to the selected serial.
     */
    atomic_inc(&native_api_busy);
    length = vsnprintk(buffer, sizeof(buffer), fmt, args);
    atomic_dec(&native_api_busy);

    if (length <= 0) {
        return 0U;
    }

    if ((size_t)length >= sizeof(buffer)) {
        length = sizeof(buffer) - 1;
    }

    return native_api_serial_write(port, (const uint8_t *)buffer,
                                   (size_t)length);
}

static int32_t native_api_serial_read(uint32_t port)
{
    uint8_t byte;

    if (atomic_get(&native_console_suppressed) != 0) {
        return -1;
    }

    if (rtbus_runtime_serial_read(port, &byte, 1U) == 0U) {
        return -1;
    }

    return (int32_t)byte;
}

/* Strong override for mod_rtbus's weak async-post hook. */
__strong int rtbus_process_async_posts(void)
{
    struct native_rtbus_post_msg msg;

    while (k_msgq_get(&native_rtbus_post_msgq, &msg, K_NO_WAIT) == 0) {
        int ret;

        /*
         * Rtbus execution happens in runtime context. Mark it busy so
         * upgrade handoff waits instead of aborting the producer while this
         * request is being consumed.
         */
        atomic_inc(&native_api_busy);
        ret = rtbus_post(msg.task_id,
                            msg.ctx_id,
                            msg.payload_len == 0U ? NULL : msg.payload,
                            msg.payload_len);
        atomic_dec(&native_api_busy);

        /* EMOS is the single authority for accepting an event. Return its
         * admission result to the producer rather than leaving the native
         * wrapper waiting for a completion from a nonexistent service.
         */
        native_post_status = ret == -ENOTSUP ? -RTBUS_ENOTSUP : ret;
        k_sem_give(&native_post_sem);
    }

    return 0;
}
