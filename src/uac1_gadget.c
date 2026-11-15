/*
 * UAC1 USB Audio Gadget for Phicomm R1 (Rockchip RK3229 / Linux 3.10)
 *
 * Implements USB Audio Class 1.0 Sink (Speaker):
 * - High-Speed / Full-Speed Isochronous OUT endpoint (Adaptive, Stereo, 16-bit S16_LE, 48000 Hz)
 * - EP0 Class Control handling for Sampling Frequency
 * - Lockless ring buffer + /dev/uac1_audio misc device for low-latency userspace streaming
 * - Hot-pluggable hook into Android USB gadget framework (supported_functions[8])
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
typedef unsigned long size_t;
typedef signed long ssize_t;
typedef unsigned int gfp_t;
typedef unsigned short umode_t;
typedef long loff_t;
typedef _Bool bool;

#define NULL ((void *)0)
#define true 1
#define false 0

#define GFP_KERNEL 0xd0
#define GFP_ATOMIC 0x20

#define EINVAL 22
#define ENOMEM 12
#define EBUSY 16
#define ENODEV 19
#define EAGAIN 11
#define EOPNOTSUPP 95
#define ERESTARTSYS 512

#define POLLIN 0x0001
#define POLLPRI 0x0002
#define POLLOUT 0x0004
#define POLLERR 0x0008
#define POLLHUP 0x0010
#define POLLNVAL 0x0020
#define POLLRDNORM 0x0040

#define TASK_INTERRUPTIBLE 1
#define TASK_RUNNING 0

#define MISC_DYNAMIC_MINOR 255

/* USB Definitions */
#define USB_DIR_OUT 0
#define USB_DIR_IN 0x80

#define USB_DT_DEVICE 0x01
#define USB_DT_CONFIG 0x02
#define USB_DT_STRING 0x03
#define USB_DT_INTERFACE 0x04
#define USB_DT_ENDPOINT 0x05
#define USB_DT_INTERFACE_ASSOCIATION 0x0b
#define USB_DT_CS_INTERFACE 0x24
#define USB_DT_CS_ENDPOINT 0x25

#define USB_CLASS_AUDIO 0x01
#define USB_SUBCLASS_AUDIOCONTROL 0x01
#define USB_SUBCLASS_AUDIOSTREAMING 0x02

#define UAC_HEADER 0x01
#define UAC_INPUT_TERMINAL 0x02
#define UAC_OUTPUT_TERMINAL 0x03
#define UAC_AS_GENERAL 0x01
#define UAC_FORMAT_TYPE 0x02
#define UAC_FORMAT_TYPE_I 0x01
#define UAC_EP_GENERAL 0x01

#define UAC_TERMINAL_STREAMING 0x0101
#define UAC_OUTPUT_TERMINAL_SPEAKER 0x0301

#define UAC_SET_CUR 0x01
#define UAC_GET_CUR 0x81
#define UAC_GET_MIN 0x82
#define UAC_GET_MAX 0x83
#define UAC_GET_RES 0x84

#define UAC_EP_CS_ATTR_SAMPLE_RATE 0x01

#define UAC_FEATURE_UNIT 0x06
#define UAC_FU_MUTE      0x01
#define UAC_FU_VOLUME    0x02

#define USB_ENDPOINT_XFER_ISOC 1
#define USB_ENDPOINT_SYNC_ADAPTIVE (2 << 2)

#define USB_TYPE_MASK (0x03 << 5)
#define USB_TYPE_STANDARD (0x00 << 5)
#define USB_TYPE_CLASS (0x01 << 5)
#define USB_RECIP_MASK 0x1f
#define USB_RECIP_DEVICE 0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT 0x02

#define cpu_to_le16(v) ((u16)(v))
#define cpu_to_le32(v) ((u32)(v))

/* Kernel structures */
struct list_head {
    struct list_head *next, *prev;
};

struct spinlock {
    unsigned int slock;
};

struct mutex {
    signed int count;
    struct spinlock wait_lock;
    struct list_head wait_list;
    void *owner;
    void *spin_mlock;
};

struct wait_queue_head {
    struct spinlock lock;
    struct list_head task_list;
};

struct wait_queue_entry {
    unsigned int flags;
    void *private;
    int (*func)(void *wait, unsigned int mode, int flags, void *key);
    struct list_head entry;
};

struct device;
struct file;
struct poll_table_struct;

typedef void (*poll_queue_proc)(struct file *, struct wait_queue_head *, struct poll_table_struct *);

struct poll_table_struct {
    poll_queue_proc _qproc;
    unsigned long _key;
};

struct file_operations {
    void *owner;
    loff_t (*llseek)(struct file *, loff_t, int);
    ssize_t (*read)(struct file *, char *, size_t, loff_t *);
    ssize_t (*write)(struct file *, const char *, size_t, loff_t *);
    void *aio_read;
    void *aio_write;
    void *read_iter;
    void *write_iter;
    unsigned int (*poll)(struct file *, struct poll_table_struct *);
    long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
    long (*compat_ioctl)(struct file *, unsigned int, unsigned long);
    void *mmap;
    int (*open)(void *inode, struct file *);
    void *flush;
    int (*release)(void *inode, struct file *);
};

struct miscdevice {
    int minor;
    const char *name;
    const struct file_operations *fops;
    struct list_head list;
    struct device *parent;
    struct device *this_device;
    const char *nodename;
    umode_t mode;
};

