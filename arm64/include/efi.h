#ifndef SNSX_ARM64_EFI_H
#define SNSX_ARM64_EFI_H

#include <stddef.h>
#include <stdint.h>

typedef uint64_t EFI_STATUS;
typedef void *EFI_HANDLE;
typedef void *EFI_EVENT;
typedef uint64_t UINTN;
typedef uint16_t CHAR16;
typedef uint8_t BOOLEAN;
typedef uint32_t EFI_TPL;
typedef void *EFI_LBA;
typedef int32_t INT32;

#define EFI_SUCCESS 0
#define EFI_LOAD_ERROR 0x8000000000000001ULL
#define EFI_INVALID_PARAMETER 0x8000000000000002ULL
#define EFI_UNSUPPORTED 0x8000000000000003ULL
#define EFI_BAD_BUFFER_SIZE 0x8000000000000004ULL
#define EFI_ABORTED 0x8000000000000005ULL
#define EFI_NOT_READY 0x8000000000000006ULL
#define EFI_DEVICE_ERROR 0x8000000000000007ULL
#define EFI_WRITE_PROTECTED 0x8000000000000008ULL
#define EFI_OUT_OF_RESOURCES 0x8000000000000009ULL
#define EFI_VOLUME_FULL 0x800000000000000BULL
#define EFI_ACCESS_DENIED 0x800000000000000FULL
#define EFI_NOT_FOUND 0x8000000000000014ULL

#define EFI_TEXT_ATTR(fg, bg) ((fg) | ((bg) << 4))

enum {
    EFI_BLACK = 0x00,
    EFI_BLUE = 0x01,
    EFI_GREEN = 0x02,
    EFI_CYAN = 0x03,
    EFI_RED = 0x04,
    EFI_MAGENTA = 0x05,
    EFI_BROWN = 0x06,
    EFI_LIGHTGRAY = 0x07,
    EFI_BRIGHT = 0x08,
    EFI_WHITE = EFI_LIGHTGRAY | EFI_BRIGHT,
    EFI_LIGHTCYAN = EFI_CYAN | EFI_BRIGHT,
    EFI_LIGHTGREEN = EFI_GREEN | EFI_BRIGHT
};

typedef struct efi_table_header {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
} EFI_TABLE_HEADER;

typedef struct efi_input_key {
    uint16_t scan_code;
    CHAR16 unicode_char;
} EFI_INPUT_KEY;

typedef struct efi_guid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} EFI_GUID;

typedef struct efi_simple_text_input_protocol EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
typedef struct efi_simple_text_output_protocol EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef struct efi_graphics_output_protocol EFI_GRAPHICS_OUTPUT_PROTOCOL;
typedef struct efi_graphics_output_protocol_mode EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;
typedef struct efi_graphics_output_mode_information EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;
typedef struct efi_simple_text_input_ex_protocol EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL;
typedef struct efi_key_state EFI_KEY_STATE;
typedef struct efi_key_data EFI_KEY_DATA;
typedef struct efi_simple_pointer_protocol EFI_SIMPLE_POINTER_PROTOCOL;
typedef struct efi_simple_pointer_mode EFI_SIMPLE_POINTER_MODE;
typedef struct efi_simple_pointer_state EFI_SIMPLE_POINTER_STATE;
typedef struct efi_absolute_pointer_protocol EFI_ABSOLUTE_POINTER_PROTOCOL;
typedef struct efi_absolute_pointer_mode EFI_ABSOLUTE_POINTER_MODE;
typedef struct efi_absolute_pointer_state EFI_ABSOLUTE_POINTER_STATE;
typedef struct efi_file_protocol EFI_FILE_PROTOCOL;
typedef struct efi_simple_file_system_protocol EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
typedef struct efi_simple_network_protocol EFI_SIMPLE_NETWORK_PROTOCOL;
typedef struct efi_simple_network_mode EFI_SIMPLE_NETWORK_MODE;
typedef struct efi_pci_io_protocol EFI_PCI_IO_PROTOCOL;
typedef struct efi_loaded_image_protocol EFI_LOADED_IMAGE_PROTOCOL;
typedef struct efi_system_table EFI_SYSTEM_TABLE;
typedef struct efi_boot_services EFI_BOOT_SERVICES;

