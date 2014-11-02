/*******************************************************
 HIDAPI simulation interface for PCAP files

 Set PCAP-file to parse by setting environment variable
 HIDAPI_PCAPSIMULATE_FILENAME to the file path
********************************************************/

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <pcap/pcap.h>

#include "hidapi.h"

/* Local definitions */
struct hid_device_ {
    pcap_t *pcap;
    char errbuf[PCAP_ERRBUF_SIZE];
    uint16_t last_write_command;
    uint16_t last_sequence_number;
    uint8_t reading_parts;
};

typedef struct device_id_mappings_s {
    uint16_t vendor_id;
    uint16_t product_id;
    char *model;
    char *manufacturer;
    char *product;
} device_id_mappings_t;

// crc16.c
uint16_t crc16_ccitt_false(unsigned char *buf, size_t buflen);
uint16_t crc16_ccitt_false_init(unsigned char *buf, size_t buflen, uint16_t crc);

/* Static functions */
static pcap_t *pcap_file_reopen(pcap_t *old_pcap, char *errbuf);
static const u_char *pcap_file_get_next(pcap_t *pcap);
static const u_char *pcap_file_find_next_pkt(pcap_t *pcap, uint16_t command, uint8_t recv_not_send, const u_char *data);
static uint16_t pcap_file_find_next_pkt_reass(pcap_t *pcap, uint16_t command, uint8_t recv_not_send, const u_char *data, u_char **buf);

static hid_device *new_hid_device(void);
static wchar_t *utf8_to_wchar_t(const char *utf8);


/* Static data */
static device_id_mappings_t device_id_mapping[] = {
    { 0x1493, 0x001d, "Greentit", "Suunto", "Ambit" },
    { 0x1493, 0x001c, "Finch", "Suunto", "Ambit" },
    { 0x1493, 0x001b, "Emu", "Suunto", "Ambit" },
    { 0x1493, 0x001a, "Colibri", "Suunto", "Ambit" },
    { 0x1493, 0x0019, "Duck", "Suunto", "Ambit" },
    { 0x1493, 0x0010, "Bluebird", "Suunto", "Ambit" }
};
static const device_id_mappings_t *detected_device = NULL;
static char *detected_device_serial = NULL;

int HID_API_EXPORT hid_init(void)
{
    pcap_t *pcap;
    char errbuf[PCAP_ERRBUF_SIZE];
    u_char *pktbuf;
    uint16_t pktbuf_len;
    char *model_string;
    char *serial_string;
    int i;

    if (detected_device == NULL) {
        pcap = pcap_file_reopen(NULL, errbuf);
        if (pcap == NULL) {
            return -1;
        }

        // Try to find device info in PCAP file
        pktbuf_len = pcap_file_find_next_pkt_reass(pcap, 0x0002, 1, NULL, &pktbuf);

        if (pktbuf_len > 0) {
            model_string = (char*)pktbuf;
            serial_string = (char*)(pktbuf + 16);

            // Try to find matching model
            for (i=0; i<sizeof(device_id_mapping)/sizeof(device_id_mapping[0]); i++) {
                if (strncmp(device_id_mapping[i].model, model_string, 16) == 0) {
                    detected_device = &device_id_mapping[i];
                    detected_device_serial = malloc(17);
                    strncpy(detected_device_serial, serial_string, 16);
                }
            }
        }
    }

    return 0;
}

int HID_API_EXPORT hid_exit(void)
{
    return 0;
}


struct hid_device_info  HID_API_EXPORT *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
    struct hid_device_info *root = NULL; /* return object */

    hid_init();

    if (detected_device != NULL) {
        struct hid_device_info *tmp;

        tmp = malloc(sizeof(struct hid_device_info));
        tmp->next = NULL;

        tmp->path = calloc(1, 1);
        tmp->vendor_id = detected_device->vendor_id;
        tmp->product_id = detected_device->product_id;
        tmp->serial_number = utf8_to_wchar_t(detected_device_serial);
        tmp->release_number = 0x0;
        tmp->interface_number = -1;
        tmp->manufacturer_string = utf8_to_wchar_t(detected_device->manufacturer);
        tmp->product_string = utf8_to_wchar_t(detected_device->product);

        root = tmp;
    }

    return root;
}