struct usb_ctrlrequest {
    u8 bRequestType;
    u8 bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
} __attribute__((packed));

struct usb_descriptor_header {
    u8 bLength;
    u8 bDescriptorType;
} __attribute__((packed));

struct usb_endpoint_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bEndpointAddress;
    u8 bmAttributes;
    u16 wMaxPacketSize;
    u8 bInterval;
    u8 bRefresh;
    u8 bSynchAddress;
} __attribute__((packed));

struct usb_interface_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bInterfaceNumber;
    u8 bAlternateSetting;
    u8 bNumEndpoints;
    u8 bInterfaceClass;
    u8 bInterfaceSubClass;
    u8 bInterfaceProtocol;
    u8 iInterface;
} __attribute__((packed));

struct usb_interface_assoc_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bFirstInterface;
    u8 bInterfaceCount;
    u8 bFunctionClass;
    u8 bFunctionSubClass;
    u8 bFunctionProtocol;
    u8 iFunction;
} __attribute__((packed));

/* UAC1 Specific Descriptors */
struct uac1_ac_header_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bDescriptorSubtype;
    u16 bcdADC;
    u16 wTotalLength;
    u8 bInCollection;
    u8 baInterfaceNr[1];
} __attribute__((packed));

struct uac1_input_terminal_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bDescriptorSubtype;
    u8 bTerminalID;
    u16 wTerminalType;
    u8 bAssocTerminal;
    u8 bNrChannels;
    u16 wChannelConfig;
    u8 iChannelNames;
    u8 iTerminal;
} __attribute__((packed));

struct uac1_output_terminal_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bDescriptorSubtype;
    u8 bTerminalID;
    u16 wTerminalType;
    u8 bAssocTerminal;
    u8 bSourceID;
    u8 iTerminal;
} __attribute__((packed));

struct uac1_feature_unit_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bDescriptorSubtype;
    u8 bUnitID;
    u8 bSourceID;
    u8 bControlSize;
    u8 bmaControls[3];
    u8 iFeature;
} __attribute__((packed));

struct uac1_as_general_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bDescriptorSubtype;
    u8 bTerminalLink;
    u8 bDelay;
    u16 wFormatTag;
} __attribute__((packed));

struct uac1_format_type_i_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bDescriptorSubtype;
    u8 bFormatType;
    u8 bNrChannels;
    u8 bSubFrameSize;
    u8 bBitResolution;
    u8 bSamFreqType;
    u8 tSamFreq[3];
} __attribute__((packed));

struct uac1_iso_ep_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bDescriptorSubtype;
    u8 bmAttributes;
    u8 bLockDelayUnits;
    u16 wLockDelay;
} __attribute__((packed));

struct usb_string {
    u8 id;
    const char *s;
};

struct usb_gadget_strings {
    u16 language;
    struct usb_string *strings;
};

struct usb_request {
    void *buf;
    unsigned int length;
    u32 dma;

    void *sg;
    unsigned int num_sgs;
    unsigned int num_mapped_sgs;

    unsigned short stream_id;
    unsigned short flags;

    void (*complete)(void *ep, struct usb_request *req);
    void *context;
    struct list_head list;

    int status;
    unsigned int actual;
};

struct usb_ep_ops {
    int (*enable)(void *ep, const struct usb_endpoint_descriptor *desc);
    int (*disable)(void *ep);
    struct usb_request *(*alloc_request)(void *ep, gfp_t gfp_flags);
    void (*free_request)(void *ep, struct usb_request *req);
    int (*queue)(void *ep, struct usb_request *req, gfp_t gfp_flags);
    int (*dequeue)(void *ep, struct usb_request *req);
    // ...
};

struct usb_ep {
    void *driver_data;
    const char *name;
    const struct usb_ep_ops *ops;
    struct list_head ep_list;
    unsigned short maxpacket;
    unsigned short max_streams;
    u8 mult;
    u8 maxburst;
    u8 address;
    const struct usb_endpoint_descriptor *desc;
};

struct usb_gadget {
    void *ops;
    struct usb_ep *ep0;
    struct list_head ep_list;
    unsigned int speed;
    // ...
};

struct usb_configuration;
struct usb_function;

struct usb_composite_dev {
    struct usb_gadget *gadget;
    struct usb_request *req;
    // ...
};

struct usb_function {
    const char *name;                              // +0x00
    struct usb_gadget_strings **strings;          // +0x04
    struct usb_descriptor_header **fs_descriptors; // +0x08
    struct usb_descriptor_header **hs_descriptors; // +0x0c
    struct usb_descriptor_header **ss_descriptors; // +0x10
    struct usb_configuration *config;              // +0x14
    int (*bind)(struct usb_configuration *, struct usb_function *); // +0x18
    void (*unbind)(struct usb_configuration *, struct usb_function *); // +0x1c
    void (*free_func)(struct usb_function *f);     // +0x20
    void *mod;                                     // +0x24
    int (*set_alt)(struct usb_function *, unsigned interface, unsigned alt); // +0x28
    int (*get_alt)(struct usb_function *, unsigned interface); // +0x2c
    void (*disable)(struct usb_function *);        // +0x30
    int (*setup)(struct usb_function *, const struct usb_ctrlrequest *); // +0x34
    void (*suspend)(struct usb_function *);        // +0x38
    void (*resume)(struct usb_function *);         // +0x3c
    void *pad1;                                    // +0x40
    void *pad2;                                    // +0x44
    struct list_head list;                         // +0x48
    unsigned long endpoints;                       // +0x50
};