typedef EFI_STATUS (*EFI_INPUT_RESET)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *self, BOOLEAN extended_verification);
typedef EFI_STATUS (*EFI_INPUT_READ_KEY)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *self, EFI_INPUT_KEY *key);
typedef EFI_STATUS (*EFI_INPUT_EX_RESET)(EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *self, BOOLEAN extended_verification);
typedef EFI_STATUS (*EFI_INPUT_READ_KEY_EX)(EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *self, EFI_KEY_DATA *key_data);
typedef EFI_STATUS (*EFI_LOCATE_PROTOCOL)(EFI_GUID *protocol, void *registration, void **interface);
typedef EFI_STATUS (*EFI_HANDLE_PROTOCOL)(EFI_HANDLE handle, EFI_GUID *protocol, void **interface);
typedef EFI_STATUS (*EFI_FREE_POOL)(void *buffer);
typedef EFI_STATUS (*EFI_STALL)(UINTN microseconds);
typedef EFI_STATUS (*EFI_WAIT_FOR_EVENT)(UINTN number_of_events, EFI_EVENT *events, UINTN *index);
typedef EFI_STATUS (*EFI_LOCATE_HANDLE_BUFFER)(uint32_t search_type, EFI_GUID *protocol, void *search_key,
                                               UINTN *no_handles, EFI_HANDLE **buffer);
typedef EFI_STATUS (*EFI_SET_WATCHDOG_TIMER)(UINTN timeout, uint64_t watchdog_code, UINTN data_size, CHAR16 *watchdog_data);
typedef EFI_STATUS (*EFI_CONNECT_CONTROLLER)(EFI_HANDLE controller_handle, EFI_HANDLE *driver_image_handle,
                                             void *remaining_device_path, BOOLEAN recursive);

enum {
    EFI_SCAN_UP = 0x01,
    EFI_SCAN_DOWN = 0x02,
    EFI_SCAN_RIGHT = 0x03,
    EFI_SCAN_LEFT = 0x04,
    EFI_SCAN_HOME = 0x05,
    EFI_SCAN_END = 0x06,
    EFI_SCAN_INSERT = 0x07,
    EFI_SCAN_DELETE = 0x08,
    EFI_SCAN_PAGE_UP = 0x09,
    EFI_SCAN_PAGE_DOWN = 0x0A,
    EFI_SCAN_F1 = 0x0B,
    EFI_SCAN_F2 = 0x0C,
    EFI_SCAN_F3 = 0x0D,
    EFI_SCAN_F4 = 0x0E,
    EFI_SCAN_F10 = 0x14,
    EFI_SCAN_ESC = 0x17
};

enum {
    PixelRedGreenBlueReserved8BitPerColor = 0,
    PixelBlueGreenRedReserved8BitPerColor = 1,
    PixelBitMask = 2,
    PixelBltOnly = 3
};

enum {
    EFI_ALL_HANDLES = 0,
    EFI_BY_PROTOCOL = 2
};

struct efi_simple_text_input_protocol {
    EFI_INPUT_RESET reset;
    EFI_INPUT_READ_KEY read_key_stroke;
    EFI_EVENT wait_for_key;
};

struct efi_key_state {
    uint32_t key_shift_state;
    uint8_t key_toggle_state;
};

struct efi_key_data {
    EFI_INPUT_KEY key;
    EFI_KEY_STATE key_state;
};

struct efi_simple_text_input_ex_protocol {
    EFI_INPUT_EX_RESET reset;
    EFI_INPUT_READ_KEY_EX read_key_stroke_ex;
    EFI_EVENT wait_for_key_ex;
    void *set_state;
    void *register_key_notify;
    void *unregister_key_notify;
};

typedef EFI_STATUS (*EFI_TEXT_RESET)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, BOOLEAN extended_verification);
typedef EFI_STATUS (*EFI_TEXT_STRING)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, const CHAR16 *string);
typedef EFI_STATUS (*EFI_TEXT_TEST_STRING)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, const CHAR16 *string);
typedef EFI_STATUS (*EFI_TEXT_QUERY_MODE)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, UINTN mode_number, UINTN *columns, UINTN *rows);
typedef EFI_STATUS (*EFI_TEXT_SET_MODE)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, UINTN mode_number);
typedef EFI_STATUS (*EFI_TEXT_SET_ATTRIBUTE)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, UINTN attribute);
typedef EFI_STATUS (*EFI_TEXT_CLEAR_SCREEN)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self);
typedef EFI_STATUS (*EFI_TEXT_SET_CURSOR_POSITION)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, UINTN column, UINTN row);
typedef EFI_STATUS (*EFI_TEXT_ENABLE_CURSOR)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, BOOLEAN enable);

