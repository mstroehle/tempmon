#include "devtypes.h"
#include "array.h"

#include <ftdi.h>
#include <libusb-1.0/libusb.h>
#include <stdio.h>

uint16_t make_crc(uint8_t *byte_array, int end_position)
{
  /*
    make a cyclic redundancy check for the given byte array; this is per the
    rules found in the SEM710 specifications provided by Status Instruments;
    This crc currently causes the device to respond to messages sent, however
    the crc the device returns is incorrect by comparison to the one determined
    here.
  */
  int i;
  uint16_t crc;
  char j;
  char char_in;
  int lsBit;

  /*
    The CRC is based on the following components of this message:

    Start
    Command
    Length
    Data
  */

  crc = 0xFFFF;
  for (i = 1; i <= end_position; i++) {
    char_in = byte_array[i];
    crc = crc ^ char_in;
    for (j = 1; j <= 8; j++) {
      lsBit = crc & 1;
      crc = crc >> 1;
      if (lsBit == 1) {
	crc = crc ^ 0xA001;
      }
    }
  }
  return crc;
}

int crc_pass(uint8_t *rx_data, int rx_pointer)
{
  /*
    returns whether the received crc is the same as the calculated crc from
    the byte array passed by the USB device; if it is not, then the usb
    transfer was "faulty"
  */
  uint16_t calculated_crc;
  uint16_t rx_crc;
  
  calculated_crc = make_crc(rx_data, rx_pointer - 2);
  rx_crc = (((uint16_t) rx_data[rx_pointer]) << 8) + 
    ((uint16_t) rx_data[rx_pointer - 1]);

  /* printf("Calculated crc: %d\n", calculated_crc); */
  /* printf("Received crc: %d\n", rx_crc); */

  return (calculated_crc == rx_crc);
}

int detach_device_kernel(int vendor_id, int product_id)
{
  /*
    if the device kernel with the given vendor id and product id is currently
    active then detach it and return whether or not this action failed. This
    must be done using libusb, since ftd2xx does not have this capability.
  */

  int detach_failed;
  libusb_context *context;
  libusb_device_handle *handle;

  context = NULL;
  handle = NULL;

  libusb_init(&context);
  libusb_set_debug(context, 3);

  /* finds the first, as mentioned */
  handle = libusb_open_device_with_vid_pid(context, vendor_id, product_id);
  if (handle == NULL) {
    return -1;
  }

  if (libusb_kernel_driver_active(handle, 0)) {
    detach_failed = libusb_detach_kernel_driver(handle, 0);
    if (detach_failed) {
      return -1;
    }
  }
  libusb_release_interface(handle, 0);
  libusb_close(handle);
  libusb_exit(context);

  return 0;
}

int open_device(struct ftdi_context *ctx, int vendor_id, int product_id)
{
  /*
    use libftdi to open the device; this open device function simply opens the
    first device it finds with the given vendor and product id, and cannot
    differentiate between multiple devices with the same vID and pID
  */

  int err = 0;

  err = ftdi_init(ctx);
  if (err) { return err; }

  err = ftdi_usb_open(ctx, vendor_id, product_id);
  if (err) { return err; }

  /* open successful */
  return 0;
}


int prepare_device(struct ftdi_context *ctx)
{
  /*
    Ready the given device for reading/writing.
  */
  int err = 0;

  err =  ftdi_usb_purge_rx_buffer(ctx);
  if (err) { return err; }

  err =  ftdi_usb_purge_tx_buffer(ctx);
  if (err) { return err; }

  err = ftdi_set_baudrate(ctx, 19200);
  if (err) { return err; }

  err = ftdi_set_line_property(ctx, BITS_8, STOP_BIT_1, NONE);
  if (err) { return err; }

  err = ftdi_setflowctrl(ctx, SIO_DISABLE_FLOW_CTRL);
  if (err) { return err; }

  err = ftdi_set_latency_timer(ctx, 3);
  if (err) { return err; }

  return 0;
}

int generate_message(uint8_t COMMAND, uint8_t* byte_array,
		     int byte_array_upper_index)
{
  /*
    Generate a message to send to the device, based on the given command and
    byte array; returns the size of the message
  */
  uint8_t output[259];
  int i;
  uint8_t x;
  uint16_t crc;
  uint8_t lbyte;
  uint8_t rbyte;

  /* start byte */
  i = 0;
  output[i] = 0x55;

  /* command */
  i++;
  output[i] = COMMAND;

  /* length */
  i++;
  output[i] = byte_array_upper_index;

  /* data */
  for (x = 0; x <= byte_array_upper_index; x++) {
    i++;
    output[i] = byte_array[x];
  }

  /* crc */
  crc = make_crc(output, i);
  lbyte = (crc >> 8) & 0xFF;
  rbyte = (crc) & 0xFF;

  /* put CRC on byte_array, little Endian */
  i++;
  output[i] = rbyte;
  i++;
  output[i] = lbyte;

  /* add end byte */
  i++;
  output[i] = 0xAA;

  for (x = 0; x <= i; x++) {
    byte_array[x] = output[x];
  }

  return (i + 1); /* returns the size, NOT the highest index */
}

int generate_block_message(uint8_t device_address, int command,
			   uint8_t byte_count, long block_address,
			   uint8_t *byte_array, uint8_t read)
{
  /* generate a block message and return the length of said block message */
  uint8_t output[280];

  int i = 0;
  uint8_t x = 0;

  uint16_t crc;
  uint8_t lbyte;
  uint8_t rbyte;

  long temp;

  /* data enters in the byte_array */
  output[0] = device_address;
  output[1] = command;

  output[2] = byte_count;
  /* start address Msb then lsb */
  temp = block_address & 0xFF00;
  output[4] = block_address - (temp);
  output[3] = temp / (2 << 8);

  i = 4;
  if (!read) {
    for (x = 0; x < byte_count; x++) {
      i++;
      output[i] = byte_array[x];
    }
  }

  crc = make_crc(output, i);
  lbyte = (crc >> 8);
  rbyte = (crc) & 0xFF;

  /* put CRC on byte_array, little Endian */
  output[++i] = rbyte;
  output[++i] = lbyte;

  for (x = 0; x <= i; x++) {
    byte_array[x] = output[x];
  }

  return i + 1;
}

int read_device(struct ftdi_context *ctx, int command, uint8_t *incoming_buff)
{
  /*
     sends a read command to the device, and returns the length of the received
     byte array
  */
  uint8_t outgoing_bytes[280];

  int len;
  int i;

  int written;
  int received;
  int confirmation_byte;

  for (i = 0; i < 280; i++) {
    incoming_buff[i] = 0;
  }
  confirmation_byte = get_confirmation_byte((SEM_COMMANDS)command);

  outgoing_bytes[0] = 0;
  len = generate_message(command, outgoing_bytes, 0);
  if (len <= 0) {
    return -1;
  }

  received = 0;
  for (i = 0; (i < 4 && (incoming_buff[1] != confirmation_byte)); i++) {
    /*  until positive response received, or 4 no-replies (2.8 seconds) */
    written = ftdi_write_data(ctx, outgoing_bytes, len);
    if (written < 0) {
      return written;
    }

    usleep(700000); /* give device some time to transmit process readings */

    received = ftdi_read_data(ctx, incoming_buff, 280);
    if (received < 0) {
      return received;
    }
  }

  if (incoming_buff[1] != confirmation_byte) {
    return -1;
  }

  if (!crc_pass(incoming_buff, received - 2)) {
    /* do nothing for now  */
  }
  return received;
}
