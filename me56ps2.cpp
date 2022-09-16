#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <unistd.h>

#include "usb_raw_gadget.h"
#include "usb_raw_control_event.h"
#include "ring_buffer.h"
#include "tcp_sock.h"

#include "me56ps2.h"

std::thread *thread_bulk_in = nullptr;
std::thread *thread_bulk_out = nullptr;

ring_buffer<char> usb_tx_buffer(524288);
tcp_sock *sock;

int debug_level = 0;

std::atomic<bool> connected(false);

void ring_callback()
{
    const std::string ring = "RING\r\n";
    usb_tx_buffer.enqueue(ring.c_str(), ring.length());
    usb_tx_buffer.notify_one();

    printf("Clinet connected.\n");
}

void recv_callback(const char *buffer, size_t length)
{
    if (connected.load()) {
        const auto sent_length = usb_tx_buffer.enqueue(buffer, length);
        if (sent_length < length) {
            printf("Transmit buffer is full! (overflow %ld bytes.)\n", length - sent_length);
        }
        usb_tx_buffer.notify_one();
    }
}

void *usb_bulk_in_thread(usb_raw_gadget *usb, int ep_num)
{
    struct usb_packet_control pkt;
    auto timeout_at = std::chrono::steady_clock::now();

    while (true) {
        const auto now = std::chrono::steady_clock::now();
        while (timeout_at <= now) {
            timeout_at += std::chrono::milliseconds(40);
        }
        usb_tx_buffer.wait(timeout_at);

        pkt.data[0] = 0x31;
        pkt.data[1] = 0x60;
        int payload_length = usb_tx_buffer.dequeue(&pkt.data[2], sizeof(pkt.data) - 2);

        pkt.header.ep = ep_num;
        pkt.header.flags = 0;
        pkt.header.length = 2 + payload_length;

        if (connected.load()) {pkt.data[0] |= 0x80;}

        usb->ep_write(reinterpret_cast<struct usb_raw_ep_io *>(&pkt));
    }

    return NULL;
}

void *usb_bulk_out_thread(usb_raw_gadget *usb, int ep_num) {
    struct usb_packet_bulk pkt;
    std::string buffer;

    // modem echo flag
    bool echo = false;

    while (true) {
        pkt.header.ep = ep_num;
        pkt.header.flags = 0;
        pkt.header.length = sizeof(pkt.data);

        int ret = usb->ep_read(reinterpret_cast<struct usb_raw_ep_io *>(&pkt));
        int payload_length = pkt.data[0] >> 2;
        if (payload_length != ret - 1) {
            printf("Payload length mismatch! (payload length in header: %d, received payload: %d)\n", payload_length, ret - 1);
            payload_length = std::min(payload_length, ret - 1);
        }
        buffer.append(&pkt.data[1], payload_length);

        // Off-line mode loop
        while (!connected.load()) {
            bool enter_online = false;

            // Fetch one line from the receive buffer
            auto newline_pos = buffer.find('\x0d');
            if (newline_pos == std::string::npos) {break;}
            std::string line = buffer.substr(0, newline_pos);
            buffer.erase(0, newline_pos + 1);
            if (line.empty()) {break;}

            printf("AT command: %s\n", line.c_str());

            if (echo) {
                const auto s = line + "\r\n";
                usb_tx_buffer.enqueue(s.c_str(), s.length());
            }

            std::string reply = "OK\r\n";
            if (line == "AT&F") {echo = true;} // Restore factory default (turn on echo only in this emulator)
            if (line == "ATE0") {echo = false;} // Turn off echo
            if (line == "ATA") {
                // Answer an incoming call
                reply = "CONNECT 57600 V42\r\n";
                enter_online = true;
            }
            if (strncmp(line.c_str(), "ATD", 3) == 0) {
                // Dial. Ignore after "ATD"
                if (sock->connect()) {
                    reply = "CONNECT 57600 V42\r\n";
                    enter_online = true;
                } else {
                    reply = "BUSY\r\n";
                }
            }

            usb_tx_buffer.enqueue(reply.c_str(), reply.length());
            usb_tx_buffer.notify_one();

            if (enter_online) {
                printf("Enter on-line mode.\n");
                connected.store(true);
            }
        }

        // On-line mode loop
        while (connected.load() && buffer.length() > 0) {
            sock->send(buffer.c_str(), buffer.length());
            buffer.clear();
        }
    }

    return NULL;
}