struct efi_simple_text_output_protocol {
    EFI_TEXT_RESET reset;
    EFI_TEXT_STRING output_string;
    EFI_TEXT_TEST_STRING test_string;
    EFI_TEXT_QUERY_MODE query_mode;
    EFI_TEXT_SET_MODE set_mode;
    EFI_TEXT_SET_ATTRIBUTE set_attribute;
    EFI_TEXT_CLEAR_SCREEN clear_screen;
    EFI_TEXT_SET_CURSOR_POSITION set_cursor_position;
    EFI_TEXT_ENABLE_CURSOR enable_cursor;
    void *mode;
};

struct efi_simple_pointer_state {
    INT32 relative_movement_x;
    INT32 relative_movement_y;
    INT32 relative_movement_z;
    BOOLEAN left_button;
    BOOLEAN right_button;
};

struct efi_simple_pointer_mode {
    uint64_t resolution_x;
    uint64_t resolution_y;
    uint64_t resolution_z;
    BOOLEAN left_button;
    BOOLEAN right_button;
};

typedef EFI_STATUS (*EFI_SIMPLE_POINTER_RESET)(EFI_SIMPLE_POINTER_PROTOCOL *self, BOOLEAN extended_verification);
typedef EFI_STATUS (*EFI_SIMPLE_POINTER_GET_STATE)(EFI_SIMPLE_POINTER_PROTOCOL *self, EFI_SIMPLE_POINTER_STATE *state);

struct efi_simple_pointer_protocol {
    EFI_SIMPLE_POINTER_RESET reset;
    EFI_SIMPLE_POINTER_GET_STATE get_state;
    EFI_EVENT wait_for_input;
    EFI_SIMPLE_POINTER_MODE *mode;
};

struct efi_absolute_pointer_state {
    uint64_t current_x;
    uint64_t current_y;
    uint64_t current_z;
    uint32_t active_buttons;
};

struct efi_absolute_pointer_mode {
    uint64_t absolute_min_x;
    uint64_t absolute_min_y;
    uint64_t absolute_min_z;
    uint64_t absolute_max_x;
    uint64_t absolute_max_y;
    uint64_t absolute_max_z;
    uint32_t attributes;
};

typedef EFI_STATUS (*EFI_ABSOLUTE_POINTER_RESET)(EFI_ABSOLUTE_POINTER_PROTOCOL *self, BOOLEAN extended_verification);
typedef EFI_STATUS (*EFI_ABSOLUTE_POINTER_GET_STATE)(EFI_ABSOLUTE_POINTER_PROTOCOL *self, EFI_ABSOLUTE_POINTER_STATE *state);

struct efi_absolute_pointer_protocol {
    EFI_ABSOLUTE_POINTER_RESET reset;
    EFI_ABSOLUTE_POINTER_GET_STATE get_state;
    EFI_EVENT wait_for_input;
    EFI_ABSOLUTE_POINTER_MODE *mode;
};

typedef struct efi_pixel_bitmask {
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t reserved_mask;
} EFI_PIXEL_BITMASK;

struct efi_graphics_output_mode_information {
    uint32_t version;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixel_format;
    EFI_PIXEL_BITMASK pixel_information;
    uint32_t pixels_per_scan_line;
};

struct efi_graphics_output_protocol_mode {
    uint32_t max_mode;
    uint32_t mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    UINTN size_of_info;
    uint64_t frame_buffer_base;
    UINTN frame_buffer_size;
};

typedef EFI_STATUS (*EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *self,
    uint32_t mode_number,
    UINTN *size_of_info,
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **info
);

typedef EFI_STATUS (*EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *self,
    uint32_t mode_number
);

struct efi_graphics_output_protocol {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE query_mode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE set_mode;
    void *blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
};

typedef struct efi_mac_address {
    uint8_t addr[32];
} EFI_MAC_ADDRESS;

