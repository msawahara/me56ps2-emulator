#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>

#include <linux/usb/raw_gadget.h>

#include "usb_raw_control_event.h"

bool usb_raw_control_event::is_event(__u8 request_type)
{
    return get_request_type() == request_type;
}

bool usb_raw_control_event::is_event(__u8 request_type, __u8 request)
{
    return is_event(request_type) && get_request() == request;
}

void usb_raw_control_event::print_debug_log(void)
{
    switch(event.type) {
        case USB_RAW_EVENT_CONNECT:
            printf("event type: USB_RAW_EVENT_CONNECT, length: %u\n", event.length);
            break;
        case USB_RAW_EVENT_CONTROL:
            printf("event type: USB_RAW_EVENT_CONTROL, length: %u\n", event.length);
            break;
        default:
            printf("event type: unknown(%u), length: %u\n", event.type, event.length);
            break;
    }
    if (event.type != USB_RAW_EVENT_CONTROL) {
        return;
    }
    printf("  request type = %s (0x%02x)\n", get_request_type_string(), get_request_type());
    printf("  request = %s (0x%02x)\n", get_request_string(), get_request());
    printf("  wIndex = 0x%04x, wLength = 0x%04x, wValue = 0x%04x\n", ctrl.wIndex, ctrl.wLength, ctrl.wValue);
    if (is_event(USB_TYPE_STANDARD, USB_REQ_GET_DESCRIPTOR)) {
        printf("  descriptor type = %s (0x%02x)\n", get_descriptor_type_string(), get_descriptor_type());
    }
}

uint8_t usb_raw_control_event::get_request_type(void)
{
    return ctrl.bRequestType & USB_TYPE_MASK;
}

const char *usb_raw_control_event::get_request_type_string(void)
{
    switch (get_request_type()) {
        case USB_TYPE_STANDARD:
            return "USB_TYPE_STANDARD";
        case USB_TYPE_VENDOR:
            return "USB_TYPE_VENDOR";
        default:
            return "unknown";
    }
}

uint8_t usb_raw_control_event::get_request(void)
{
    return ctrl.bRequest;
}

const char* usb_raw_control_event::get_request_string(void)
{
    static std::map<uint8_t, const char *> request_string_map = {
        {USB_REQ_GET_DESCRIPTOR, "USB_REQ_GET_DESCRIPTOR"},
        {USB_REQ_SET_CONFIGURATION, "USB_REQ_SET_CONFIGURATION"},
        {USB_REQ_SET_INTERFACE, "USB_REQ_SET_INTERFACE"}
    };

    auto request = get_request();

    if (request_string_map.find(request) != request_string_map.end()) {
        return request_string_map[request];
    }

    return "unknown";
}

unsigned int usb_raw_control_event::get_descriptor_type(void)
{
    return ctrl.wValue >> 8;
}

const char* usb_raw_control_event::get_descriptor_type_string(void)
{
    static std::map<unsigned int, const char *> descriptor_type_string_map = {
        {USB_DT_DEVICE, "USB_DT_DEVICE"},
        {USB_DT_CONFIG, "USB_DT_CONFIG"},
        {USB_DT_STRING, "USB_DT_STRING"},
        {USB_DT_INTERFACE, "USB_DT_INTERFACE"},
        {USB_DT_ENDPOINT, "USB_DT_ENDPOINT"},
        {USB_DT_DEVICE_QUALIFIER, "USB_DT_DEVICE_QUALIFIER"},
        {USB_DT_OTHER_SPEED_CONFIG, "USB_DT_OTHER_SPEED_CONFIG"},
        {USB_DT_INTERFACE_POWER, "USB_DT_INTERFACE_POWER"}
    };

    auto descriptor_type = get_descriptor_type();

    if (descriptor_type_string_map.find(descriptor_type) != descriptor_type_string_map.end()) {
        return descriptor_type_string_map[descriptor_type];
    }

    return "unknown";
}
