// Automail
// =========
//
// C utility to read weight from a USB scale and upload the data to a server.
//
// Usage: Automail
//
//
/*
Automail is a modified version of usbscale.
*/
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <math.h>
#include <wiringPi.h>
#include </usr/include/mysql/mysql.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//
// This program uses libusb-1.0 (not the older libusb-0.1) for USB
// functionality.
//
#include <libusb-1.0/libusb.h>
//
// To enable a bunch of extra debugging data, simply define `#define DEBUG`
// here and recompile.
//
//     #define DEBUG
//
// The "scales.h" file contains a listing of all the Vendor and Product codes
// that this program will try to read. To try your scale, add your scale's
// vendor and product codes to scales.h and recompile.
//
#include "scales.h"

#define WEIGH_REPORT_SIZE 0x06

//
// **find_scale** takes a libusb device list and finds the first USB device
// that matches a device listed in scales.h.
//
static libusb_device* find_scale(libusb_device**);
//
// **print_scale_data** takes the 6-byte output from the scale and interprets
// it, printing out the result to the screen. It also returns a 1 if the
// program should read again (i.e. continue looping).
//
static int print_scale_data(unsigned char*);

//
// **get_scale_data** takes the 6-byte output from the scale and returns the value.
static double get_scale_data(unsigned char*);

//
// take device and fetch bEndpointAddress for the first endpoint
//
uint8_t get_first_endpoint_address(libusb_device* dev);

//
// **UNITS** is an array of all the unit abbreviations as set forth by *HID
// Point of Sale Usage Tables*, version 1.02, by the USB Implementers' Forum.
// The list is laid out so that the unit code returned by the scale is the
// index of its corresponding string.
// 
const char* UNITS[13] = {
    "units",        // unknown unit
    "mg",           // milligram
    "g",            // gram
    "kg",           // kilogram
    "cd",           // carat
    "taels",        // lian
    "gr",           // grain
    "dwt",          // pennyweight
    "tonnes",       // metric tons
    "tons",         // avoir ton
    "ozt",          // troy ounce
    "oz",           // ounce
    "lbs"           // pound
};

//
// main
// ----
//
int main(void)
{
   // Let the wifi connect
   sleep(30);

   MYSQL *conn;
   MYSQL_RES *res;
   MYSQL_ROW row;
   FILE *file;

   char *server = "sql3.freesqldatabase.com";
   char *user = "sql3110061";
   char *password = "ecsD2HSjvg"; /* set me first */
   char *database = "sql3110061";
   //int num_fields = mysql_num_fields(res);
   conn = mysql_init(NULL);
   /* Connect to database */
   if (!mysql_real_connect(conn, server,
         user, password, database, 0, NULL, 0)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   }

   char select[200] = "SELECT * FROM UIDList LIMIT 1";
   char delete[200] = "DELETE FROM UIDList WHERE uid = '"; 
   char reggy[200] = "INSERT INTO AutoMail(`uid`,`registered`,`email`,`password`) VALUES("; 
   char storage[37];  
   memset(storage, 0, sizeof(storage));   

   mysql_query(conn, select);
   res = mysql_store_result(conn);
   row = mysql_fetch_row(res);
   char buff[100];

   strcpy(buff, row[0]);
   strcat(buff, "\0");
   printf("%s \n", buff);
   //while((row = mysql_fetch_row(res)) != NULL)
   //printf("row%s \n", row[0]);

   file = fopen("/home/pi/pi/usbscale/uid.txt","r+");
   fscanf(file, "%s", storage);
   printf("%d \n", strlen(buff));
   printf("%d \n", strlen(storage));
   if (strlen(storage) < 1 ) {
     fputs(buff, file);
     //printf("%s \n", buff);
     fflush(file);
     strcat(delete, buff);
     strcat(delete, "'");
     //printf("%s", delete);
     mysql_query(conn, delete);
     strcat(reggy, "'");
     strcat(reggy, buff);
     strcat(reggy, "',0,'','')");
     //printf("%s", reggy);
     mysql_query(conn, reggy);
   } 
   
    libusb_device **devs;
    int r; // holds return codes
    ssize_t cnt;
    libusb_device* dev;
    libusb_device_handle* handle;

    int weigh_count = WEIGH_COUNT -1;
    
    if (wiringPiSetupGpio() == -1)
	exit (1);
    pinMode (15, INPUT);
    //
    // We first try to init libusb.
    //
    r = libusb_init(NULL);
    // 
    // If `libusb_init` errored, then we quit immediately.
    //
    if (r < 0)
        return r;

#ifdef DEBUG
        libusb_set_debug(NULL, 3);
#endif

    //
    // Next, we try to get a list of USB devices on this computer.
    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0)
        return (int) cnt;

    //
    // Once we have the list, we use **find_scale** to loop through and match
    // every device against the scales.h list. **find_scale** will return the
    // first device that matches, or 0 if none of them matched.
    //
    dev = find_scale(devs);
    if(dev == 0) {
        fprintf(stderr, "No USB scale found on this computer.\n");
        return -1;
    }
    
    //
    // Once we have a pointer to the USB scale in question, we open it.
    //
    r = libusb_open(dev, &handle);
    //
    // Note that this requires that we have permission to access this device.
    // If you get the "permission denied" error, check your udev rules.
    //
    if(r < 0) {
        if(r == LIBUSB_ERROR_ACCESS) {
            fprintf(stderr, "Permission denied to scale.\n");
        }
        else if(r == LIBUSB_ERROR_NO_DEVICE) {
            fprintf(stderr, "Scale has been disconnected.\n");
        }
        return -1;
    }
    //
    // On Linux, we typically need to detach the kernel driver so that we can
    // handle this USB device. We are a userspace tool, after all.
    //