struct usb_configuration {
    const char *label;                             // +0x00
    struct usb_gadget_strings **strings;           // +0x04
    const struct usb_descriptor_header **descriptors; // +0x08
    void (*unbind)(struct usb_configuration *);    // +0x0c
    int (*setup)(struct usb_configuration *, const struct usb_ctrlrequest *); // +0x10
    u8 bConfigurationValue;                        // +0x14
    u8 iConfiguration;                             // +0x15
    u8 bmAttributes;                               // +0x16
    u16 MaxPower;                                  // +0x18
    struct usb_composite_dev *cdev;                // +0x1c
    // ...
    void *pad_c[2];                                // +0x20, +0x24
    struct list_head functions;                    // +0x28
    u8 next_interface_id;                          // +0x30
    // ...
    struct usb_function *interface[16];            // +0x34
};

struct android_usb_function {
    char *name;                                    // +0x00
    void *config;                                  // +0x04
    struct device *dev;                            // +0x08
    char *dev_name;                                // +0x0c
    void **attributes;                             // +0x10
    struct list_head enabled_list;                 // +0x14
    int (*init)(struct android_usb_function *, struct usb_composite_dev *); // +0x1c
    void (*cleanup)(struct android_usb_function *); // +0x20
    void (*enable)(struct android_usb_function *);  // +0x24
    void (*disable)(struct android_usb_function *); // +0x28
    int (*bind_config)(struct android_usb_function *, struct usb_configuration *); // +0x2c
    void (*unbind_config)(struct android_usb_function *, struct usb_configuration *); // +0x30
    int (*ctrlrequest)(struct android_usb_function *, struct usb_composite_dev *, const struct usb_ctrlrequest *); // +0x34
};

struct android_dev {
    struct android_usb_function **functions;       // +0x00
    struct list_head enabled_functions;            // +0x04
    struct usb_composite_dev *cdev;                // +0x0c
    struct device *dev;                            // +0x10
    bool enabled;                                  // +0x14 (or +0x18)
    int disable_depth;                             // +0x18 / +0x1c
    struct mutex mutex;                            // +0x20
};

/* Verification of struct layouts */
_Static_assert(sizeof(struct android_usb_function) == 56, "android_usb_function size mismatch");
_Static_assert(__builtin_offsetof(struct android_usb_function, bind_config) == 44, "bind_config offset mismatch");
_Static_assert(__builtin_offsetof(struct usb_function, set_alt) == 40, "set_alt offset mismatch");
_Static_assert(__builtin_offsetof(struct usb_function, disable) == 48, "disable offset mismatch");
_Static_assert(__builtin_offsetof(struct usb_function, setup) == 52, "setup offset mismatch");
_Static_assert(__builtin_offsetof(struct usb_function, list) == 72, "list offset mismatch");
_Static_assert(__builtin_offsetof(struct usb_function, endpoints) == 80, "endpoints offset mismatch");

/* External Kernel Symbols */
extern int printk(const char *fmt, ...);
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern unsigned long __copy_to_user(void *to, const void *from, unsigned long n);
extern void *__kmalloc(size_t size, gfp_t flags);
extern void kfree(const void *p);

extern int misc_register(struct miscdevice *misc);
extern int misc_deregister(struct miscdevice *misc);

extern void mutex_lock(struct mutex *lock);
extern void mutex_unlock(struct mutex *lock);
extern unsigned long _raw_spin_lock_irqsave(struct spinlock *lock);
extern void _raw_spin_unlock_irqrestore(struct spinlock *lock, unsigned long flags);

extern void __init_waitqueue_head(struct wait_queue_head *q, const char *name, void *key);
extern void __wake_up(struct wait_queue_head *q, unsigned int mode, int nr, void *key);
extern void prepare_to_wait(struct wait_queue_head *q, struct wait_queue_entry *wait, int state);
extern void finish_wait(struct wait_queue_head *q, struct wait_queue_entry *wait);
extern long schedule_timeout(long timeout);

extern int usb_interface_id(struct usb_configuration *config, struct usb_function *function);
extern int usb_string_id(struct usb_composite_dev *cdev);
extern struct usb_ep *usb_ep_autoconfig(struct usb_gadget *gadget, struct usb_endpoint_descriptor *desc);
extern int config_ep_by_speed(struct usb_gadget *g, struct usb_function *f, struct usb_ep *_ep);
extern int usb_add_function(struct usb_configuration *config, struct usb_function *function);
extern int usb_assign_descriptors(struct usb_function *f,
                                  struct usb_descriptor_header **fs,
                                  struct usb_descriptor_header **hs,
                                  struct usb_descriptor_header **ss);
extern void usb_free_all_descriptors(struct usb_function *f);
extern int usb_ep_queue(struct usb_ep *ep, struct usb_request *req, gfp_t gfp_flags);

/* Constants & Ring Buffer */
#define NUM_REQ 8
#define REQ_BUF_SIZE 256
#define RING_BUF_SIZE 65536
#define RING_BUF_MASK (RING_BUF_SIZE - 1)

