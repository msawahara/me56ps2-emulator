#include <cstdint>

#include <linux/usb/raw_gadget.h>

class usb_raw_gadget
{
    private:
        int fd;
        int debug_level = 0;
        void dump_hex_and_ascii(void *data, const size_t length);
    public:
        usb_raw_gadget(const char *file);
        ~usb_raw_gadget();
        void set_debug_level(const int level);
        void init(enum usb_device_speed speed, const char *driver_name, const char *device_name);
        void run(void);
        void close(void);
        void event_fetch(struct usb_raw_event *event);
        int eps_info(struct usb_raw_eps_info *info);
        int ep0_write(struct usb_raw_ep_io *io);
        int ep0_read(struct usb_raw_ep_io *io);
        void ep0_stall(void);
        int ep_enable(struct usb_endpoint_descriptor *desc);
        int ep_write(struct usb_raw_ep_io *io);
        int ep_read(struct usb_raw_ep_io *io);
        void vbus_draw(uint32_t bMaxPower);
        void configure();
};