#ifdef __linux__
    libusb_detach_kernel_driver(handle, 0);
#endif
    //
    // Finally, we can claim the interface to this device and begin I/O.
    //
    libusb_claim_interface(handle, 0);



    /*
     * Try to transfer data about status
     *
     * http://rowsandcolumns.blogspot.com/2011/02/read-from-magtek-card-swipe-reader-in.html
     */
    unsigned char data[WEIGH_REPORT_SIZE];
    int len;
    double scale_result = -1;
    
    //
    // For some reason, we get old data the first time, so let's just get that
    // out of the way now. It can't hurt to grab another packet from the scale.
    //
    r = libusb_interrupt_transfer(
        handle,
        //bmRequestType => direction: in, type: class,
                //    recipient: interface
        LIBUSB_ENDPOINT_IN | //LIBUSB_REQUEST_TYPE_CLASS |
            LIBUSB_RECIPIENT_INTERFACE,
        data,
        WEIGH_REPORT_SIZE, // length of data
        &len,
        10000 //timeout => 10 sec
        );
    // 
    // We read data from the scale in an infinite loop, stopping when
    // **print_scale_data** tells us that it's successfully gotten the weight
    // from the scale, or if the scale or transmissions indicates an error.
    //
	double weight = -1;
	int flag = 0;
    for(;;) {
        //
        // A `libusb_interrupt_transfer` of 6 bytes from the scale is the
        // typical scale data packet, and the usage is laid out in *HID Point
        // of Sale Usage Tables*, version 1.02.
        //
        r = libusb_interrupt_transfer(
            handle,
            //bmRequestType => direction: in, type: class,
                    //    recipient: interface
            get_first_endpoint_address(dev),
            data,
            WEIGH_REPORT_SIZE, // length of data
            &len,
            10000 //timeout => 10 sec
            );
        // 
        // If the data transfer succeeded, then we pass along the data we
        // received tot **print_scale_data**.
        //
        if(r == 0) {
#ifdef DEBUG
            int i;
            for(i = 0; i < WEIGH_REPORT_SIZE; i++) {
                printf("%x\n", data[i]);
            }
#endif
            if (weigh_count < 1) {
		if (digitalRead(15) == 0) {
                    scale_result = get_scale_data(data);
		    //printf("cr%.3f \n", scale_result);
		    if(scale_result != weight) {
   			char dataMail[200] = "INSERT INTO AutoMailData(`uid`,`weight`) VALUES(";
		        printf("weight = %g", weight);
			weight = scale_result;
                        char doubleToString[5] = "";
                        double value = weight;
                        sprintf(doubleToString, "%.1f", value);
			//printToString(value, doubleToString);
			strcat(dataMail, "'");
                        strcat(dataMail, buff); 
                        strcat(dataMail, "',");
                        strcat(dataMail, doubleToString);
                        strcat(dataMail, ")");
                        printf("%s \n", dataMail);
		        mysql_query(conn, dataMail);
		    }
		}
               // if(scale_result != 1)
                   //break;
            }
            weigh_count--;
        }
        else {
            fprintf(stderr, "Error in USB transfer\n");
            scale_result = -1;
            break;
        }
    }
    
    // 
    // At the end, we make sure that we reattach the kernel driver that we
    // detached earlier, close the handle to the device, free the device list
    // that we retrieved, and exit libusb.
    //