static u8 ring_buffer[RING_BUF_SIZE];
static volatile u32 ring_head = 0;
static volatile u32 ring_tail = 0;
static struct wait_queue_head read_wait;
static volatile bool streaming_active = false;
static volatile u8 uac_mute = 0;
static volatile s16 uac_vol_db = -1669; // ~180/207 of [-12800, 0]
static volatile u8 uac_vol_0_207 = 180;
static volatile u16 uac_vol_seq = 0;
static u32 current_sample_rate = 48000;

/* UAC Context */
struct uac1_dev {
    struct usb_function func;
    struct usb_composite_dev *cdev;
    struct usb_ep *out_ep;
    struct usb_request *reqs[NUM_REQ];
    u8 ac_intf;
    u8 as_intf;
    u8 alt_setting;
};

static struct uac1_dev g_uac;

/* Descriptors Definition */
#define STR_AC_IF 0
#define STR_AS_IF 1

static struct usb_string uac_strings[] = {
    { STR_AC_IF, "Phicomm R1 USB Audio Control" },
    { STR_AS_IF, "Phicomm R1 USB Audio Stream" },
    { }
};

static struct usb_gadget_strings uac_string_tab = {
    .language = 0x0409, // en-us
    .strings = uac_strings,
};

static struct usb_gadget_strings *uac_strings_array[] = {
    &uac_string_tab,
    NULL,
};

/* Descriptors Template */
static struct usb_interface_assoc_descriptor iad_desc = {
    .bLength = sizeof(iad_desc),
    .bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
    .bFirstInterface = 0,
    .bInterfaceCount = 2,
    .bFunctionClass = USB_CLASS_AUDIO,
    .bFunctionSubClass = USB_SUBCLASS_AUDIOSTREAMING,
    .bFunctionProtocol = 0,
    .iFunction = 0,
};

static struct usb_interface_descriptor ac_intf_desc = {
    .bLength = sizeof(ac_intf_desc),
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 0,
    .bInterfaceClass = USB_CLASS_AUDIO,
    .bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
    .bInterfaceProtocol = 0,
    .iInterface = 0,
};

static struct uac1_ac_header_descriptor ac_header_desc = {
    .bLength = sizeof(ac_header_desc),
    .bDescriptorType = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC_HEADER,
    .bcdADC = cpu_to_le16(0x0100),
    .wTotalLength = cpu_to_le16(sizeof(ac_header_desc) + sizeof(struct uac1_input_terminal_descriptor) + sizeof(struct uac1_feature_unit_descriptor) + sizeof(struct uac1_output_terminal_descriptor)),
    .bInCollection = 1,
    .baInterfaceNr = { 0 },
};

static struct uac1_input_terminal_descriptor it_desc = {
    .bLength = sizeof(it_desc),
    .bDescriptorType = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC_INPUT_TERMINAL,
    .bTerminalID = 1,
    .wTerminalType = cpu_to_le16(UAC_TERMINAL_STREAMING),
    .bAssocTerminal = 0,
    .bNrChannels = 2,
    .wChannelConfig = cpu_to_le16(0x0003), // L + R
    .iChannelNames = 0,
    .iTerminal = 0,
};

static struct uac1_feature_unit_descriptor fu_desc = {
    .bLength = sizeof(fu_desc),
    .bDescriptorType = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC_FEATURE_UNIT,
    .bUnitID = 3,
    .bSourceID = 1,
    .bControlSize = 1,
    .bmaControls = { 0x03, 0x00, 0x00 }, // Master Mute + Volume
    .iFeature = 0,
};

static struct uac1_output_terminal_descriptor ot_desc = {
    .bLength = sizeof(ot_desc),
    .bDescriptorType = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC_OUTPUT_TERMINAL,
    .bTerminalID = 2,
    .wTerminalType = cpu_to_le16(UAC_OUTPUT_TERMINAL_SPEAKER),
    .bAssocTerminal = 0,
    .bSourceID = 3,
    .iTerminal = 0,
};

static struct usb_interface_descriptor as_intf_alt0_desc = {
    .bLength = sizeof(as_intf_alt0_desc),
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 0,
    .bInterfaceClass = USB_CLASS_AUDIO,
    .bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol = 0,
    .iInterface = 0,
};

static struct usb_interface_descriptor as_intf_alt1_desc = {
    .bLength = sizeof(as_intf_alt1_desc),
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 1,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_AUDIO,
    .bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol = 0,
    .iInterface = 0,
};

static struct uac1_as_general_descriptor as_gen_desc = {
    .bLength = sizeof(as_gen_desc),
    .bDescriptorType = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC_AS_GENERAL,
    .bTerminalLink = 1,
    .bDelay = 1,
    .wFormatTag = cpu_to_le16(0x0001), // PCM
};

static struct uac1_format_type_i_descriptor format_desc = {
    .bLength = sizeof(format_desc),
    .bDescriptorType = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC_FORMAT_TYPE,
    .bFormatType = UAC_FORMAT_TYPE_I,
    .bNrChannels = 2,
    .bSubFrameSize = 2,
    .bBitResolution = 16,
    .bSamFreqType = 1,
    .tSamFreq = { 0x80, 0xbb, 0x00 }, // 48000 Hz in LE
};