struct efi_simple_network_mode {
    uint32_t state;
    uint32_t hw_address_size;
    uint32_t media_header_size;
    uint32_t max_packet_size;
    uint32_t nv_ram_size;
    uint32_t nv_ram_access_size;
    uint32_t receive_filter_mask;
    uint32_t receive_filter_setting;
    uint32_t max_mcast_filter_count;
    uint32_t mcast_filter_count;
    EFI_MAC_ADDRESS mcast_filter[16];
    EFI_MAC_ADDRESS current_address;
    EFI_MAC_ADDRESS broadcast_address;
    EFI_MAC_ADDRESS permanent_address;
    uint8_t if_type;
    BOOLEAN mac_address_changeable;
    BOOLEAN multiple_tx_supported;
    BOOLEAN media_present_supported;
    BOOLEAN media_present;
};

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_START)(EFI_SIMPLE_NETWORK_PROTOCOL *self);
typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_STOP)(EFI_SIMPLE_NETWORK_PROTOCOL *self);
typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_INITIALIZE)(EFI_SIMPLE_NETWORK_PROTOCOL *self,
                                                    UINTN extra_rx_buffer_size,
                                                    UINTN extra_tx_buffer_size);
typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_RESET)(EFI_SIMPLE_NETWORK_PROTOCOL *self, BOOLEAN extended_verification);
typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_SHUTDOWN)(EFI_SIMPLE_NETWORK_PROTOCOL *self);
typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_RECEIVE_FILTERS)(EFI_SIMPLE_NETWORK_PROTOCOL *self,
                                                         uint32_t enable,
                                                         uint32_t disable,
                                                         BOOLEAN reset_mcast_filter,
                                                         UINTN mcast_filter_count,
                                                         EFI_MAC_ADDRESS *mcast_filter);
typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_GET_STATUS)(EFI_SIMPLE_NETWORK_PROTOCOL *self,
                                                    uint32_t *interrupt_status,
                                                    void **tx_buf);
typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_TRANSMIT)(EFI_SIMPLE_NETWORK_PROTOCOL *self,
                                                  UINTN header_size,
                                                  UINTN buffer_size,
                                                  void *buffer,
                                                  EFI_MAC_ADDRESS *src_addr,
                                                  EFI_MAC_ADDRESS *dest_addr,
                                                  uint16_t *protocol);
typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_RECEIVE)(EFI_SIMPLE_NETWORK_PROTOCOL *self,
                                                 UINTN *header_size,
                                                 UINTN *buffer_size,
                                                 void *buffer,
                                                 EFI_MAC_ADDRESS *src_addr,
                                                 EFI_MAC_ADDRESS *dest_addr,
                                                 uint16_t *protocol);
typedef EFI_STATUS (*EFI_PCI_IO_PROTOCOL_MEM_ACCESS)(EFI_PCI_IO_PROTOCOL *self,
                                                     uint32_t width,
                                                     uint8_t bar_index,
                                                     uint64_t offset,
                                                     UINTN count,
                                                     void *buffer);
typedef EFI_STATUS (*EFI_PCI_IO_PROTOCOL_IO_ACCESS)(EFI_PCI_IO_PROTOCOL *self,
                                                    uint32_t width,
                                                    uint8_t bar_index,
                                                    uint64_t offset,
                                                    UINTN count,
                                                    void *buffer);
typedef EFI_STATUS (*EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS)(EFI_PCI_IO_PROTOCOL *self,
                                                        uint32_t width,
                                                        uint32_t offset,
                                                        UINTN count,
                                                        void *buffer);
typedef EFI_STATUS (*EFI_PCI_IO_PROTOCOL_GET_LOCATION)(EFI_PCI_IO_PROTOCOL *self,
                                                       UINTN *segment,
                                                       UINTN *bus,
                                                       UINTN *device,
                                                       UINTN *function);

enum {
    EfiPciIoWidthUint8 = 0,
    EfiPciIoWidthUint16 = 1,
    EfiPciIoWidthUint32 = 2
};

typedef struct efi_pci_io_protocol_access {
    EFI_PCI_IO_PROTOCOL_MEM_ACCESS read;
    EFI_PCI_IO_PROTOCOL_MEM_ACCESS write;
} EFI_PCI_IO_PROTOCOL_ACCESS;