#ifdef __linux__
    libusb_attach_kernel_driver(handle, 0);
#endif
    libusb_close(handle);
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);

    // 
    // The return code will be 0 for success or -1 for errors (see
    // `libusb_init` above if it's neither 0 nor -1).
    // 
   fclose(file);
   /* close connection */
   mysql_free_result(res);
   mysql_close(conn);
    return scale_result;
}

//
// print_scale_data
// ----------------
//
// **print_scale_data** takes the 6 bytes of binary data sent by the scale and
// interprets and prints it out.
//
// **Returns:** `0` if weight data was successfully read, `1` if the data
// indicates that more data needs to be read (i.e. keep looping), and `-1` if
// the scale data indicates that some error occurred and that the program
// should terminate.
//
static double get_scale_data(unsigned char* dat) {

    // 
    // We keep around `lastStatus` so that we're not constantly printing the
    // same status message while waiting for a weighing. If the status hasn't
    // changed from last time, **print_scale_data** prints nothing.
    // 
    static uint8_t lastStatus = 0;

    //
    // Gently rip apart the scale's data packet according to *HID Point of Sale
    // Usage Tables*.
    //
    uint8_t report = dat[0];
    uint8_t status = dat[1];
    uint8_t unit   = dat[2];
    // Accoring to the docs, scaling applied to the data as a base ten exponent
    int8_t  expt   = dat[3];
    // convert to machine order at all times
    double weight = (double) le16toh(dat[5] << 8 | dat[4]);
    // since the expt is signed, we do not need no trickery
    weight = weight * pow(10, expt);

    //
    // The scale's first byte, its "report", is always 3.
    //
    if(report != 0x03 && report != 0x04) {
        fprintf(stderr, "Error reading scale data\n");
        return -1;
    }

    //
    // Switch on the status byte given by the scale. Note that we make a
    // distinction between statuses that we simply wait on, and statuses that
    // cause us to stop (`return -1`).
    //
    switch(status) {
        case 0x01:
            fprintf(stderr, "Scale reports Fault\n");
            return -1;
        case 0x02:
            if(status != lastStatus)
                fprintf(stderr, "Scale is zero'd...\n");
            break;
        case 0x03:
            if(status != lastStatus)
                fprintf(stderr, "Weighing...\n");
            break;
        //
        // 0x04 is the only final, successful status, and it indicates that we
        // have a finalized weight ready to print. Here is where we make use of
        // the `UNITS` lookup table for unit names.
        //
        case 0x04:
            // printf("%g %s\n", weight, UNITS[unit]);
            printf("%g \n", weight);
            return weight;
        case 0x05:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Under Zero\n");
            break;
        case 0x06:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Over Weight\n");
            break;
        case 0x07:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Calibration Needed\n");
            break;
        case 0x08:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Re-zeroing Needed!\n");
            break;
        default:
            if(status != lastStatus)
                fprintf(stderr, "Unknown status code: %d\n", status);
            return -1;
    }
    
    lastStatus = status;
    return -1;
}
static int print_scale_data(unsigned char* dat) {

    // 
    // We keep around `lastStatus` so that we're not constantly printing the
    // same status message while waiting for a weighing. If the status hasn't
    // changed from last time, **print_scale_data** prints nothing.
    // 
    static uint8_t lastStatus = 0;

    //
    // Gently rip apart the scale's data packet according to *HID Point of Sale
    // Usage Tables*.
    //
    uint8_t report = dat[0];
    uint8_t status = dat[1];
    uint8_t unit   = dat[2];
    // Accoring to the docs, scaling applied to the data as a base ten exponent
    int8_t  expt   = dat[3];
    // convert to machine order at all times
    double weight = (double) le16toh(dat[5] << 8 | dat[4]);
    // since the expt is signed, we do not need no trickery
    weight = weight * pow(10, expt);

    //
    // The scale's first byte, its "report", is always 3.
    //
    if(report != 0x03 && report != 0x04) {
        fprintf(stderr, "Error reading scale data\n");
        return -1;
    }

    //
    // Switch on the status byte given by the scale. Note that we make a
    // distinction between statuses that we simply wait on, and statuses that
    // cause us to stop (`return -1`).
    //
    switch(status) {
        case 0x01:
            fprintf(stderr, "Scale reports Fault\n");
            return -1;
        case 0x02:
            if(status != lastStatus)
                fprintf(stderr, "Scale is zero'd...\n");
            break;
        case 0x03:
            if(status != lastStatus)
                fprintf(stderr, "Weighing...\n");
            break;
        //
        // 0x04 is the only final, successful status, and it indicates that we
        // have a finalized weight ready to print. Here is where we make use of
        // the `UNITS` lookup table for unit names.
        //
        case 0x04:
            // printf("%g %s\n", weight, UNITS[unit]);
            printf("%g \n", weight);
            return 0;
        case 0x05:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Under Zero\n");
            break;
        case 0x06:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Over Weight\n");
            break;
        case 0x07:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Calibration Needed\n");
            break;
        case 0x08:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Re-zeroing Needed!\n");
            break;
        default:
            if(status != lastStatus)
                fprintf(stderr, "Unknown status code: %d\n", status);
            return -1;
    }
    
    lastStatus = status;
    return 1;
}