/* Full-Speed Isochronous Endpoint (bInterval = 1) */
static struct usb_endpoint_descriptor fs_ep_out_desc = {
    .bLength = sizeof(fs_ep_out_desc),
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_OUT,
    .bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ADAPTIVE,
    .wMaxPacketSize = cpu_to_le16(200),
    .bInterval = 1,
    .bRefresh = 0,
    .bSynchAddress = 0,
};

/* High-Speed Isochronous Endpoint (bInterval = 4 = 1ms in DWC_OTG microframes) */
static struct usb_endpoint_descriptor hs_ep_out_desc = {
    .bLength = sizeof(hs_ep_out_desc),
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_OUT,
    .bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ADAPTIVE,
    .wMaxPacketSize = cpu_to_le16(200),
    .bInterval = 4,
    .bRefresh = 0,
    .bSynchAddress = 0,
};

static struct uac1_iso_ep_descriptor as_ep_cs_desc = {
    .bLength = sizeof(as_ep_cs_desc),
    .bDescriptorType = USB_DT_CS_ENDPOINT,
    .bDescriptorSubtype = UAC_EP_GENERAL,
    .bmAttributes = UAC_EP_CS_ATTR_SAMPLE_RATE,
    .bLockDelayUnits = 0,
    .wLockDelay = 0,
};

/* Descriptor lists */
static struct usb_descriptor_header *fs_audio_desc[] = {
    (struct usb_descriptor_header *)&iad_desc,
    (struct usb_descriptor_header *)&ac_intf_desc,
    (struct usb_descriptor_header *)&ac_header_desc,
    (struct usb_descriptor_header *)&it_desc,
    (struct usb_descriptor_header *)&fu_desc,
    (struct usb_descriptor_header *)&ot_desc,
    (struct usb_descriptor_header *)&as_intf_alt0_desc,
    (struct usb_descriptor_header *)&as_intf_alt1_desc,
    (struct usb_descriptor_header *)&as_gen_desc,
    (struct usb_descriptor_header *)&format_desc,
    (struct usb_descriptor_header *)&fs_ep_out_desc,
    (struct usb_descriptor_header *)&as_ep_cs_desc,
    NULL,
};

static struct usb_descriptor_header *hs_audio_desc[] = {
    (struct usb_descriptor_header *)&iad_desc,
    (struct usb_descriptor_header *)&ac_intf_desc,
    (struct usb_descriptor_header *)&ac_header_desc,
    (struct usb_descriptor_header *)&it_desc,
    (struct usb_descriptor_header *)&fu_desc,
    (struct usb_descriptor_header *)&ot_desc,
    (struct usb_descriptor_header *)&as_intf_alt0_desc,
    (struct usb_descriptor_header *)&as_intf_alt1_desc,
    (struct usb_descriptor_header *)&as_gen_desc,
    (struct usb_descriptor_header *)&format_desc,
    (struct usb_descriptor_header *)&hs_ep_out_desc,
    (struct usb_descriptor_header *)&as_ep_cs_desc,
    NULL,
};

/* Isochronous Request Complete Callback */
static void uac1_out_complete(void *ep_ptr, struct usb_request *req)
{
    struct usb_ep *ep = (struct usb_ep *)ep_ptr;
    struct uac1_dev *uac = &g_uac;
    static unsigned int rx_pkt_cnt = 0;

    if (req->actual > 0 && rx_pkt_cnt < 5) {
        printk("uac1: packet received actual=%u status=%d\n", req->actual, req->status);
        rx_pkt_cnt++;
    }

    if (req->status == 0 && req->actual > 0) {
        u32 len = req->actual;
        u8 *src = (u8 *)req->buf;
        u32 head = ring_head;
        u32 tail = ring_tail;
        u32 space = (tail - head - 1) & RING_BUF_MASK;

        if (len > space)
            len = space; // drop overflow if ring buffer is full

        if (len > 0) {
            u32 chunk1 = RING_BUF_SIZE - (head & RING_BUF_MASK);
            if (chunk1 > len)
                chunk1 = len;
            memcpy(ring_buffer + (head & RING_BUF_MASK), src, chunk1);
            if (len > chunk1)
                memcpy(ring_buffer, src + chunk1, len - chunk1);

            ring_head = (head + len) & RING_BUF_MASK;
            __wake_up(&read_wait, 1, 1, NULL);
        }
    }

    if (streaming_active && uac->alt_setting == 1 && ep) {
        req->length = REQ_BUF_SIZE;
        ep->ops->queue(ep, req, GFP_ATOMIC);
    }
}

/* EP0 Feature Unit SET_CUR completion */
static void uac_fu_set_complete(void *ep_ptr, struct usb_request *req)
{
    unsigned long cs = (unsigned long)req->context;
    (void)ep_ptr;
    req->complete = NULL;
    if (req->status != 0)
        return;

    if (cs == UAC_FU_MUTE && req->actual >= 1) {
        u8 *buf = (u8 *)req->buf;
        uac_mute = (buf[0] != 0) ? 1 : 0;
        uac_vol_seq++;
    } else if (cs == UAC_FU_VOLUME && req->actual >= 2) {
        u8 *buf = (u8 *)req->buf;
        s16 db = (s16)((u16)buf[0] | ((u16)buf[1] << 8));
        if (db < -12800) db = -12800;
        if (db > 0) db = 0;
        uac_vol_db = db;
        int v = (db <= -12672) ? 0 : (207 + (db / 128));
        if (v < 0) v = 0;
        if (v > 207) v = 207;
        uac_vol_0_207 = (u8)v;
        uac_vol_seq++;
    }
}