typedef struct efi_pci_io_protocol_config_access {
    EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS read;
    EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS write;
} EFI_PCI_IO_PROTOCOL_CONFIG_ACCESSORS;

typedef struct efi_pci_io_protocol_io_access {
    EFI_PCI_IO_PROTOCOL_IO_ACCESS read;
    EFI_PCI_IO_PROTOCOL_IO_ACCESS write;
} EFI_PCI_IO_PROTOCOL_IO_ACCESSORS;

struct efi_simple_network_protocol {
    uint64_t revision;
    EFI_SIMPLE_NETWORK_START start;
    EFI_SIMPLE_NETWORK_STOP stop;
    EFI_SIMPLE_NETWORK_INITIALIZE initialize;
    EFI_SIMPLE_NETWORK_RESET reset;
    EFI_SIMPLE_NETWORK_SHUTDOWN shutdown;
    EFI_SIMPLE_NETWORK_RECEIVE_FILTERS receive_filters;
    void *station_address;
    void *statistics;
    void *mcast_ip_to_mac;
    void *nv_data;
    EFI_SIMPLE_NETWORK_GET_STATUS get_status;
    EFI_SIMPLE_NETWORK_TRANSMIT transmit;
    EFI_SIMPLE_NETWORK_RECEIVE receive;
    void *wait_for_packet;
    EFI_SIMPLE_NETWORK_MODE *mode;
};

struct efi_pci_io_protocol {
    void *poll_mem;
    void *poll_io;
    EFI_PCI_IO_PROTOCOL_ACCESS mem;
    EFI_PCI_IO_PROTOCOL_IO_ACCESSORS io;
    EFI_PCI_IO_PROTOCOL_CONFIG_ACCESSORS pci;
    void *copy_mem;
    void *map;
    void *unmap;
    void *allocate_buffer;
    void *free_buffer;
    void *flush;
    EFI_PCI_IO_PROTOCOL_GET_LOCATION get_location;
    void *attributes;
    void *get_bar_attributes;
    void *set_bar_attributes;
    UINTN rom_size;
    void *rom_image;
};

typedef EFI_STATUS (*EFI_IMAGE_UNLOAD)(EFI_HANDLE image_handle);

struct efi_loaded_image_protocol {
    uint32_t revision;
    EFI_HANDLE parent_handle;
    EFI_SYSTEM_TABLE *system_table;
    EFI_HANDLE device_handle;
    void *file_path;
    void *reserved;
    uint32_t load_options_size;
    void *load_options;
    void *image_base;
    uint64_t image_size;
    uint32_t image_code_type;
    uint32_t image_data_type;
    EFI_IMAGE_UNLOAD unload;
};

typedef EFI_STATUS (*EFI_SIMPLE_FILE_SYSTEM_OPEN_VOLUME)(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *self, EFI_FILE_PROTOCOL **root);

struct efi_simple_file_system_protocol {
    uint64_t revision;
    EFI_SIMPLE_FILE_SYSTEM_OPEN_VOLUME open_volume;
};

typedef EFI_STATUS (*EFI_FILE_OPEN)(EFI_FILE_PROTOCOL *self, EFI_FILE_PROTOCOL **new_handle, CHAR16 *file_name,
                                    uint64_t open_mode, uint64_t attributes);
typedef EFI_STATUS (*EFI_FILE_CLOSE)(EFI_FILE_PROTOCOL *self);
typedef EFI_STATUS (*EFI_FILE_DELETE)(EFI_FILE_PROTOCOL *self);
typedef EFI_STATUS (*EFI_FILE_READ)(EFI_FILE_PROTOCOL *self, UINTN *buffer_size, void *buffer);
typedef EFI_STATUS (*EFI_FILE_WRITE)(EFI_FILE_PROTOCOL *self, UINTN *buffer_size, void *buffer);
typedef EFI_STATUS (*EFI_FILE_GET_POSITION)(EFI_FILE_PROTOCOL *self, uint64_t *position);
typedef EFI_STATUS (*EFI_FILE_SET_POSITION)(EFI_FILE_PROTOCOL *self, uint64_t position);
typedef EFI_STATUS (*EFI_FILE_GET_INFO)(EFI_FILE_PROTOCOL *self, EFI_GUID *info_type, UINTN *buffer_size, void *buffer);
typedef EFI_STATUS (*EFI_FILE_SET_INFO)(EFI_FILE_PROTOCOL *self, EFI_GUID *info_type, UINTN buffer_size, void *buffer);
typedef EFI_STATUS (*EFI_FILE_FLUSH)(EFI_FILE_PROTOCOL *self);

