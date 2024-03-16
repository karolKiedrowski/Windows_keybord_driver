#ifndef _PUBLIC_H
#define _PUBLIC_H

#define IOCTL_INDEX             0x802

#define IOCTL_KBFILTR_GET_KEYBOARD_ATTRIBUTES CTL_CODE( FILE_DEVICE_KEYBOARD,   \
                                                        IOCTL_INDEX,    \
                                                        METHOD_BUFFERED,    \
                                                        FILE_READ_DATA)

#define IOCTL_INDEX_2           0x800

#define IOCTL_KBFILTR_GET_KEYBOARD_INPUT_DATA CTL_CODE( FILE_DEVICE_KEYBOARD,   \
                                                        IOCTL_INDEX_2,      \
                                                        METHOD_BUFFERED,    \
                                                        FILE_READ_DATA)

#endif