/* Setup EP0 Class Requests Handler */
static int uac1_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
    struct usb_composite_dev *cdev = f->config->cdev;
    struct usb_request *req = cdev->req;
    int value = -EOPNOTSUPP;
    u16 w_index = ctrl->wIndex;
    u16 w_value = ctrl->wValue;
    u16 w_length = ctrl->wLength;

    if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
        if ((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE) {
            u8 unit_id = (u8)((w_index >> 8) & 0xff);
            u8 intf_id = (u8)(w_index & 0xff);
            if (unit_id == 3 && intf_id == ac_intf_desc.bInterfaceNumber) {
                u8 cs = (u8)((w_value >> 8) & 0xff);
                u8 *buf = (u8 *)req->buf;
                if (cs == UAC_FU_MUTE) {
                    switch (ctrl->bRequest) {
                    case UAC_SET_CUR:
                        if (w_length == 1) {
                            req->complete = uac_fu_set_complete;
                            req->context = (void *)(unsigned long)UAC_FU_MUTE;
                            value = 1;
                        }
                        break;
                    case UAC_GET_CUR:
                        if (w_length >= 1) {
                            buf[0] = uac_mute;
                            value = 1;
                        }
                        break;
                    }
                } else if (cs == UAC_FU_VOLUME) {
                    switch (ctrl->bRequest) {
                    case UAC_SET_CUR:
                        if (w_length == 2) {
                            req->complete = uac_fu_set_complete;
                            req->context = (void *)(unsigned long)UAC_FU_VOLUME;
                            value = 2;
                        }
                        break;
                    case UAC_GET_CUR:
                        if (w_length >= 2) {
                            s16 db = uac_vol_db;
                            buf[0] = (u8)(db & 0xff);
                            buf[1] = (u8)((db >> 8) & 0xff);
                            value = 2;
                        }
                        break;
                    case UAC_GET_MIN:
                        if (w_length >= 2) {
                            buf[0] = 0x00;
                            buf[1] = 0xce; // -12800 = -50.0 dB
                            value = 2;
                        }
                        break;
                    case UAC_GET_MAX:
                        if (w_length >= 2) {
                            buf[0] = 0x00;
                            buf[1] = 0x00; // 0.0 dB
                            value = 2;
                        }
                        break;
                    case UAC_GET_RES:
                        if (w_length >= 2) {
                            buf[0] = 0x80;
                            buf[1] = 0x00; // 128 = 0.5 dB
                            value = 2;
                        }
                        break;
                    }
                }
            }
        } else if ((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_ENDPOINT) {
            if ((w_index & 0xff) == fs_ep_out_desc.bEndpointAddress) {
                switch (ctrl->bRequest) {
                case UAC_SET_CUR:
                    if (w_length == 3) {
                        // Host will send 3 bytes in data stage
                        value = w_length;
                    }
                    break;
                case UAC_GET_CUR:
                case UAC_GET_MIN:
                case UAC_GET_MAX:
                    if (w_length >= 3) {
                        u8 *buf = (u8 *)req->buf;
                        buf[0] = (u8)(current_sample_rate & 0xff);
                        buf[1] = (u8)((current_sample_rate >> 8) & 0xff);
                        buf[2] = (u8)((current_sample_rate >> 16) & 0xff);
                        value = 3;
                    }
                    break;
                case UAC_GET_RES:
                    if (w_length >= 3) {
                        u8 *buf = (u8 *)req->buf;
                        buf[0] = 1;
                        buf[1] = 0;
                        buf[2] = 0;
                        value = 3;
                    }
                    break;
                }
            }
        }
    }

    if (value >= 0) {
        req->length = value;
        req->status = 0;
        return cdev->gadget->ep0->ops->queue(cdev->gadget->ep0, req, GFP_ATOMIC);
    }
    return value;
}

static int uac1_set_alt(struct usb_function *f, unsigned interface, unsigned alt)
{
    struct uac1_dev *uac = &g_uac;
    struct usb_composite_dev *cdev = f->config->cdev;

    if (interface == uac->as_intf) {
        if (alt == 0) {
            // Stop streaming
            streaming_active = false;
            uac->alt_setting = 0;
            if (uac->out_ep) {
                uac->out_ep->ops->disable(uac->out_ep);
            }
            __wake_up(&read_wait, 1, 1, NULL);
            printk("uac1_gadget: Audio Streaming Stopped (alt=0)\n");
            return 0;
        } else if (alt == 1) {
            // Start streaming
            int i;
            config_ep_by_speed(cdev->gadget, f, uac->out_ep);
            uac->out_ep->ops->enable(uac->out_ep, uac->out_ep->desc);
            uac->alt_setting = 1;
            streaming_active = true;
            ring_head = 0;
            ring_tail = 0;

            for (i = 0; i < NUM_REQ; i++) {
                if (uac->reqs[i]) {
                    uac->reqs[i]->length = REQ_BUF_SIZE;
                    uac->out_ep->ops->queue(uac->out_ep, uac->reqs[i], GFP_ATOMIC);
                }
            }
            __wake_up(&read_wait, 1, 1, NULL);
            printk("uac1_gadget: Audio Streaming Started (alt=1)\n");
            return 0;
        }
    }
    return 0;
}