//
// find_scale
// ----------
// 
// **find_scale** takes a `libusb_device\*\*` list and loop through it,
// matching each device's vendor and product IDs to the scales.h list. It
// return the first matching `libusb_device\*` or 0 if no matching device is
// found.
//
static libusb_device* find_scale(libusb_device **devs)
{

    int i = 0;
    libusb_device* dev = 0;

    //
    // Loop through each USB device, and for each device, loop through the
    // scales list to see if it's one of our listed scales.
    //
    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor");
            return NULL;
        }
        int i;
        for (i = 0; i < NSCALES; i++) {
            if(desc.idVendor  == scales[i][0] && 
               desc.idProduct == scales[i][1]) {
                /*
                 * Debugging data about found scale
                 */
#ifdef DEBUG

                fprintf(stderr,
                        "Found scale %04x:%04x (bus %d, device %d)\n",
                        desc.idVendor,
                        desc.idProduct,
                        libusb_get_bus_number(dev),
                        libusb_get_device_address(dev));

                    fprintf(stderr,
                            "It has descriptors:\n\tmanufc: %d\n\tprodct: %d\n\tserial: %d\n\tclass: %d\n\tsubclass: %d\n",
                            desc.iManufacturer,
                            desc.iProduct,
                            desc.iSerialNumber,
                            desc.bDeviceClass,
                            desc.bDeviceSubClass);

                    /*
                     * A char buffer to pull string descriptors in from the device
                     */
                    unsigned char string[256];
                    libusb_device_handle* hand;
                    libusb_open(dev, &hand);

                    r = libusb_get_string_descriptor_ascii(hand, desc.iManufacturer,
                            string, 256);
                    fprintf(stderr,
                            "Manufacturer: %s\n", string);
                    libusb_close(hand);
#endif
                    return dev;
                    
                break;
            }
        }
    }
    return NULL;
}

uint8_t get_first_endpoint_address(libusb_device* dev)
{
    // default value
    uint8_t endpoint_address = LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE; //| LIBUSB_RECIPIENT_ENDPOINT;

    struct libusb_config_descriptor *config;
    int r = libusb_get_config_descriptor(dev, 0, &config);
    if (r == 0) {
        // assuming we have only one endpoint
        endpoint_address = config->interface[0].altsetting[0].endpoint[0].bEndpointAddress;
        libusb_free_config_descriptor(config);
    }

    #ifdef DEBUG
    printf("bEndpointAddress 0x%02x\n", endpoint_address);
    #endif

    return endpoint_address;
}