void  HID_API_EXPORT hid_free_enumeration(struct hid_device_info *devs)
{
    struct hid_device_info *d = devs;
    while (d) {
        struct hid_device_info *next = d->next;
        free(d->path);
        free(d->serial_number);
        free(d->manufacturer_string);
        free(d->product_string);
        free(d);
        d = next;
    }
}

hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number)
{
    hid_device *handle = NULL;

    handle = hid_open_path(NULL);

    return handle;
}

hid_device * HID_API_EXPORT hid_open_path(const char *path)
{
    hid_device *dev = NULL;

    hid_init();

    if (detected_device != NULL) {
        dev = new_hid_device();
        dev->pcap = pcap_file_reopen(NULL, dev->errbuf);
        if (dev->pcap == NULL) {
            return NULL;
        }
    }

    return dev;
}


int HID_API_EXPORT hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
    const u_char *pkt;
    uint16_t command;
    uint8_t pkt_part;
    uint8_t len;

    // Parse a few parts of the data
    len = data[1];
    pkt_part = data[2];
    command = be16toh(*(uint16_t*)(data + 8));

    // If this is a contineous write, we have to assume this is just fine, do
    // nothing
    if (pkt_part == 0x5d) {
        // First try, search to end
        pkt = pcap_file_find_next_pkt(dev->pcap, command, 0, len != 20 ? data + 20 : NULL);
        if (pkt != NULL) {
        }
        // Second try, read from top
        if (pkt == NULL) {
            dev->pcap = pcap_file_reopen(dev->pcap, dev->errbuf);
            pkt = pcap_file_find_next_pkt(dev->pcap, command, 0, len != 20 ? data + 20 : NULL);
        }
        if (pkt != NULL) {
        }
        // Third try, do not care about data
        if (pkt == NULL) {
            dev->pcap = pcap_file_reopen(dev->pcap, dev->errbuf);
            pkt = pcap_file_find_next_pkt(dev->pcap, command, 0, NULL);
        }
        if (pkt != NULL) {
        }

        dev->last_write_command = command;
        dev->last_sequence_number = le16toh(*(uint16_t*)(data + 14));
        dev->reading_parts = 0;
    }
    
    return pkt != NULL ? 0 : -1;
}


int HID_API_EXPORT hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds)
{
    const u_char *pkt = NULL;
    u_char tmpbuf[64];
    uint16_t *payload_crc, tmpcrc;

    // If we have already started to read parts, we should just continue with
    // next valid packet, else we need to resolve first packet
    if (dev->reading_parts) {
        pkt = pcap_file_get_next(dev->pcap);
    }
    else {
        // Search for next receive packet
        pkt = pcap_file_find_next_pkt(dev->pcap, 0xffff, 1, NULL);
    }

    if (pkt != NULL) {
        if (dev->reading_parts == 0) {
            // If we found firstpacket, we need to rewrite sequence number and crc
            memcpy(tmpbuf, pkt, 64);
            *(uint16_t*)(tmpbuf + 14) = htole16(dev->last_sequence_number);
            tmpcrc = crc16_ccitt_false(&tmpbuf[2], 4);
            *(uint16_t*)(tmpbuf + 6) = htole16(tmpcrc);
            payload_crc = (uint16_t *)&tmpbuf[tmpbuf[1]];
            *payload_crc = htole16(crc16_ccitt_false_init(&tmpbuf[8], tmpbuf[3], tmpcrc));
            pkt = tmpbuf;
        }
        dev->reading_parts = 1;
        // Fix length
        if (length > 64)
            length = 64;
        if (data != NULL) {
            memcpy(data, pkt, length);
        }
        return length;
    }
    return -1;
}

int HID_API_EXPORT hid_read(hid_device *dev, unsigned char *data, size_t length)
{
    return hid_read_timeout(dev, data, length, 0);
}

int HID_API_EXPORT hid_set_nonblocking(hid_device *dev, int nonblock)
{
    return 0; /* Success */
}


int HID_API_EXPORT hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length)
{
    return 0;
}

int HID_API_EXPORT hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length)
{
    return 0;
}


void HID_API_EXPORT hid_close(hid_device *dev)
{
    if (!dev)
        return;

    pcap_close(dev->pcap);
}


int HID_API_EXPORT_CALL hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
    return -1;
}

int HID_API_EXPORT_CALL hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
    return -1;
}

int HID_API_EXPORT_CALL hid_get_serial_number_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
    return -1;
}