static int uac1_get_alt(struct usb_function *f, unsigned interface)
{
    struct uac1_dev *uac = &g_uac;
    if (interface == uac->as_intf)
        return uac->alt_setting;
    return 0;
}

static void uac1_disable(struct usb_function *f)
{
    struct uac1_dev *uac = &g_uac;
    streaming_active = false;
    uac->alt_setting = 0;
    if (uac->out_ep)
        uac->out_ep->ops->disable(uac->out_ep);
    __wake_up(&read_wait, 1, 1, NULL);
    printk("uac1_gadget: Audio Disabled\n");
}

static int uac1_bind(struct usb_configuration *c, struct usb_function *f)
{
    struct uac1_dev *uac = &g_uac;
    struct usb_composite_dev *cdev = c->cdev;
    int status, i;

    // Allocate interface IDs
    status = usb_interface_id(c, f);
    if (status < 0) return status;
    uac->ac_intf = (u8)status;
    ac_intf_desc.bInterfaceNumber = uac->ac_intf;
    iad_desc.bFirstInterface = uac->ac_intf;

    status = usb_interface_id(c, f);
    if (status < 0) return status;
    uac->as_intf = (u8)status;
    as_intf_alt0_desc.bInterfaceNumber = uac->as_intf;
    as_intf_alt1_desc.bInterfaceNumber = uac->as_intf;
    ac_header_desc.baInterfaceNr[0] = uac->as_intf;

    // Assign strings
    status = usb_string_id(cdev);
    if (status > 0) {
        uac_strings[STR_AC_IF].id = (u8)status;
        ac_intf_desc.iInterface = (u8)status;
        iad_desc.iFunction = (u8)status;
    }

    status = usb_string_id(cdev);
    if (status > 0) {
        uac_strings[STR_AS_IF].id = (u8)status;
        as_intf_alt0_desc.iInterface = (u8)status;
        as_intf_alt1_desc.iInterface = (u8)status;
    }

    // Auto-configure Isochronous OUT endpoint
    uac->out_ep = usb_ep_autoconfig(cdev->gadget, &fs_ep_out_desc);
    if (!uac->out_ep) {
        printk("uac1_gadget: failed to autoconfig OUT endpoint\n");
        return -ENODEV;
    }
    uac->out_ep->driver_data = uac;
    hs_ep_out_desc.bEndpointAddress = fs_ep_out_desc.bEndpointAddress;

    // Claim endpoint in function endpoint bitmask (0x50)
    f->endpoints = (1UL << (fs_ep_out_desc.bEndpointAddress & 0x0f));

    // Allocate USB requests
    for (i = 0; i < NUM_REQ; i++) {
        struct usb_request *req = uac->out_ep->ops->alloc_request(uac->out_ep, GFP_KERNEL);
        if (!req) return -ENOMEM;
        req->buf = __kmalloc(REQ_BUF_SIZE, GFP_KERNEL);
        if (!req->buf) {
            uac->out_ep->ops->free_request(uac->out_ep, req);
            return -ENOMEM;
        }
        req->length = REQ_BUF_SIZE;
        req->complete = uac1_out_complete;
        req->context = uac;
        uac->reqs[i] = req;
    }

    // Assign descriptors
    status = usb_assign_descriptors(f, fs_audio_desc, hs_audio_desc, NULL);
    if (status) {
        printk("uac1_gadget: failed to assign descriptors\n");
        return status;
    }

    printk("uac1_gadget: bind success, ep=%s (addr 0x%02x)\n",
           uac->out_ep->name, fs_ep_out_desc.bEndpointAddress);
    return 0;
}

static void uac1_unbind(struct usb_configuration *c, struct usb_function *f)
{
    struct uac1_dev *uac = &g_uac;
    int i;

    streaming_active = false;
    for (i = 0; i < NUM_REQ; i++) {
        if (uac->reqs[i]) {
            if (uac->out_ep)
                uac->out_ep->ops->dequeue(uac->out_ep, uac->reqs[i]);
            kfree(uac->reqs[i]->buf);
            if (uac->out_ep)
                uac->out_ep->ops->free_request(uac->out_ep, uac->reqs[i]);
            uac->reqs[i] = NULL;
        }
    }
    usb_free_all_descriptors(f);
}

/* Character device file operations (/dev/uac1_audio) */
static int uac1_dev_open(void *inode, struct file *file)
{
    return 0;
}

static int uac1_dev_release(void *inode, struct file *file)
{
    return 0;
}

static ssize_t uac1_dev_read(struct file *file, char *user_buf, size_t count, loff_t *ppos)
{
    u32 head, tail, avail;
    u32 chunk1;

    head = ring_head;
    tail = ring_tail;
    avail = (head - tail) & RING_BUF_MASK;

    if (avail == 0)
        return 0;

    if (count > avail)
        count = avail;

    chunk1 = RING_BUF_SIZE - (tail & RING_BUF_MASK);
    if (chunk1 > count)
        chunk1 = count;

    if (__copy_to_user(user_buf, ring_buffer + (tail & RING_BUF_MASK), chunk1))
        return -14; // -EFAULT

    if (count > chunk1) {
        if (__copy_to_user(user_buf + chunk1, ring_buffer, count - chunk1))
            return -14; // -EFAULT
    }

    ring_tail = (tail + count) & RING_BUF_MASK;
    return count;
}