struct efi_file_protocol {
    uint64_t revision;
    EFI_FILE_OPEN open;
    EFI_FILE_CLOSE close;
    EFI_FILE_DELETE delete;
    EFI_FILE_READ read;
    EFI_FILE_WRITE write;
    EFI_FILE_GET_POSITION get_position;
    EFI_FILE_SET_POSITION set_position;
    EFI_FILE_GET_INFO get_info;
    EFI_FILE_SET_INFO set_info;
    EFI_FILE_FLUSH flush;
    void *open_ex;
    void *read_ex;
    void *write_ex;
    void *flush_ex;
};

typedef struct efi_file_info {
    uint64_t size;
    uint64_t file_size;
    uint64_t physical_size;
    uint64_t create_time[2];
    uint64_t last_access_time[2];
    uint64_t modification_time[2];
    uint64_t attribute;
    CHAR16 file_name[1];
} EFI_FILE_INFO;

typedef struct efi_file_system_info {
    uint64_t size;
    BOOLEAN read_only;
    uint8_t reserved[7];
    uint64_t volume_size;
    uint64_t free_space;
    uint32_t block_size;
    CHAR16 volume_label[1];
} EFI_FILE_SYSTEM_INFO;

#define EFI_FILE_MODE_READ   0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE  0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE 0x8000000000000000ULL

#define EFI_FILE_READ_ONLY   0x0000000000000001ULL
#define EFI_FILE_HIDDEN      0x0000000000000002ULL
#define EFI_FILE_SYSTEM      0x0000000000000004ULL
#define EFI_FILE_DIRECTORY   0x0000000000000010ULL
#define EFI_FILE_ARCHIVE     0x0000000000000020ULL

struct efi_boot_services {
    EFI_TABLE_HEADER header;
    void *raise_tpl;
    void *restore_tpl;
    void *allocate_pages;
    void *free_pages;
    void *get_memory_map;
    void *allocate_pool;
    EFI_FREE_POOL free_pool;
    void *create_event;
    void *set_timer;
    EFI_WAIT_FOR_EVENT wait_for_event;
    void *signal_event;
    void *close_event;
    void *check_event;
    void *install_protocol_interface;
    void *reinstall_protocol_interface;
    void *uninstall_protocol_interface;
    EFI_HANDLE_PROTOCOL handle_protocol;
    void *reserved;
    void *register_protocol_notify;
    void *locate_handle;
    void *locate_device_path;
    void *install_configuration_table;
    void *load_image;
    void *start_image;
    void *exit;
    void *unload_image;
    void *exit_boot_services;
    void *get_next_monotonic_count;
    EFI_STALL stall;
    EFI_SET_WATCHDOG_TIMER set_watchdog_timer;
    EFI_CONNECT_CONTROLLER connect_controller;
    void *disconnect_controller;
    void *open_protocol;
    void *close_protocol;
    void *open_protocol_information;
    void *protocols_per_handle;
    EFI_LOCATE_HANDLE_BUFFER locate_handle_buffer;
    EFI_LOCATE_PROTOCOL locate_protocol;
    void *install_multiple_protocol_interfaces;
    void *uninstall_multiple_protocol_interfaces;
    void *calculate_crc32;
    void *copy_mem;
    void *set_mem;
    void *create_event_ex;
};

typedef struct efi_system_table {
    EFI_TABLE_HEADER header;
    CHAR16 *firmware_vendor;
    uint32_t firmware_revision;
    EFI_HANDLE console_in_handle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *con_in;
    EFI_HANDLE console_out_handle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *con_out;
    EFI_HANDLE standard_error_handle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *std_err;
    void *runtime_services;
    EFI_BOOT_SERVICES *boot_services;
    UINTN number_of_table_entries;
    void *configuration_table;
} EFI_SYSTEM_TABLE;

#endif
