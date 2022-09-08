#include <cstdint>

#include <linux/usb/raw_gadget.h>

class usb_raw_control_event
{
    public:
        struct usb_raw_event event;
        struct usb_ctrlrequest ctrl;
        bool is_event(__u8 request_type);
        bool is_event(__u8 request_type, __u8 request);
        void print_debug_log(void);
        uint8_t get_request_type(void);
        const char* get_request_type_string(void);
        uint8_t get_request(void);
        const char* get_request_string(void);
        unsigned int get_descriptor_type(void);
        const char* get_descriptor_type_string(void);
};