int HID_API_EXPORT_CALL hid_get_indexed_string(hid_device *dev, int string_index, wchar_t *string, size_t maxlen)
{
    return -1;
}


HID_API_EXPORT const wchar_t * HID_API_CALL  hid_error(hid_device *dev)
{
    return NULL;
}

static pcap_t *pcap_file_reopen(pcap_t *old_pcap, char *errbuf)
{
    char *pcap_file;
    pcap_t *pcap = NULL;

    if (old_pcap != NULL) {
        pcap_close(old_pcap);
    }

    pcap_file = getenv("HIDAPI_PCAPSIMULATE_FILENAME");
    if (pcap_file != NULL) {
        pcap = pcap_open_offline(pcap_file, errbuf);
        if (pcap == NULL) {
            printf("Error: Failed to open pcap file %s\n", pcap_file);
        }
    }
    else {
        printf("Error: No HIDAPI_PCAPSIMULATE_FILENAME variable defined\n");
    }

    return pcap;
}

static const u_char *pcap_file_get_next(pcap_t *pcap)
{
    struct pcap_pkthdr h;
    const u_char *pktdata;
    size_t data_offset;

    do {
        pktdata = pcap_next(pcap, &h);
        if (h.len == 128 || h.len == 91) { // Linux USB vs USBPcap
            data_offset = h.len - 64;
            if (pktdata[data_offset] == 0x3f) { // Sanity check, first byte
                return pktdata + data_offset;
            }
        }
    } while(pktdata != NULL);

    return NULL;
}

static const u_char *pcap_file_find_next_pkt(pcap_t *pcap, uint16_t command, uint8_t recv_not_send, const u_char *data)
{
    const u_char *pktdata;

    do {
        pktdata = pcap_file_get_next(pcap);
        if (pktdata != NULL) {
            uint8_t len = pktdata[1];
            uint8_t pkttype = pktdata[2];
            uint16_t pktcommand = be16toh(*(uint16_t*)(pktdata + 8));
            uint8_t send_recv = (pktdata[10] & 0x03) >> 1;
            if (pkttype == 0x5d &&
                (command == 0xffff || pktcommand == command) &&
                send_recv == recv_not_send) {
                if (data != NULL) {
                    // Compare data as well
                    if (memcmp(data, pktdata + 20, len - 20) == 0) {
                        break;
                    }
                }
                else {
                    break;
                }
            }
        }
    } while(pktdata != NULL);

    return pktdata;
}

static uint16_t pcap_file_find_next_pkt_reass(pcap_t *pcap, uint16_t command, uint8_t recv_not_send, const u_char *data, u_char **buf)
{
    const u_char *pkt;
    u_char *retbuf = NULL;
    uint16_t retlen = 0;
    uint16_t msg_parts, msg_part;
    uint16_t pkts_captured = 1;

    pkt = pcap_file_find_next_pkt(pcap, command, recv_not_send, data);

    if (pkt != NULL) {
        // Check how many packets this entry should consist of
        msg_parts = le16toh(*(uint16_t*)(pkt + 4));
        retlen = le32toh(*(uint32_t*)(pkt + 16));
        retbuf = malloc(44 + 56*(msg_parts-1));

        if (retbuf != NULL) {
            memcpy(retbuf, pkt + 20, 44);
            while (msg_parts > pkts_captured) {
                pkt = pcap_file_get_next(pcap);
                if (pkt == NULL) {
                    free(retbuf);
                    retbuf = NULL;
                    retlen = 0;
                    break;
                }
                msg_part = le16toh(*(uint16_t*)(pkt + 4));
                memcpy(retbuf + 44 + 56*(msg_part-1), pkt + 8, 56);
                pkts_captured++;
            }
        }
    }

    *buf = retbuf;
    return retlen;
}

static hid_device *new_hid_device(void)
{
    hid_device *dev = calloc(1, sizeof(hid_device));

    return dev;
}

/* The caller must free the returned string with free(). */
static wchar_t *utf8_to_wchar_t(const char *utf8)
{
    wchar_t *ret = NULL;

    if (utf8) {
        size_t wlen = mbstowcs(NULL, utf8, 0);
        if ((size_t) -1 == wlen) {
            return wcsdup(L"");
        }
        ret = calloc(wlen+1, sizeof(wchar_t));
        mbstowcs(ret, utf8, wlen+1);
        ret[wlen] = 0x0000;
    }

    return ret;
}
