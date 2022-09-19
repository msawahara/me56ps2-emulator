#if defined(HW_NANOPI_NEO2) // for NanoPi NEO2
constexpr char USB_RAW_GADGET_DRIVER_DEFAULT[] = "musb-hdrc";
constexpr char USB_RAW_GADGET_DEVICE_DEFAULT[] = "musb-hdrc.2.auto";
#elif defined(HW_RPI_ZERO) // for Raspberry Pi Zero W
constexpr char USB_RAW_GADGET_DRIVER_DEFAULT[] = "20980000.usb";
constexpr char USB_RAW_GADGET_DEVICE_DEFAULT[] = "20980000.usb";
#elif defined(HW_RPI_ZERO2) // for Raspberry Pi Zero 2 W
constexpr char USB_RAW_GADGET_DRIVER_DEFAULT[] = "3f980000.usb";
constexpr char USB_RAW_GADGET_DEVICE_DEFAULT[] = "3f980000.usb";
#else // for Raspberry Pi 4 Model B
constexpr char USB_RAW_GADGET_DRIVER_DEFAULT[] = "fe980000.usb";
constexpr char USB_RAW_GADGET_DEVICE_DEFAULT[] = "fe980000.usb";
#endif

constexpr auto TCP_DEFAULT_PORT = 10023;

constexpr auto BCD_USB = 0x0110U; // USB 1.1
constexpr auto BCD_DEVICE = 0x0101U;
constexpr auto USB_VENDOR = 0x0590U; // Omron Corp.
constexpr auto USB_PRODUCT = 0x001aU; // ME56PS2

constexpr auto ENDPOINT_ADDR_BULK = 2U;
constexpr auto MAX_PACKET_SIZE_CONTROL = 64U; // 8 in original ME56PS2
constexpr auto MAX_PACKET_SIZE_BULK = 64U;

constexpr auto STRING_ID_MANUFACTURER = 1U;
constexpr auto STRING_ID_PRODUCT = 2U;
constexpr auto STRING_ID_SERIAL = 3U;

struct usb_packet_control {
    struct usb_raw_ep_io header;
    char data[MAX_PACKET_SIZE_CONTROL];
};

struct usb_packet_bulk {
    struct usb_raw_ep_io header;
    char data[MAX_PACKET_SIZE_BULK];
};

struct _usb_endpoint_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__ ((packed));

template<int N>
struct _usb_string_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    char16_t wData[N];
} __attribute__ ((packed));

struct usb_config_descriptors {
    struct usb_config_descriptor config;
    struct usb_interface_descriptor interface;
    struct _usb_endpoint_descriptor endpoint_bulk_in;
    struct _usb_endpoint_descriptor endpoint_bulk_out;
};

struct usb_device_descriptor me56ps2_device_descriptor = {
    .bLength            = USB_DT_DEVICE_SIZE,
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = __constant_cpu_to_le16(BCD_USB),
    .bDeviceClass       = 0,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = MAX_PACKET_SIZE_CONTROL,
    .idVendor           = __constant_cpu_to_le16(USB_VENDOR),
    .idProduct          = __constant_cpu_to_le16(USB_PRODUCT),
    .bcdDevice          = __constant_cpu_to_le16(BCD_DEVICE),
    .iManufacturer      = STRING_ID_MANUFACTURER,
    .iProduct           = STRING_ID_PRODUCT,
    .iSerialNumber      = STRING_ID_SERIAL,
    .bNumConfigurations = 1,
};

struct usb_config_descriptors me56ps2_config_descriptors = {
    .config = {
        .bLength             = USB_DT_CONFIG_SIZE,
        .bDescriptorType     = USB_DT_CONFIG,
        .wTotalLength        = __cpu_to_le16(sizeof(me56ps2_config_descriptors)),
        .bNumInterfaces      = 1,
        .bConfigurationValue = 1,
        .iConfiguration      = 2,
        .bmAttributes        = USB_CONFIG_ATT_WAKEUP,
        .bMaxPower           = 0x1e, // 60mA
    },
    .interface = {
        .bLength             = USB_DT_INTERFACE_SIZE,
        .bDescriptorType     = USB_DT_INTERFACE,
        .bInterfaceNumber    = 0,
        .bAlternateSetting   = 0,
        .bNumEndpoints       = 2,
        .bInterfaceClass     = 0xff, // Vendor Specific class
        .bInterfaceSubClass  = 0xff,
        .bInterfaceProtocol  = 0xff,
        .iInterface          = 2,
    },
    .endpoint_bulk_in = {
        .bLength             = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType     = USB_DT_ENDPOINT,
        .bEndpointAddress    = USB_DIR_IN | ENDPOINT_ADDR_BULK,
        .bmAttributes        = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize      = MAX_PACKET_SIZE_BULK,
        .bInterval           = 0,
    },
    .endpoint_bulk_out = {
        .bLength             = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType     = USB_DT_ENDPOINT,
        .bEndpointAddress    = USB_DIR_OUT | ENDPOINT_ADDR_BULK,
        .bmAttributes        = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize      = MAX_PACKET_SIZE_BULK,
        .bInterval           = 0,
    }
};

const struct _usb_string_descriptor<1> me56ps2_string_descriptor_0 = {
    .bLength = sizeof(me56ps2_string_descriptor_0),
    .bDescriptorType = USB_DT_STRING,
    .wData = {__constant_cpu_to_le16(0x0409)},
};

// FIXME: specify byte order
const struct _usb_string_descriptor<3> me56ps2_string_descriptor_1 = { // Manufacturer
    .bLength = sizeof(me56ps2_string_descriptor_1),
    .bDescriptorType = USB_DT_STRING,
    .wData = {u'N', u'/', u'A'},
};

const struct _usb_string_descriptor<14> me56ps2_string_descriptor_2 = { // Product
    .bLength = sizeof(me56ps2_string_descriptor_2),
    .bDescriptorType = USB_DT_STRING,
    .wData = {u'M', u'o', u'd', u'e', u'm', u' ', u'e', u'm', u'u', u'l', u'a', u't', u'o', u'r'},
};

const struct _usb_string_descriptor<3> me56ps2_string_descriptor_3 = { // Serial
    .bLength = sizeof(me56ps2_string_descriptor_3),
    .bDescriptorType = USB_DT_STRING,
    .wData = {u'N', u'/', u'A'},
};

constexpr auto STRING_DESCRIPTORS_NUM = 4;
const void *me56ps2_string_descriptors[STRING_DESCRIPTORS_NUM] = {
    &me56ps2_string_descriptor_0,
    &me56ps2_string_descriptor_1,
    &me56ps2_string_descriptor_2,
    &me56ps2_string_descriptor_3,
};
