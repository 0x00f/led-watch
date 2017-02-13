/**
 * STM32 USB Peripheral Driver
 *
 * Core USB functionality and abstraction layer
 *
 * Kevin Cuzner
 */
#ifndef _USB_H_
#define _USB_H_

#include <stdint.h>

#define USB_CONTROL_ENDPOINT_SIZE 64

/**
 * Endpoint types passed to the setup function
 */
typedef enum { USB_ENDPOINT_BULK, USB_ENDPOINT_CONTROL, USB_ENDPOINT_INTERRUPT } USBEndpointType;

/**
 * Direction of a USB transfer from the host perspective
 */
typedef enum { USB_HOST_IN = 1 << 0, USB_HOST_OUT = 1 << 1 } USBDirection;

/**
 * Setup packet type definition
 */
typedef struct {
    union {
        uint16_t wRequestAndType;
        struct {
            uint8_t bmRequestType;
            uint8_t bRequest;
        };
    };
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} USBSetupPacket;

/**
 * Basic data needed to initiate a transfer
 */
typedef struct {
    void *addr;
    uint16_t len;
} USBTransferData;

/**
 * Result of a control setup request handler
 */
typedef enum { USB_CTL_OK, USB_CTL_STALL } USBControlResult;

#define USB_REQ_DIR_IN   (1 << 7)
#define USB_REQ_DIR_OUT  (0 << 7)
#define USB_REQ_TYPE_STD (0 << 5)
#define USB_REQ_TYPE_CLS (1 << 5)
#define USB_REQ_TYPE_VND (2 << 5)
#define USB_REQ_RCP_DEV  (0)
#define USB_REQ_RCP_IFACE (1)
#define USB_REQ_RCP_ENDP  (2)
#define USB_REQ_RCP_OTHER (3)

#define USB_REQ(REQUEST, TYPE) (uint16_t)(((REQUEST) << 8) | ((TYPE) & 0xFF))

/**
 * Initializes the USB peripheral. Before calling this, the USB divider
 * must be set appropriately
 */
void usb_init(void);

/**
 * Enables the usb peripheral
 */
void usb_enable(void);

/**
 * Disables the USB peripheral
 */
void usb_disable(void);

/**
 * Enables an endpoint
 *
 * Notes about size: The size must conform the the following constraints to not
 * cause unexpected behavior interacting with the STM32 hardware (i.e. conflicting
 * unexpectedly with descriptor definitions of endpoints):
 * - It must be no greater than 512
 * - If greater than 62, it must be a multiple of 32
 * - If less than or equal to 62, it must be even
 * Size is merely the packet size. Data actually sent and received does not need
 * to conform to these parameters. If the endpoint is to be used only as a bulk
 * IN endpoint (i.e. transmitting only), these constraints do not apply so long
 * as the size conforms to the USB specification itself.
 *
 * endpoint: Endpoint to set up
 * address: Endpoint address
 * size: Endpoint maximum packet size
 * type: Endpoint type
 */
void usb_endpoint_setup(uint8_t endpoint, uint8_t address, uint16_t size, USBEndpointType type);

/**
 * Sets up or disables send operations from the passed buffer. A send operation
 * is started when the host sends an IN token. The host will continue sending
 * IN tokens until it receives all data (dentoed by sending either a packet
 * less than the endpoint size or a zero length packet, in the case where len
 * is an exact multiple of the endpoint size).
 *
 * endpoint: Endpoint to set up
 * buf: Buffer to send from or NULL if transmit operations are to be disabled
 * len: Length of the buffer
 */
void usb_endpoint_send(uint8_t endpoint, void *buf, uint16_t len);

/**
 * Sets up or disables receive operations into the passed buffer. A receive
 * operation is started when the host sends either an OUT or SETUP token and
 * is completed when the host sends a packet less than the endpoint size or
 * sends a zero length packet.
 *
 * endpoint: Endpoint to set up
 * buf: Buffer to receive into or NULL if receive operations are to be disabled
 * len: Length of the buffer
 */
void usb_endpoint_receive(uint8_t endpoint, void *buf, uint16_t len);

/**
 * Places an endpoint in a stalled state, which persists until usb_endpoint_send
 * or usb_endpoint_receive is called. Note that setup packets can still be
 * received.
 *
 * endpoint: Endpoint to stall
 * direction: Direction to stall
 */
void usb_endpoint_stall(uint8_t endpoint, USBDirection direction);


/**
 * Hook function implemented by the application which is called when a
 * non-standard setup request arrives on endpoint zero.
 *
 * setup: Setup packet received
 * nextTransfer: Filled during this function call with any data for the next state
 *
 * Returns whether to continue with the control pipeline or stall
 */
USBControlResult hook_usb_handle_setup_request(USBSetupPacket const *setup, USBTransferData *nextTransfer);

/**
 * Hook function implemented by the application which is called when the status
 * stage of a setup request is completed on endpoint zero.
 *
 * setup: Setup packet received
 */
void hook_usb_control_complete(USBSetupPacket const *setup);

/**
 * Hook function implemented by the application which is called when the
 * USB peripheral has been reset
 */
void hook_usb_reset(void);

/**
 * Hook function implemented by the application which is called when an SOF is
 * received (1ms intervals from host)
 */
void hook_usb_sof(void);

/**
 * Hook function implemented by the application which is called when the host
 * sets a configuration. The configuration index is passed.
 */
void hook_usb_set_configuration(uint16_t configuration);

/**
 * Hook function implemented by the application which is called when the host
 * sets an [alternate] interface for the current configuration.
 */
void hook_usb_set_interface(uint16_t interface);

/**
 * Hook function implemented by the application which is called when a setup
 * token has been received. Setup tokens will always be processed, regardless
 * of NAK or STALL status.
 */
void hook_usb_endpoint_setup(uint8_t endpoint, USBSetupPacket const *setup);

/**
 * Hook function implemented by the application which is called when data has
 * been received into the latest buffer set up by usb_endpoint_receive.
 */
void hook_usb_endpoint_received(uint8_t endpoint, void *buf, uint16_t len);

/**
 * Hook function implemented by the application which is called when data has
 * been sent from the latest buffer set up by usb_endpoint_send.
 */
void hook_usb_endpoint_sent(uint8_t endpoint, void *buf, uint16_t len);

#endif //_USB_H_