bool process_control_packet(usb_raw_gadget *usb, usb_raw_control_event *e, struct usb_packet_control *pkt)
{
    if (e->is_event(USB_TYPE_STANDARD, USB_REQ_GET_DESCRIPTOR)) {
        const auto descriptor_type = e->get_descriptor_type();
        if (descriptor_type == USB_DT_DEVICE) {
            memcpy(pkt->data, &me56ps2_device_descriptor, sizeof(me56ps2_device_descriptor));
            pkt->header.length = sizeof(me56ps2_device_descriptor);
            return true;
        }
        if (descriptor_type == USB_DT_CONFIG) {
            memcpy(pkt->data, &me56ps2_config_descriptors, sizeof(me56ps2_config_descriptors));
            pkt->header.length = sizeof(me56ps2_config_descriptors);
            return true;
        }
        if (descriptor_type == USB_DT_STRING) {
            const auto id = e->ctrl.wValue & 0x00ff;
            if (id >= STRING_DESCRIPTORS_NUM) {return false;} // invalid string id
            const auto len = reinterpret_cast<const struct _usb_string_descriptor<1> *>(me56ps2_string_descriptors[id])->bLength;
            memcpy(pkt->data, me56ps2_string_descriptors[id], len);
            pkt->header.length = len;
            return true;
        }
    }
    if (e->is_event(USB_TYPE_STANDARD, USB_REQ_SET_CONFIGURATION)) {
        if (thread_bulk_in == nullptr) {
            const int ep_num_bulk_in = usb->ep_enable(
                reinterpret_cast<struct usb_endpoint_descriptor *>(&me56ps2_config_descriptors.endpoint_bulk_in));
            thread_bulk_in = new std::thread(usb_bulk_in_thread, usb, ep_num_bulk_in);
        }
        if (thread_bulk_out == nullptr) {
            const int ep_num_bulk_out = usb->ep_enable(
                reinterpret_cast<struct usb_endpoint_descriptor *>(&me56ps2_config_descriptors.endpoint_bulk_out));
            thread_bulk_out = new std::thread(usb_bulk_out_thread, usb, ep_num_bulk_out);
        }
        usb->vbus_draw(me56ps2_config_descriptors.config.bMaxPower);
        usb->configure();
        printf("USB configurated.\n");
        pkt->header.length = 0;
        return true;
    }
    if (e->is_event(USB_TYPE_STANDARD, USB_REQ_SET_INTERFACE)) {
        pkt->header.length = 0;
        return true;
    }
    if (e->is_event(USB_TYPE_VENDOR, 0x01)) {
        pkt->header.length = 0;

        if ((e->ctrl.wValue & 0x0101) == 0x0100) {
            // set DTR to LOW for on-hook
            if (debug_level >= 2) {printf("on-hook\n");};
        } else if ((e->ctrl.wValue & 0x0101) == 0x0101) {
            // set DTR to HIGH for off-hook
            if (debug_level >= 2) {printf("off-hook\n");};
        }

        return true;
    }
    if (e->is_event(USB_TYPE_VENDOR)) {
        pkt->header.length = 0;
        return true;
    }

    return false;
}

bool event_usb_control_loop(usb_raw_gadget *usb)
{
    usb_raw_control_event e;
    e.event.type = 0;
    e.event.length = sizeof(e.ctrl);

    struct usb_packet_control pkt;
    pkt.header.ep = 0;
    pkt.header.flags = 0;
    pkt.header.length = 0;

    usb->event_fetch(&e.event);
    if (debug_level >= 1) {e.print_debug_log();}

    switch(e.event.type) {
        case USB_RAW_EVENT_CONNECT:
            break;
        case USB_RAW_EVENT_CONTROL:
            if (!process_control_packet(usb, &e, &pkt)) {
                usb->ep0_stall();
                break;
            }

            pkt.header.length = std::min(pkt.header.length, static_cast<unsigned int>(e.ctrl.wLength));
            if (e.ctrl.bRequestType & USB_DIR_IN) {
                usb->ep0_write(reinterpret_cast<struct usb_raw_ep_io *>(&pkt));
            } else {
                usb->ep0_read(reinterpret_cast<struct usb_raw_ep_io *>(&pkt));
            }
            break;
        default:
            break;
    }

    return true;
}

void show_usage(char *prog_name, bool verbose)
{
    printf("Usage: %s [-svh] ip_addr port [usb_driver] [usb_device]\n", prog_name);
    if (!verbose) {return;}

    printf("\n");
    printf("Options:\n");
    printf("  -s    run as server\n");
    printf("  -v    verbose. increment log level\n");
    printf("  -h    show this help message.\n");
    printf("\n");
    printf("Parameters:\n");
    printf("  ip_addr       server IPv4 address\n");
    printf("  port          port number\n");
    printf("  usb_driver    driver name (default: %s)\n", USB_RAW_GADGET_DRIVER_DEFAULT);
    printf("  usb_device    device name (default: %s)\n", USB_RAW_GADGET_DEVICE_DEFAULT);
    return;
}

int main(int argc, char *argv[])
{
    const char *driver = USB_RAW_GADGET_DRIVER_DEFAULT;
    const char *device = USB_RAW_GADGET_DEVICE_DEFAULT;
    const char *ip_addr = nullptr;
    int port = -1;
    bool is_server = false;

    int opt;
    while((opt = getopt(argc, argv, "svh")) != -1) {
        switch(opt) {
            case 's':
                is_server = true;
                break;
            case 'v':
                debug_level++;
                break;
            case 'h':
                show_usage(argv[0], true);
                exit(0);
            default:
                show_usage(argv[0], false);
                exit(1);
        }
    }

    if (optind < argc) {ip_addr = argv[optind++];}
    if (optind < argc) {port = atoi(argv[optind++]);}
    if (optind < argc) {driver = argv[optind++];}
    if (optind < argc) {device = argv[optind++];}

    if (ip_addr == nullptr || port == -1) {
        show_usage(argv[0], false);
        exit(1);
    }

    usb_raw_gadget *usb = new usb_raw_gadget("/dev/raw-gadget");
    usb->set_debug_level(debug_level);
    usb->init(USB_SPEED_HIGH, driver, device);
    usb->run();

    sock = new tcp_sock(is_server, ip_addr, port);
    sock->set_debug_level(debug_level);
    sock->set_ring_callback(ring_callback);
    sock->set_recv_callback(recv_callback);

    while(event_usb_control_loop(usb));

    delete usb;

    return 0;
}