static unsigned int uac1_dev_poll(struct file *file, struct poll_table_struct *pt)
{
    u32 head = ring_head;
    u32 tail = ring_tail;
    u32 avail = (head - tail) & RING_BUF_MASK;
    unsigned int mask = 0;

    if (pt && pt->_qproc)
        pt->_qproc(file, &read_wait, pt);

    if (avail > 0)
        mask |= (POLLIN | POLLRDNORM);

    return mask;
}

static long uac1_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    (void)file;
    (void)arg;
    if (cmd == 0x5501) {
        return (long)(((u32)uac_vol_seq << 16) | ((u32)(uac_mute & 1) << 8) | (u32)(uac_vol_0_207 & 0xff));
    }
    return -EOPNOTSUPP;
}

static const struct file_operations uac1_fops = {
    .owner = NULL,
    .open = uac1_dev_open,
    .release = uac1_dev_release,
    .read = uac1_dev_read,
    .poll = uac1_dev_poll,
    .unlocked_ioctl = uac1_dev_ioctl,
};

static struct miscdevice uac1_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "uac1_audio",
    .fops = &uac1_fops,
    .mode = 0666,
};

/* Android USB function definition */
static int uac1_android_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
    struct uac1_dev *uac = &g_uac;
    uac->func.name = "uac1";
    uac->func.strings = uac_strings_array;
    uac->func.bind = uac1_bind;
    uac->func.unbind = uac1_unbind;
    uac->func.set_alt = uac1_set_alt;
    uac->func.get_alt = uac1_get_alt;
    uac->func.disable = uac1_disable;
    uac->func.setup = uac1_setup;

    return usb_add_function(c, &uac->func);
}

static struct android_usb_function uac1_android_func = {
    .name = "uac1",
    .bind_config = uac1_android_bind_config,
};

/* Pointers in kernel binary */
#define ANDROID_DEV_PTR 0xc0f95600
#define SUPPORTED_FUNCTIONS_PTR 0xc0e14364

static struct android_usb_function *orig_slot8 = NULL;

extern unsigned long kallsyms_lookup_name(const char *name);

/* Module Init & Exit */
int init_module(void)
{
    struct android_dev *dev = *(struct android_dev **)ANDROID_DEV_PTR;
    struct android_usb_function **funcs = (struct android_usb_function **)SUPPORTED_FUNCTIONS_PTR;
    int ret;

    printk("uac1_gadget: initializing module\n");

    /* 0. Patch SELinux to permissive mode */
    int *p_enforce = (int *)kallsyms_lookup_name("selinux_enforcing");
    if (p_enforce) {
        printk("uac1_gadget: selinux_enforcing @ 0x%08lx = %d -> setting to 0 (PERMISSIVE)\n",
               (unsigned long)p_enforce, *p_enforce);
        *p_enforce = 0;
    } else {
        printk("uac1_gadget: selinux_enforcing symbol not found via kallsyms\n");
    }

    __init_waitqueue_head(&read_wait, "uac1_read", NULL);

    ret = misc_register(&uac1_miscdev);
    if (ret) {
        printk("uac1_gadget: misc_register failed (%d)\n", ret);
        return ret;
    }

    if (funcs) {
        orig_slot8 = funcs[8];
        funcs[8] = &uac1_android_func;
        printk("uac1_gadget: hooked into supported_functions[8] (was %s)\n",
               orig_slot8 ? orig_slot8->name : "null");
    } else {
        printk("uac1_gadget: warning: funcs NULL\n");
    }

    printk("uac1_gadget: loaded successfully, device=/dev/uac1_audio\n");
    return 0;
}

void cleanup_module(void)
{
    struct android_dev *dev = *(struct android_dev **)ANDROID_DEV_PTR;
    struct android_usb_function **funcs = (struct android_usb_function **)SUPPORTED_FUNCTIONS_PTR;

    printk("uac1_gadget: unloading module\n");

    if (funcs && orig_slot8) {
        funcs[8] = orig_slot8;
        printk("uac1_gadget: restored supported_functions[8]\n");
    }

    misc_deregister(&uac1_miscdev);
    printk("uac1_gadget: unregistered misc device\n");
}

/* Modinfo section */
static const char __mod_vermagic[] __attribute__((section(".modinfo"), used)) =
    "vermagic=3.10.0 SMP preempt mod_unload ARMv7 p2v8 ";
static const char __mod_license[] __attribute__((section(".modinfo"), used)) =
    "license=GPL";
static const char __mod_depends[] __attribute__((section(".modinfo"), used)) =
    "depends=";
static const char __mod_intree[] __attribute__((section(".modinfo"), used)) =
    "intree=Y";

/* Module struct section (368 bytes) */
struct module_stub {
    unsigned int state;
    struct list_head list;
    char name[60];
    u8 pad[116];
    int (*init)(void);
    u8 pad2[168];
    void (*exit)(void);
    u8 pad3[4];
};

_Static_assert(sizeof(struct module_stub) == 368, "module_stub size mismatch");

struct module_stub __this_module __attribute__((section(".gnu.linkonce.this_module"), used)) = {
    .name = "uac1_gadget",
    .init = init_module,
    .exit = cleanup_module,
};
