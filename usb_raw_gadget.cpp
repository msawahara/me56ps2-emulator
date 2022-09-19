#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

#include "usb_raw_gadget.h"

void usb_raw_gadget::dump_hex_and_ascii(void *data, const size_t length)
{
    uint8_t *c = reinterpret_cast<uint8_t *>(data);
    for (size_t offset = 0; offset < length; offset += 16) {
        printf("  %04lx: ", offset);
        for (size_t p = 0; p < 16; p++) {
            if (offset + p < length) {
                printf("%02x ", c[offset + p]);
            } else {
                printf("   ");
            }
        }
        for (size_t p = 0; p < 16 && offset + p < length; p++) {
            printf("%c", isprint(c[offset + p]) ? c[offset + p] : '.');
        }
        printf("\n");
    }
}

usb_raw_gadget::usb_raw_gadget(const char *file)
{
    fd = open(file, O_RDWR);
    if (fd < 0) {
        throw std::runtime_error((std::string) "open(): " + std::strerror(errno));
    }
}

usb_raw_gadget::~usb_raw_gadget(void)
{
    close();
}

void usb_raw_gadget::set_debug_level(const int level)
{
    debug_level = level;
}

void usb_raw_gadget::init(enum usb_device_speed speed, const char *driver_name, const char *device_name)
{
    struct usb_raw_init arg;
    strcpy((char *) arg.driver_name, driver_name);
    strcpy((char *) arg.device_name, device_name);
    arg.speed = speed;

    int ret = ioctl(fd, USB_RAW_IOCTL_INIT, &arg);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_INIT): " + std::strerror(errno));
    }
}

void usb_raw_gadget::run(void)
{
    int ret = ioctl(fd, USB_RAW_IOCTL_RUN, 0);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_RUN): " + std::strerror(errno));
    }
}

void usb_raw_gadget::close(void)
{
    if (fd < 0) {
        return;
    }

    ::close(fd);
    fd = -1;
}

void usb_raw_gadget::event_fetch(struct usb_raw_event *event)
{
    int ret = ioctl(fd, USB_RAW_IOCTL_EVENT_FETCH, event);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_EVENT_FETCH): " + std::strerror(errno));
    }
}

int usb_raw_gadget::eps_info(struct usb_raw_eps_info *info)
{
    int ret = ioctl(fd, USB_RAW_IOCTL_EPS_INFO, info);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_EPS_INFO): " + std::strerror(errno));
    }
    return ret;
}

int usb_raw_gadget::ep0_write(struct usb_raw_ep_io *io)
{
    int ret = ioctl(fd, USB_RAW_IOCTL_EP0_WRITE, io);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_EP0_WRITE): " + std::strerror(errno));
    }
    if (debug_level >= 1) {printf("ep0: write: transferred %d bytes.\n", ret);}
    if (debug_level >= 3) {dump_hex_and_ascii(io->data, ret);}
    return ret;
}

int usb_raw_gadget::ep0_read(struct usb_raw_ep_io *io)
{
    int ret = ioctl(fd, USB_RAW_IOCTL_EP0_READ, io);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_EP0_READ): " + std::strerror(errno));
    }
    if (debug_level >= 1) {printf("ep0: read: transferred %d bytes.\n", ret);}
    if (debug_level >= 3) {dump_hex_and_ascii(io->data, ret);}
    return ret;
}

void usb_raw_gadget::ep0_stall(void)
{
    if (debug_level >= 1) {printf("ep0: stall\n");}
    int ret = ioctl(fd, USB_RAW_IOCTL_EP0_STALL, 0);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_EP0_STALL): " + std::strerror(errno));
    }
}

int usb_raw_gadget::ep_enable(struct usb_endpoint_descriptor *desc)
{
    int ret = ioctl(fd, USB_RAW_IOCTL_EP_ENABLE, desc);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_EP_ENABLE): " + std::strerror(errno));
    }
    return ret;
}

int usb_raw_gadget::ep_write(struct usb_raw_ep_io *io)
{
    int ret = ioctl(fd, USB_RAW_IOCTL_EP_WRITE, io);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_EP_WRITE): " + std::strerror(errno));
    }
    if (debug_level >= 1) {printf("ep%d: write: transferred %d bytes.\n", io->ep, ret);}
    if (debug_level >= 3) {dump_hex_and_ascii(io->data, ret);}
    return ret;
}

int usb_raw_gadget::ep_read(struct usb_raw_ep_io *io)
{
    int ret = ioctl(fd, USB_RAW_IOCTL_EP_READ, io);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_EP_READ): " + std::strerror(errno));
    }
    if (debug_level >= 1) {printf("ep%d: read: transferred %d bytes.\n", io->ep, ret);}
    if (debug_level >= 3) {dump_hex_and_ascii(io->data, ret);}
    return ret;
}

void usb_raw_gadget::vbus_draw(uint32_t bMaxPower)
{
    int ret = ioctl(fd, USB_RAW_IOCTL_VBUS_DRAW, bMaxPower);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_VBUS_DRAW): " + std::strerror(errno));
    }
}

void usb_raw_gadget::configure()
{
    int ret = ioctl(fd, USB_RAW_IOCTL_CONFIGURE, 0);
    if (ret < 0) {
        throw std::runtime_error((std::string) "ioctl(USB_RAW_IOCTL_CONFIGURE): " + std::strerror(errno));
    }
}
