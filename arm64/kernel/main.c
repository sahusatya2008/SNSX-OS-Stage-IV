#include "common.h"
#include "efi.h"
#include "string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)
#define FS_PATH_MAX 96
#define FS_MAX_ENTRIES 160
#define FS_MAX_FILE_SIZE 16384
#define SHELL_INPUT_MAX 256
#define SHELL_ARG_MAX 16
#define FILE_MANAGER_VIEW_MAX 48
#define PERSIST_MAX_VOLUMES 8
#define PERSIST_MAX_STATE_SIZE 524288
#define PERSIST_BOOT_MAX_SIZE 2097152
#define GFX_BACKBUFFER_MAX_WIDTH 1920
#define GFX_BACKBUFFER_MAX_HEIGHT 1080
#define GFX_BACKBUFFER_PIXELS (GFX_BACKBUFFER_MAX_WIDTH * GFX_BACKBUFFER_MAX_HEIGHT)
#define WIFI_PROFILE_MAX 8
#define BLUETOOTH_DEVICE_MAX 8
#define BROWSER_ADDRESS_MAX 192
#define BROWSER_PAGE_MAX 4096
#define NET_FRAME_MAX 2048
#define NET_HTTP_BUFFER_MAX 8192
#define NET_URL_MAX 768
#define NET_HTTP_REQUEST_MAX 1024
#define NET_TCP_SEGMENT_MAX 1024
#define SNAKE_MAX_SEGMENTS 256
#define STORE_PACKAGE_MAX 8
#define INPUT_PROTOCOL_MAX 8
#define BROWSER_TAB_MAX 4
#define BROWSER_BOOKMARK_MAX 12
#define BROWSER_HISTORY_MAX 16
#define BROWSER_DOWNLOAD_MAX 12
#define BROWSER_LINK_MAX 16
#define STUDIO_OUTPUT_MAX 4096
#define SNSX_HTTPS_BRIDGE_HOST "10.0.2.2"
#define SNSX_HTTPS_BRIDGE_PORT 8787
#define SNSX_DEV_BRIDGE_HOST "10.0.2.2"
#define SNSX_DEV_BRIDGE_PORT 8790

enum {
    ARM_FS_TEXT = 1,
    ARM_FS_DIR = 2
};

typedef struct arm_fs_entry {
    char path[FS_PATH_MAX];
    uint8_t data[FS_MAX_FILE_SIZE];
    size_t size;
    uint32_t type;
} arm_fs_entry_t;

typedef struct calc_state {
    const char *cursor;
    bool ok;
} calc_state_t;

typedef struct wifi_profile {
    bool used;
    bool auto_connect;
    bool remembered;
    bool connected;
    char ssid[32];
    char password[64];
} wifi_profile_t;

typedef struct bluetooth_device {
    bool used;
    bool paired;
    bool connected;
    char name[32];
    char kind[24];
} bluetooth_device_t;

typedef struct store_package_manifest {
    const char *id;
    const char *name;
    const char *category;
    const char *summary;
    const char *asset_path;
    bool essential;
    bool installed_by_default;
} store_package_manifest_t;

typedef enum network_preference {
    NETWORK_PREF_AUTO = 0,
    NETWORK_PREF_ETHERNET = 1,
    NETWORK_PREF_WIFI = 2,
    NETWORK_PREF_HOTSPOT = 3
} network_preference_t;

typedef enum shell_result {
    SHELL_RESULT_DESKTOP = 0,
    SHELL_RESULT_FIRMWARE = 1
} shell_result_t;

typedef struct gfx_state {
    bool available;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t draw_stride;
    uint32_t pixel_format;
    uint32_t *framebuffer;
    uint32_t *drawbuffer;
} gfx_state_t;

typedef struct ui_file_view_state {
    char cwd[FS_PATH_MAX];
    size_t selected;
} ui_file_view_state_t;

typedef enum pointer_kind {
    POINTER_KIND_NONE = 0,
    POINTER_KIND_SIMPLE = 1,
    POINTER_KIND_ABSOLUTE = 2
} pointer_kind_t;

typedef struct pointer_state {
    bool available;
    pointer_kind_t kind;
    EFI_SIMPLE_POINTER_PROTOCOL *simple_protocol;
    EFI_ABSOLUTE_POINTER_PROTOCOL *absolute_protocol;
    int x;
    int y;
    bool left_down;
    bool engaged;
} pointer_state_t;

typedef struct input_state {
    bool standard_available;
    bool extended_available;
    size_t standard_count;
    size_t extended_count;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *standard_protocols[INPUT_PROTOCOL_MAX];
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *extended_protocols[INPUT_PROTOCOL_MAX];
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *text_ex;
} input_state_t;

typedef struct network_state {
    bool available;
    bool native_available;
    bool media_present;
    bool media_present_supported;
    bool started;
    bool initialized;
    bool dhcp_ready;
    bool internet_ready;
    bool internet_services_ready;
    bool wifi_profile_ready;
    bool hotspot_profile_ready;
    uint32_t mode_state;
    uint32_t max_packet_size;
    uint32_t if_type;
    uint32_t hw_address_size;
    uint32_t preference;
    EFI_SIMPLE_NETWORK_PROTOCOL *protocol;
    char driver_name[40];
    char mac[32];
    char permanent_mac[32];
    char transport[40];
    char preferred_transport[24];
    char hostname[32];
    char profile[96];
    char ipv4[24];
    char subnet_mask[24];
    char gateway[24];
    char dns_server[24];
    char internet[128];
    char capabilities[128];
    char status[128];
} network_state_t;

typedef struct netstack_state {
    bool configured;
    bool gateway_mac_valid;
    bool static_profile;
    uint32_t client_ip;
    uint32_t subnet_mask;
    uint32_t gateway_ip;
    uint32_t dns_ip;
    uint32_t lease_seconds;
    uint32_t xid;
    uint16_t next_port;
    uint16_t next_ip_id;
    uint8_t client_mac[6];
    uint8_t gateway_mac[6];
    char browser_status[128];
    char last_host[64];
    char last_ip[24];
} netstack_state_t;

typedef struct network_diagnostics_state {
    uint32_t snp_handles;
    uint32_t pxe_handles;
    uint32_t ip4_handles;
    uint32_t udp4_handles;
    uint32_t tcp4_handles;
    uint32_t dhcp4_handles;
    uint32_t pci_network_handles;
    char firmware_stack[160];
    char hardware[128];
    char wireless[160];
} network_diagnostics_state_t;

typedef struct e1000_rx_desc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} e1000_rx_desc_t;

typedef struct e1000_tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} e1000_tx_desc_t;

typedef struct e1000_driver {
    bool present;
    bool use_mmio;
    bool initialized;
    bool link_up;
    uint8_t bar_index;
    uint16_t vendor_id;
    uint16_t device_id;
    EFI_PCI_IO_PROTOCOL *pci;
    uint32_t tx_tail;
    uint32_t rx_head;
    uint8_t mac[6];
} e1000_driver_t;

typedef struct persist_volume {
    EFI_HANDLE handle;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    bool boot_device;
    bool writable;
} persist_volume_t;

typedef struct persist_state {
    bool available;
    bool dirty;
    bool installed;
    size_t volume_index;
    char status[96];
} persist_state_t;

typedef struct disk_volume_info {
    bool present;
    bool writable;
    bool has_bootloader;
    bool has_state;
    bool selected;
    size_t state_size;
    char name[24];
    char role[80];
} disk_volume_info_t;

typedef struct desktop_state {
    uint32_t theme;
    bool welcome_seen;
    bool autosave;
    bool pointer_trail;
    bool wifi_enabled;
    bool bluetooth_enabled;
    bool network_autoconnect;
    uint32_t launch_count;
    uint32_t pointer_speed;
    uint32_t network_preference;
    char last_app[32];
    char hostname[32];
    char wifi_ssid[32];
    char hotspot_name[32];
    char status_line[128];
} desktop_state_t;

typedef struct boot_diagnostics_state {
    bool ran;
    bool keyboard_protocol;
    bool keyboard_extended_protocol;
    bool key_seen;
    bool enter_seen;
    bool graphics_ready;
    bool pointer_protocol;
    bool pointer_moved;
    bool pointer_clicked;
    bool timed_out;
} boot_diagnostics_state_t;

typedef struct browser_state {
    size_t tab_count;
    size_t active_tab;
    size_t bookmark_count;
    size_t history_count;
    size_t download_count;
    char tabs[BROWSER_TAB_MAX][BROWSER_ADDRESS_MAX];
    char bookmarks[BROWSER_BOOKMARK_MAX][BROWSER_ADDRESS_MAX];
    char history[BROWSER_HISTORY_MAX][BROWSER_ADDRESS_MAX];
    char downloads[BROWSER_DOWNLOAD_MAX][FS_PATH_MAX];
} browser_state_t;

typedef struct code_studio_state {
    char active_path[FS_PATH_MAX];
    char language[16];
    char status[128];
    char last_action[24];
    char output[STUDIO_OUTPUT_MAX];
} code_studio_state_t;

typedef struct ui_event {
    bool key_ready;
    EFI_INPUT_KEY key;
    bool mouse_moved;
    bool mouse_clicked;
    bool mouse_released;
    bool mouse_down;
    int mouse_x;
    int mouse_y;
} ui_event_t;

static EFI_SYSTEM_TABLE *g_st;
static EFI_HANDLE g_image_handle;
static arm_fs_entry_t g_fs[FS_MAX_ENTRIES];
static size_t g_fs_count;
static char g_cwd[FS_PATH_MAX] = "/home";
static gfx_state_t g_gfx;
static ui_file_view_state_t g_file_view = { "/home", 0 };
static char g_last_opened_file[FS_PATH_MAX] = "/home/NOTES.TXT";
static input_state_t g_input;
static pointer_state_t g_pointer;
static network_state_t g_network;
static persist_state_t g_persist = { false, false, false, 0, "No persistence disk detected." };
static desktop_state_t g_desktop = {
    0,
    false,
    true,
    true,
    true,
    true,
    true,
    0,
    2,
    NETWORK_PREF_AUTO,
    "WELCOME",
    "SNSX",
    "SNSX-LAB",
    "SNSX-HOTSPOT",
    "SNSX desktop ready."
};
static boot_diagnostics_state_t g_boot_diag;
static bool g_desktop_request_firmware;
static bool g_force_text_ui;
static persist_volume_t g_persist_volumes[PERSIST_MAX_VOLUMES];
static size_t g_persist_volume_count;
static uint8_t g_persist_state_buffer[PERSIST_MAX_STATE_SIZE];
static uint8_t g_persist_boot_buffer[PERSIST_BOOT_MAX_SIZE];
static uint8_t g_text_editor_buffer[FS_MAX_FILE_SIZE];
static char g_console_editor_buffer[FS_MAX_FILE_SIZE];
static uint8_t g_append_buffer[FS_MAX_FILE_SIZE];
static uint32_t g_gfx_backbuffer[GFX_BACKBUFFER_PIXELS];
static uint8_t g_net_rx_frame[NET_FRAME_MAX];
static uint8_t g_net_tx_frame[NET_FRAME_MAX];
static char g_net_http_buffer[NET_HTTP_BUFFER_MAX];
static char g_net_http_request[NET_HTTP_REQUEST_MAX];
static wifi_profile_t g_wifi_profiles[WIFI_PROFILE_MAX];
static bluetooth_device_t g_bluetooth_devices[BLUETOOTH_DEVICE_MAX];
static bool g_store_installed[STORE_PACKAGE_MAX];
static char g_browser_address[BROWSER_ADDRESS_MAX] = "snsx://home";
static char g_browser_page[BROWSER_PAGE_MAX];
static char g_browser_links[BROWSER_LINK_MAX][BROWSER_ADDRESS_MAX];
static size_t g_browser_link_count;
static browser_state_t g_browser;
static code_studio_state_t g_studio = {
    "/projects/main.snsx",
    "snsx",
    "SNSX Code Studio is ready.",
    "idle",
    "Host-backed toolchains are available for Python, JavaScript, C, C++, and SNSX.\n"
};
static netstack_state_t g_netstack;
static network_diagnostics_state_t g_netdiag;
static e1000_driver_t g_e1000;
static e1000_rx_desc_t g_e1000_rx_ring[32] __attribute__((aligned(16)));
static e1000_tx_desc_t g_e1000_tx_ring[8] __attribute__((aligned(16)));
static uint8_t g_e1000_rx_buffers[32][2048] __attribute__((aligned(16)));
static uint8_t g_e1000_tx_buffers[8][NET_FRAME_MAX] __attribute__((aligned(16)));

enum {
    STORE_PKG_BROWSER_SUITE = 0,
    STORE_PKG_SNAKE_ARCADE,
    STORE_PKG_INTEL_82540EM,
    STORE_PKG_BAREMETAL_KIT,
    STORE_PKG_WIFI_DRIVER_LAB,
    STORE_PKG_BLUETOOTH_DRIVER_LAB,
    STORE_PKG_PRINTER_DRIVER_KIT,
    STORE_PKG_DEV_STARTER
};

static const store_package_manifest_t g_store_catalog[STORE_PACKAGE_MAX] = {
    {
        "browser-suite",
        "SNSX Browser Suite",
        "APP",
        "Core browser app, SNSX protocol pages, bookmarks, and plain HTTP browsing shell.",
        "/apps/BROWSER-SUITE.TXT",
        true,
        true
    },
    {
        "snake-arcade",
        "Snake Arcade",
        "GAME",
        "Retro Snake arcade package with keyboard controls and score tracking.",
        "/games/SNAKE-ARCADE.TXT",
        true,
        true
    },
    {
        "intel-82540em",
        "Intel 82540EM Driver",
        "DRIVER",
        "Native VirtualBox Ethernet path for NAT or bridged wired networking on supported ARM64 targets.",
        "/drivers/INTEL-82540EM.TXT",
        true,
        true
    },
    {
        "baremetal-kit",
        "Bare-Metal Install Kit",
        "KIT",
        "Guides and files for running SNSX from a bootable USB stick or an internal UEFI disk.",
        "/kits/PRIMARY-OS.TXT",
        false,
        false
    },
    {
        "wifi-driver-lab",
        "Wi-Fi Driver Lab",
        "DRIVER",
        "Driver-pack framework for future real-hardware USB and PCIe Wi-Fi adapters.",
        "/drivers/WIFI-LAB.TXT",
        false,
        false
    },
    {
        "bluetooth-driver-lab",
        "Bluetooth Driver Lab",
        "DRIVER",
        "Pairing and discovery framework for future guest-visible Bluetooth controllers and dongles.",
        "/drivers/BLUETOOTH-LAB.TXT",
        false,
        false
    },
    {
        "printer-driver-kit",
        "Printer Driver Kit",
        "DRIVER",
        "SNSX spooler notes, printer-driver layout, and future device package hooks.",
        "/drivers/PRINTER-KIT.TXT",
        false,
        false
    },
    {
        "dev-starter",
        "Developer Starter",
        "APP",
        "SNSX app, driver, and protocol development starter notes for extending the OS.",
        "/apps/DEV-STARTER.TXT",
        false,
        false
    }
};

enum {
    THEME_ROLE_BG_TOP = 0,
    THEME_ROLE_BG_BOTTOM,
    THEME_ROLE_TOPBAR,
    THEME_ROLE_PANEL,
    THEME_ROLE_PANEL_ALT,
    THEME_ROLE_BORDER,
    THEME_ROLE_TEXT,
    THEME_ROLE_TEXT_SOFT,
    THEME_ROLE_TEXT_LIGHT,
    THEME_ROLE_ACCENT,
    THEME_ROLE_ACCENT_STRONG,
    THEME_ROLE_DOCK,
    THEME_ROLE_WARM,
    THEME_ROLE_DANGER,
    THEME_ROLE_COUNT
};

static const char *g_theme_names[] = {
    "OCEAN",
    "SUNRISE",
    "SLATE",
    "EMERALD"
};

static const uint8_t g_theme_palette[][THEME_ROLE_COUNT][3] = {
    {
        {14, 24, 44}, {38, 64, 108}, {7, 16, 33}, {236, 242, 248}, {223, 235, 247},
        {36, 56, 84}, {27, 40, 61}, {79, 98, 124}, {214, 226, 241}, {36, 118, 224},
        {17, 76, 166}, {12, 20, 38}, {255, 214, 149}, {201, 79, 68}
    },
    {
        {63, 33, 28}, {182, 108, 74}, {43, 24, 26}, {250, 241, 233}, {255, 232, 212},
        {111, 63, 47}, {66, 36, 31}, {121, 78, 66}, {255, 238, 223}, {209, 106, 60},
        {142, 73, 40}, {51, 29, 29}, {255, 211, 155}, {180, 69, 49}
    },
    {
        {23, 28, 38}, {71, 83, 104}, {16, 20, 29}, {235, 239, 245}, {216, 223, 233},
        {55, 67, 87}, {31, 41, 55}, {87, 98, 116}, {225, 232, 242}, {92, 145, 210},
        {55, 95, 149}, {18, 24, 34}, {228, 209, 168}, {173, 82, 77}
    },
    {
        {13, 34, 28}, {39, 112, 94}, {8, 24, 22}, {232, 245, 241}, {214, 238, 231},
        {35, 82, 69}, {25, 47, 43}, {72, 101, 92}, {221, 243, 236}, {47, 152, 118},
        {25, 108, 83}, {10, 25, 23}, {235, 220, 171}, {170, 75, 68}
    }
};

static const EFI_GUID g_guid_gop = {
    0x9042a9de, 0x23dc, 0x4a38,
    {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}
};

static const EFI_GUID g_guid_simple_text_input = {
    0x387477c1, 0x69c7, 0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

static const EFI_GUID g_guid_simple_text_input_ex = {
    0xdd9e7534, 0x7762, 0x4698,
    {0x8c, 0x14, 0xf5, 0x85, 0x17, 0xa6, 0x25, 0xaa}
};

static const EFI_GUID g_guid_simple_pointer = {
    0x31878c87, 0x0b75, 0x11d5,
    {0x9a, 0x4f, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}
};

static const EFI_GUID g_guid_absolute_pointer = {
    0x8d59d32b, 0xc655, 0x4ae9,
    {0x9b, 0x15, 0xf2, 0x59, 0x04, 0x99, 0x2a, 0x43}
};

static const EFI_GUID g_guid_simple_fs = {
    0x964e5b22, 0x6459, 0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

static const EFI_GUID g_guid_simple_network = {
    0xa19832b9, 0xac25, 0x11d3,
    {0x9a, 0x2d, 0x00, 0x98, 0x03, 0xc9, 0x69, 0x3b}
};

static const EFI_GUID g_guid_loaded_image = {
    0x5b1b31a1, 0x9562, 0x11d2,
    {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

static const EFI_GUID g_guid_file_info = {
    0x09576e92, 0x6d3f, 0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

static const EFI_GUID g_guid_file_system_info = {
    0x09576e93, 0x6d3f, 0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

static const EFI_GUID g_guid_pxe_base_code = {
    0x03c4e603, 0xac28, 0x11d3,
    {0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}
};

static const EFI_GUID g_guid_ip4_service_binding = {
    0xc51711e7, 0xb4bf, 0x404a,
    {0xbf, 0xb8, 0x0a, 0x04, 0x8e, 0xf1, 0xff, 0xe4}
};

static const EFI_GUID g_guid_udp4_service_binding = {
    0x83f01464, 0x99bd, 0x45e5,
    {0xb3, 0x83, 0xaf, 0x63, 0x05, 0xd8, 0xe9, 0xe6}
};

static const EFI_GUID g_guid_tcp4_service_binding = {
    0x00720665, 0x67eb, 0x4a99,
    {0xba, 0xf7, 0xd3, 0xc3, 0x3a, 0x1c, 0x7c, 0xc9}
};

static const EFI_GUID g_guid_dhcp4_service_binding = {
    0x9d9a39d8, 0xbd42, 0x4a73,
    {0xa4, 0xd5, 0x8e, 0xe9, 0x4b, 0xe1, 0x13, 0x80}
};

static const EFI_GUID g_guid_pci_io = {
    0x4cf5b200, 0x68b8, 0x4ca5,
    {0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x02, 0x9a}
};

static size_t fs_list(const char *directory, arm_fs_entry_t **results, size_t max_results);
static void command_ls(const char *path);
static void command_help(void);
static void command_apps(void);
static void command_about(void);
static void command_devices(void);
static void command_theme(const char *value);
static shell_result_t shell_run(bool allow_desktop_return);
static void ui_edit_text_file(const char *path);
static void persist_mark_dirty(void);
static void persist_flush_state(void);
static void gfx_fill_rect(int x, int y, int width, int height, uint32_t color);
static void gfx_present(void);
static void desktop_load_config(void);
static void desktop_save_config(void);
static const char *desktop_theme_name(void);
static const char *desktop_pointer_speed_name(void);
static const char *desktop_network_mode_name(void);
static void desktop_cycle_network_mode(void);
static void desktop_prompt_network_profile(void);
static void network_compose_snsx_address(char *output, size_t output_size);
static void network_compose_session_address(char *output, size_t output_size);
static void network_compose_link_address(char *output, size_t output_size);
static size_t wifi_collect_slots(size_t *slots, size_t capacity);
static size_t bluetooth_collect_slots(size_t *slots, size_t capacity);
static void boot_diagnostics_reset(void);
static void boot_diagnostics_note_event(const ui_event_t *event);
static void boot_run_input_diagnostics(void);
static void ui_run_setup_app(void);
static void ui_run_network_app(void);
static void ui_run_wifi_app(void);
static void ui_run_bluetooth_app(void);
static void ui_run_disk_app(void);
static void ui_run_welcome_app(bool mark_seen_on_exit);
static void ui_run_settings_app(void);
static void ui_run_browser_app(void);
static void ui_run_store_app(void);
static void ui_run_snake_app(void);
static void ui_run_code_studio_app(void);
static void app_setup_console(void);
static void app_disk_console(void);
static void app_settings_console(void);
static void app_installer_console(void);
static void app_store_console(void);
static void app_system_console(void);
static void app_code_studio_console(void);
static bool ui_prompt_line(const char *title, const char *message, const char *seed,
                           char *output, size_t limit);
static void firmware_connect_all_controllers(void);
static void input_init(void);
static void pointer_init(void);
static bool input_try_read_key(EFI_INPUT_KEY *key);
static bool input_wait_for_key(EFI_INPUT_KEY *key);
static void input_pause(void);
static bool input_wait_for_events(EFI_EVENT *events, size_t count, size_t *index_out);
static bool pointer_poll_event(ui_event_t *event);
static bool boot_diagnostics_confirmed(void);
static void input_recover_live_devices(void);
static void wireless_profiles_load(void);
static void wireless_profiles_save(void);
static void store_load_state(void);
static void store_save_state(void);
static void browser_state_load(void);
static void browser_state_save(void);
static void browser_record_visit(const char *address);
static bool browser_toggle_bookmark(const char *address, bool *added);
static bool browser_save_page_snapshot(const char *address, const char *page, char *saved_path, size_t saved_path_size);
static void browser_navigate_to(const char *address);
static void browser_links_reset(void);
static void browser_extract_text_links(const char *text);
static void browser_extract_html_links(const char *html);
static void browser_append_links_summary(char *page, size_t page_size);
static void browser_finalize_page(char *page, size_t page_size);
static bool browser_open_link_index(size_t index);
static void browser_compose_link_label(size_t index, char *label, size_t label_size);
static void studio_state_load(void);
static void studio_state_save(void);
static void studio_seed_workspace(void);
static void command_store_list(void);
static bool store_install_package_by_id(const char *id, char *status, size_t status_size);
static bool store_remove_package_by_id(const char *id, char *status, size_t status_size);
static bool wifi_radio_available(void);
static bool bluetooth_controller_available(void);
static void show_network_diagnostics(void);
static void netstack_reset(void);
static bool netstack_renew_dhcp(void);
static bool netstack_apply_virtualbox_nat_profile(void);
static bool netstack_resolve_dns_name(const char *host, uint32_t *out_ip);
static const char *netstack_find_text(const char *text, const char *needle);
static bool netstack_build_https_bridge_url(const char *url, char *bridge_url, size_t bridge_url_size);
static bool netstack_build_dev_bridge_url(const char *path, char *bridge_url, size_t bridge_url_size);
static bool netstack_http_request_text(const char *method, const char *url, const char *content_type,
                                       const uint8_t *body, size_t body_len,
                                       char *output, size_t output_size);
static bool netstack_http_fetch_text(const char *url, char *output, size_t output_size);
static bool e1000_probe(void);
static bool e1000_initialize(void);
static bool e1000_send_frame(const uint8_t *frame, size_t len);
static bool e1000_receive_frame(uint8_t *buffer, size_t *len, uint32_t timeout_ms);
static void e1000_drain(void);
static void network_refresh(bool try_connect);
static bool network_connect_wired(void);
static void network_apply_profile_status(bool connect_attempted);
static void network_collect_diagnostics(void);
static size_t disk_collect_info(disk_volume_info_t *infos, size_t max_infos);

void __chkstk(void) {
}

static void console_write_utf16(const CHAR16 *text) {
    g_st->con_out->output_string(g_st->con_out, text);
}

static void console_write(const char *text) {
    CHAR16 buffer[512];
    size_t out = 0;

    while (*text != '\0') {
        char ch = *text++;
        if (ch == '\n') {
            if (out + 2 >= ARRAY_SIZE(buffer)) {
                buffer[out] = 0;
                console_write_utf16(buffer);
                out = 0;
            }
            buffer[out++] = '\r';
            buffer[out++] = '\n';
        } else {
            if (out + 1 >= ARRAY_SIZE(buffer)) {
                buffer[out] = 0;
                console_write_utf16(buffer);
                out = 0;
            }
            buffer[out++] = (CHAR16)(unsigned char)ch;
        }
    }

    buffer[out] = 0;
    console_write_utf16(buffer);
}

static void console_writeline(const char *text) {
    console_write(text);
    console_write("\n");
}

static void console_putchar(char ch) {
    CHAR16 buffer[3];
    if (ch == '\n') {
        buffer[0] = '\r';
        buffer[1] = '\n';
        buffer[2] = 0;
    } else {
        buffer[0] = (CHAR16)(unsigned char)ch;
        buffer[1] = 0;
    }
    console_write_utf16(buffer);
}

static void console_set_color(uint8_t fg, uint8_t bg) {
    g_st->con_out->set_attribute(g_st->con_out, EFI_TEXT_ATTR(fg, bg));
}

static void console_clear(void) {
    g_st->con_out->clear_screen(g_st->con_out);
}

static void console_backspace(void) {
    console_putchar('\b');
    console_putchar(' ');
    console_putchar('\b');
}

static void console_write_dec(uint32_t value) {
    char buffer[16];
    u32_to_dec(value, buffer);
    console_write(buffer);
}

static void console_write_signed(int32_t value) {
    if (value < 0) {
        uint32_t magnitude = (uint32_t)(-(value + 1)) + 1u;
        console_putchar('-');
        console_write_dec(magnitude);
        return;
    }
    console_write_dec((uint32_t)value);
}

static void firmware_connect_all_controllers(void) {
    EFI_HANDLE *handles = NULL;
    UINTN count = 0;

    if (g_st == NULL || g_st->boot_services == NULL ||
        g_st->boot_services->locate_handle_buffer == NULL ||
        g_st->boot_services->connect_controller == NULL) {
        return;
    }
    if (g_st->boot_services->locate_handle_buffer(EFI_ALL_HANDLES, NULL, NULL, &count, &handles) != EFI_SUCCESS) {
        return;
    }
    for (UINTN i = 0; i < count; ++i) {
        g_st->boot_services->connect_controller(handles[i], NULL, NULL, 1);
    }
    if (g_st->boot_services->free_pool != NULL && handles != NULL) {
        g_st->boot_services->free_pool(handles);
    }
}

static bool input_has_standard_protocol(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *protocol) {
    for (size_t i = 0; i < g_input.standard_count; ++i) {
        if (g_input.standard_protocols[i] == protocol) {
            return true;
        }
    }
    return false;
}

static void input_add_standard_protocol(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *protocol) {
    if (protocol == NULL || protocol->read_key_stroke == NULL ||
        g_input.standard_count >= INPUT_PROTOCOL_MAX ||
        input_has_standard_protocol(protocol)) {
        return;
    }
    g_input.standard_protocols[g_input.standard_count++] = protocol;
    g_input.standard_available = true;
}

static bool input_has_extended_protocol(EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *protocol) {
    for (size_t i = 0; i < g_input.extended_count; ++i) {
        if (g_input.extended_protocols[i] == protocol) {
            return true;
        }
    }
    return false;
}

static void input_add_extended_protocol(EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *protocol) {
    if (protocol == NULL || protocol->read_key_stroke_ex == NULL ||
        g_input.extended_count >= INPUT_PROTOCOL_MAX ||
        input_has_extended_protocol(protocol)) {
        return;
    }
    g_input.extended_protocols[g_input.extended_count++] = protocol;
    g_input.extended_available = true;
    if (g_input.text_ex == NULL) {
        g_input.text_ex = protocol;
    }
}

static void input_init(void) {
    EFI_HANDLE *handles = NULL;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *standard_in = NULL;
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *text_ex = NULL;
    UINTN handle_count = 0;

    memset(&g_input, 0, sizeof(g_input));
    if (g_st == NULL) {
        return;
    }
    if (g_st->con_in != NULL) {
        input_add_standard_protocol(g_st->con_in);
    }
    if (g_st->boot_services == NULL) {
        return;
    }
    if (g_st->console_in_handle != NULL &&
        g_st->boot_services->handle_protocol != NULL &&
        g_st->boot_services->handle_protocol(g_st->console_in_handle,
                                             (EFI_GUID *)&g_guid_simple_text_input_ex,
                                             (void **)&text_ex) == EFI_SUCCESS &&
        text_ex != NULL) {
        input_add_extended_protocol(text_ex);
    }
    if (g_st->console_in_handle != NULL &&
        g_st->boot_services->handle_protocol != NULL &&
        g_st->boot_services->handle_protocol(g_st->console_in_handle,
                                             (EFI_GUID *)&g_guid_simple_text_input,
                                             (void **)&standard_in) == EFI_SUCCESS &&
        standard_in != NULL) {
        input_add_standard_protocol(standard_in);
    }
    if (g_st->boot_services->locate_handle_buffer != NULL &&
        g_st->boot_services->handle_protocol != NULL &&
        g_st->boot_services->locate_handle_buffer(EFI_BY_PROTOCOL, (EFI_GUID *)&g_guid_simple_text_input, NULL,
                                                  &handle_count, &handles) == EFI_SUCCESS &&
        handles != NULL) {
        for (UINTN i = 0; i < handle_count; ++i) {
            EFI_SIMPLE_TEXT_INPUT_PROTOCOL *protocol = NULL;
            if (g_st->boot_services->handle_protocol(handles[i], (EFI_GUID *)&g_guid_simple_text_input,
                                                     (void **)&protocol) == EFI_SUCCESS) {
                input_add_standard_protocol(protocol);
            }
        }
        if (g_st->boot_services->free_pool != NULL) {
            g_st->boot_services->free_pool(handles);
        }
    }
    handles = NULL;
    handle_count = 0;
    if (g_st->boot_services->locate_handle_buffer != NULL &&
        g_st->boot_services->handle_protocol != NULL &&
        g_st->boot_services->locate_handle_buffer(EFI_BY_PROTOCOL, (EFI_GUID *)&g_guid_simple_text_input_ex, NULL,
                                                  &handle_count, &handles) == EFI_SUCCESS &&
        handles != NULL) {
        for (UINTN i = 0; i < handle_count; ++i) {
            text_ex = NULL;
            if (g_st->boot_services->handle_protocol(handles[i], (EFI_GUID *)&g_guid_simple_text_input_ex,
                                                     (void **)&text_ex) == EFI_SUCCESS) {
                input_add_extended_protocol(text_ex);
            }
        }
        if (g_st->boot_services->free_pool != NULL) {
            g_st->boot_services->free_pool(handles);
        }
    }
    if (g_input.text_ex == NULL &&
        g_st->boot_services->locate_protocol != NULL &&
        g_st->boot_services->locate_protocol((EFI_GUID *)&g_guid_simple_text_input_ex, NULL, (void **)&text_ex) == EFI_SUCCESS &&
        text_ex != NULL) {
        input_add_extended_protocol(text_ex);
    }
}

static bool input_try_read_key(EFI_INPUT_KEY *key) {
    for (size_t i = 0; i < g_input.standard_count; ++i) {
        EFI_SIMPLE_TEXT_INPUT_PROTOCOL *protocol = g_input.standard_protocols[i];
        EFI_STATUS status;
        if (protocol == NULL || protocol->read_key_stroke == NULL) {
            continue;
        }
        status = protocol->read_key_stroke(protocol, key);
        if (status == EFI_SUCCESS) {
            return true;
        }
    }
    for (size_t i = 0; i < g_input.extended_count; ++i) {
        EFI_KEY_DATA key_data;
        EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *protocol = g_input.extended_protocols[i];
        EFI_STATUS status;
        if (protocol == NULL || protocol->read_key_stroke_ex == NULL) {
            continue;
        }
        status = protocol->read_key_stroke_ex(protocol, &key_data);
        if (status == EFI_SUCCESS) {
            *key = key_data.key;
            return true;
        }
    }
    return false;
}

static void input_pause(void) {
    if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
        g_st->boot_services->stall(2000);
    }
}

static bool input_wait_for_events(EFI_EVENT *events, size_t count, size_t *index_out) {
    UINTN index = 0;

    if (count == 0 || g_st == NULL || g_st->boot_services == NULL ||
        g_st->boot_services->wait_for_event == NULL) {
        return false;
    }
    if (g_st->boot_services->wait_for_event((UINTN)count, events, &index) != EFI_SUCCESS) {
        return false;
    }
    if (index_out != NULL) {
        *index_out = (size_t)index;
    }
    return true;
}

static bool input_wait_for_key(EFI_INPUT_KEY *key) {
    size_t idle_loops = 0;

    while (1) {
        EFI_EVENT events[INPUT_PROTOCOL_MAX * 2];
        size_t count = 0;

        if (input_try_read_key(key)) {
            return true;
        }
        if (g_force_text_ui || !boot_diagnostics_confirmed()) {
            if (idle_loops > 0 && (idle_loops % 300) == 0) {
                input_recover_live_devices();
            }
            input_pause();
            idle_loops++;
            continue;
        }
        for (size_t i = 0; i < g_input.standard_count && count < ARRAY_SIZE(events); ++i) {
            EFI_SIMPLE_TEXT_INPUT_PROTOCOL *protocol = g_input.standard_protocols[i];
            if (protocol != NULL && protocol->wait_for_key != NULL) {
                events[count++] = protocol->wait_for_key;
            }
        }
        for (size_t i = 0; i < g_input.extended_count && count < ARRAY_SIZE(events); ++i) {
            EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *protocol = g_input.extended_protocols[i];
            if (protocol != NULL && protocol->wait_for_key_ex != NULL) {
                events[count++] = protocol->wait_for_key_ex;
            }
        }
        if (input_wait_for_events(events, count, NULL)) {
            continue;
        }
        input_pause();
    }
}

static void line_read(char *buffer, size_t limit) {
    size_t len = 0;

    while (1) {
        EFI_INPUT_KEY key;
        if (!input_wait_for_key(&key)) {
            continue;
        }

        if (key.unicode_char == '\r') {
            console_write("\n");
            break;
        }
        if (key.unicode_char == '\b') {
            if (len > 0) {
                --len;
                console_backspace();
            }
            continue;
        }
        if (key.unicode_char < 32 || key.unicode_char > 126) {
            continue;
        }
        if (len + 1 >= limit) {
            continue;
        }

        buffer[len++] = (char)key.unicode_char;
        console_putchar((char)key.unicode_char);
    }

    buffer[len] = '\0';
}

static void strcopy_limit(char *dst, const char *src, size_t limit) {
    size_t i = 0;
    if (limit == 0) {
        return;
    }
    while (src[i] != '\0' && i + 1 < limit) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static bool text_contains(const char *text, const char *needle) {
    size_t needle_len;
    if (needle == NULL || needle[0] == '\0') {
        return true;
    }
    needle_len = strlen(needle);
    while (*text != '\0') {
        if (strncmp(text, needle, needle_len) == 0) {
            return true;
        }
        ++text;
    }
    return false;
}

static char ascii_lower(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return (char)(ch - 'A' + 'a');
    }
    return ch;
}

static void ascii_to_utf16_path(const char *input, CHAR16 *output, size_t limit) {
    size_t i = 0;
    if (limit == 0) {
        return;
    }
    while (input[i] != '\0' && i + 1 < limit) {
        char ch = input[i];
        output[i] = (CHAR16)(unsigned char)(ch == '/' ? '\\' : ch);
        ++i;
    }
    output[i] = 0;
}

static void persist_set_status(const char *text) {
    strcopy_limit(g_persist.status, text, sizeof(g_persist.status));
}

static void format_mac_address(const EFI_MAC_ADDRESS *mac, uint32_t hw_size, char *output, size_t limit) {
    static const char digits[] = "0123456789ABCDEF";
    size_t out = 0;

    if (output == NULL || limit == 0) {
        return;
    }
    output[0] = '\0';
    if (mac == NULL || hw_size == 0) {
        strcopy_limit(output, "Unavailable", limit);
        return;
    }
    for (uint32_t i = 0; i < hw_size && i < 6 && out + 3 < limit; ++i) {
        uint8_t byte = mac->addr[i];
        if (i > 0) {
            output[out++] = ':';
        }
        output[out++] = digits[byte >> 4];
        output[out++] = digits[byte & 0x0F];
    }
    output[out] = '\0';
}

static void boot_diagnostics_reset(void) {
    memset(&g_boot_diag, 0, sizeof(g_boot_diag));
    g_boot_diag.ran = true;
    g_boot_diag.keyboard_protocol = g_input.standard_available;
    g_boot_diag.keyboard_extended_protocol = g_input.extended_available;
    g_boot_diag.graphics_ready = g_gfx.available;
    g_boot_diag.pointer_protocol = g_pointer.available;
}

static void boot_diagnostics_note_event(const ui_event_t *event) {
    if (event == NULL) {
        return;
    }
    if (event->key_ready) {
        g_boot_diag.key_seen = true;
        if (event->key.unicode_char == '\r') {
            g_boot_diag.enter_seen = true;
        }
    }
    if (event->mouse_moved) {
        g_boot_diag.pointer_moved = true;
    }
    if (event->mouse_clicked) {
        g_boot_diag.pointer_clicked = true;
    }
}

static bool boot_diagnostics_confirmed(void) {
    return g_boot_diag.key_seen || g_boot_diag.pointer_clicked;
}

static void input_recover_live_devices(void) {
    firmware_connect_all_controllers();
    input_init();
    pointer_init();
    g_boot_diag.keyboard_protocol = g_input.standard_available;
    g_boot_diag.keyboard_extended_protocol = g_input.extended_available;
    g_boot_diag.pointer_protocol = g_pointer.available;
}

static size_t shell_tokenize(char *line, char **argv, size_t max_args) {
    size_t argc = 0;
    char *cursor = line;

    while (*cursor != '\0' && argc < max_args) {
        while (isspace(*cursor)) {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        argv[argc++] = cursor;
        while (*cursor != '\0' && !isspace(*cursor)) {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        *cursor++ = '\0';
    }
    return argc;
}

static void shell_join_args(size_t start, size_t argc, char **argv, char *output, size_t limit) {
    size_t out = 0;
    output[0] = '\0';
    for (size_t i = start; i < argc; ++i) {
        size_t j = 0;
        if (i > start && out + 1 < limit) {
            output[out++] = ' ';
        }
        while (argv[i][j] != '\0' && out + 1 < limit) {
            output[out++] = argv[i][j++];
        }
    }
    output[out] = '\0';
}

static void wait_for_enter(const char *message) {
    char temp[4];
    if (message != NULL) {
        console_writeline(message);
    }
    console_write("Press Enter to continue...");
    line_read(temp, sizeof(temp));
}

static const char *path_basename(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash == NULL ? path : slash + 1;
}

static void parent_directory(char *path) {
    char *slash;
    if (strcmp(path, "/") == 0) {
        return;
    }
    slash = strrchr(path, '/');
    if (slash == path) {
        path[1] = '\0';
    } else if (slash != NULL) {
        *slash = '\0';
    }
}

static void resolve_path_from(const char *cwd, const char *input, char *output) {
    char raw[FS_PATH_MAX];
    char components[16][FS_PATH_MAX];
    size_t depth = 0;
    size_t i = 0;
    size_t out = 0;

    if (input == NULL || input[0] == '\0') {
        strcopy_limit(output, cwd, FS_PATH_MAX);
        return;
    }

    memset(raw, 0, sizeof(raw));
    if (input[0] == '/') {
        strcopy_limit(raw, input, sizeof(raw));
    } else if (strcmp(cwd, "/") == 0) {
        raw[0] = '/';
        raw[1] = '\0';
        strcat(raw, input);
    } else {
        strcopy_limit(raw, cwd, sizeof(raw));
        if (strlen(raw) + 1 < sizeof(raw)) {
            strcat(raw, "/");
        }
        if (strlen(raw) + strlen(input) + 1 < sizeof(raw)) {
            strcat(raw, input);
        }
    }

    while (raw[i] != '\0') {
        char token[FS_PATH_MAX];
        size_t token_len = 0;

        while (raw[i] == '/') {
            ++i;
        }
        if (raw[i] == '\0') {
            break;
        }
        while (raw[i] != '\0' && raw[i] != '/' && token_len + 1 < sizeof(token)) {
            token[token_len++] = raw[i++];
        }
        token[token_len] = '\0';

        if (strcmp(token, ".") == 0 || token_len == 0) {
            continue;
        }
        if (strcmp(token, "..") == 0) {
            if (depth > 0) {
                --depth;
            }
            continue;
        }
        if (depth < ARRAY_SIZE(components)) {
            strcopy_limit(components[depth++], token, FS_PATH_MAX);
        }
    }

    output[out++] = '/';
    if (depth == 0) {
        output[out] = '\0';
        return;
    }
    for (size_t component = 0; component < depth; ++component) {
        size_t j = 0;
        while (components[component][j] != '\0' && out + 1 < FS_PATH_MAX) {
            output[out++] = components[component][j++];
        }
        if (component + 1 < depth && out + 1 < FS_PATH_MAX) {
            output[out++] = '/';
        }
    }
    output[out] = '\0';
}

static void resolve_path(const char *input, char *output) {
    resolve_path_from(g_cwd, input, output);
}

static EFI_FILE_PROTOCOL *efi_open_root(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs) {
    EFI_FILE_PROTOCOL *root = NULL;
    if (fs == NULL || fs->open_volume == NULL) {
        return NULL;
    }
    if (fs->open_volume(fs, &root) != EFI_SUCCESS) {
        return NULL;
    }
    return root;
}

static EFI_LOADED_IMAGE_PROTOCOL *efi_loaded_image(void) {
    EFI_LOADED_IMAGE_PROTOCOL *loaded = NULL;

    if (g_st == NULL || g_st->boot_services == NULL || g_st->boot_services->handle_protocol == NULL ||
        g_image_handle == NULL) {
        return NULL;
    }
    if (g_st->boot_services->handle_protocol(g_image_handle, (EFI_GUID *)&g_guid_loaded_image,
                                             (void **)&loaded) != EFI_SUCCESS) {
        return NULL;
    }
    return loaded;
}

static bool persist_volume_is_boot_device(EFI_HANDLE handle) {
    EFI_LOADED_IMAGE_PROTOCOL *loaded = efi_loaded_image();
    return loaded != NULL && loaded->device_handle == handle;
}

static bool efi_get_filesystem_read_only(EFI_FILE_PROTOCOL *root, bool *read_only_out) {
    uint8_t info_buffer[256];
    UINTN info_size = sizeof(info_buffer);
    EFI_FILE_SYSTEM_INFO *info = (EFI_FILE_SYSTEM_INFO *)info_buffer;

    if (root == NULL || read_only_out == NULL || root->get_info == NULL) {
        return false;
    }
    if (root->get_info(root, (EFI_GUID *)&g_guid_file_system_info, &info_size, info_buffer) != EFI_SUCCESS ||
        info_size < sizeof(EFI_FILE_SYSTEM_INFO)) {
        return false;
    }
    *read_only_out = info->read_only != 0;
    return true;
}

static void efi_configure_file_info(EFI_FILE_INFO *info, size_t info_size, uint64_t file_size) {
    if (info == NULL || info_size < sizeof(EFI_FILE_INFO)) {
        return;
    }
    info->size = (uint64_t)info_size;
    info->file_size = file_size;
    info->physical_size = file_size;
}

static void efi_try_truncate_file(EFI_FILE_PROTOCOL *file) {
    uint8_t info_buffer[512];
    UINTN info_size = sizeof(info_buffer);
    EFI_FILE_INFO *info = (EFI_FILE_INFO *)info_buffer;

    if (file == NULL || file->get_info == NULL || file->set_info == NULL) {
        return;
    }
    if (file->get_info(file, (EFI_GUID *)&g_guid_file_info, &info_size, info_buffer) != EFI_SUCCESS ||
        info_size < sizeof(EFI_FILE_INFO)) {
        return;
    }
    efi_configure_file_info(info, (size_t)info_size, 0);
    file->set_info(file, (EFI_GUID *)&g_guid_file_info, info_size, info);
}

static EFI_FILE_PROTOCOL *efi_open_path(EFI_FILE_PROTOCOL *root, const char *path,
                                        uint64_t open_mode, uint64_t attributes,
                                        bool create_directories) {
    EFI_FILE_PROTOCOL *current = root;
    EFI_FILE_PROTOCOL *next = NULL;
    bool close_current = false;
    size_t i = 0;

    if (root == NULL || path == NULL) {
        return NULL;
    }
    while (path[i] == '/' || path[i] == '\\') {
        ++i;
    }
    if (path[i] == '\0') {
        return root;
    }

    while (path[i] != '\0') {
        char component[FS_PATH_MAX];
        CHAR16 component16[FS_PATH_MAX];
        size_t len = 0;
        bool last;
        EFI_STATUS status;

        while (path[i] == '/' || path[i] == '\\') {
            ++i;
        }
        while (path[i] != '\0' && path[i] != '/' && path[i] != '\\' && len + 1 < sizeof(component)) {
            component[len++] = path[i++];
        }
        component[len] = '\0';
        while (path[i] == '/' || path[i] == '\\') {
            ++i;
        }
        last = path[i] == '\0';
        ascii_to_utf16_path(component, component16, ARRAY_SIZE(component16));

        status = current->open(
            current,
            &next,
            component16,
            last ? open_mode
                 : (create_directories ? (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE)
                                       : EFI_FILE_MODE_READ),
            last ? attributes : EFI_FILE_DIRECTORY);
        if (status != EFI_SUCCESS || next == NULL) {
            if (close_current) {
                current->close(current);
            }
            return NULL;
        }
        if (close_current) {
            current->close(current);
        }
        current = next;
        close_current = true;
        next = NULL;
    }
    return current;
}

static bool efi_write_file(EFI_FILE_PROTOCOL *root, const char *path, const uint8_t *data, size_t size) {
    EFI_FILE_PROTOCOL *file;
    UINTN bytes = (UINTN)size;

    if (root == NULL) {
        return false;
    }
    file = efi_open_path(root, path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                         EFI_FILE_ARCHIVE, true);
    if (file == NULL || file == root) {
        return false;
    }
    if (file->set_position != NULL) {
        file->set_position(file, 0);
    }
    efi_try_truncate_file(file);
    if (file->set_position != NULL) {
        file->set_position(file, 0);
    }
    if (size > 0 && file->write(file, &bytes, (void *)data) != EFI_SUCCESS) {
        file->close(file);
        return false;
    }
    if (file->flush != NULL) {
        file->flush(file);
    }
    file->close(file);
    return true;
}

static bool efi_read_file(EFI_FILE_PROTOCOL *root, const char *path, uint8_t *buffer, size_t capacity, size_t *size_out) {
    EFI_FILE_PROTOCOL *file;
    size_t total = 0;

    if (size_out != NULL) {
        *size_out = 0;
    }
    file = efi_open_path(root, path, EFI_FILE_MODE_READ, 0, false);
    if (file == NULL || file == root) {
        return false;
    }
    while (total < capacity) {
        UINTN chunk = (UINTN)(capacity - total);
        EFI_STATUS status = file->read(file, &chunk, buffer + total);
        if (status != EFI_SUCCESS) {
            file->close(file);
            return false;
        }
        if (chunk == 0) {
            break;
        }
        total += (size_t)chunk;
    }
    file->close(file);
    if (size_out != NULL) {
        *size_out = total;
    }
    return true;
}

static bool efi_probe_writable(EFI_HANDLE handle, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs) {
    EFI_FILE_PROTOCOL *root = efi_open_root(fs);
    bool read_only = true;
    bool ok = false;

    if (root == NULL) {
        return false;
    }
    if (efi_get_filesystem_read_only(root, &read_only)) {
        ok = !read_only;
    } else {
        ok = !persist_volume_is_boot_device(handle);
    }
    root->close(root);
    return ok;
}

static void persist_scan_volumes(void) {
    EFI_HANDLE *handles = NULL;
    UINTN count = 0;

    g_persist_volume_count = 0;
    if (g_st->boot_services == NULL || g_st->boot_services->locate_handle_buffer == NULL) {
        return;
    }
    if (g_st->boot_services->locate_handle_buffer(EFI_BY_PROTOCOL, (EFI_GUID *)&g_guid_simple_fs, NULL,
                                                  &count, &handles) != EFI_SUCCESS) {
        return;
    }
    for (UINTN i = 0; i < count && g_persist_volume_count < ARRAY_SIZE(g_persist_volumes); ++i) {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
        if (g_st->boot_services->handle_protocol(handles[i], (EFI_GUID *)&g_guid_simple_fs, (void **)&fs) != EFI_SUCCESS ||
            fs == NULL) {
            continue;
        }
        g_persist_volumes[g_persist_volume_count].handle = handles[i];
        g_persist_volumes[g_persist_volume_count].fs = fs;
        g_persist_volumes[g_persist_volume_count].boot_device = persist_volume_is_boot_device(handles[i]);
        g_persist_volumes[g_persist_volume_count].writable = efi_probe_writable(handles[i], fs);
        ++g_persist_volume_count;
    }
    if (g_st->boot_services->free_pool != NULL && handles != NULL) {
        g_st->boot_services->free_pool(handles);
    }
}

static arm_fs_entry_t *fs_find(const char *path) {
    for (size_t i = 0; i < g_fs_count; ++i) {
        if (strcmp(g_fs[i].path, path) == 0) {
            return &g_fs[i];
        }
    }
    return NULL;
}

static bool fs_parent_exists(const char *path) {
    char parent[FS_PATH_MAX];
    strcopy_limit(parent, path, sizeof(parent));
    parent_directory(parent);
    if (strcmp(parent, "") == 0 || strcmp(parent, "/") == 0) {
        return true;
    }
    arm_fs_entry_t *entry = fs_find(parent);
    return entry != NULL && entry->type == ARM_FS_DIR;
}

static bool fs_has_children(const char *path) {
    size_t len = strlen(path);
    if (strcmp(path, "/") == 0) {
        return g_fs_count > 0;
    }
    for (size_t i = 0; i < g_fs_count; ++i) {
        if (strncmp(g_fs[i].path, path, len) == 0 && g_fs[i].path[len] == '/') {
            return true;
        }
    }
    return false;
}

static size_t fs_count_children(const char *path) {
    arm_fs_entry_t *entries[FILE_MANAGER_VIEW_MAX];
    return fs_list(path, entries, ARRAY_SIZE(entries));
}

static bool fs_push(const char *path, uint32_t type) {
    if (g_fs_count >= FS_MAX_ENTRIES || strlen(path) + 1 > FS_PATH_MAX) {
        return false;
    }
    memset(&g_fs[g_fs_count], 0, sizeof(g_fs[g_fs_count]));
    strcopy_limit(g_fs[g_fs_count].path, path, FS_PATH_MAX);
    g_fs[g_fs_count].type = type;
    ++g_fs_count;
    return true;
}

static bool fs_write(const char *path, const uint8_t *data, size_t size, uint32_t type) {
    arm_fs_entry_t *entry;
    if (path == NULL || path[0] != '/' || size > FS_MAX_FILE_SIZE || type == ARM_FS_DIR) {
        return false;
    }
    entry = fs_find(path);
    if (entry != NULL) {
        if (entry->type == ARM_FS_DIR) {
            return false;
        }
        memset(entry->data, 0, sizeof(entry->data));
        if (size > 0) {
            memcpy(entry->data, data, size);
        }
        entry->size = size;
        entry->type = type;
        persist_mark_dirty();
        return true;
    }
    if (!fs_parent_exists(path) || !fs_push(path, type)) {
        return false;
    }
    if (size > 0) {
        memcpy(g_fs[g_fs_count - 1].data, data, size);
    }
    g_fs[g_fs_count - 1].size = size;
    persist_mark_dirty();
    return true;
}

static bool fs_write_if_changed(const char *path, const uint8_t *data, size_t size, uint32_t type) {
    arm_fs_entry_t *entry = fs_find(path);

    if (entry != NULL && entry->type == type && entry->size == size &&
        (size == 0 || memcmp(entry->data, data, size) == 0)) {
        return true;
    }
    return fs_write(path, data, size, type);
}

static bool fs_mkdir(const char *path) {
    arm_fs_entry_t *entry;
    if (path == NULL || path[0] != '/' || strcmp(path, "/") == 0) {
        return false;
    }
    entry = fs_find(path);
    if (entry != NULL) {
        return entry->type == ARM_FS_DIR;
    }
    if (!fs_parent_exists(path)) {
        return false;
    }
    if (!fs_push(path, ARM_FS_DIR)) {
        return false;
    }
    persist_mark_dirty();
    return true;
}

static bool fs_remove(const char *path) {
    if (path == NULL || path[0] != '/' || strcmp(path, "/") == 0) {
        return false;
    }
    for (size_t i = 0; i < g_fs_count; ++i) {
        if (strcmp(g_fs[i].path, path) == 0) {
            if (g_fs[i].type == ARM_FS_DIR && fs_has_children(path)) {
                return false;
            }
            for (size_t j = i + 1; j < g_fs_count; ++j) {
                g_fs[j - 1] = g_fs[j];
            }
            --g_fs_count;
            persist_mark_dirty();
            return true;
        }
    }
    return false;
}

static bool fs_copy(const char *source, const char *destination) {
    arm_fs_entry_t *src = fs_find(source);
    if (src == NULL || src->type == ARM_FS_DIR || fs_find(destination) != NULL) {
        return false;
    }
    return fs_write(destination, src->data, src->size, src->type);
}

static bool fs_move(const char *source, const char *destination) {
    size_t source_len;
    arm_fs_entry_t *src = fs_find(source);
    if (src == NULL || fs_find(destination) != NULL || !fs_parent_exists(destination)) {
        return false;
    }
    source_len = strlen(source);
    if (src->type == ARM_FS_DIR &&
        strncmp(destination, source, source_len) == 0 &&
        destination[source_len] == '/') {
        return false;
    }
    for (size_t i = 0; i < g_fs_count; ++i) {
        if (strcmp(g_fs[i].path, source) == 0 ||
            (strncmp(g_fs[i].path, source, source_len) == 0 && g_fs[i].path[source_len] == '/')) {
            char suffix[FS_PATH_MAX];
            strcopy_limit(suffix, g_fs[i].path + source_len, sizeof(suffix));
            strcopy_limit(g_fs[i].path, destination, sizeof(g_fs[i].path));
            if (suffix[0] != '\0') {
                strcat(g_fs[i].path, suffix);
            }
        }
    }
    persist_mark_dirty();
    return true;
}

static bool fs_append_line(const char *path, const char *text) {
    size_t length = 0;
    size_t text_len = strlen(text);
    arm_fs_entry_t *entry = fs_find(path);

    memset(g_append_buffer, 0, sizeof(g_append_buffer));
    if (entry != NULL) {
        if (entry->type == ARM_FS_DIR) {
            return false;
        }
        memcpy(g_append_buffer, entry->data, entry->size);
        length = entry->size;
    } else if (!fs_parent_exists(path)) {
        return false;
    }

    if (length > 0 && g_append_buffer[length - 1] != '\n') {
        if (length + 1 >= sizeof(g_append_buffer)) {
            return false;
        }
        g_append_buffer[length++] = '\n';
    }
    if (length + text_len + 1 > sizeof(g_append_buffer)) {
        return false;
    }
    memcpy(g_append_buffer + length, text, text_len);
    length += text_len;
    g_append_buffer[length++] = '\n';
    return fs_write(path, g_append_buffer, length, ARM_FS_TEXT);
}

static size_t fs_list(const char *directory, arm_fs_entry_t **results, size_t max_results) {
    size_t count = 0;
    size_t dir_len = strlen(directory);
    for (size_t i = 0; i < g_fs_count && count < max_results; ++i) {
        const char *path = g_fs[i].path;
        if (strcmp(directory, "/") == 0) {
            const char *tail = path + 1;
            if (strchr(tail, '/') == NULL) {
                results[count++] = &g_fs[i];
            }
        } else if (strncmp(path, directory, dir_len) == 0 && path[dir_len] == '/') {
            const char *tail = path + dir_len + 1;
            if (strchr(tail, '/') == NULL) {
                results[count++] = &g_fs[i];
            }
        }
    }
    return count;
}

typedef struct persist_header {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_count;
    uint32_t reserved;
} persist_header_t;

typedef struct persist_entry_header {
    uint16_t path_length;
    uint16_t type;
    uint32_t size;
} persist_entry_header_t;

#define SNSX_STATE_MAGIC 0x58434e53u
#define SNSX_STATE_VERSION 1u

static bool persist_serialize(uint8_t *buffer, size_t capacity, size_t *size_out) {
    persist_header_t *header;
    size_t offset = sizeof(persist_header_t);

    if (capacity < sizeof(persist_header_t)) {
        return false;
    }
    header = (persist_header_t *)buffer;
    header->magic = SNSX_STATE_MAGIC;
    header->version = SNSX_STATE_VERSION;
    header->entry_count = 0;
    header->reserved = 0;

    for (size_t i = 0; i < g_fs_count; ++i) {
        persist_entry_header_t entry;
        size_t path_len = strlen(g_fs[i].path);
        entry.path_length = (uint16_t)path_len;
        entry.type = (uint16_t)g_fs[i].type;
        entry.size = (uint32_t)g_fs[i].size;
        if (offset + sizeof(entry) + path_len + g_fs[i].size > capacity) {
            return false;
        }
        memcpy(buffer + offset, &entry, sizeof(entry));
        offset += sizeof(entry);
        memcpy(buffer + offset, g_fs[i].path, path_len);
        offset += path_len;
        if (g_fs[i].size > 0) {
            memcpy(buffer + offset, g_fs[i].data, g_fs[i].size);
            offset += g_fs[i].size;
        }
        header->entry_count++;
    }
    if (size_out != NULL) {
        *size_out = offset;
    }
    return true;
}

static bool persist_deserialize(const uint8_t *buffer, size_t size) {
    const persist_header_t *header;
    size_t offset = sizeof(persist_header_t);

    if (size < sizeof(persist_header_t)) {
        return false;
    }
    header = (const persist_header_t *)buffer;
    if (header->magic != SNSX_STATE_MAGIC || header->version != SNSX_STATE_VERSION) {
        return false;
    }
    for (uint32_t i = 0; i < header->entry_count; ++i) {
        persist_entry_header_t entry;
        char path[FS_PATH_MAX];
        if (offset + sizeof(entry) > size) {
            return false;
        }
        memcpy(&entry, buffer + offset, sizeof(entry));
        offset += sizeof(entry);
        if (entry.path_length + 1 > sizeof(path) || offset + entry.path_length + entry.size > size) {
            return false;
        }
        memcpy(path, buffer + offset, entry.path_length);
        path[entry.path_length] = '\0';
        offset += entry.path_length;
        if (entry.type == ARM_FS_DIR) {
            fs_mkdir(path);
        } else {
            fs_write(path, buffer + offset, entry.size, ARM_FS_TEXT);
        }
        offset += entry.size;
    }
    return true;
}

static bool persist_read_state_from_volume(size_t volume_index, bool apply_state) {
    EFI_FILE_PROTOCOL *root;
    size_t size = 0;
    bool ok;

    if (volume_index >= g_persist_volume_count) {
        return false;
    }
    root = efi_open_root(g_persist_volumes[volume_index].fs);
    if (root == NULL) {
        return false;
    }
    ok = efi_read_file(root, "/SNSX/STATE.BIN", g_persist_state_buffer, sizeof(g_persist_state_buffer), &size);
    root->close(root);
    if (!ok) {
        return false;
    }
    if (!apply_state) {
        return true;
    }
    return persist_deserialize(g_persist_state_buffer, size);
}

static void persist_discover(void) {
    g_persist.available = false;
    g_persist.installed = false;
    g_persist.dirty = false;
    persist_scan_volumes();
    for (size_t i = 0; i < g_persist_volume_count; ++i) {
        if (g_persist_volumes[i].writable && persist_read_state_from_volume(i, false)) {
            g_persist.available = true;
            g_persist.installed = true;
            g_persist.volume_index = i;
            persist_set_status("Persistent SNSX disk found.");
            persist_read_state_from_volume(i, true);
            g_persist.dirty = false;
            return;
        }
    }
    for (size_t i = 0; i < g_persist_volume_count; ++i) {
        if (g_persist_volumes[i].writable) {
            g_persist.available = true;
            g_persist.volume_index = i;
            persist_set_status("Writable disk found. Run Installer to persist SNSX.");
            return;
        }
    }
    persist_set_status("No writable FAT disk is attached.");
}

static void persist_mark_dirty(void) {
    if (g_persist.available) {
        g_persist.dirty = true;
    }
}

static void persist_flush_state(void) {
    EFI_FILE_PROTOCOL *root;
    size_t size = 0;

    if (!g_persist.available || !g_persist.dirty) {
        return;
    }
    if (!persist_serialize(g_persist_state_buffer, sizeof(g_persist_state_buffer), &size)) {
        persist_set_status("Persistence image is too large to save.");
        return;
    }
    root = efi_open_root(g_persist_volumes[g_persist.volume_index].fs);
    if (root == NULL) {
        persist_set_status("Could not open persistence disk.");
        return;
    }
    if (efi_write_file(root, "/SNSX/STATE.BIN", g_persist_state_buffer, size)) {
        g_persist.dirty = false;
        g_persist.installed = true;
        persist_set_status("State saved to persistent SNSX disk.");
    } else {
        persist_set_status("Saving state to disk failed.");
    }
    root->close(root);
}

static bool persist_read_boot_image(uint8_t *buffer, size_t capacity, size_t *size_out) {
    for (size_t i = 0; i < g_persist_volume_count; ++i) {
        EFI_FILE_PROTOCOL *root = efi_open_root(g_persist_volumes[i].fs);
        bool ok;
        if (root == NULL) {
            continue;
        }
        ok = efi_read_file(root, "/EFI/BOOT/BOOTAA64.EFI", buffer, capacity, size_out);
        root->close(root);
        if (ok) {
            return true;
        }
    }
    return false;
}

static bool persist_install_boot_disk(void) {
    size_t boot_size = 0;
    EFI_FILE_PROTOCOL *root;
    static const char install_note[] =
        "SNSX installed disk\n"
        "This volume stores BOOTAA64.EFI and the persistent SNSX state snapshot.\n";

    if (!g_persist.available) {
        persist_set_status("No writable volume is available for install.");
        return false;
    }
    if (!persist_read_boot_image(g_persist_boot_buffer, sizeof(g_persist_boot_buffer), &boot_size)) {
        persist_set_status("Could not find the boot image on any mounted volume.");
        return false;
    }
    root = efi_open_root(g_persist_volumes[g_persist.volume_index].fs);
    if (root == NULL) {
        persist_set_status("Could not open the target install disk.");
        return false;
    }
    if (!efi_write_file(root, "/EFI/BOOT/BOOTAA64.EFI", g_persist_boot_buffer, boot_size)) {
        root->close(root);
        persist_set_status("Writing BOOTAA64.EFI failed.");
        return false;
    }
    efi_write_file(root, "/SNSX/README.TXT", (const uint8_t *)install_note, sizeof(install_note) - 1);
    root->close(root);
    g_persist.installed = true;
    g_persist.dirty = true;
    persist_flush_state();
    persist_set_status("SNSX boot files installed to the persistent disk.");
    return true;
}

static size_t disk_collect_info(disk_volume_info_t *infos, size_t max_infos) {
    size_t count = 0;

    if (infos == NULL || max_infos == 0) {
        return 0;
    }
    for (size_t i = 0; i < g_persist_volume_count && count < max_infos; ++i) {
        EFI_FILE_PROTOCOL *root = efi_open_root(g_persist_volumes[i].fs);
        size_t state_size = 0;
        bool has_state = false;
        bool has_bootloader = false;

        memset(&infos[count], 0, sizeof(infos[count]));
        infos[count].present = true;
        infos[count].writable = g_persist_volumes[i].writable;
        infos[count].selected = i == g_persist.volume_index;
        strcopy_limit(infos[count].name, "VOLUME ", sizeof(infos[count].name));
        {
            char number[8];
            u32_to_dec((uint32_t)(i + 1), number);
            strcat(infos[count].name, number);
        }

        if (root != NULL) {
            has_state = efi_read_file(root, "/SNSX/STATE.BIN", g_persist_state_buffer,
                                      sizeof(g_persist_state_buffer), &state_size);
            has_bootloader = efi_read_file(root, "/EFI/BOOT/BOOTAA64.EFI", g_persist_boot_buffer,
                                           sizeof(g_persist_boot_buffer), NULL);
            root->close(root);
        }

        infos[count].has_state = has_state;
        infos[count].has_bootloader = has_bootloader;
        infos[count].state_size = state_size;

        if (has_bootloader && has_state) {
            strcopy_limit(infos[count].role, "Bootable SNSX disk with a saved live filesystem snapshot.", sizeof(infos[count].role));
        } else if (has_bootloader) {
            strcopy_limit(infos[count].role, "Bootable SNSX disk. No saved filesystem snapshot yet.", sizeof(infos[count].role));
        } else if (has_state) {
            strcopy_limit(infos[count].role, "Writable disk with an SNSX state snapshot, but no bootloader copy.", sizeof(infos[count].role));
        } else if (infos[count].writable) {
            strcopy_limit(infos[count].role, "Writable FAT volume ready for install/save.", sizeof(infos[count].role));
        } else {
            strcopy_limit(infos[count].role, "Read-only volume detected by UEFI.", sizeof(infos[count].role));
        }
        count++;
    }
    return count;
}

static void fs_seed(void) {
    static const char welcome[] =
        "Welcome to SNSX ARM64 for VirtualBox.\n"
        "This is the Apple Silicon compatible live OS image.\n"
        "The VirtualBox build now starts with input diagnostics and a first-run setup screen.\n"
        "After setup you land in the graphical desktop, then use Install and Save to persist your work.\n";
    static const char readme[] =
        "SNSX ARM64 VirtualBox edition\n"
        "Built as a UEFI AArch64 live operating system for Apple Silicon hosts.\n"
        "Core features: desktop home screen, shell, writable RAM filesystem,\n"
        "persistence, installer, settings, network center, disk manager,\n"
        "system center, calculator, snake, app store, text editor, journal, file manager, and guides.\n";
    static const char vbox[] =
        "SNSX VirtualBox quick guide\n"
        "1. Build SNSX-VBox-ARM64.iso.\n"
        "2. Create a new ARM64 VM in VirtualBox or run make virtualbox-vm VM_NAME=SNS.\n"
        "3. Leave EFI enabled.\n"
        "4. Attach the ISO as the optical disk.\n"
        "5. Attach the auto-created persistence disk if you want an installed, writable session.\n"
        "6. Start the VM and choose the optical boot entry if firmware asks.\n";
    static const char quickstart[] =
        "SNSX quick start\n"
        "================\n"
        "help                 list shell commands\n"
        "setup                reopen the first-run setup app\n"
        "desktop              return to the desktop home screen\n"
        "files                open the file manager\n"
        "journal              open the journal editor\n"
        "calc 7*(8+1)         run the calculator\n"
        "edit /home/NOTES.TXT open the editor\n"
        "theme sunrise        switch the desktop theme\n"
        "pointer 3            set mouse speed\n"
        "netmode ethernet     prefer the VirtualBox wired LAN path\n"
        "dhcp                 renew a wired DHCP lease\n"
        "dns example.com      resolve a host name\n"
        "http http://example.com/ fetch a remote page\n"
        "hostname SNSX-LAB    rename the SNSX network profile\n"
        "snsxaddr             show SNSX protocol identities\n"
        "snake                open the Snake arcade app\n"
        "store                open the SNSX App Store\n"
        "installer            open the Installer app\n"
        "system               open the System Center\n"
        "open /docs/INSTALL.TXT  read bundled guides\n";
    static const char install[] =
        "SNSX on VirtualBox (Apple Silicon)\n"
        "==================================\n"
        "1. Run: make vbox-arm64\n"
        "2. Optional one-command setup: make virtualbox-vm VM_NAME=SNSX\n"
        "3. In VirtualBox, use an ARM64 guest. Old x86 VMs will not boot on Apple Silicon.\n"
        "4. EFI must stay enabled.\n"
        "5. Attach SNSX-VBox-ARM64.iso and start the VM.\n"
        "6. The VM opens diagnostics and a first-run setup screen before the desktop.\n"
        "7. At the desktop press Enter for the shell, then run install and save.\n"
        "8. Remove the ISO and boot from the installed disk if desired.\n"
        "\n"
        "For USB sticks and real laptops, read /docs/BAREMETAL.TXT.\n";
    static const char desktop[] =
        "SNSX desktop guide\n"
        "==================\n"
        "SNSX boots through diagnostics, then enters the graphical desktop in VirtualBox.\n"
        "Choose apps such as Files, Notes, Calc, Snake, Guide, Installer, Network, Browser, and Store.\n"
        "Press Enter for the shell from Terminal, or type desktop later to come back.\n"
        "Use install and save to place SNSX onto the writable VM disk.\n";
    static const char commands[] =
        "SNSX command groups\n"
        "===================\n"
        "Navigation: pwd, ls, tree, cd, open, find, stat\n"
        "Files: touch, new, write, append, mkdir, rm, cp, mv\n"
        "Desktop: desktop, launcher, theme, pointer, trail, save, install, net, devices, snsxaddr\n"
        "Network: network, netmode, autonet, hostname, wifi, hotspot, netconnect, netrefresh, netdiag, ip, dhcp, dns, http\n"
        "Store: store, pkglist, installpkg ID, removepkg ID, baremetal\n"
        "Apps: files, notes, journal, calc, snake, store, installer, settings, system, guide, quickstart, browser, about\n";
    static const char devices[] =
        "SNSX device diagnostics\n"
        "=======================\n"
        "devices               show firmware and desktop device state\n"
        "about                 show overall system summary\n"
        "network / net         show network adapter visibility\n"
        "pointer 1..5          tune relative mouse speed for VirtualBox capture\n"
        "If keyboard input does not respond in VirtualBox, verify the VM uses\n"
        "ARM64, EFI, xHCI, USB Keyboard, and USB Tablet in the SNSX VM helper.\n";
    static const char network_doc[] =
        "SNSX Network Center\n"
        "===================\n"
        "SNSX first tries to use a firmware network adapter from UEFI.\n"
        "If firmware transport is missing, SNSX can also target the VirtualBox Intel 82540EM NIC directly.\n"
        "On this Apple Silicon VirtualBox target, host Wi-Fi is presented to the guest as virtual Ethernet.\n"
        "That means nearby hotspot names do not appear inside SNSX unless the guest receives a real Wi-Fi device.\n"
        "The Network Center can refresh, inspect firmware handles, and report PCI NIC hardware while also\n"
        "storing preferred profiles for Auto, Ethernet, Wi-Fi, and Hotspot modes.\n"
        "SNSX now also exposes its own identity strings: snsx://device/HOSTNAME, snsxnet://HOSTNAME@IP,\n"
        "and snsx-id://HOSTNAME-MACSUFFIX for browser pages and dashboard routing.\n"
        "Wi-Fi and hotspot controls are part of Settings and the Network Center UI,\n"
        "and the wired path now supports DHCP, IPv4, DNS, plain HTTP, and HTTPS through the optional host bridge.\n"
        "Use netconnect, netdiag, dhcp, ip, dns, http, netmode, autonet, hostname, wifi, and hotspot from the shell.\n";
    static const char bluetooth_doc[] =
        "SNSX Bluetooth Center\n"
        "=====================\n"
        "SNSX now includes a Bluetooth pairing dashboard and saved-device store.\n"
        "On the current Apple Silicon VirtualBox target there is no Bluetooth controller\n"
        "exposed to the guest firmware, so nearby device discovery and live pairing stay unavailable.\n"
        "Saved pairings are kept so the UI and settings model are ready for future hardware-backed builds.\n";
    static const char browser_doc[] =
        "SNSX Browser\n"
        "============\n"
        "SNSX Browser is the OS browsing shell for local docs, bookmarks, SNSX protocols, diagnostics, and wired web access.\n"
        "Use snsx://... pages for local system routes, snsxnet://... pages for live network state,\n"
        "snsxdiag://network for firmware and PCI diagnostics, snsx://store for the package catalog,\n"
        "and plain http:// URLs for remote text and HTML pages over the current SNSX network stack.\n"
        "The browser keeps tabs, history, bookmarks, and page snapshots in /downloads.\n"
        "On VirtualBox, https:// URLs can also flow through the optional SNSX host bridge started with make https-bridge.\n";
    static const char store_doc[] =
        "SNSX App Store\n"
        "==============\n"
        "The SNSX App Store manages built-in apps, games, driver packs, and install kits.\n"
        "Core packages stay installed. Optional kits can be installed or removed from the Store app or shell.\n"
        "Installed package assets appear under /apps, /games, /drivers, or /kits.\n"
        "Useful commands: store, pkglist, installpkg ID, removepkg ID.\n";
    static const char studio_doc[] =
        "SNSX Code Studio\n"
        "================\n"
        "Code Studio manages live project files under /projects.\n"
        "Use EDIT to change a file, RUN to execute it, and BUILD to compile it.\n"
        "Execution flows through the optional host bridge started with make dev-bridge.\n"
        "\n"
        "Languages available in this build:\n"
        "- SNSX (.snsx)\n"
        "- Python (.py)\n"
        "- JavaScript (.js)\n"
        "- C (.c)\n"
        "- C++ (.cpp)\n"
        "- HTML / CSS source workspaces\n";
    static const char devbridge_doc[] =
        "SNSX Dev Bridge\n"
        "===============\n"
        "The dev bridge lets the SNSX VM call host toolchains over the wired guest link.\n"
        "\n"
        "Host commands:\n"
        "make dev-bridge\n"
        "make dev-bridge-stop\n"
        "make dev-bridge-status\n"
        "\n"
        "Guest commands:\n"
        "studio\n"
        "run [FILE]\n"
        "build [FILE]\n";
    static const char baremetal_doc[] =
        "SNSX Bare-Metal Install\n"
        "=======================\n"
        "SNSX boots as an ARM64 UEFI application. For a real laptop or development board:\n"
        "1. Prepare a FAT32 USB stick or EFI System Partition.\n"
        "2. Copy BOOTAA64.EFI to /EFI/BOOT/BOOTAA64.EFI, or write the SNSX ARM64 ISO to removable media.\n"
        "3. Boot the target in UEFI mode. Disable Secure Boot if the platform rejects unsigned EFI apps.\n"
        "4. Inside the live SNSX session, open Disk Manager or Installer.\n"
        "5. Select the writable FAT volume that should carry SNSX boot files.\n"
        "6. Run INSTALL to place BOOTAA64.EFI, then SAVE to write /SNSX/STATE.BIN.\n"
        "7. Set that disk first in the firmware boot order if you want SNSX to become the primary OS.\n"
        "\n"
        "Current limits:\n"
        "- SNSX does not yet partition disks automatically.\n"
        "- Nearby Wi-Fi and Bluetooth discovery still need chipset-specific drivers.\n"
        "- The App Store now carries the driver-pack framework for those future builds.\n";
    static const char drivers_doc[] =
        "SNSX Drivers\n"
        "============\n"
        "Built-in today:\n"
        "- Intel 82540EM Ethernet path for VirtualBox-style wired networking work.\n"
        "\n"
        "Driver-pack framework available through the SNSX App Store:\n"
        "- Wi-Fi Driver Lab\n"
        "- Bluetooth Driver Lab\n"
        "- Printer Driver Kit\n"
        "\n"
        "Real nearby SSID or Bluetooth scanning on bare metal requires actual adapter-specific drivers.\n";
    static const char protocols_doc[] =
        "SNSX Protocols\n"
        "==============\n"
        "snsx://device/HOSTNAME      local SNSX node identity\n"
        "snsx://home                 browser home and bookmarks\n"
        "snsx://tabs                 browser tab overview\n"
        "snsx://bookmarks            saved browser bookmarks\n"
        "snsx://history              recent browser history\n"
        "snsx://downloads            saved browser snapshots\n"
        "snsx://studio               Code Studio overview\n"
        "snsx://preview              text preview of /projects/index.html\n"
        "snsx://store                local SNSX App Store summary page\n"
        "snsx://baremetal            bare-metal install guide page\n"
        "snsx://network              local Network Center summary page\n"
        "snsx://wireless             Wi-Fi and Bluetooth summary page\n"
        "snsx://apps                 local app catalog page\n"
        "snsx://snake                Snake app landing page\n"
        "snsxdiag://network          firmware and PCI network diagnostics\n"
        "snsxnet://HOST@IP           live network route identity\n"
        "snsxnet://status            network status summary page\n"
        "snsx-id://HOST-MAC          short hardware-linked identity string\n";
    static const char snake_doc[] =
        "SNSX Snake\n"
        "==========\n"
        "Use Arrow keys or WASD to steer.\n"
        "Press Space to pause.\n"
        "Press R to restart.\n"
        "Press ESC to leave the arcade.\n";
    static const char disks_doc[] =
        "SNSX Disk Manager\n"
        "=================\n"
        "Use the Disk Manager to inspect writable FAT volumes found by UEFI.\n"
        "It shows whether a volume contains BOOTAA64.EFI and STATE.BIN.\n"
        "USE selects the active SNSX target volume.\n"
        "Install writes BOOTAA64.EFI to the selected writable disk.\n"
        "Save writes the live SNSX filesystem snapshot to STATE.BIN.\n";
    static const char notes[] =
        "SNSX Notes\n"
        "==========\n"
        "This text file can be edited from the live shell.\n";
    static const char journal[] =
        "SNSX Journal\n"
        "============\n"
        "Use append /home/JOURNAL.TXT your text here\n"
        "to keep a running boot-session log.\n";
    static const char demo[] =
        "SNSX Project Notes\n"
        "==================\n"
        "This workspace is bundled to demonstrate directories,\n"
        "text editing, and file management inside SNSX.\n";

    g_fs_count = 0;
    fs_mkdir("/docs");
    fs_mkdir("/home");
    fs_mkdir("/projects");
    fs_mkdir("/tmp");
    fs_mkdir("/apps");
    fs_mkdir("/games");
    fs_mkdir("/drivers");
    fs_mkdir("/kits");
    fs_write("/README.TXT", (const uint8_t *)readme, sizeof(readme) - 1, ARM_FS_TEXT);
    fs_write("/docs/VBOX.TXT", (const uint8_t *)vbox, sizeof(vbox) - 1, ARM_FS_TEXT);
    fs_write("/docs/QUICKSTART.TXT", (const uint8_t *)quickstart, sizeof(quickstart) - 1, ARM_FS_TEXT);
    fs_write("/docs/INSTALL.TXT", (const uint8_t *)install, sizeof(install) - 1, ARM_FS_TEXT);
    fs_write("/docs/DESKTOP.TXT", (const uint8_t *)desktop, sizeof(desktop) - 1, ARM_FS_TEXT);
    fs_write("/docs/COMMANDS.TXT", (const uint8_t *)commands, sizeof(commands) - 1, ARM_FS_TEXT);
    fs_write("/docs/DEVICES.TXT", (const uint8_t *)devices, sizeof(devices) - 1, ARM_FS_TEXT);
    fs_write("/docs/NETWORK.TXT", (const uint8_t *)network_doc, sizeof(network_doc) - 1, ARM_FS_TEXT);
    fs_write("/docs/BLUETOOTH.TXT", (const uint8_t *)bluetooth_doc, sizeof(bluetooth_doc) - 1, ARM_FS_TEXT);
    fs_write("/docs/BROWSER.TXT", (const uint8_t *)browser_doc, sizeof(browser_doc) - 1, ARM_FS_TEXT);
    fs_write("/docs/STORE.TXT", (const uint8_t *)store_doc, sizeof(store_doc) - 1, ARM_FS_TEXT);
    fs_write("/docs/STUDIO.TXT", (const uint8_t *)studio_doc, sizeof(studio_doc) - 1, ARM_FS_TEXT);
    fs_write("/docs/DEVBRIDGE.TXT", (const uint8_t *)devbridge_doc, sizeof(devbridge_doc) - 1, ARM_FS_TEXT);
    fs_write("/docs/BAREMETAL.TXT", (const uint8_t *)baremetal_doc, sizeof(baremetal_doc) - 1, ARM_FS_TEXT);
    fs_write("/docs/DRIVERS.TXT", (const uint8_t *)drivers_doc, sizeof(drivers_doc) - 1, ARM_FS_TEXT);
    fs_write("/docs/PROTOCOLS.TXT", (const uint8_t *)protocols_doc, sizeof(protocols_doc) - 1, ARM_FS_TEXT);
    fs_write("/docs/SNAKE.TXT", (const uint8_t *)snake_doc, sizeof(snake_doc) - 1, ARM_FS_TEXT);
    fs_write("/docs/DISKS.TXT", (const uint8_t *)disks_doc, sizeof(disks_doc) - 1, ARM_FS_TEXT);
    fs_write("/home/WELCOME.TXT", (const uint8_t *)welcome, sizeof(welcome) - 1, ARM_FS_TEXT);
    fs_write("/home/NOTES.TXT", (const uint8_t *)notes, sizeof(notes) - 1, ARM_FS_TEXT);
    fs_write("/home/JOURNAL.TXT", (const uint8_t *)journal, sizeof(journal) - 1, ARM_FS_TEXT);
    fs_write("/projects/DEMO.TXT", (const uint8_t *)demo, sizeof(demo) - 1, ARM_FS_TEXT);
}

static void desktop_set_status(const char *text) {
    strcopy_limit(g_desktop.status_line, text, sizeof(g_desktop.status_line));
}

static void desktop_note_launch(const char *app_name, const char *status) {
    strcopy_limit(g_desktop.last_app, app_name, sizeof(g_desktop.last_app));
    if (status != NULL && status[0] != '\0') {
        desktop_set_status(status);
    }
}

static void desktop_commit_preferences(void) {
    desktop_save_config();
    wireless_profiles_save();
}

static int desktop_find_theme(const char *value) {
    if (value == NULL || value[0] == '\0') {
        return -1;
    }
    if (value[0] >= '0' && value[0] <= '9') {
        int index = atoi(value);
        return (index >= 0 && (size_t)index < ARRAY_SIZE(g_theme_names)) ? index : -1;
    }
    for (size_t i = 0; i < ARRAY_SIZE(g_theme_names); ++i) {
        const char *name = g_theme_names[i];
        size_t j = 0;
        while (name[j] != '\0' && value[j] != '\0' && ascii_lower(name[j]) == ascii_lower(value[j])) {
            ++j;
        }
        if (name[j] == '\0' && value[j] == '\0') {
            return (int)i;
        }
    }
    return -1;
}

static const char *desktop_pointer_speed_name_for(uint32_t speed) {
    switch (speed) {
        case 1: return "SLOW";
        case 2: return "BALANCED";
        case 3: return "FAST";
        case 4: return "FLUID";
        case 5: return "PRECISE";
        default: return "BALANCED";
    }
}

static const char *desktop_pointer_speed_name(void) {
    return desktop_pointer_speed_name_for(g_desktop.pointer_speed);
}

static const char *desktop_network_mode_name(void) {
    switch (g_desktop.network_preference) {
        case NETWORK_PREF_ETHERNET:
            return "ETHERNET";
        case NETWORK_PREF_WIFI:
            return "WIFI";
        case NETWORK_PREF_HOTSPOT:
            return "HOTSPOT";
        case NETWORK_PREF_AUTO:
        default:
            return "AUTO";
    }
}

static void desktop_cycle_network_mode(void) {
    g_desktop.network_preference = (g_desktop.network_preference + 1u) % 4u;
}

static void desktop_set_default_preferences(void) {
    g_desktop.theme = 0;
    g_desktop.welcome_seen = false;
    g_desktop.autosave = true;
    g_desktop.pointer_trail = true;
    g_desktop.wifi_enabled = true;
    g_desktop.bluetooth_enabled = true;
    g_desktop.network_autoconnect = true;
    g_desktop.launch_count = 0;
    g_desktop.pointer_speed = 2;
    g_desktop.network_preference = NETWORK_PREF_AUTO;
    strcopy_limit(g_desktop.last_app, "WELCOME", sizeof(g_desktop.last_app));
    strcopy_limit(g_desktop.hostname, "SNSX", sizeof(g_desktop.hostname));
    strcopy_limit(g_desktop.wifi_ssid, "SNSX-LAB", sizeof(g_desktop.wifi_ssid));
    strcopy_limit(g_desktop.hotspot_name, "SNSX-HOTSPOT", sizeof(g_desktop.hotspot_name));
    desktop_set_status("SNSX desktop ready.");
}

static size_t wifi_collect_slots(size_t *slots, size_t capacity) {
    size_t count = 0;

    for (size_t i = 0; i < WIFI_PROFILE_MAX && count < capacity; ++i) {
        if (g_wifi_profiles[i].used) {
            slots[count++] = i;
        }
    }
    return count;
}

static size_t bluetooth_collect_slots(size_t *slots, size_t capacity) {
    size_t count = 0;

    for (size_t i = 0; i < BLUETOOTH_DEVICE_MAX && count < capacity; ++i) {
        if (g_bluetooth_devices[i].used) {
            slots[count++] = i;
        }
    }
    return count;
}

static void network_compose_snsx_address(char *output, size_t output_size) {
    strcopy_limit(output, "snsx://device/", output_size);
    strcat(output, g_desktop.hostname);
}

static void network_compose_session_address(char *output, size_t output_size) {
    strcopy_limit(output, "snsxnet://", output_size);
    strcat(output, g_desktop.hostname);
    strcat(output, "@");
    strcat(output, g_netstack.configured ? g_network.ipv4 : "offline");
}

static void network_compose_link_address(char *output, size_t output_size) {
    char suffix[16];
    size_t mac_len = strlen(g_network.mac);

    strcopy_limit(output, "snsx-id://", output_size);
    strcat(output, g_desktop.hostname);
    strcat(output, "-");
    if (mac_len >= 5) {
        strcopy_limit(suffix, g_network.mac + mac_len - 5, sizeof(suffix));
    } else {
        strcopy_limit(suffix, "guest", sizeof(suffix));
    }
    for (size_t i = 0; suffix[i] != '\0'; ++i) {
        if (suffix[i] == ':') {
            suffix[i] = '-';
        }
    }
    strcat(output, suffix);
}

static void __attribute__((unused)) desktop_prompt_network_profile(void) {
    char input[FS_PATH_MAX];

    if (!g_gfx.available) {
        return;
    }

    if (ui_prompt_line("HOST NAME", "ENTER A HOST NAME FOR THIS SNSX SESSION",
                       g_desktop.hostname, input, sizeof(input)) && input[0] != '\0') {
        strcopy_limit(g_desktop.hostname, input, sizeof(g_desktop.hostname));
    }
    if (ui_prompt_line("WI-FI PROFILE", "STORE A WI-FI SSID FOR FUTURE SNSX NETWORKING",
                       g_desktop.wifi_ssid, input, sizeof(input)) && input[0] != '\0') {
        strcopy_limit(g_desktop.wifi_ssid, input, sizeof(g_desktop.wifi_ssid));
    }
    if (ui_prompt_line("HOTSPOT PROFILE", "STORE A HOTSPOT NAME FOR FUTURE SNSX AP MODE",
                       g_desktop.hotspot_name, input, sizeof(input)) && input[0] != '\0') {
        strcopy_limit(g_desktop.hotspot_name, input, sizeof(g_desktop.hotspot_name));
    }

    desktop_note_launch("NETWORK", "Updated the stored SNSX network profile.");
    desktop_commit_preferences();
    network_refresh(g_desktop.network_autoconnect);
}

static bool wifi_radio_available(void) {
    return false;
}

static bool bluetooth_controller_available(void) {
    return false;
}

static void wireless_profiles_save(void) {
    char wifi_buffer[1024];
    char bt_buffer[1024];

    wifi_buffer[0] = '\0';
    bt_buffer[0] = '\0';

    for (size_t i = 0; i < WIFI_PROFILE_MAX; ++i) {
        if (!g_wifi_profiles[i].used) {
            continue;
        }
        strcat(wifi_buffer, g_wifi_profiles[i].ssid);
        strcat(wifi_buffer, "|");
        strcat(wifi_buffer, g_wifi_profiles[i].password);
        strcat(wifi_buffer, "|");
        strcat(wifi_buffer, g_wifi_profiles[i].auto_connect ? "1" : "0");
        strcat(wifi_buffer, "|");
        strcat(wifi_buffer, g_wifi_profiles[i].remembered ? "1" : "0");
        strcat(wifi_buffer, "\n");
    }

    for (size_t i = 0; i < BLUETOOTH_DEVICE_MAX; ++i) {
        if (!g_bluetooth_devices[i].used) {
            continue;
        }
        strcat(bt_buffer, g_bluetooth_devices[i].name);
        strcat(bt_buffer, "|");
        strcat(bt_buffer, g_bluetooth_devices[i].kind);
        strcat(bt_buffer, "|");
        strcat(bt_buffer, g_bluetooth_devices[i].paired ? "1" : "0");
        strcat(bt_buffer, "\n");
    }

    fs_write("/home/.SNSX_WIFI.TXT", (const uint8_t *)wifi_buffer, strlen(wifi_buffer), ARM_FS_TEXT);
    fs_write("/home/.SNSX_BT.TXT", (const uint8_t *)bt_buffer, strlen(bt_buffer), ARM_FS_TEXT);
}

static void wireless_profiles_load(void) {
    arm_fs_entry_t *wifi_entry = fs_find("/home/.SNSX_WIFI.TXT");
    arm_fs_entry_t *bt_entry = fs_find("/home/.SNSX_BT.TXT");
    char buffer[1024];
    char *line;

    memset(g_wifi_profiles, 0, sizeof(g_wifi_profiles));
    memset(g_bluetooth_devices, 0, sizeof(g_bluetooth_devices));

    if (wifi_entry != NULL && wifi_entry->type == ARM_FS_TEXT) {
        size_t copy = wifi_entry->size < sizeof(buffer) - 1 ? wifi_entry->size : sizeof(buffer) - 1;
        memcpy(buffer, wifi_entry->data, copy);
        buffer[copy] = '\0';
        line = buffer;
        for (size_t index = 0; index < WIFI_PROFILE_MAX && line != NULL && *line != '\0'; ++index) {
            char *next = strchr(line, '\n');
            char *sep1;
            char *sep2;
            char *sep3;
            if (next != NULL) {
                *next++ = '\0';
            }
            sep1 = strchr(line, '|');
            sep2 = sep1 != NULL ? strchr(sep1 + 1, '|') : NULL;
            sep3 = sep2 != NULL ? strchr(sep2 + 1, '|') : NULL;
            if (sep1 != NULL && sep2 != NULL && sep3 != NULL) {
                *sep1++ = '\0';
                *sep2++ = '\0';
                *sep3++ = '\0';
                g_wifi_profiles[index].used = line[0] != '\0';
                strcopy_limit(g_wifi_profiles[index].ssid, line, sizeof(g_wifi_profiles[index].ssid));
                strcopy_limit(g_wifi_profiles[index].password, sep1, sizeof(g_wifi_profiles[index].password));
                g_wifi_profiles[index].auto_connect = atoi(sep2) != 0;
                g_wifi_profiles[index].remembered = atoi(sep3) != 0;
            }
            line = next;
        }
    }

    if (bt_entry != NULL && bt_entry->type == ARM_FS_TEXT) {
        size_t copy = bt_entry->size < sizeof(buffer) - 1 ? bt_entry->size : sizeof(buffer) - 1;
        memcpy(buffer, bt_entry->data, copy);
        buffer[copy] = '\0';
        line = buffer;
        for (size_t index = 0; index < BLUETOOTH_DEVICE_MAX && line != NULL && *line != '\0'; ++index) {
            char *next = strchr(line, '\n');
            char *sep1;
            char *sep2;
            if (next != NULL) {
                *next++ = '\0';
            }
            sep1 = strchr(line, '|');
            sep2 = sep1 != NULL ? strchr(sep1 + 1, '|') : NULL;
            if (sep1 != NULL && sep2 != NULL) {
                *sep1++ = '\0';
                *sep2++ = '\0';
                g_bluetooth_devices[index].used = line[0] != '\0';
                strcopy_limit(g_bluetooth_devices[index].name, line, sizeof(g_bluetooth_devices[index].name));
                strcopy_limit(g_bluetooth_devices[index].kind, sep1, sizeof(g_bluetooth_devices[index].kind));
                g_bluetooth_devices[index].paired = atoi(sep2) != 0;
            }
            line = next;
        }
    }
}

static size_t store_find_package_index(const char *id) {
    if (id == NULL || id[0] == '\0') {
        return STORE_PACKAGE_MAX;
    }
    for (size_t i = 0; i < ARRAY_SIZE(g_store_catalog); ++i) {
        if (strcmp(g_store_catalog[i].id, id) == 0) {
            return i;
        }
    }
    return STORE_PACKAGE_MAX;
}

static const char *store_status_label(size_t index) {
    if (index >= ARRAY_SIZE(g_store_catalog)) {
        return "UNKNOWN";
    }
    if (g_store_catalog[index].essential) {
        return "ESSENTIAL";
    }
    return g_store_installed[index] ? "INSTALLED" : "AVAILABLE";
}

static const char *store_asset_body(size_t index) {
    switch (index) {
        case STORE_PKG_BROWSER_SUITE:
            return "SNSX Browser Suite\n"
                   "==================\n"
                   "This package represents the built-in SNSX Browser app.\n"
                   "Routes: snsx://home, snsx://network, snsx://store, snsx://baremetal,\n"
                   "snsxdiag://network, and plain http:// URLs over the current SNSX stack.\n";
        case STORE_PKG_SNAKE_ARCADE:
            return "SNSX Snake Arcade\n"
                   "=================\n"
                   "Run snake from the desktop or shell.\n"
                   "Controls: Arrow keys or WASD, Space pause, R restart, ESC back.\n";
        case STORE_PKG_INTEL_82540EM:
            return "SNSX Intel 82540EM Driver\n"
                   "=========================\n"
                   "This is the native Ethernet driver path for the VirtualBox 82540EM NIC.\n"
                   "It backs the DHCP, IPv4, DNS, and plain HTTP foundation when firmware\n"
                   "network protocols are not exposed to SNSX.\n";
        case STORE_PKG_BAREMETAL_KIT:
            return "SNSX Bare-Metal Install Kit\n"
                   "===========================\n"
                   "Use /docs/BAREMETAL.TXT for the full USB and internal-disk procedure.\n"
                   "This kit prepares the file layout and guidance for making SNSX the\n"
                   "primary UEFI OS on supported ARM64 hardware.\n";
        case STORE_PKG_WIFI_DRIVER_LAB:
            return "SNSX Wi-Fi Driver Lab\n"
                   "=====================\n"
                   "This package seeds the development notes and layout for future real\n"
                   "Wi-Fi drivers. Nearby SSID discovery still needs chipset-specific code.\n";
        case STORE_PKG_BLUETOOTH_DRIVER_LAB:
            return "SNSX Bluetooth Driver Lab\n"
                   "=========================\n"
                   "This package seeds the development notes and layout for future real\n"
                   "Bluetooth controller support, pairing, and nearby device discovery.\n";
        case STORE_PKG_PRINTER_DRIVER_KIT:
            return "SNSX Printer Driver Kit\n"
                   "=======================\n"
                   "This package outlines the SNSX spooler and driver hooks for USB and\n"
                   "network printers on future hardware-backed builds.\n";
        case STORE_PKG_DEV_STARTER:
            return "SNSX Developer Starter\n"
                   "======================\n"
                   "This package seeds SNSX Code Studio, the host-backed dev bridge, and\n"
                   "example workspaces for SNSX, Python, JavaScript, C, C++, HTML, and CSS.\n";
        default:
            return NULL;
    }
}

static void store_sync_files(void) {
    fs_mkdir("/apps");
    fs_mkdir("/drivers");
    fs_mkdir("/games");
    fs_mkdir("/kits");

    for (size_t i = 0; i < ARRAY_SIZE(g_store_catalog); ++i) {
        const char *path = g_store_catalog[i].asset_path;
        const char *body = store_asset_body(i);
        arm_fs_entry_t *entry;

        if (path == NULL || body == NULL) {
            continue;
        }
        entry = fs_find(path);
        if (g_store_installed[i]) {
            fs_write_if_changed(path, (const uint8_t *)body, strlen(body), ARM_FS_TEXT);
        } else if (entry != NULL) {
            fs_remove(path);
        }
    }
}

static void store_save_state(void) {
    char buffer[1024];

    buffer[0] = '\0';
    for (size_t i = 0; i < ARRAY_SIZE(g_store_catalog); ++i) {
        strcat(buffer, g_store_catalog[i].id);
        strcat(buffer, "|");
        strcat(buffer, g_store_installed[i] ? "1" : "0");
        strcat(buffer, "\n");
    }
    fs_write_if_changed("/home/.SNSX_STORE.TXT", (const uint8_t *)buffer, strlen(buffer), ARM_FS_TEXT);
}

static void store_load_state(void) {
    arm_fs_entry_t *entry = fs_find("/home/.SNSX_STORE.TXT");
    char buffer[1024];
    bool save_defaults = false;
    char *line;

    memset(g_store_installed, 0, sizeof(g_store_installed));
    for (size_t i = 0; i < ARRAY_SIZE(g_store_catalog); ++i) {
        g_store_installed[i] = g_store_catalog[i].installed_by_default;
    }

    if (entry != NULL && entry->type == ARM_FS_TEXT) {
        size_t copy = entry->size < sizeof(buffer) - 1 ? entry->size : sizeof(buffer) - 1;
        memcpy(buffer, entry->data, copy);
        buffer[copy] = '\0';
        line = buffer;
        while (line != NULL && *line != '\0') {
            char *next = strchr(line, '\n');
            char *sep = strchr(line, '|');
            if (next != NULL) {
                *next++ = '\0';
            }
            if (sep != NULL) {
                size_t index;
                *sep++ = '\0';
                index = store_find_package_index(line);
                if (index < ARRAY_SIZE(g_store_catalog) && !g_store_catalog[index].essential) {
                    g_store_installed[index] = atoi(sep) != 0;
                }
            }
            line = next;
        }
    } else {
        save_defaults = true;
    }

    for (size_t i = 0; i < ARRAY_SIZE(g_store_catalog); ++i) {
        if (g_store_catalog[i].essential) {
            g_store_installed[i] = true;
        }
    }

    store_sync_files();
    if (save_defaults) {
        store_save_state();
    }
}

static void store_commit_state(void) {
    store_sync_files();
    store_save_state();
    if (g_desktop.autosave && g_persist.available) {
        g_persist.dirty = true;
        persist_flush_state();
    }
}

static bool store_install_package_index(size_t index, char *status, size_t status_size) {
    if (index >= ARRAY_SIZE(g_store_catalog)) {
        strcopy_limit(status, "Package not found.", status_size);
        return false;
    }
    if (g_store_installed[index]) {
        strcopy_limit(status, "Package already installed.", status_size);
        return true;
    }
    g_store_installed[index] = true;
    store_commit_state();
    strcopy_limit(status, "Installed ", status_size);
    strcat(status, g_store_catalog[index].name);
    strcat(status, ".");
    return true;
}

static bool store_remove_package_index(size_t index, char *status, size_t status_size) {
    if (index >= ARRAY_SIZE(g_store_catalog)) {
        strcopy_limit(status, "Package not found.", status_size);
        return false;
    }
    if (g_store_catalog[index].essential) {
        strcopy_limit(status, "Core SNSX packages cannot be removed.", status_size);
        return false;
    }
    if (!g_store_installed[index]) {
        strcopy_limit(status, "Package is not installed.", status_size);
        return false;
    }
    g_store_installed[index] = false;
    store_commit_state();
    strcopy_limit(status, "Removed ", status_size);
    strcat(status, g_store_catalog[index].name);
    strcat(status, ".");
    return true;
}

static bool store_install_package_by_id(const char *id, char *status, size_t status_size) {
    return store_install_package_index(store_find_package_index(id), status, status_size);
}

static bool store_remove_package_by_id(const char *id, char *status, size_t status_size) {
    return store_remove_package_index(store_find_package_index(id), status, status_size);
}

static void command_store_list(void) {
    console_writeline("SNSX App Store catalog:");
    for (size_t i = 0; i < ARRAY_SIZE(g_store_catalog); ++i) {
        console_write(g_store_catalog[i].id);
        console_write("  ");
        console_write("[");
        console_write(store_status_label(i));
        console_write("] ");
        console_write(g_store_catalog[i].category);
        console_write("  ");
        console_writeline(g_store_catalog[i].name);
    }
}

static void desktop_save_config(void) {
    char buffer[640];
    char number[16];

    buffer[0] = '\0';
    strcat(buffer, "THEME=");
    u32_to_dec(g_desktop.theme, number);
    strcat(buffer, number);
    strcat(buffer, "\nWELCOME=");
    strcat(buffer, g_desktop.welcome_seen ? "1" : "0");
    strcat(buffer, "\nAUTOSAVE=");
    strcat(buffer, g_desktop.autosave ? "1" : "0");
    strcat(buffer, "\nPOINTERTRAIL=");
    strcat(buffer, g_desktop.pointer_trail ? "1" : "0");
    strcat(buffer, "\nWIFIEN=");
    strcat(buffer, g_desktop.wifi_enabled ? "1" : "0");
    strcat(buffer, "\nBTEN=");
    strcat(buffer, g_desktop.bluetooth_enabled ? "1" : "0");
    strcat(buffer, "\nNETAUTOCONNECT=");
    strcat(buffer, g_desktop.network_autoconnect ? "1" : "0");
    strcat(buffer, "\nLAUNCHES=");
    u32_to_dec(g_desktop.launch_count, number);
    strcat(buffer, number);
    strcat(buffer, "\nPOINTERSPEED=");
    u32_to_dec(g_desktop.pointer_speed, number);
    strcat(buffer, number);
    strcat(buffer, "\nNETMODE=");
    u32_to_dec(g_desktop.network_preference, number);
    strcat(buffer, number);
    strcat(buffer, "\nLASTAPP=");
    strcat(buffer, g_desktop.last_app);
    strcat(buffer, "\nHOSTNAME=");
    strcat(buffer, g_desktop.hostname);
    strcat(buffer, "\nWIFISSID=");
    strcat(buffer, g_desktop.wifi_ssid);
    strcat(buffer, "\nHOTSPOT=");
    strcat(buffer, g_desktop.hotspot_name);
    strcat(buffer, "\n");

    fs_write("/home/.SNSX_DESKTOP.TXT", (const uint8_t *)buffer, strlen(buffer), ARM_FS_TEXT);
}

static void desktop_load_config(void) {
    arm_fs_entry_t *entry = fs_find("/home/.SNSX_DESKTOP.TXT");
    char buffer[768];
    char *line;

    desktop_set_default_preferences();

    if (entry != NULL && entry->type == ARM_FS_TEXT) {
        size_t copy = entry->size < sizeof(buffer) - 1 ? entry->size : sizeof(buffer) - 1;
        memcpy(buffer, entry->data, copy);
        buffer[copy] = '\0';
        line = buffer;
        while (line != NULL && *line != '\0') {
            char *next = strchr(line, '\n');
            if (next != NULL) {
                *next++ = '\0';
            }
            if (strncmp(line, "THEME=", 6) == 0) {
                int index = atoi(line + 6);
                if (index >= 0 && (size_t)index < ARRAY_SIZE(g_theme_names)) {
                    g_desktop.theme = (uint32_t)index;
                }
            } else if (strncmp(line, "WELCOME=", 8) == 0) {
                g_desktop.welcome_seen = atoi(line + 8) != 0;
            } else if (strncmp(line, "AUTOSAVE=", 9) == 0) {
                g_desktop.autosave = atoi(line + 9) != 0;
            } else if (strncmp(line, "POINTERTRAIL=", 13) == 0) {
                g_desktop.pointer_trail = atoi(line + 13) != 0;
            } else if (strncmp(line, "WIFIEN=", 7) == 0) {
                g_desktop.wifi_enabled = atoi(line + 7) != 0;
            } else if (strncmp(line, "BTEN=", 5) == 0) {
                g_desktop.bluetooth_enabled = atoi(line + 5) != 0;
            } else if (strncmp(line, "NETAUTOCONNECT=", 15) == 0) {
                g_desktop.network_autoconnect = atoi(line + 15) != 0;
            } else if (strncmp(line, "LAUNCHES=", 9) == 0) {
                g_desktop.launch_count = (uint32_t)atoi(line + 9);
            } else if (strncmp(line, "POINTERSPEED=", 13) == 0) {
                int speed = atoi(line + 13);
                if (speed >= 1 && speed <= 5) {
                    g_desktop.pointer_speed = (uint32_t)speed;
                }
            } else if (strncmp(line, "NETMODE=", 8) == 0) {
                int mode = atoi(line + 8);
                if (mode >= (int)NETWORK_PREF_AUTO && mode <= (int)NETWORK_PREF_HOTSPOT) {
                    g_desktop.network_preference = (uint32_t)mode;
                }
            } else if (strncmp(line, "LASTAPP=", 8) == 0) {
                strcopy_limit(g_desktop.last_app, line + 8, sizeof(g_desktop.last_app));
            } else if (strncmp(line, "HOSTNAME=", 9) == 0) {
                strcopy_limit(g_desktop.hostname, line + 9, sizeof(g_desktop.hostname));
            } else if (strncmp(line, "WIFISSID=", 9) == 0) {
                strcopy_limit(g_desktop.wifi_ssid, line + 9, sizeof(g_desktop.wifi_ssid));
            } else if (strncmp(line, "HOTSPOT=", 8) == 0) {
                strcopy_limit(g_desktop.hotspot_name, line + 8, sizeof(g_desktop.hotspot_name));
            }
            line = next;
        }
    }

    g_desktop.launch_count++;
    desktop_save_config();
}

static void browser_set_default_state(void) {
    memset(&g_browser, 0, sizeof(g_browser));
    g_browser.tab_count = 4;
    g_browser.active_tab = 0;
    strcopy_limit(g_browser.tabs[0], "snsx://home", sizeof(g_browser.tabs[0]));
    strcopy_limit(g_browser.tabs[1], "snsxnet://status", sizeof(g_browser.tabs[1]));
    strcopy_limit(g_browser.tabs[2], "snsx://store", sizeof(g_browser.tabs[2]));
    strcopy_limit(g_browser.tabs[3], "https://example.com/", sizeof(g_browser.tabs[3]));
    g_browser.bookmark_count = 6;
    strcopy_limit(g_browser.bookmarks[0], "snsx://home", sizeof(g_browser.bookmarks[0]));
    strcopy_limit(g_browser.bookmarks[1], "snsxnet://status", sizeof(g_browser.bookmarks[1]));
    strcopy_limit(g_browser.bookmarks[2], "snsx://store", sizeof(g_browser.bookmarks[2]));
    strcopy_limit(g_browser.bookmarks[3], "snsx://browser", sizeof(g_browser.bookmarks[3]));
    strcopy_limit(g_browser.bookmarks[4], "http://example.com/", sizeof(g_browser.bookmarks[4]));
    strcopy_limit(g_browser.bookmarks[5], "https://example.com/", sizeof(g_browser.bookmarks[5]));
    strcopy_limit(g_browser_address, g_browser.tabs[g_browser.active_tab], sizeof(g_browser_address));
}

static void browser_state_save(void) {
    char buffer[4096];

    buffer[0] = '\0';
    strcat(buffer, "ACTIVE=");
    {
        char number[16];
        u32_to_dec((uint32_t)g_browser.active_tab, number);
        strcat(buffer, number);
        strcat(buffer, "\n");
    }
    for (size_t i = 0; i < g_browser.tab_count; ++i) {
        strcat(buffer, "TAB=");
        strcat(buffer, g_browser.tabs[i]);
        strcat(buffer, "\n");
    }
    for (size_t i = 0; i < g_browser.bookmark_count; ++i) {
        strcat(buffer, "BOOKMARK=");
        strcat(buffer, g_browser.bookmarks[i]);
        strcat(buffer, "\n");
    }
    for (size_t i = 0; i < g_browser.history_count; ++i) {
        strcat(buffer, "HISTORY=");
        strcat(buffer, g_browser.history[i]);
        strcat(buffer, "\n");
    }
    for (size_t i = 0; i < g_browser.download_count; ++i) {
        strcat(buffer, "DOWNLOAD=");
        strcat(buffer, g_browser.downloads[i]);
        strcat(buffer, "\n");
    }
    fs_write_if_changed("/home/.SNSX_BROWSER.TXT", (const uint8_t *)buffer, strlen(buffer), ARM_FS_TEXT);
}

static void browser_state_load(void) {
    arm_fs_entry_t *entry = fs_find("/home/.SNSX_BROWSER.TXT");
    char buffer[4096];
    char *line;
    bool save_defaults = false;

    browser_set_default_state();
    if (entry == NULL || entry->type != ARM_FS_TEXT) {
        browser_state_save();
        return;
    }

    {
        size_t copy = entry->size < sizeof(buffer) - 1 ? entry->size : sizeof(buffer) - 1;
        memcpy(buffer, entry->data, copy);
        buffer[copy] = '\0';
    }
    memset(&g_browser, 0, sizeof(g_browser));
    line = buffer;
    while (line != NULL && *line != '\0') {
        char *next = strchr(line, '\n');
        if (next != NULL) {
            *next++ = '\0';
        }
        if (strncmp(line, "ACTIVE=", 7) == 0) {
            g_browser.active_tab = (size_t)atoi(line + 7);
        } else if (strncmp(line, "TAB=", 4) == 0) {
            if (g_browser.tab_count < BROWSER_TAB_MAX) {
                strcopy_limit(g_browser.tabs[g_browser.tab_count++], line + 4, BROWSER_ADDRESS_MAX);
            }
        } else if (strncmp(line, "BOOKMARK=", 9) == 0) {
            if (g_browser.bookmark_count < BROWSER_BOOKMARK_MAX) {
                strcopy_limit(g_browser.bookmarks[g_browser.bookmark_count++], line + 9, BROWSER_ADDRESS_MAX);
            }
        } else if (strncmp(line, "HISTORY=", 8) == 0) {
            if (g_browser.history_count < BROWSER_HISTORY_MAX) {
                strcopy_limit(g_browser.history[g_browser.history_count++], line + 8, BROWSER_ADDRESS_MAX);
            }
        } else if (strncmp(line, "DOWNLOAD=", 9) == 0) {
            if (g_browser.download_count < BROWSER_DOWNLOAD_MAX) {
                strcopy_limit(g_browser.downloads[g_browser.download_count++], line + 9, FS_PATH_MAX);
            }
        }
        line = next;
    }

    if (g_browser.tab_count == 0) {
        browser_set_default_state();
        save_defaults = true;
    }
    if (g_browser.active_tab >= g_browser.tab_count) {
        g_browser.active_tab = 0;
    }
    strcopy_limit(g_browser_address, g_browser.tabs[g_browser.active_tab], sizeof(g_browser_address));
    if (save_defaults) {
        browser_state_save();
    }
}

static void browser_record_visit(const char *address) {
    if (address == NULL || address[0] == '\0') {
        return;
    }
    if (g_browser.history_count > 0 && strcmp(g_browser.history[0], address) == 0) {
        return;
    }
    for (size_t i = g_browser.history_count; i > 0; --i) {
        if (i < BROWSER_HISTORY_MAX) {
            strcopy_limit(g_browser.history[i], g_browser.history[i - 1], sizeof(g_browser.history[i]));
        }
    }
    strcopy_limit(g_browser.history[0], address, sizeof(g_browser.history[0]));
    if (g_browser.history_count < BROWSER_HISTORY_MAX) {
        g_browser.history_count++;
    }
    browser_state_save();
}

static bool browser_toggle_bookmark(const char *address, bool *added) {
    if (added != NULL) {
        *added = false;
    }
    if (address == NULL || address[0] == '\0') {
        return false;
    }
    for (size_t i = 0; i < g_browser.bookmark_count; ++i) {
        if (strcmp(g_browser.bookmarks[i], address) == 0) {
            for (size_t j = i; j + 1 < g_browser.bookmark_count; ++j) {
                strcopy_limit(g_browser.bookmarks[j], g_browser.bookmarks[j + 1], sizeof(g_browser.bookmarks[j]));
            }
            if (g_browser.bookmark_count > 0) {
                g_browser.bookmark_count--;
            }
            browser_state_save();
            return true;
        }
    }
    if (g_browser.bookmark_count >= BROWSER_BOOKMARK_MAX) {
        for (size_t i = BROWSER_BOOKMARK_MAX - 1; i > 0; --i) {
            strcopy_limit(g_browser.bookmarks[i], g_browser.bookmarks[i - 1], sizeof(g_browser.bookmarks[i]));
        }
        strcopy_limit(g_browser.bookmarks[0], address, sizeof(g_browser.bookmarks[0]));
    } else {
        for (size_t i = g_browser.bookmark_count; i > 0; --i) {
            strcopy_limit(g_browser.bookmarks[i], g_browser.bookmarks[i - 1], sizeof(g_browser.bookmarks[i]));
        }
        strcopy_limit(g_browser.bookmarks[0], address, sizeof(g_browser.bookmarks[0]));
        g_browser.bookmark_count++;
    }
    if (added != NULL) {
        *added = true;
    }
    browser_state_save();
    return true;
}

static bool browser_save_page_snapshot(const char *address, const char *page, char *saved_path, size_t saved_path_size) {
    char path[FS_PATH_MAX];
    char number[16];
    size_t index;

    fs_mkdir("/downloads");
    index = g_browser.download_count + 1;
    for (;;) {
        strcopy_limit(path, "/downloads/PAGE-", sizeof(path));
        u32_to_dec((uint32_t)index, number);
        strcat(path, number);
        strcat(path, ".TXT");
        if (fs_find(path) == NULL) {
            break;
        }
        index++;
    }
    if (!fs_write(path, (const uint8_t *)(page != NULL ? page : ""), strlen(page != NULL ? page : ""), ARM_FS_TEXT)) {
        return false;
    }
    if (g_browser.download_count < BROWSER_DOWNLOAD_MAX) {
        strcopy_limit(g_browser.downloads[g_browser.download_count++], path, sizeof(g_browser.downloads[0]));
    } else {
        for (size_t i = 1; i < BROWSER_DOWNLOAD_MAX; ++i) {
            strcopy_limit(g_browser.downloads[i - 1], g_browser.downloads[i], sizeof(g_browser.downloads[i - 1]));
        }
        strcopy_limit(g_browser.downloads[BROWSER_DOWNLOAD_MAX - 1], path, sizeof(g_browser.downloads[BROWSER_DOWNLOAD_MAX - 1]));
    }
    if (saved_path != NULL && saved_path_size > 0) {
        strcopy_limit(saved_path, path, saved_path_size);
    }
    (void)address;
    browser_state_save();
    persist_flush_state();
    return true;
}

static void browser_links_reset(void) {
    g_browser_link_count = 0;
    memset(g_browser_links, 0, sizeof(g_browser_links));
}

static void browser_links_add(const char *address) {
    if (address == NULL || address[0] == '\0') {
        return;
    }
    for (size_t i = 0; i < g_browser_link_count; ++i) {
        if (strcmp(g_browser_links[i], address) == 0) {
            return;
        }
    }
    if (g_browser_link_count >= BROWSER_LINK_MAX) {
        return;
    }
    strcopy_limit(g_browser_links[g_browser_link_count++], address, sizeof(g_browser_links[0]));
}

static bool browser_matches_protocol(const char *text, const char *protocol) {
    size_t len = strlen(protocol);
    return strncmp(text, protocol, len) == 0;
}

static void browser_extract_text_links(const char *text) {
    static const char *protocols[] = {
        "https://",
        "http://",
        "snsx://",
        "snsxnet://",
        "snsxdiag://",
        "snsx-id://"
    };

    if (text == NULL) {
        return;
    }

    while (*text != '\0') {
        for (size_t i = 0; i < ARRAY_SIZE(protocols); ++i) {
            const char *protocol = protocols[i];
            if (browser_matches_protocol(text, protocol)) {
                char link[BROWSER_ADDRESS_MAX];
                size_t len = 0;
                while (text[len] != '\0' && text[len] != '\n' && text[len] != '\r' &&
                       text[len] != ' ' && text[len] != '\t' && text[len] != ')' &&
                       text[len] != ']' && text[len] != '"' && text[len] != '\'' &&
                       text[len] != ',' && text[len] != ';' && text[len] != '|' &&
                       len + 1 < sizeof(link)) {
                    link[len] = text[len];
                    ++len;
                }
                while (len > 0 && (link[len - 1] == '.' || link[len - 1] == ':' || link[len - 1] == '>')) {
                    --len;
                }
                link[len] = '\0';
                browser_links_add(link);
            }
        }
        ++text;
    }
}

static void browser_extract_html_links(const char *html) {
    if (html == NULL) {
        return;
    }

    while (*html != '\0') {
        const char *href = netstack_find_text(html, "href=");
        if (href == NULL) {
            break;
        }
        href += 5;
        if (*href == '"' || *href == '\'') {
            char quote = *href++;
            char link[BROWSER_ADDRESS_MAX];
            size_t len = 0;
            while (*href != '\0' && *href != quote && len + 1 < sizeof(link)) {
                link[len++] = *href++;
            }
            link[len] = '\0';
            if (len > 0) {
                browser_links_add(link);
            }
            html = href;
            continue;
        }
        html = href;
    }
}

static void browser_append_links_summary(char *page, size_t page_size) {
    if (page == NULL || page_size == 0 || g_browser_link_count == 0) {
        return;
    }
    if (strlen(page) + 16 >= page_size) {
        return;
    }
    strcat(page, "\n\nLinks\n-----\n");
    for (size_t i = 0; i < g_browser_link_count; ++i) {
        char number[8];
        u32_to_dec((uint32_t)(i + 1), number);
        if (strlen(page) + strlen(g_browser_links[i]) + 16 >= page_size) {
            break;
        }
        strcat(page, "[");
        strcat(page, number);
        strcat(page, "] ");
        strcat(page, g_browser_links[i]);
        strcat(page, "\n");
    }
}

static bool browser_open_link_index(size_t index) {
    if (index >= g_browser_link_count) {
        return false;
    }
    browser_navigate_to(g_browser_links[index]);
    return true;
}

static void browser_compose_link_label(size_t index, char *label, size_t label_size) {
    char number[8];
    size_t used;

    if (label == NULL || label_size == 0) {
        return;
    }
    label[0] = '\0';
    if (index >= g_browser_link_count) {
        strcopy_limit(label, "LINK", label_size);
        return;
    }

    u32_to_dec((uint32_t)(index + 1), number);
    strcopy_limit(label, "[", label_size);
    strcat(label, number);
    strcat(label, "] ");
    used = strlen(label);

    if (used + strlen(g_browser_links[index]) + 1 < label_size) {
        strcat(label, g_browser_links[index]);
        return;
    }

    if (label_size <= used + 4) {
        return;
    }

    {
        size_t remaining = label_size - used - 4;
        memcpy(label + used, g_browser_links[index], remaining);
        label[used + remaining] = '\0';
        strcat(label, "...");
    }
}

static void browser_finalize_page(char *page, size_t page_size) {
    if (page == NULL || page[0] == '\0') {
        return;
    }
    if (g_browser_link_count == 0) {
        browser_extract_text_links(page);
    }
    if (g_browser_link_count > 0 && netstack_find_text(page, "\n\nLinks\n-----\n") == NULL) {
        browser_append_links_summary(page, page_size);
    }
}

static const char *studio_language_for_path(const char *path) {
    const char *dot = strrchr(path != NULL ? path : "", '.');
    if (dot == NULL) {
        return "text";
    }
    if (strcmp(dot, ".py") == 0) {
        return "python";
    }
    if (strcmp(dot, ".js") == 0) {
        return "javascript";
    }
    if (strcmp(dot, ".c") == 0) {
        return "c";
    }
    if (strcmp(dot, ".cpp") == 0 || strcmp(dot, ".cc") == 0 || strcmp(dot, ".cxx") == 0) {
        return "cpp";
    }
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) {
        return "html";
    }
    if (strcmp(dot, ".css") == 0) {
        return "css";
    }
    if (strcmp(dot, ".snsx") == 0) {
        return "snsx";
    }
    return "text";
}

static void studio_state_save(void) {
    char buffer[512];

    buffer[0] = '\0';
    strcat(buffer, "PATH=");
    strcat(buffer, g_studio.active_path);
    strcat(buffer, "\nLANG=");
    strcat(buffer, g_studio.language);
    strcat(buffer, "\nACTION=");
    strcat(buffer, g_studio.last_action);
    strcat(buffer, "\n");
    fs_write_if_changed("/home/.SNSX_STUDIO.TXT", (const uint8_t *)buffer, strlen(buffer), ARM_FS_TEXT);
}

static void studio_state_load(void) {
    arm_fs_entry_t *entry = fs_find("/home/.SNSX_STUDIO.TXT");
    char buffer[512];
    char *line;

    strcopy_limit(g_studio.active_path, "/projects/main.snsx", sizeof(g_studio.active_path));
    strcopy_limit(g_studio.language, "snsx", sizeof(g_studio.language));
    strcopy_limit(g_studio.status, "SNSX Code Studio is ready.", sizeof(g_studio.status));
    strcopy_limit(g_studio.last_action, "idle", sizeof(g_studio.last_action));
    strcopy_limit(g_studio.output,
                  "Use EDIT to change the current file, RUN to execute it through the host bridge, and BUILD for compiled targets.\n",
                  sizeof(g_studio.output));
    if (entry == NULL || entry->type != ARM_FS_TEXT) {
        studio_state_save();
        return;
    }
    {
        size_t copy = entry->size < sizeof(buffer) - 1 ? entry->size : sizeof(buffer) - 1;
        memcpy(buffer, entry->data, copy);
        buffer[copy] = '\0';
    }
    line = buffer;
    while (line != NULL && *line != '\0') {
        char *next = strchr(line, '\n');
        if (next != NULL) {
            *next++ = '\0';
        }
        if (strncmp(line, "PATH=", 5) == 0) {
            strcopy_limit(g_studio.active_path, line + 5, sizeof(g_studio.active_path));
        } else if (strncmp(line, "LANG=", 5) == 0) {
            strcopy_limit(g_studio.language, line + 5, sizeof(g_studio.language));
        } else if (strncmp(line, "ACTION=", 7) == 0) {
            strcopy_limit(g_studio.last_action, line + 7, sizeof(g_studio.last_action));
        }
        line = next;
    }
}

static void studio_seed_workspace(void) {
    static const char snsx_main[] =
        "bring <module/std.io> as io\n"
        "bring <module/std.ai> as ai\n"
        "\n"
        "mind summarize\n"
        "    takes text: String\n"
        "    gives String\n"
        "    from \"Summarize this text\"\n"
        "\n"
        "entry\n"
        "    show \"hello from SNSX Code Studio\"\n"
        "    hold summary = \"Edit this file, then run it from the Studio app.\" |> summarize\n"
        "    show summary\n"
        "    0\n";
    static const char python_main[] =
        "print('hello from SNSX Python bridge')\n";
    static const char js_main[] =
        "console.log('hello from SNSX JavaScript bridge');\n";
    static const char c_main[] =
        "#include <stdio.h>\n"
        "int main(void) {\n"
        "    puts(\"hello from SNSX C bridge\");\n"
        "    return 0;\n"
        "}\n";
    static const char cpp_main[] =
        "#include <iostream>\n"
        "int main() {\n"
        "    std::cout << \"hello from SNSX C++ bridge\" << std::endl;\n"
        "    return 0;\n"
        "}\n";
    static const char html_main[] =
        "<!doctype html>\n"
        "<html><head><title>SNSX Preview</title></head><body><h1>SNSX HTML Workspace</h1><p>Edit this file in Code Studio.</p></body></html>\n";
    static const char css_main[] =
        "body { font-family: sans-serif; background: #eef3f8; color: #223; }\n"
        "h1 { color: #0b5bd3; }\n";

    fs_mkdir("/projects");
    fs_mkdir("/downloads");
    fs_write_if_changed("/projects/main.snsx", (const uint8_t *)snsx_main, sizeof(snsx_main) - 1, ARM_FS_TEXT);
    fs_write_if_changed("/projects/main.py", (const uint8_t *)python_main, sizeof(python_main) - 1, ARM_FS_TEXT);
    fs_write_if_changed("/projects/main.js", (const uint8_t *)js_main, sizeof(js_main) - 1, ARM_FS_TEXT);
    fs_write_if_changed("/projects/main.c", (const uint8_t *)c_main, sizeof(c_main) - 1, ARM_FS_TEXT);
    fs_write_if_changed("/projects/main.cpp", (const uint8_t *)cpp_main, sizeof(cpp_main) - 1, ARM_FS_TEXT);
    fs_write_if_changed("/projects/index.html", (const uint8_t *)html_main, sizeof(html_main) - 1, ARM_FS_TEXT);
    fs_write_if_changed("/projects/style.css", (const uint8_t *)css_main, sizeof(css_main) - 1, ARM_FS_TEXT);
}

static bool browser_is_bookmarked(const char *address) {
    if (address == NULL || address[0] == '\0') {
        return false;
    }
    for (size_t i = 0; i < g_browser.bookmark_count; ++i) {
        if (strcmp(g_browser.bookmarks[i], address) == 0) {
            return true;
        }
    }
    return false;
}

static void browser_sync_active_tab(void) {
    if (g_browser.tab_count == 0) {
        browser_set_default_state();
    }
    if (g_browser.active_tab >= g_browser.tab_count) {
        g_browser.active_tab = 0;
    }
    strcopy_limit(g_browser.tabs[g_browser.active_tab], g_browser_address, sizeof(g_browser.tabs[g_browser.active_tab]));
    browser_state_save();
}

static void browser_activate_tab(size_t index) {
    if (index >= g_browser.tab_count) {
        return;
    }
    g_browser.active_tab = index;
    strcopy_limit(g_browser_address, g_browser.tabs[index], sizeof(g_browser_address));
    browser_record_visit(g_browser_address);
    browser_state_save();
}

static void browser_navigate_to(const char *address) {
    if (address == NULL || address[0] == '\0') {
        return;
    }
    strcopy_limit(g_browser_address, address, sizeof(g_browser_address));
    browser_sync_active_tab();
    browser_record_visit(g_browser_address);
}

static void browser_open_tab(const char *address) {
    if (address == NULL || address[0] == '\0') {
        return;
    }
    if (g_browser.tab_count < BROWSER_TAB_MAX) {
        strcopy_limit(g_browser.tabs[g_browser.tab_count], address, sizeof(g_browser.tabs[g_browser.tab_count]));
        g_browser.active_tab = g_browser.tab_count;
        g_browser.tab_count++;
        strcopy_limit(g_browser_address, address, sizeof(g_browser_address));
        browser_record_visit(g_browser_address);
        browser_state_save();
        return;
    }
    browser_navigate_to(address);
}

static void studio_select_path(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return;
    }
    strcopy_limit(g_studio.active_path, path, sizeof(g_studio.active_path));
    strcopy_limit(g_studio.language, studio_language_for_path(path), sizeof(g_studio.language));
    strcopy_limit(g_studio.status, "Selected the active workspace file.", sizeof(g_studio.status));
    studio_state_save();
}

static void studio_resolve_input_path(const char *input, char *path) {
    if (input == NULL || input[0] == '\0') {
        strcopy_limit(path, g_studio.active_path, FS_PATH_MAX);
        return;
    }
    if (input[0] == '/') {
        strcopy_limit(path, input, FS_PATH_MAX);
        return;
    }
    strcopy_limit(path, "/projects/", FS_PATH_MAX);
    strcat(path, input);
}

static void studio_ensure_path_exists(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return;
    }
    fs_mkdir("/projects");
    if (fs_find(path) == NULL) {
        fs_write(path, (const uint8_t *)"", 0, ARM_FS_TEXT);
    }
}

static bool studio_collect_source(const char *path, const uint8_t **data, size_t *size) {
    arm_fs_entry_t *entry = fs_find(path);
    if (entry == NULL || entry->type != ARM_FS_TEXT) {
        strcopy_limit(g_studio.status, "Studio could not find that source file.", sizeof(g_studio.status));
        strcopy_limit(g_studio.output, "The selected workspace file does not exist yet.\n", sizeof(g_studio.output));
        return false;
    }
    if (data != NULL) {
        *data = entry->data;
    }
    if (size != NULL) {
        *size = entry->size;
    }
    return true;
}

static bool studio_execute_bridge(const char *action) {
    const uint8_t *source = NULL;
    size_t source_len = 0;
    char route[NET_URL_MAX];
    char url[NET_URL_MAX];

    if (action == NULL || action[0] == '\0') {
        return false;
    }
    if (!studio_collect_source(g_studio.active_path, &source, &source_len)) {
        studio_state_save();
        return false;
    }

    strcopy_limit(route, action, sizeof(route));
    strcat(route, "?lang=");
    strcat(route, g_studio.language);
    strcat(route, "&path=");
    strcat(route, g_studio.active_path);

    if (!netstack_build_dev_bridge_url(route, url, sizeof(url))) {
        strcopy_limit(g_studio.status, "Dev bridge request URL is too long.", sizeof(g_studio.status));
        strcopy_limit(g_studio.output, "SNSX could not build the host bridge request URL.\n", sizeof(g_studio.output));
        studio_state_save();
        return false;
    }
    if (!netstack_http_request_text("POST", url, "text/plain; charset=utf-8", source, source_len,
                                    g_studio.output, sizeof(g_studio.output))) {
        if (g_studio.output[0] == '\0') {
            strcopy_limit(g_studio.output,
                          "SNSX could not reach the host dev bridge.\nRun `make dev-bridge` on the host Mac and try again.\n",
                          sizeof(g_studio.output));
        }
        strcopy_limit(g_studio.status, "The host dev bridge is offline or returned an error.", sizeof(g_studio.status));
        strcopy_limit(g_studio.last_action, action, sizeof(g_studio.last_action));
        studio_state_save();
        return false;
    }

    if (strcmp(action, "run") == 0) {
        strcopy_limit(g_studio.status, "Run completed through the host dev bridge.", sizeof(g_studio.status));
    } else {
        strcopy_limit(g_studio.status, "Build completed through the host dev bridge.", sizeof(g_studio.status));
    }
    strcopy_limit(g_studio.last_action, action, sizeof(g_studio.last_action));
    studio_state_save();
    return true;
}

static void shell_print_entry(arm_fs_entry_t *entry) {
    const char *name = path_basename(entry->path);
    if (entry->type == ARM_FS_DIR) {
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_write(name);
        console_write("/");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_writeline("  <dir>");
    } else {
        console_write(name);
        console_write("  ");
        console_write_dec((uint32_t)entry->size);
        console_writeline(" bytes  text");
    }
}

static void view_text(const char *path) {
    arm_fs_entry_t *entry = fs_find(path);
    console_clear();
    if (entry == NULL) {
        console_writeline("Path not found.");
        wait_for_enter(NULL);
        return;
    }
    console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
    console_write("SNSX Viewer: ");
    console_set_color(EFI_WHITE, EFI_BLACK);
    console_writeline(path);
    console_writeline("");
    if (entry->type == ARM_FS_DIR) {
        console_writeline("This path is a directory.");
    } else {
        for (size_t i = 0; i < entry->size; ++i) {
            console_putchar((char)entry->data[i]);
        }
        if (entry->size == 0 || entry->data[entry->size - 1] != '\n') {
            console_putchar('\n');
        }
    }
    wait_for_enter(NULL);
}

static int32_t calc_parse_expr(calc_state_t *state);

static void calc_skip(calc_state_t *state) {
    while (isspace(*state->cursor)) {
        ++state->cursor;
    }
}

static int32_t calc_parse_factor(calc_state_t *state) {
    int32_t value = 0;
    bool negative = false;
    calc_skip(state);
    while (*state->cursor == '+' || *state->cursor == '-') {
        if (*state->cursor == '-') {
            negative = !negative;
        }
        ++state->cursor;
        calc_skip(state);
    }
    if (*state->cursor == '(') {
        ++state->cursor;
        value = calc_parse_expr(state);
        calc_skip(state);
        if (*state->cursor != ')') {
            state->ok = false;
            return 0;
        }
        ++state->cursor;
        return negative ? -value : value;
    }
    if (*state->cursor < '0' || *state->cursor > '9') {
        state->ok = false;
        return 0;
    }
    while (*state->cursor >= '0' && *state->cursor <= '9') {
        value = value * 10 + (*state->cursor - '0');
        ++state->cursor;
    }
    return negative ? -value : value;
}

static int32_t calc_parse_term(calc_state_t *state) {
    int32_t value = calc_parse_factor(state);
    while (state->ok) {
        int32_t rhs;
        calc_skip(state);
        if (*state->cursor != '*' && *state->cursor != '/') {
            break;
        }
        char op = *state->cursor++;
        rhs = calc_parse_factor(state);
        if (!state->ok) {
            return 0;
        }
        if (op == '*') {
            value *= rhs;
        } else {
            if (rhs == 0) {
                state->ok = false;
                return 0;
            }
            value /= rhs;
        }
    }
    return value;
}

static int32_t calc_parse_expr(calc_state_t *state) {
    int32_t value = calc_parse_term(state);
    while (state->ok) {
        int32_t rhs;
        calc_skip(state);
        if (*state->cursor != '+' && *state->cursor != '-') {
            break;
        }
        char op = *state->cursor++;
        rhs = calc_parse_term(state);
        if (!state->ok) {
            return 0;
        }
        if (op == '+') {
            value += rhs;
        } else {
            value -= rhs;
        }
    }
    return value;
}

static bool calc_eval(const char *expr, int32_t *result) {
    calc_state_t state;
    state.cursor = expr;
    state.ok = true;
    *result = calc_parse_expr(&state);
    calc_skip(&state);
    return state.ok && *state.cursor == '\0';
}

static void app_calc(size_t argc, char **argv) {
    char line[SHELL_INPUT_MAX];
    if (argc > 1) {
        int32_t result;
        shell_join_args(1, argc, argv, line, sizeof(line));
        if (!calc_eval(line, &result)) {
            console_writeline("Invalid expression.");
            return;
        }
        console_write("Result: ");
        console_write_signed(result);
        console_writeline("");
        return;
    }
    console_writeline("SNSX Calculator. Type an expression or 'exit'.");
    while (1) {
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_write("calc");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("> ");
        line_read(line, sizeof(line));
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            return;
        }
        if (line[0] == '\0') {
            continue;
        }
        int32_t result;
        if (!calc_eval(line, &result)) {
            console_writeline("Invalid expression.");
            continue;
        }
        console_write("= ");
        console_write_signed(result);
        console_writeline("");
    }
}

static void app_edit(const char *path) {
    char line[SHELL_INPUT_MAX];
    size_t length = 0;
    arm_fs_entry_t *entry = fs_find(path);

    memset(g_console_editor_buffer, 0, sizeof(g_console_editor_buffer));
    if (entry != NULL) {
        if (entry->type == ARM_FS_DIR) {
            console_writeline("Cannot edit a directory.");
            return;
        }
        memcpy(g_console_editor_buffer, entry->data, entry->size);
        length = entry->size;
    }

    console_clear();
    console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
    console_write("SNSX Editor: ");
    console_set_color(EFI_WHITE, EFI_BLACK);
    console_writeline(path);
    console_writeline("Commands: .save  .quit  .show  .clear  .help");
    console_writeline("");

    while (1) {
        console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
        console_write("edit");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("> ");
        line_read(line, sizeof(line));

        if (strcmp(line, ".help") == 0) {
            console_writeline(".save save and leave");
            console_writeline(".quit leave without saving");
            console_writeline(".show show current buffer");
            console_writeline(".clear clear the buffer");
            continue;
        }
        if (strcmp(line, ".quit") == 0) {
            console_writeline("Editor closed without saving.");
            return;
        }
        if (strcmp(line, ".clear") == 0) {
            memset(g_console_editor_buffer, 0, sizeof(g_console_editor_buffer));
            length = 0;
            console_writeline("Buffer cleared.");
            continue;
        }
        if (strcmp(line, ".show") == 0) {
            console_writeline("-----");
            if (length == 0) {
                console_writeline("<empty>");
            } else {
                for (size_t i = 0; i < length; ++i) {
                    console_putchar(g_console_editor_buffer[i]);
                }
                if (g_console_editor_buffer[length - 1] != '\n') {
                    console_putchar('\n');
                }
            }
            console_writeline("-----");
            continue;
        }
        if (strcmp(line, ".save") == 0) {
            if (!fs_write(path, (const uint8_t *)g_console_editor_buffer, length, ARM_FS_TEXT)) {
                console_writeline("Save failed.");
                continue;
            }
            console_writeline("File saved.");
            return;
        }
        {
            size_t line_len = strlen(line);
            if (length + line_len + 1 >= sizeof(g_console_editor_buffer)) {
                console_writeline("Editor buffer full.");
                continue;
            }
            memcpy(g_console_editor_buffer + length, line, line_len);
            length += line_len;
            g_console_editor_buffer[length++] = '\n';
        }
    }
}

static bool parse_index(const char *text, size_t max_count, size_t *index) {
    int parsed = atoi(text);
    if (parsed <= 0 || (size_t)parsed > max_count) {
        return false;
    }
    *index = (size_t)parsed - 1;
    return true;
}

static void u16_to_hex4(uint16_t value, char *buffer) {
    static const char hex[] = "0123456789ABCDEF";
    buffer[0] = hex[(value >> 12) & 0x0Fu];
    buffer[1] = hex[(value >> 8) & 0x0Fu];
    buffer[2] = hex[(value >> 4) & 0x0Fu];
    buffer[3] = hex[value & 0x0Fu];
    buffer[4] = '\0';
}

static void network_append_diag_count(char *output, size_t output_size, const char *label, uint32_t value) {
    char digits[16];
    if (output[0] != '\0') {
        strcat(output, "  ");
    }
    strcat(output, label);
    strcat(output, " ");
    u32_to_dec(value, digits);
    strcat(output, digits);
    output[output_size - 1] = '\0';
}

static uint32_t firmware_protocol_handle_count(const EFI_GUID *guid) {
    EFI_HANDLE *handles = NULL;
    UINTN handle_count = 0;
    uint32_t count = 0;

    if (g_st == NULL || g_st->boot_services == NULL || g_st->boot_services->locate_handle_buffer == NULL) {
        return 0;
    }
    if (g_st->boot_services->locate_handle_buffer(EFI_BY_PROTOCOL, (EFI_GUID *)guid, NULL,
                                                  &handle_count, &handles) == EFI_SUCCESS) {
        count = (uint32_t)handle_count;
        if (g_st->boot_services->free_pool != NULL && handles != NULL) {
            g_st->boot_services->free_pool(handles);
        }
    }
    return count;
}

static const char *network_pci_model_name(uint16_t vendor_id, uint16_t device_id) {
    if (vendor_id == 0x8086 && device_id == 0x100E) {
        return "Intel 82540EM";
    }
    if (vendor_id == 0x8086 && device_id == 0x100F) {
        return "Intel 82545EM";
    }
    if (vendor_id == 0x8086 && device_id == 0x1019) {
        return "Intel 82547EI";
    }
    if (vendor_id == 0x1022 && device_id == 0x2000) {
        return "AMD PCnet";
    }
    if (vendor_id == 0x1AF4) {
        return "Virtio network";
    }
    if (vendor_id == 0x80EE) {
        return "VirtualBox virtual NIC";
    }
    return NULL;
}

static void network_collect_diagnostics(void) {
    EFI_HANDLE *handles = NULL;
    UINTN handle_count = 0;

    memset(&g_netdiag, 0, sizeof(g_netdiag));
    g_netdiag.snp_handles = firmware_protocol_handle_count(&g_guid_simple_network);
    g_netdiag.pxe_handles = firmware_protocol_handle_count(&g_guid_pxe_base_code);
    g_netdiag.ip4_handles = firmware_protocol_handle_count(&g_guid_ip4_service_binding);
    g_netdiag.udp4_handles = firmware_protocol_handle_count(&g_guid_udp4_service_binding);
    g_netdiag.tcp4_handles = firmware_protocol_handle_count(&g_guid_tcp4_service_binding);
    g_netdiag.dhcp4_handles = firmware_protocol_handle_count(&g_guid_dhcp4_service_binding);

    g_netdiag.firmware_stack[0] = '\0';
    network_append_diag_count(g_netdiag.firmware_stack, sizeof(g_netdiag.firmware_stack), "SNP", g_netdiag.snp_handles);
    network_append_diag_count(g_netdiag.firmware_stack, sizeof(g_netdiag.firmware_stack), "PXE", g_netdiag.pxe_handles);
    network_append_diag_count(g_netdiag.firmware_stack, sizeof(g_netdiag.firmware_stack), "IP4", g_netdiag.ip4_handles);
    network_append_diag_count(g_netdiag.firmware_stack, sizeof(g_netdiag.firmware_stack), "UDP4", g_netdiag.udp4_handles);
    network_append_diag_count(g_netdiag.firmware_stack, sizeof(g_netdiag.firmware_stack), "TCP4", g_netdiag.tcp4_handles);
    network_append_diag_count(g_netdiag.firmware_stack, sizeof(g_netdiag.firmware_stack), "DHCP4", g_netdiag.dhcp4_handles);

    strcopy_limit(g_netdiag.hardware, "No PCI network controller detected.", sizeof(g_netdiag.hardware));
    strcopy_limit(g_netdiag.wireless,
                  "VirtualBox routes host Wi-Fi to this guest as Ethernet. Nearby Wi-Fi and Bluetooth discovery need guest-visible radios or passthrough.",
                  sizeof(g_netdiag.wireless));

    if (g_st == NULL || g_st->boot_services == NULL ||
        g_st->boot_services->locate_handle_buffer == NULL ||
        g_st->boot_services->handle_protocol == NULL) {
        return;
    }
    if (g_st->boot_services->locate_handle_buffer(EFI_BY_PROTOCOL, (EFI_GUID *)&g_guid_pci_io, NULL,
                                                  &handle_count, &handles) != EFI_SUCCESS) {
        return;
    }
    for (UINTN i = 0; i < handle_count; ++i) {
        EFI_PCI_IO_PROTOCOL *pci = NULL;
        uint8_t config[64];

        if (g_st->boot_services->handle_protocol(handles[i], (EFI_GUID *)&g_guid_pci_io,
                                                 (void **)&pci) != EFI_SUCCESS ||
            pci == NULL || pci->pci.read == NULL) {
            continue;
        }
        memset(config, 0, sizeof(config));
        if (pci->pci.read(pci, EfiPciIoWidthUint8, 0, sizeof(config), config) != EFI_SUCCESS) {
            continue;
        }
        if (config[11] == 0x02) {
            uint16_t vendor_id = (uint16_t)config[0] | ((uint16_t)config[1] << 8);
            uint16_t device_id = (uint16_t)config[2] | ((uint16_t)config[3] << 8);
            const char *model = network_pci_model_name(vendor_id, device_id);
            ++g_netdiag.pci_network_handles;
            if (g_netdiag.pci_network_handles == 1) {
                char vendor_hex[5];
                char device_hex[5];
                char digits[16];
                UINTN segment = 0;
                UINTN bus = 0;
                UINTN device = 0;
                UINTN function = 0;

                u16_to_hex4(vendor_id, vendor_hex);
                u16_to_hex4(device_id, device_hex);
                g_netdiag.hardware[0] = '\0';
                if (model != NULL) {
                    strcopy_limit(g_netdiag.hardware, model, sizeof(g_netdiag.hardware));
                } else {
                    strcopy_limit(g_netdiag.hardware, "PCI NIC ", sizeof(g_netdiag.hardware));
                    strcat(g_netdiag.hardware, vendor_hex);
                    strcat(g_netdiag.hardware, ":");
                    strcat(g_netdiag.hardware, device_hex);
                }
                if (pci->get_location != NULL &&
                    pci->get_location(pci, &segment, &bus, &device, &function) == EFI_SUCCESS) {
                    strcat(g_netdiag.hardware, " @ ");
                    u32_to_dec((uint32_t)segment, digits);
                    strcat(g_netdiag.hardware, digits);
                    strcat(g_netdiag.hardware, ":");
                    u32_to_dec((uint32_t)bus, digits);
                    strcat(g_netdiag.hardware, digits);
                    strcat(g_netdiag.hardware, ".");
                    u32_to_dec((uint32_t)device, digits);
                    strcat(g_netdiag.hardware, digits);
                    strcat(g_netdiag.hardware, ".");
                    u32_to_dec((uint32_t)function, digits);
                    strcat(g_netdiag.hardware, digits);
                }
            }
        }
    }
    if (g_st->boot_services->free_pool != NULL && handles != NULL) {
        g_st->boot_services->free_pool(handles);
    }
}

static void app_files(void) {
    char cwd[FS_PATH_MAX];
    char line[SHELL_INPUT_MAX];
    char *argv[SHELL_ARG_MAX];
    arm_fs_entry_t *entries[FILE_MANAGER_VIEW_MAX];

    strcopy_limit(cwd, g_cwd, sizeof(cwd));
    while (1) {
        console_clear();
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_writeline("SNSX File Manager");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("Directory: ");
        console_writeline(cwd);
        console_writeline("");
        size_t count = fs_list(cwd, entries, ARRAY_SIZE(entries));
        if (count == 0) {
            console_writeline("<empty>");
        } else {
            for (size_t i = 0; i < count; ++i) {
                console_write_dec((uint32_t)(i + 1));
                console_write(". ");
                shell_print_entry(entries[i]);
            }
        }
        console_writeline("");
        console_writeline("Commands: open N, edit N, cd N, del N, mkdir NAME, new NAME, ren N NAME, copy N NAME, up, quit");
        console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
        console_write("files");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("> ");
        line_read(line, sizeof(line));

        size_t argc = shell_tokenize(line, argv, ARRAY_SIZE(argv));
        if (argc == 0) {
            continue;
        }
        if (strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "exit") == 0) {
            strcopy_limit(g_cwd, cwd, sizeof(g_cwd));
            return;
        }
        if (strcmp(argv[0], "up") == 0) {
            parent_directory(cwd);
            continue;
        }
        if (argc < 2) {
            wait_for_enter("More input is needed for that command.");
            continue;
        }

        size_t index = 0;
        if ((strcmp(argv[0], "open") == 0 || strcmp(argv[0], "edit") == 0 || strcmp(argv[0], "cd") == 0 ||
             strcmp(argv[0], "del") == 0 || strcmp(argv[0], "ren") == 0 || strcmp(argv[0], "copy") == 0) &&
            !parse_index(argv[1], count, &index)) {
            wait_for_enter("Invalid entry number.");
            continue;
        }

        if (strcmp(argv[0], "open") == 0) {
            if (entries[index]->type == ARM_FS_DIR) {
                strcopy_limit(cwd, entries[index]->path, sizeof(cwd));
            } else {
                view_text(entries[index]->path);
            }
            continue;
        }
        if (strcmp(argv[0], "edit") == 0) {
            app_edit(entries[index]->path);
            continue;
        }
        if (strcmp(argv[0], "cd") == 0) {
            if (entries[index]->type != ARM_FS_DIR) {
                wait_for_enter("That entry is not a directory.");
            } else {
                strcopy_limit(cwd, entries[index]->path, sizeof(cwd));
            }
            continue;
        }
        if (strcmp(argv[0], "del") == 0) {
            if (!fs_remove(entries[index]->path)) {
                wait_for_enter("Delete failed. Directories must be empty.");
            }
            continue;
        }
        if (strcmp(argv[0], "mkdir") == 0) {
            char path[FS_PATH_MAX];
            resolve_path_from(cwd, argv[1], path);
            if (!fs_mkdir(path)) {
                wait_for_enter("Directory creation failed.");
            }
            continue;
        }
        if (strcmp(argv[0], "new") == 0) {
            char path[FS_PATH_MAX];
            resolve_path_from(cwd, argv[1], path);
            if (!fs_write(path, (const uint8_t *)"", 0, ARM_FS_TEXT)) {
                wait_for_enter("File creation failed.");
            } else {
                app_edit(path);
            }
            continue;
        }
        if (strcmp(argv[0], "ren") == 0 || strcmp(argv[0], "copy") == 0) {
            char source[FS_PATH_MAX];
            char destination[FS_PATH_MAX];
            char parent[FS_PATH_MAX];
            if (argc < 3) {
                wait_for_enter("A new name is required.");
                continue;
            }
            strcopy_limit(source, entries[index]->path, sizeof(source));
            strcopy_limit(parent, source, sizeof(parent));
            parent_directory(parent);
            if (strcmp(parent, "/") == 0) {
                strcopy_limit(destination, "/", sizeof(destination));
                strcat(destination, argv[2]);
            } else {
                strcopy_limit(destination, parent, sizeof(destination));
                strcat(destination, "/");
                strcat(destination, argv[2]);
            }
            if (strcmp(argv[0], "ren") == 0) {
                if (!fs_move(source, destination)) {
                    wait_for_enter("Rename failed.");
                }
            } else {
                if (!fs_copy(source, destination)) {
                    wait_for_enter("Copy failed.");
                }
            }
            continue;
        }
        wait_for_enter("Unknown file manager command.");
    }
}

static void show_network_diagnostics(void) {
    char digits[16];

    console_write("Firmware stack: ");
    console_writeline(g_netdiag.firmware_stack);
    console_write("PCI network hardware: ");
    console_writeline(g_netdiag.hardware);
    console_write("Wireless note: ");
    console_writeline(g_netdiag.wireless);
    console_write("SNP handles: ");
    u32_to_dec(g_netdiag.snp_handles, digits);
    console_writeline(digits);
    console_write("PXE handles: ");
    u32_to_dec(g_netdiag.pxe_handles, digits);
    console_writeline(digits);
    console_write("IP4 handles: ");
    u32_to_dec(g_netdiag.ip4_handles, digits);
    console_writeline(digits);
    console_write("UDP4 handles: ");
    u32_to_dec(g_netdiag.udp4_handles, digits);
    console_writeline(digits);
    console_write("TCP4 handles: ");
    u32_to_dec(g_netdiag.tcp4_handles, digits);
    console_writeline(digits);
    console_write("DHCP4 handles: ");
    u32_to_dec(g_netdiag.dhcp4_handles, digits);
    console_writeline(digits);
    console_write("PCI NICs: ");
    u32_to_dec(g_netdiag.pci_network_handles, digits);
    console_writeline(digits);
}

static void show_network_status(void) {
    console_write("Profile: ");
    console_writeline(g_network.profile);
    console_write("Preferred mode: ");
    console_writeline(g_network.preferred_transport);
    console_write("Host name: ");
    console_writeline(g_network.hostname);
    console_write("Transport: ");
    console_writeline(g_network.transport);
    console_write("Driver: ");
    console_writeline(g_network.driver_name);
    console_write("IPv4: ");
    console_writeline(g_network.ipv4);
    console_write("Mask: ");
    console_writeline(g_network.subnet_mask);
    console_write("Gateway: ");
    console_writeline(g_network.gateway);
    console_write("DNS: ");
    console_writeline(g_network.dns_server);
    console_write("MAC: ");
    console_writeline(g_network.mac);
    console_write("Permanent MAC: ");
    console_writeline(g_network.permanent_mac);
    console_write("Wired link: ");
    if (!g_network.available) {
        console_writeline("no firmware adapter");
    } else if (!g_network.media_present_supported) {
        console_writeline("adapter present, link reporting unavailable");
    } else {
        console_writeline(g_network.media_present ? "present" : "not reported");
    }
    console_write("Adapter state: ");
    console_writeline(g_network.status);
    console_write("Internet: ");
    console_writeline(g_network.internet);
    console_write("Capabilities: ");
    console_writeline(g_network.capabilities);
    show_network_diagnostics();
}

static void show_settings_summary(void) {
    console_write("Theme: ");
    console_writeline(desktop_theme_name());
    console_write("Autosave: ");
    console_writeline(g_desktop.autosave ? "on" : "off");
    console_write("Pointer speed: ");
    console_writeline(desktop_pointer_speed_name());
    console_write("Pointer trail: ");
    console_writeline(g_desktop.pointer_trail ? "on" : "off");
    console_write("Network mode: ");
    console_writeline(desktop_network_mode_name());
    console_write("Network autoconnect: ");
    console_writeline(g_desktop.network_autoconnect ? "on" : "off");
    console_write("Host name: ");
    console_writeline(g_desktop.hostname);
    console_write("Last app: ");
    console_writeline(g_desktop.last_app);
    console_write("Persistence: ");
    console_writeline(g_persist.status);
}

static void console_divider(void) {
    console_writeline("------------------------------------------------------------");
}

static void desktop_draw_text_ui(void) {
    console_clear();
    console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
    console_writeline("SNSX Desktop");
    console_set_color(EFI_WHITE, EFI_BLACK);
    console_writeline("ARM64 VirtualBox Edition");
    console_divider();
    console_write("Theme: ");
    console_writeline(desktop_theme_name());
    console_write("Persistence: ");
    console_writeline(g_persist.status);
    console_write("Network: ");
    console_writeline(g_network.available ? g_network.mac : "Unavailable");
    console_write("Network mode: ");
    console_writeline(desktop_network_mode_name());
    console_write("Pointer: ");
    console_writeline(desktop_pointer_speed_name());
    console_write("Workspace: ");
    console_writeline(g_cwd);
    console_divider();
    console_writeline("Applications");
    console_writeline("[1] Files       [2] Notes       [3] Journal     [4] Calculator");
    console_writeline("[5] Terminal    [6] Guide       [7] Quickstart  [8] Welcome");
    console_writeline("[13] Snake      [14] Store      [15] Studio");
    console_writeline("");
    console_writeline("System");
    console_writeline("[9] Installer   [10] Settings   [11] System     [12] Save Session");
    console_writeline("");
    console_writeline("Use an item number or name. Press Enter for Terminal.");
    console_writeline("Commands: setup files notes journal calc terminal guide quickstart");
    console_writeline("          welcome installer settings system disks network snake store studio save reboot");
    console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
    console_write("desktop");
    console_set_color(EFI_WHITE, EFI_BLACK);
    console_write("> ");
}

static void app_setup_console(void) {
    char line[SHELL_INPUT_MAX];

    while (1) {
        console_clear();
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_writeline("SNSX First-Run Setup");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_divider();
        console_writeline("Use this setup screen before the desktop to verify input and prepare persistence.");
        if (g_force_text_ui) {
            console_writeline("SNSX rescue mode is active: the graphical desktop was skipped until a real key or click event is confirmed.");
        }
        console_write("Keyboard protocol: ");
        console_writeline(g_boot_diag.keyboard_protocol ? "present" : "missing");
        console_write("Extended keyboard protocol: ");
        console_writeline(g_boot_diag.keyboard_extended_protocol ? "present" : "missing");
        console_write("Live key event: ");
        console_writeline(g_boot_diag.key_seen ? "detected" : "not detected yet");
        console_write("Persistence: ");
        console_writeline(g_persist.status);
        console_divider();
        console_writeline("Commands: desktop  install  save  guide  diag  devices");
        console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
        console_write("setup");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("> ");
        line_read(line, sizeof(line));

        if (line[0] == '\0' || strcmp(line, "desktop") == 0 || strcmp(line, "back") == 0) {
            break;
        }
        if (strcmp(line, "install") == 0) {
            persist_install_boot_disk();
            desktop_note_launch("SETUP", g_persist.status);
            wait_for_enter(g_persist.status);
            continue;
        }
        if (strcmp(line, "save") == 0) {
            g_persist.dirty = true;
            persist_flush_state();
            desktop_note_launch("SETUP", g_persist.status);
            wait_for_enter(g_persist.status);
            continue;
        }
        if (strcmp(line, "guide") == 0) {
            view_text("/docs/INSTALL.TXT");
            continue;
        }
        if (strcmp(line, "diag") == 0 || strcmp(line, "diagnostics") == 0) {
            boot_run_input_diagnostics();
            continue;
        }
        if (strcmp(line, "devices") == 0) {
            command_devices();
            wait_for_enter(NULL);
            continue;
        }
        wait_for_enter("Unknown setup command.");
    }

    g_desktop.welcome_seen = true;
    desktop_note_launch("SETUP", "First-run setup completed.");
    desktop_commit_preferences();
}

static void app_settings_console(void) {
    char line[SHELL_INPUT_MAX];
    char *argv[SHELL_ARG_MAX];

    while (1) {
        console_clear();
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_writeline("SNSX Settings");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_divider();
        show_settings_summary();
        console_divider();
        console_writeline("Commands:");
        console_writeline("theme ocean|sunrise|slate|emerald");
        console_writeline("autosave on|off|toggle");
        console_writeline("pointer 1|2|3|4|5");
        console_writeline("trail on|off|toggle");
        console_writeline("mode auto|ethernet|wifi|hotspot");
        console_writeline("autonet on|off|toggle");
        console_writeline("hostname NAME");
        console_writeline("wifi NAME");
        console_writeline("hotspot NAME");
        console_writeline("network");
        console_writeline("disks");
        console_writeline("save");
        console_writeline("back");
        console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
        console_write("settings");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("> ");
        line_read(line, sizeof(line));

        size_t argc = shell_tokenize(line, argv, ARRAY_SIZE(argv));
        if (argc == 0) {
            continue;
        }
        if (strcmp(argv[0], "back") == 0 || strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "exit") == 0) {
            return;
        }
        if (strcmp(argv[0], "theme") == 0) {
            if (argc < 2) {
                wait_for_enter("Usage: theme ocean|sunrise|slate|emerald");
                continue;
            }
            command_theme(argv[1]);
            desktop_note_launch("SETTINGS", "Theme updated from Settings.");
            continue;
        }
        if (strcmp(argv[0], "autosave") == 0) {
            if (argc < 2 || strcmp(argv[1], "toggle") == 0) {
                g_desktop.autosave = !g_desktop.autosave;
            } else if (strcmp(argv[1], "on") == 0) {
                g_desktop.autosave = true;
            } else if (strcmp(argv[1], "off") == 0) {
                g_desktop.autosave = false;
            } else {
                wait_for_enter("Usage: autosave on|off|toggle");
                continue;
            }
            desktop_note_launch("SETTINGS", g_desktop.autosave ? "Autosave enabled." : "Autosave disabled.");
            desktop_commit_preferences();
            continue;
        }
        if (strcmp(argv[0], "pointer") == 0) {
            int speed;
            if (argc < 2) {
                wait_for_enter("Usage: pointer 1|2|3|4|5");
                continue;
            }
            speed = atoi(argv[1]);
            if (speed < 1 || speed > 5) {
                wait_for_enter("Pointer speed must be between 1 and 5.");
                continue;
            }
            g_desktop.pointer_speed = (uint32_t)speed;
            desktop_note_launch("SETTINGS", "Pointer speed updated.");
            desktop_commit_preferences();
            continue;
        }
        if (strcmp(argv[0], "trail") == 0) {
            if (argc < 2 || strcmp(argv[1], "toggle") == 0) {
                g_desktop.pointer_trail = !g_desktop.pointer_trail;
            } else if (strcmp(argv[1], "on") == 0) {
                g_desktop.pointer_trail = true;
            } else if (strcmp(argv[1], "off") == 0) {
                g_desktop.pointer_trail = false;
            } else {
                wait_for_enter("Usage: trail on|off|toggle");
                continue;
            }
            desktop_note_launch("SETTINGS", g_desktop.pointer_trail ? "Pointer trail enabled." : "Pointer trail disabled.");
            desktop_commit_preferences();
            continue;
        }
        if (strcmp(argv[0], "mode") == 0) {
            if (argc < 2) {
                wait_for_enter("Usage: mode auto|ethernet|wifi|hotspot");
                continue;
            }
            if (strcmp(argv[1], "auto") == 0) {
                g_desktop.network_preference = NETWORK_PREF_AUTO;
            } else if (strcmp(argv[1], "ethernet") == 0 || strcmp(argv[1], "lan") == 0) {
                g_desktop.network_preference = NETWORK_PREF_ETHERNET;
            } else if (strcmp(argv[1], "wifi") == 0) {
                g_desktop.network_preference = NETWORK_PREF_WIFI;
            } else if (strcmp(argv[1], "hotspot") == 0) {
                g_desktop.network_preference = NETWORK_PREF_HOTSPOT;
            } else {
                wait_for_enter("Usage: mode auto|ethernet|wifi|hotspot");
                continue;
            }
            desktop_note_launch("SETTINGS", "Updated the preferred network mode.");
            desktop_commit_preferences();
            network_refresh(g_desktop.network_autoconnect);
            continue;
        }
        if (strcmp(argv[0], "autonet") == 0) {
            if (argc < 2 || strcmp(argv[1], "toggle") == 0) {
                g_desktop.network_autoconnect = !g_desktop.network_autoconnect;
            } else if (strcmp(argv[1], "on") == 0) {
                g_desktop.network_autoconnect = true;
            } else if (strcmp(argv[1], "off") == 0) {
                g_desktop.network_autoconnect = false;
            } else {
                wait_for_enter("Usage: autonet on|off|toggle");
                continue;
            }
            desktop_note_launch("SETTINGS", g_desktop.network_autoconnect ? "Network autoconnect enabled." :
                                                                  "Network autoconnect disabled.");
            desktop_commit_preferences();
            network_refresh(g_desktop.network_autoconnect);
            continue;
        }
        if (strcmp(argv[0], "hostname") == 0) {
            char value[SHELL_INPUT_MAX];
            if (argc < 2) {
                wait_for_enter("Usage: hostname NAME");
                continue;
            }
            shell_join_args(1, argc, argv, value, sizeof(value));
            strcopy_limit(g_desktop.hostname, value, sizeof(g_desktop.hostname));
            desktop_note_launch("SETTINGS", "Updated the SNSX host name.");
            desktop_commit_preferences();
            network_refresh(g_desktop.network_autoconnect);
            continue;
        }
        if (strcmp(argv[0], "wifi") == 0) {
            char value[SHELL_INPUT_MAX];
            if (argc < 2) {
                wait_for_enter("Usage: wifi NAME");
                continue;
            }
            shell_join_args(1, argc, argv, value, sizeof(value));
            strcopy_limit(g_desktop.wifi_ssid, value, sizeof(g_desktop.wifi_ssid));
            desktop_note_launch("SETTINGS", "Updated the stored Wi-Fi profile.");
            desktop_commit_preferences();
            network_refresh(g_desktop.network_autoconnect);
            continue;
        }
        if (strcmp(argv[0], "hotspot") == 0) {
            char value[SHELL_INPUT_MAX];
            if (argc < 2) {
                wait_for_enter("Usage: hotspot NAME");
                continue;
            }
            shell_join_args(1, argc, argv, value, sizeof(value));
            strcopy_limit(g_desktop.hotspot_name, value, sizeof(g_desktop.hotspot_name));
            desktop_note_launch("SETTINGS", "Updated the stored hotspot profile.");
            desktop_commit_preferences();
            network_refresh(g_desktop.network_autoconnect);
            continue;
        }
        if (strcmp(argv[0], "save") == 0) {
            g_persist.dirty = true;
            persist_flush_state();
            desktop_note_launch("SETTINGS", g_persist.status);
            wait_for_enter(g_persist.status);
            continue;
        }
        if (strcmp(argv[0], "network") == 0) {
            if (g_gfx.available) {
                ui_run_network_app();
            } else {
                show_network_status();
                wait_for_enter(NULL);
            }
            continue;
        }
        if (strcmp(argv[0], "disks") == 0) {
            if (g_gfx.available) {
                ui_run_disk_app();
            } else {
                app_disk_console();
            }
            continue;
        }
        wait_for_enter("Unknown settings command.");
    }
}

static void app_installer_console(void) {
    char line[SHELL_INPUT_MAX];

    while (1) {
        console_clear();
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_writeline("SNSX Installer");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_divider();
        console_writeline("This installs BOOTAA64.EFI and the SNSX state snapshot to the selected FAT disk.");
        console_write("Status: ");
        console_writeline(g_persist.status);
        console_divider();
        console_writeline("Commands: install  save  status  guide  baremetal  back");
        console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
        console_write("installer");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("> ");
        line_read(line, sizeof(line));

        if (line[0] == '\0') {
            continue;
        }
        if (strcmp(line, "back") == 0 || strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            return;
        }
        if (strcmp(line, "install") == 0) {
            persist_install_boot_disk();
            desktop_note_launch("INSTALLER", g_persist.status);
            wait_for_enter(g_persist.status);
            continue;
        }
        if (strcmp(line, "save") == 0) {
            g_persist.dirty = true;
            persist_flush_state();
            desktop_note_launch("INSTALLER", g_persist.status);
            wait_for_enter(g_persist.status);
            continue;
        }
        if (strcmp(line, "status") == 0) {
            wait_for_enter(g_persist.status);
            continue;
        }
        if (strcmp(line, "guide") == 0) {
            view_text("/docs/INSTALL.TXT");
            continue;
        }
        if (strcmp(line, "baremetal") == 0 || strcmp(line, "primary") == 0) {
            view_text("/docs/BAREMETAL.TXT");
            continue;
        }
        wait_for_enter("Unknown installer command.");
    }
}

static void app_disk_console(void) {
    char line[SHELL_INPUT_MAX];
    char *argv[SHELL_ARG_MAX];

    while (1) {
        disk_volume_info_t infos[PERSIST_MAX_VOLUMES];
        size_t count = disk_collect_info(infos, ARRAY_SIZE(infos));

        console_clear();
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_writeline("SNSX Disk Manager");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_divider();
        if (count == 0) {
            console_writeline("No UEFI file-system volumes were detected.");
        } else {
            for (size_t i = 0; i < count; ++i) {
                console_write(infos[i].selected ? "* " : "  ");
                console_writeline(infos[i].name);
                console_write("    ");
                console_writeline(infos[i].role);
            }
        }
        console_divider();
        console_writeline("Commands: use INDEX  install  save  rescan  guide  back");
        console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
        console_write("disks");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("> ");
        line_read(line, sizeof(line));

        size_t argc = shell_tokenize(line, argv, ARRAY_SIZE(argv));
        if (argc == 0) {
            continue;
        }
        if (strcmp(argv[0], "back") == 0 || strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "exit") == 0) {
            return;
        }
        if (strcmp(argv[0], "use") == 0) {
            int index;
            if (argc < 2) {
                wait_for_enter("Usage: use INDEX");
                continue;
            }
            index = atoi(argv[1]);
            if (index < 0 || (size_t)index >= count) {
                wait_for_enter("Disk index is out of range.");
                continue;
            }
            g_persist.available = true;
            g_persist.volume_index = (size_t)index;
            persist_set_status(infos[index].writable ?
                "Selected writable volume as the active SNSX disk." :
                "Selected read-only volume. Install and Save remain disabled.");
            wait_for_enter(g_persist.status);
            continue;
        }
        if (strcmp(argv[0], "install") == 0) {
            persist_install_boot_disk();
            wait_for_enter(g_persist.status);
            continue;
        }
        if (strcmp(argv[0], "save") == 0) {
            g_persist.dirty = true;
            persist_flush_state();
            wait_for_enter(g_persist.status);
            continue;
        }
        if (strcmp(argv[0], "rescan") == 0) {
            persist_discover();
            network_refresh(g_desktop.network_autoconnect);
            wait_for_enter(g_persist.status);
            continue;
        }
        if (strcmp(argv[0], "guide") == 0) {
            view_text("/docs/DISKS.TXT");
            continue;
        }
        wait_for_enter("Unknown disk manager command.");
    }
}

static void app_system_console(void) {
    char line[SHELL_INPUT_MAX];

    while (1) {
        console_clear();
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_writeline("SNSX System Center");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_divider();
        console_write("Theme: ");
        console_writeline(desktop_theme_name());
        console_write("Last app: ");
        console_writeline(g_desktop.last_app);
        console_write("Persistence: ");
        console_writeline(g_persist.status);
        console_divider();
        console_writeline("[1] Devices");
        console_writeline("[2] Network");
        console_writeline("[3] About");
        console_writeline("[4] Disk Manager");
        console_writeline("[5] Save Session");
        console_writeline("[6] Back");
        console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
        console_write("system");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("> ");
        line_read(line, sizeof(line));

        if (line[0] == '\0') {
            continue;
        }
        if (strcmp(line, "6") == 0 || strcmp(line, "back") == 0 || strcmp(line, "quit") == 0 ||
            strcmp(line, "exit") == 0) {
            return;
        }
        if (strcmp(line, "1") == 0 || strcmp(line, "devices") == 0) {
            console_clear();
            command_devices();
            wait_for_enter(NULL);
            continue;
        }
        if (strcmp(line, "2") == 0 || strcmp(line, "network") == 0 || strcmp(line, "net") == 0) {
            console_clear();
            show_network_status();
            wait_for_enter(NULL);
            continue;
        }
        if (strcmp(line, "3") == 0 || strcmp(line, "about") == 0) {
            console_clear();
            command_about();
            wait_for_enter(NULL);
            continue;
        }
        if (strcmp(line, "4") == 0 || strcmp(line, "disks") == 0 || strcmp(line, "storage") == 0) {
            app_disk_console();
            continue;
        }
        if (strcmp(line, "5") == 0 || strcmp(line, "save") == 0) {
            g_persist.dirty = true;
            persist_flush_state();
            wait_for_enter(g_persist.status);
            continue;
        }
        wait_for_enter("Unknown system command.");
    }
}

static void gfx_bind_buffers(void) {
    g_gfx.drawbuffer = g_gfx.framebuffer;
    g_gfx.draw_stride = g_gfx.stride;

    if (!g_gfx.available || g_gfx.framebuffer == NULL) {
        return;
    }
    if (g_gfx.width > GFX_BACKBUFFER_MAX_WIDTH || g_gfx.height > GFX_BACKBUFFER_MAX_HEIGHT) {
        return;
    }

    memset(g_gfx_backbuffer, 0, sizeof(g_gfx_backbuffer));
    g_gfx.drawbuffer = g_gfx_backbuffer;
    g_gfx.draw_stride = g_gfx.width;
}

static bool __attribute__((unused)) gfx_init(void) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    uint32_t current_mode = 0;

    memset(&g_gfx, 0, sizeof(g_gfx));
    if (g_st->boot_services == NULL || g_st->boot_services->locate_protocol == NULL) {
        return false;
    }
    if (g_st->boot_services->locate_protocol((EFI_GUID *)&g_guid_gop, NULL, (void **)&gop) != EFI_SUCCESS ||
        gop == NULL || gop->mode == NULL || gop->mode->info == NULL) {
        return false;
    }

    current_mode = gop->mode->mode;
    if (gop->set_mode != NULL) {
        gop->set_mode(gop, current_mode);
    }

    if (gop->mode != NULL && gop->mode->info != NULL &&
        gop->mode->info->pixel_format != PixelBltOnly &&
        gop->mode->frame_buffer_base != 0 &&
        gop->mode->info->horizontal_resolution != 0 &&
        gop->mode->info->vertical_resolution != 0 &&
        gop->mode->info->pixels_per_scan_line >= gop->mode->info->horizontal_resolution) {
        g_gfx.width = gop->mode->info->horizontal_resolution;
        g_gfx.height = gop->mode->info->vertical_resolution;
        g_gfx.stride = gop->mode->info->pixels_per_scan_line;
        g_gfx.pixel_format = gop->mode->info->pixel_format;
        g_gfx.framebuffer = (uint32_t *)(uintptr_t)gop->mode->frame_buffer_base;
        g_gfx.available = g_gfx.framebuffer != NULL;
        if (g_gfx.available) {
            gfx_bind_buffers();
            return true;
        }
    }

    if (gop->query_mode != NULL && gop->set_mode != NULL && gop->mode->max_mode > 0) {
        for (uint32_t mode = 0; mode < gop->mode->max_mode; ++mode) {
            EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = NULL;
            UINTN info_size = 0;
            if (gop->query_mode(gop, mode, &info_size, &info) != EFI_SUCCESS || info == NULL) {
                continue;
            }
            if (info->pixel_format != PixelBltOnly &&
                info->horizontal_resolution != 0 &&
                info->vertical_resolution != 0 &&
                info->pixels_per_scan_line >= info->horizontal_resolution &&
                gop->set_mode(gop, mode) == EFI_SUCCESS &&
                gop->mode != NULL && gop->mode->info != NULL &&
                gop->mode->frame_buffer_base != 0) {
                g_gfx.width = gop->mode->info->horizontal_resolution;
                g_gfx.height = gop->mode->info->vertical_resolution;
                g_gfx.stride = gop->mode->info->pixels_per_scan_line;
                g_gfx.pixel_format = gop->mode->info->pixel_format;
                g_gfx.framebuffer = (uint32_t *)(uintptr_t)gop->mode->frame_buffer_base;
                if (g_gfx.framebuffer != NULL) {
                    g_gfx.available = true;
                    gfx_bind_buffers();
                    if (g_st->boot_services->free_pool != NULL) {
                        g_st->boot_services->free_pool(info);
                    }
                    return true;
                }
            }
            if (g_st->boot_services->free_pool != NULL) {
                g_st->boot_services->free_pool(info);
            }
        }
    }

    memset(&g_gfx, 0, sizeof(g_gfx));
    return false;
}

static bool pointer_bind_protocol(const EFI_GUID *guid, bool absolute) {
    EFI_HANDLE *handles = NULL;
    UINTN count = 0;

    if (g_st == NULL || g_st->boot_services == NULL ||
        g_st->boot_services->locate_handle_buffer == NULL ||
        g_st->boot_services->handle_protocol == NULL) {
        return false;
    }
    if (g_st->boot_services->locate_handle_buffer(EFI_BY_PROTOCOL, (EFI_GUID *)guid, NULL,
                                                  &count, &handles) != EFI_SUCCESS ||
        handles == NULL) {
        return false;
    }
    for (UINTN i = 0; i < count; ++i) {
        if (absolute) {
            EFI_ABSOLUTE_POINTER_PROTOCOL *absolute_protocol = NULL;
            if (g_st->boot_services->handle_protocol(handles[i], (EFI_GUID *)guid,
                                                     (void **)&absolute_protocol) == EFI_SUCCESS &&
                absolute_protocol != NULL) {
                if (absolute_protocol->reset != NULL) {
                    absolute_protocol->reset(absolute_protocol, 0);
                }
                g_pointer.available = true;
                g_pointer.kind = POINTER_KIND_ABSOLUTE;
                g_pointer.absolute_protocol = absolute_protocol;
                if (g_st->boot_services->free_pool != NULL) {
                    g_st->boot_services->free_pool(handles);
                }
                return true;
            }
        } else {
            EFI_SIMPLE_POINTER_PROTOCOL *simple_protocol = NULL;
            if (g_st->boot_services->handle_protocol(handles[i], (EFI_GUID *)guid,
                                                     (void **)&simple_protocol) == EFI_SUCCESS &&
                simple_protocol != NULL) {
                if (simple_protocol->reset != NULL) {
                    simple_protocol->reset(simple_protocol, 0);
                }
                g_pointer.available = true;
                g_pointer.kind = POINTER_KIND_SIMPLE;
                g_pointer.simple_protocol = simple_protocol;
                if (g_st->boot_services->free_pool != NULL) {
                    g_st->boot_services->free_pool(handles);
                }
                return true;
            }
        }
    }
    if (g_st->boot_services->free_pool != NULL) {
        g_st->boot_services->free_pool(handles);
    }
    return false;
}

static void __attribute__((unused)) pointer_init(void) {
    EFI_SIMPLE_POINTER_PROTOCOL *simple = NULL;
    EFI_ABSOLUTE_POINTER_PROTOCOL *absolute = NULL;

    memset(&g_pointer, 0, sizeof(g_pointer));
    if (!g_gfx.available || g_gfx.width == 0 || g_gfx.height == 0) {
        return;
    }
    if (g_st->boot_services == NULL || g_st->boot_services->locate_protocol == NULL) {
        return;
    }

    if (pointer_bind_protocol((EFI_GUID *)&g_guid_simple_pointer, false) ||
        pointer_bind_protocol((EFI_GUID *)&g_guid_absolute_pointer, true)) {
        g_pointer.x = (int)g_gfx.width / 2;
        g_pointer.y = (int)g_gfx.height / 2;
        return;
    }

    if (g_st->boot_services->locate_protocol((EFI_GUID *)&g_guid_simple_pointer, NULL, (void **)&simple) == EFI_SUCCESS &&
        simple != NULL) {
        if (simple->reset != NULL) {
            simple->reset(simple, 0);
        }
        g_pointer.available = true;
        g_pointer.kind = POINTER_KIND_SIMPLE;
        g_pointer.simple_protocol = simple;
    } else if (g_st->boot_services->locate_protocol((EFI_GUID *)&g_guid_absolute_pointer, NULL, (void **)&absolute) == EFI_SUCCESS &&
               absolute != NULL) {
        if (absolute->reset != NULL) {
            absolute->reset(absolute, 0);
        }
        g_pointer.available = true;
        g_pointer.kind = POINTER_KIND_ABSOLUTE;
        g_pointer.absolute_protocol = absolute;
    } else {
        return;
    }
    g_pointer.x = (int)g_gfx.width / 2;
    g_pointer.y = (int)g_gfx.height / 2;
}

static int pointer_scale_axis(uint64_t value, uint64_t min, uint64_t max, uint32_t pixels) {
    uint64_t range;
    uint64_t offset;
    uint64_t scaled;

    if (pixels == 0 || max <= min) {
        return 0;
    }
    if (value < min) {
        value = min;
    }
    if (value > max) {
        value = max;
    }
    range = max - min;
    offset = value - min;
    scaled = (offset * (pixels - 1)) / range;
    return (int)scaled;
}

static bool pointer_poll_event(ui_event_t *event) {
    if (!g_pointer.available) {
        return false;
    }
    if (g_pointer.kind == POINTER_KIND_SIMPLE && g_pointer.simple_protocol != NULL) {
        EFI_SIMPLE_POINTER_STATE state;
        EFI_STATUS status = g_pointer.simple_protocol->get_state(g_pointer.simple_protocol, &state);
        if (status == EFI_SUCCESS) {
            int speed = (int)g_desktop.pointer_speed;
            int dx;
            int dy;
            if (speed < 1) {
                speed = 1;
            }
            if (speed > 5) {
                speed = 5;
            }
            dx = (int)((state.relative_movement_x * speed) / 2);
            dy = (int)((state.relative_movement_y * speed) / 2);
            if (dx == 0 && state.relative_movement_x != 0) {
                dx = state.relative_movement_x > 0 ? 1 : -1;
            }
            if (dy == 0 && state.relative_movement_y != 0) {
                dy = state.relative_movement_y > 0 ? -1 : 1;
            }
            if (dx < -32) dx = -32;
            if (dx > 32) dx = 32;
            if (dy < -32) dy = -32;
            if (dy > 32) dy = 32;
            g_pointer.x += dx;
            g_pointer.y -= dy;
            if (g_pointer.x < 0) g_pointer.x = 0;
            if (g_pointer.y < 0) g_pointer.y = 0;
            if (g_pointer.x >= (int)g_gfx.width) g_pointer.x = (int)g_gfx.width - 1;
            if (g_pointer.y >= (int)g_gfx.height) g_pointer.y = (int)g_gfx.height - 1;
            event->mouse_moved = dx != 0 || dy != 0;
            event->mouse_clicked = state.left_button && !g_pointer.left_down;
            event->mouse_released = !state.left_button && g_pointer.left_down;
            event->mouse_down = state.left_button;
            event->mouse_x = g_pointer.x;
            event->mouse_y = g_pointer.y;
            if (event->mouse_moved || event->mouse_clicked || event->mouse_released) {
                g_pointer.engaged = true;
            }
            g_pointer.left_down = state.left_button;
            return event->mouse_moved || event->mouse_clicked || event->mouse_released;
        }
        return false;
    }
    if (g_pointer.kind == POINTER_KIND_ABSOLUTE && g_pointer.absolute_protocol != NULL &&
        g_pointer.absolute_protocol->mode != NULL) {
        EFI_ABSOLUTE_POINTER_STATE state;
        EFI_STATUS status = g_pointer.absolute_protocol->get_state(g_pointer.absolute_protocol, &state);
        if (status == EFI_SUCCESS) {
            int next_x = pointer_scale_axis(state.current_x,
                                            g_pointer.absolute_protocol->mode->absolute_min_x,
                                            g_pointer.absolute_protocol->mode->absolute_max_x,
                                            g_gfx.width);
            int next_y = pointer_scale_axis(state.current_y,
                                            g_pointer.absolute_protocol->mode->absolute_min_y,
                                            g_pointer.absolute_protocol->mode->absolute_max_y,
                                            g_gfx.height);
            bool left_button = (state.active_buttons & 1u) != 0;
            event->mouse_moved = next_x != g_pointer.x || next_y != g_pointer.y;
            g_pointer.x = next_x;
            g_pointer.y = next_y;
            event->mouse_clicked = left_button && !g_pointer.left_down;
            event->mouse_released = !left_button && g_pointer.left_down;
            event->mouse_down = left_button;
            event->mouse_x = g_pointer.x;
            event->mouse_y = g_pointer.y;
            if (event->mouse_moved || event->mouse_clicked || event->mouse_released) {
                g_pointer.engaged = true;
            }
            g_pointer.left_down = left_button;
            return event->mouse_moved || event->mouse_clicked || event->mouse_released;
        }
    }
    return false;
}

enum {
    E1000_CTRL = 0x0000,
    E1000_STATUS = 0x0008,
    E1000_EERD = 0x0014,
    E1000_ICR = 0x00C0,
    E1000_IMS = 0x00D0,
    E1000_IMC = 0x00D8,
    E1000_RCTL = 0x0100,
    E1000_TCTL = 0x0400,
    E1000_TIPG = 0x0410,
    E1000_RDBAL = 0x2800,
    E1000_RDBAH = 0x2804,
    E1000_RDLEN = 0x2808,
    E1000_RDH = 0x2810,
    E1000_RDT = 0x2818,
    E1000_TDBAL = 0x3800,
    E1000_TDBAH = 0x3804,
    E1000_TDLEN = 0x3808,
    E1000_TDH = 0x3810,
    E1000_TDT = 0x3818,
    E1000_RAL = 0x5400,
    E1000_RAH = 0x5404,
    E1000_CTRL_ASDE = 0x00000020,
    E1000_CTRL_SLU = 0x00000040,
    E1000_CTRL_RST = 0x04000000,
    E1000_STATUS_LU = 0x00000002,
    E1000_RCTL_EN = 0x00000002,
    E1000_RCTL_BAM = 0x00008000,
    E1000_RCTL_SECRC = 0x04000000,
    E1000_TCTL_EN = 0x00000002,
    E1000_TCTL_PSP = 0x00000008,
    E1000_TX_CMD_EOP = 0x01,
    E1000_TX_CMD_IFCS = 0x02,
    E1000_TX_CMD_RS = 0x08,
    E1000_RX_STATUS_DD = 0x01,
    E1000_TX_STATUS_DD = 0x01,
    NET_ETHERTYPE_IPV4 = 0x0800,
    NET_ETHERTYPE_ARP = 0x0806,
    NET_IPPROTO_TCP = 6,
    NET_IPPROTO_UDP = 17,
    NET_TCP_FIN = 0x01,
    NET_TCP_SYN = 0x02,
    NET_TCP_RST = 0x04,
    NET_TCP_PSH = 0x08,
    NET_TCP_ACK = 0x10,
    NET_PORT_DHCP_SERVER = 67,
    NET_PORT_DHCP_CLIENT = 68,
    NET_PORT_DNS = 53
};

#define E1000_RAH_AV 0x80000000u

typedef struct dhcp_offer {
    bool valid;
    uint8_t message_type;
    uint32_t your_ip;
    uint32_t server_id;
    uint32_t subnet_mask;
    uint32_t router;
    uint32_t dns;
    uint32_t lease_seconds;
} dhcp_offer_t;

typedef struct tcp_packet_view {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t flags;
    const uint8_t *payload;
    size_t payload_len;
} tcp_packet_view_t;

static uint16_t net_read_be16(const uint8_t *buffer) {
    return (uint16_t)(((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1]);
}

static uint32_t net_read_be32(const uint8_t *buffer) {
    return ((uint32_t)buffer[0] << 24) |
           ((uint32_t)buffer[1] << 16) |
           ((uint32_t)buffer[2] << 8) |
           (uint32_t)buffer[3];
}

static void net_write_be16(uint8_t *buffer, uint16_t value) {
    buffer[0] = (uint8_t)(value >> 8);
    buffer[1] = (uint8_t)(value & 0xFF);
}

static void net_write_be32(uint8_t *buffer, uint32_t value) {
    buffer[0] = (uint8_t)(value >> 24);
    buffer[1] = (uint8_t)((value >> 16) & 0xFF);
    buffer[2] = (uint8_t)((value >> 8) & 0xFF);
    buffer[3] = (uint8_t)(value & 0xFF);
}

static uint32_t net_ip_make(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d;
}

static void net_ip_to_string(uint32_t ip, char *output, size_t limit) {
    char part[16];
    output[0] = '\0';
    u32_to_dec((ip >> 24) & 0xFFu, part);
    strcopy_limit(output, part, limit);
    if (strlen(output) + 2 < limit) {
        strcat(output, ".");
    }
    u32_to_dec((ip >> 16) & 0xFFu, part);
    if (strlen(output) + strlen(part) + 2 < limit) {
        strcat(output, part);
        strcat(output, ".");
    }
    u32_to_dec((ip >> 8) & 0xFFu, part);
    if (strlen(output) + strlen(part) + 2 < limit) {
        strcat(output, part);
        strcat(output, ".");
    }
    u32_to_dec(ip & 0xFFu, part);
    if (strlen(output) + strlen(part) + 1 < limit) {
        strcat(output, part);
    }
}

static bool net_parse_ipv4_text(const char *text, uint32_t *out_ip) {
    uint32_t parts[4];
    size_t count = 0;
    uint32_t value = 0;
    bool has_digit = false;

    while (*text != '\0') {
        if (*text >= '0' && *text <= '9') {
            has_digit = true;
            value = value * 10u + (uint32_t)(*text - '0');
            if (value > 255u) {
                return false;
            }
        } else if (*text == '.') {
            if (!has_digit || count >= 4) {
                return false;
            }
            parts[count++] = value;
            value = 0;
            has_digit = false;
        } else {
            return false;
        }
        ++text;
    }
    if (!has_digit || count != 3) {
        return false;
    }
    parts[count] = value;
    *out_ip = net_ip_make((uint8_t)parts[0], (uint8_t)parts[1], (uint8_t)parts[2], (uint8_t)parts[3]);
    return true;
}

static bool net_same_subnet(uint32_t left, uint32_t right, uint32_t mask) {
    return (left & mask) == (right & mask);
}

static uint16_t net_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0;
    size_t index = 0;

    while (index + 1 < len) {
        sum += (uint32_t)net_read_be16(data + index);
        index += 2;
    }
    if (index < len) {
        sum += (uint32_t)data[index] << 8;
    }
    while ((sum >> 16) != 0) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}

static uint16_t net_tcp_checksum(uint32_t src_ip, uint32_t dst_ip, const uint8_t *segment, size_t len) {
    uint8_t pseudo[12];
    uint32_t sum = 0;
    size_t index = 0;

    net_write_be32(pseudo, src_ip);
    net_write_be32(pseudo + 4, dst_ip);
    pseudo[8] = 0;
    pseudo[9] = NET_IPPROTO_TCP;
    net_write_be16(pseudo + 10, (uint16_t)len);
    while (index + 1 < sizeof(pseudo)) {
        sum += (uint32_t)net_read_be16(pseudo + index);
        index += 2;
    }

    index = 0;
    while (index + 1 < len) {
        sum += (uint32_t)net_read_be16(segment + index);
        index += 2;
    }
    if (index < len) {
        sum += (uint32_t)segment[index] << 8;
    }
    while ((sum >> 16) != 0) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}

static void net_copy_mac(uint8_t *dst, const uint8_t *src) {
    memcpy(dst, src, 6);
}

static void netstack_sync_strings(void) {
    if (g_netstack.configured) {
        net_ip_to_string(g_netstack.client_ip, g_network.ipv4, sizeof(g_network.ipv4));
        net_ip_to_string(g_netstack.subnet_mask, g_network.subnet_mask, sizeof(g_network.subnet_mask));
        net_ip_to_string(g_netstack.gateway_ip, g_network.gateway, sizeof(g_network.gateway));
        net_ip_to_string(g_netstack.dns_ip, g_network.dns_server, sizeof(g_network.dns_server));
    } else {
        strcopy_limit(g_network.ipv4, "Unassigned", sizeof(g_network.ipv4));
        strcopy_limit(g_network.subnet_mask, "-", sizeof(g_network.subnet_mask));
        strcopy_limit(g_network.gateway, "-", sizeof(g_network.gateway));
        strcopy_limit(g_network.dns_server, "-", sizeof(g_network.dns_server));
    }
}

static void netstack_reset(void) {
    memset(&g_netstack, 0, sizeof(g_netstack));
    g_netstack.next_port = 49152;
    g_netstack.next_ip_id = 1;
    strcopy_limit(g_netstack.browser_status, "SNSX HTTP engine is idle.", sizeof(g_netstack.browser_status));
    strcopy_limit(g_netstack.last_host, "-", sizeof(g_netstack.last_host));
    strcopy_limit(g_netstack.last_ip, "-", sizeof(g_netstack.last_ip));
    netstack_sync_strings();
}

static bool netstack_apply_virtualbox_nat_profile(void) {
    static const uint8_t vbox_oui[3] = {0x08, 0x00, 0x27};

    if (memcmp(g_netstack.client_mac, vbox_oui, sizeof(vbox_oui)) != 0) {
        return false;
    }

    g_netstack.configured = true;
    g_netstack.static_profile = true;
    g_netstack.client_ip = net_ip_make(10, 0, 2, 15);
    g_netstack.subnet_mask = net_ip_make(255, 255, 255, 0);
    g_netstack.gateway_ip = net_ip_make(10, 0, 2, 2);
    g_netstack.dns_ip = net_ip_make(10, 0, 2, 3);
    g_netstack.lease_seconds = 0;
    g_netstack.gateway_mac_valid = false;
    strcopy_limit(g_netstack.browser_status,
                  "SNSX is using the VirtualBox NAT fallback profile and can reach host-backed internet services.",
                  sizeof(g_netstack.browser_status));
    netstack_sync_strings();
    return true;
}

static uint32_t netstack_random_next(void) {
    if (g_netstack.xid == 0) {
        g_netstack.xid = 0x534E5358u;
        for (size_t i = 0; i < 6; ++i) {
            g_netstack.xid ^= ((uint32_t)g_netstack.client_mac[i]) << ((i % 4) * 8);
        }
    }
    g_netstack.xid ^= g_netstack.xid << 13;
    g_netstack.xid ^= g_netstack.xid >> 17;
    g_netstack.xid ^= g_netstack.xid << 5;
    if (g_netstack.xid == 0) {
        g_netstack.xid = 0xA5A55A5Au;
    }
    return g_netstack.xid;
}

static EFI_SIMPLE_NETWORK_PROTOCOL *network_locate_protocol(void) {
    EFI_HANDLE *handles = NULL;
    EFI_SIMPLE_NETWORK_PROTOCOL *network = NULL;
    UINTN handle_count = 0;

    if (g_st == NULL || g_st->boot_services == NULL) {
        return NULL;
    }
    if (g_st->boot_services->locate_handle_buffer != NULL &&
        g_st->boot_services->handle_protocol != NULL &&
        g_st->boot_services->locate_handle_buffer(EFI_BY_PROTOCOL, (EFI_GUID *)&g_guid_simple_network, NULL,
                                                  &handle_count, &handles) == EFI_SUCCESS &&
        handles != NULL) {
        for (UINTN index = 0; index < handle_count; ++index) {
            network = NULL;
            if (g_st->boot_services->handle_protocol(handles[index], (EFI_GUID *)&g_guid_simple_network,
                                                     (void **)&network) == EFI_SUCCESS &&
                network != NULL && network->mode != NULL) {
                if (g_st->boot_services->free_pool != NULL) {
                    g_st->boot_services->free_pool(handles);
                }
                return network;
            }
        }
        if (g_st->boot_services->free_pool != NULL) {
            g_st->boot_services->free_pool(handles);
        }
    }
    if (g_st->boot_services->locate_protocol != NULL &&
        g_st->boot_services->locate_protocol((EFI_GUID *)&g_guid_simple_network, NULL, (void **)&network) == EFI_SUCCESS) {
        return network;
    }
    return NULL;
}

static uint32_t e1000_reg_read32(uint32_t offset) {
    uint32_t value = 0;

    if (!g_e1000.present || g_e1000.pci == NULL) {
        return 0;
    }
    if (g_e1000.use_mmio) {
        if (g_e1000.pci->mem.read == NULL) {
            return 0;
        }
        g_e1000.pci->mem.read(g_e1000.pci, EfiPciIoWidthUint32, g_e1000.bar_index, offset, 1, &value);
    } else {
        if (g_e1000.pci->io.read == NULL) {
            return 0;
        }
        g_e1000.pci->io.read(g_e1000.pci, EfiPciIoWidthUint32, g_e1000.bar_index, offset, 1, &value);
    }
    return value;
}

static void e1000_reg_write32(uint32_t offset, uint32_t value) {
    if (!g_e1000.present || g_e1000.pci == NULL) {
        return;
    }
    if (g_e1000.use_mmio) {
        if (g_e1000.pci->mem.write != NULL) {
            g_e1000.pci->mem.write(g_e1000.pci, EfiPciIoWidthUint32, g_e1000.bar_index, offset, 1, &value);
        }
    } else if (g_e1000.pci->io.write != NULL) {
        g_e1000.pci->io.write(g_e1000.pci, EfiPciIoWidthUint32, g_e1000.bar_index, offset, 1, &value);
    }
}

static uint16_t e1000_eeprom_read(uint8_t address) {
    uint32_t value = 0;
    uint32_t command = 0x00000001u | ((uint32_t)address << 8);

    e1000_reg_write32(E1000_EERD, command);
    for (int i = 0; i < 1000; ++i) {
        value = e1000_reg_read32(E1000_EERD);
        if ((value & (1u << 4)) != 0) {
            return (uint16_t)(value >> 16);
        }
        if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
            g_st->boot_services->stall(50);
        }
    }
    return 0;
}

static void e1000_read_mac(uint8_t mac[6]) {
    uint32_t ral = e1000_reg_read32(E1000_RAL);
    uint32_t rah = e1000_reg_read32(E1000_RAH);

    mac[0] = (uint8_t)(ral & 0xFFu);
    mac[1] = (uint8_t)((ral >> 8) & 0xFFu);
    mac[2] = (uint8_t)((ral >> 16) & 0xFFu);
    mac[3] = (uint8_t)((ral >> 24) & 0xFFu);
    mac[4] = (uint8_t)(rah & 0xFFu);
    mac[5] = (uint8_t)((rah >> 8) & 0xFFu);
    if ((mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0) ||
        (mac[0] == 0xFF && mac[1] == 0xFF && mac[2] == 0xFF && mac[3] == 0xFF && mac[4] == 0xFF && mac[5] == 0xFF)) {
        uint16_t word0 = e1000_eeprom_read(0);
        uint16_t word1 = e1000_eeprom_read(1);
        uint16_t word2 = e1000_eeprom_read(2);
        mac[0] = (uint8_t)(word0 & 0xFFu);
        mac[1] = (uint8_t)(word0 >> 8);
        mac[2] = (uint8_t)(word1 & 0xFFu);
        mac[3] = (uint8_t)(word1 >> 8);
        mac[4] = (uint8_t)(word2 & 0xFFu);
        mac[5] = (uint8_t)(word2 >> 8);
    }
}

static bool e1000_mac_is_valid(const uint8_t mac[6]) {
    static const uint8_t zero_mac[6] = {0, 0, 0, 0, 0, 0};
    static const uint8_t ff_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (memcmp(mac, zero_mac, sizeof(zero_mac)) == 0 || memcmp(mac, ff_mac, sizeof(ff_mac)) == 0) {
        return false;
    }
    if ((mac[0] & 0x01u) != 0) {
        return false;
    }
    return true;
}

static void e1000_build_fallback_mac(uint8_t mac[6]) {
    mac[0] = 0x02;
    mac[1] = 0x53;
    mac[2] = 0x4E;
    mac[3] = 0x53;
    mac[4] = 0x58;
    mac[5] = (uint8_t)((g_e1000.device_id ^ 0xA5u) & 0xFFu);
}

static void e1000_program_mac(const uint8_t mac[6]) {
    uint32_t ral = (uint32_t)mac[0] |
                   ((uint32_t)mac[1] << 8) |
                   ((uint32_t)mac[2] << 16) |
                   ((uint32_t)mac[3] << 24);
    uint32_t rah = (uint32_t)mac[4] |
                   ((uint32_t)mac[5] << 8) |
                   E1000_RAH_AV;

    e1000_reg_write32(E1000_RAL, ral);
    e1000_reg_write32(E1000_RAH, rah);
}

static void e1000_enable_pci_bus_mastering(void) {
    uint16_t command = 0;

    if (!g_e1000.present || g_e1000.pci == NULL || g_e1000.pci->pci.read == NULL || g_e1000.pci->pci.write == NULL) {
        return;
    }
    if (g_e1000.pci->pci.read(g_e1000.pci, EfiPciIoWidthUint16, 0x04, 1, &command) != EFI_SUCCESS) {
        return;
    }
    command |= 0x0007u;
    g_e1000.pci->pci.write(g_e1000.pci, EfiPciIoWidthUint16, 0x04, 1, &command);
}

static bool e1000_probe(void) {
    EFI_HANDLE *handles = NULL;
    UINTN handle_count = 0;

    memset(&g_e1000, 0, sizeof(g_e1000));
    if (g_st == NULL || g_st->boot_services == NULL ||
        g_st->boot_services->locate_handle_buffer == NULL ||
        g_st->boot_services->handle_protocol == NULL) {
        return false;
    }
    if (g_st->boot_services->locate_handle_buffer(EFI_BY_PROTOCOL, (EFI_GUID *)&g_guid_pci_io, NULL,
                                                  &handle_count, &handles) != EFI_SUCCESS ||
        handles == NULL) {
        return false;
    }
    for (UINTN i = 0; i < handle_count; ++i) {
        EFI_PCI_IO_PROTOCOL *pci = NULL;
        uint8_t config[64];
        uint32_t bar0;
        uint32_t bar1;
        uint16_t vendor_id;
        uint16_t device_id;

        if (g_st->boot_services->handle_protocol(handles[i], (EFI_GUID *)&g_guid_pci_io,
                                                 (void **)&pci) != EFI_SUCCESS ||
            pci == NULL || pci->pci.read == NULL) {
            continue;
        }
        memset(config, 0, sizeof(config));
        if (pci->pci.read(pci, EfiPciIoWidthUint8, 0, sizeof(config), config) != EFI_SUCCESS) {
            continue;
        }
        vendor_id = (uint16_t)config[0] | ((uint16_t)config[1] << 8);
        device_id = (uint16_t)config[2] | ((uint16_t)config[3] << 8);
        if (config[11] != 0x02 || vendor_id != 0x8086 ||
            (device_id != 0x100E && device_id != 0x100F && device_id != 0x1010 && device_id != 0x1011)) {
            continue;
        }
        bar0 = (uint32_t)config[0x10] |
               ((uint32_t)config[0x11] << 8) |
               ((uint32_t)config[0x12] << 16) |
               ((uint32_t)config[0x13] << 24);
        bar1 = (uint32_t)config[0x14] |
               ((uint32_t)config[0x15] << 8) |
               ((uint32_t)config[0x16] << 16) |
               ((uint32_t)config[0x17] << 24);
        g_e1000.present = true;
        g_e1000.pci = pci;
        g_e1000.vendor_id = vendor_id;
        g_e1000.device_id = device_id;
        if ((bar0 & 1u) == 0) {
            g_e1000.use_mmio = true;
            g_e1000.bar_index = 0;
        } else if ((bar1 & 1u) != 0) {
            g_e1000.use_mmio = false;
            g_e1000.bar_index = 1;
        } else {
            g_e1000.use_mmio = false;
            g_e1000.bar_index = 0;
        }
        break;
    }
    if (g_st->boot_services->free_pool != NULL && handles != NULL) {
        g_st->boot_services->free_pool(handles);
    }
    return g_e1000.present;
}

static bool e1000_initialize(void) {
    uint64_t rx_ring_addr;
    uint64_t tx_ring_addr;
    uint32_t ctrl;
    uint32_t status;

    if (!g_e1000.present) {
        return false;
    }
    e1000_enable_pci_bus_mastering();
    if (g_e1000.initialized) {
        status = e1000_reg_read32(E1000_STATUS);
        g_e1000.link_up = (status & E1000_STATUS_LU) != 0;
        return true;
    }

    memset(g_e1000_rx_ring, 0, sizeof(g_e1000_rx_ring));
    memset(g_e1000_tx_ring, 0, sizeof(g_e1000_tx_ring));
    memset(g_e1000_rx_buffers, 0, sizeof(g_e1000_rx_buffers));
    memset(g_e1000_tx_buffers, 0, sizeof(g_e1000_tx_buffers));
    for (size_t i = 0; i < ARRAY_SIZE(g_e1000_rx_ring); ++i) {
        g_e1000_rx_ring[i].addr = (uint64_t)(uintptr_t)g_e1000_rx_buffers[i];
        g_e1000_rx_ring[i].status = 0;
    }
    for (size_t i = 0; i < ARRAY_SIZE(g_e1000_tx_ring); ++i) {
        g_e1000_tx_ring[i].addr = (uint64_t)(uintptr_t)g_e1000_tx_buffers[i];
        g_e1000_tx_ring[i].status = E1000_TX_STATUS_DD;
    }

    e1000_reg_write32(E1000_IMC, 0xFFFFFFFFu);
    e1000_reg_read32(E1000_ICR);
    ctrl = e1000_reg_read32(E1000_CTRL);
    e1000_reg_write32(E1000_CTRL, ctrl | E1000_CTRL_RST);
    if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
        g_st->boot_services->stall(20000);
    }
    ctrl = e1000_reg_read32(E1000_CTRL);
    e1000_reg_write32(E1000_CTRL, ctrl | E1000_CTRL_SLU | E1000_CTRL_ASDE);

    rx_ring_addr = (uint64_t)(uintptr_t)g_e1000_rx_ring;
    tx_ring_addr = (uint64_t)(uintptr_t)g_e1000_tx_ring;
    e1000_reg_write32(E1000_RDBAL, (uint32_t)rx_ring_addr);
    e1000_reg_write32(E1000_RDBAH, (uint32_t)(rx_ring_addr >> 32));
    e1000_reg_write32(E1000_RDLEN, (uint32_t)sizeof(g_e1000_rx_ring));
    e1000_reg_write32(E1000_RDH, 0);
    e1000_reg_write32(E1000_RDT, (uint32_t)(ARRAY_SIZE(g_e1000_rx_ring) - 1));

    e1000_reg_write32(E1000_TDBAL, (uint32_t)tx_ring_addr);
    e1000_reg_write32(E1000_TDBAH, (uint32_t)(tx_ring_addr >> 32));
    e1000_reg_write32(E1000_TDLEN, (uint32_t)sizeof(g_e1000_tx_ring));
    e1000_reg_write32(E1000_TDH, 0);
    e1000_reg_write32(E1000_TDT, 0);
    e1000_reg_write32(E1000_TIPG, 0x0060200Au);
    e1000_reg_write32(E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | (15u << 4) | (64u << 12));
    e1000_reg_write32(E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC);

    g_e1000.tx_tail = 0;
    g_e1000.rx_head = 0;
    e1000_read_mac(g_e1000.mac);
    if (!e1000_mac_is_valid(g_e1000.mac)) {
        e1000_build_fallback_mac(g_e1000.mac);
        e1000_program_mac(g_e1000.mac);
    } else {
        e1000_program_mac(g_e1000.mac);
    }
    status = e1000_reg_read32(E1000_STATUS);
    g_e1000.link_up = (status & E1000_STATUS_LU) != 0;
    g_e1000.initialized = true;
    return true;
}

static bool e1000_send_frame(const uint8_t *frame, size_t len) {
    e1000_tx_desc_t *desc;
    uint32_t next_tail;

    if (!g_e1000.initialized || len == 0 || len > NET_FRAME_MAX) {
        return false;
    }
    desc = &g_e1000_tx_ring[g_e1000.tx_tail];
    if ((desc->status & E1000_TX_STATUS_DD) == 0) {
        return false;
    }
    memcpy(g_e1000_tx_buffers[g_e1000.tx_tail], frame, len);
    desc->length = (uint16_t)len;
    desc->cmd = E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS | E1000_TX_CMD_RS;
    desc->status = 0;
    next_tail = (g_e1000.tx_tail + 1u) % (uint32_t)ARRAY_SIZE(g_e1000_tx_ring);
    e1000_reg_write32(E1000_TDT, next_tail);
    for (int wait = 0; wait < 1000; ++wait) {
        if ((desc->status & E1000_TX_STATUS_DD) != 0) {
            g_e1000.tx_tail = next_tail;
            return true;
        }
        if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
            g_st->boot_services->stall(10);
        }
    }
    g_e1000.tx_tail = next_tail;
    return false;
}

static bool e1000_receive_frame(uint8_t *buffer, size_t *len, uint32_t timeout_ms) {
    uint32_t waited = 0;

    if (!g_e1000.initialized) {
        return false;
    }
    while (waited <= timeout_ms) {
        e1000_rx_desc_t *desc = &g_e1000_rx_ring[g_e1000.rx_head];
        if ((desc->status & E1000_RX_STATUS_DD) != 0) {
            size_t copy = desc->length;
            if (copy > NET_FRAME_MAX) {
                copy = NET_FRAME_MAX;
            }
            memcpy(buffer, g_e1000_rx_buffers[g_e1000.rx_head], copy);
            *len = copy;
            desc->status = 0;
            e1000_reg_write32(E1000_RDT, g_e1000.rx_head);
            g_e1000.rx_head = (g_e1000.rx_head + 1u) % (uint32_t)ARRAY_SIZE(g_e1000_rx_ring);
            return true;
        }
        if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
            g_st->boot_services->stall(10000);
        }
        waited += 10;
    }
    return false;
}

static void e1000_drain(void) {
    size_t len = 0;
    while (e1000_receive_frame(g_net_rx_frame, &len, 1)) {
        if (len == 0) {
            break;
        }
    }
}

static bool net_frame_send(const uint8_t *frame, size_t len) {
    EFI_STATUS status;

    if (g_e1000.initialized) {
        return e1000_send_frame(frame, len);
    }
    if (g_network.protocol == NULL || g_network.protocol->transmit == NULL || len == 0 || len > NET_FRAME_MAX) {
        return false;
    }
    status = g_network.protocol->transmit(g_network.protocol, 0, (UINTN)len, (void *)frame, NULL, NULL, NULL);
    if (g_network.protocol->get_status != NULL) {
        uint32_t interrupt_status = 0;
        void *tx_buf = NULL;
        g_network.protocol->get_status(g_network.protocol, &interrupt_status, &tx_buf);
        (void)interrupt_status;
        (void)tx_buf;
    }
    return status == EFI_SUCCESS;
}

static void net_frame_drain(void) {
    if (g_e1000.initialized) {
        e1000_drain();
        return;
    }
    while (1) {
        UINTN header_size = 0;
        UINTN buffer_size = sizeof(g_net_rx_frame);
        EFI_STATUS status;

        if (g_network.protocol == NULL || g_network.protocol->receive == NULL) {
            return;
        }
        status = g_network.protocol->receive(g_network.protocol, &header_size, &buffer_size,
                                             g_net_rx_frame, NULL, NULL, NULL);
        if (status != EFI_SUCCESS) {
            return;
        }
        if ((size_t)buffer_size == 0) {
            return;
        }
    }
}

static bool net_frame_receive(uint8_t *buffer, size_t *len, uint32_t timeout_ms) {
    uint32_t waited = 0;

    if (g_e1000.initialized) {
        return e1000_receive_frame(buffer, len, timeout_ms);
    }
    if (g_network.protocol == NULL || g_network.protocol->receive == NULL) {
        return false;
    }
    while (waited <= timeout_ms) {
        UINTN header_size = 0;
        UINTN buffer_size = NET_FRAME_MAX;
        EFI_STATUS status = g_network.protocol->receive(g_network.protocol, &header_size, &buffer_size,
                                                        buffer, NULL, NULL, NULL);
        if (status == EFI_SUCCESS) {
            *len = (size_t)buffer_size;
            return true;
        }
        if (status != EFI_NOT_READY && status != EFI_DEVICE_ERROR && status != EFI_BAD_BUFFER_SIZE) {
            return false;
        }
        if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
            g_st->boot_services->stall(10000);
        }
        waited += 10;
    }
    return false;
}

static size_t net_build_udp_frame(uint8_t *frame, const uint8_t dst_mac[6],
                                  uint32_t src_ip, uint32_t dst_ip,
                                  uint16_t src_port, uint16_t dst_port,
                                  const uint8_t *payload, size_t payload_len) {
    uint8_t *ip = frame + 14;
    uint8_t *udp = ip + 20;
    uint16_t total_length = (uint16_t)(20 + 8 + payload_len);

    memset(frame, 0, 14 + total_length);
    memset(frame, 0xFF, 6);
    if (dst_mac != NULL) {
        net_copy_mac(frame, dst_mac);
    }
    net_copy_mac(frame + 6, g_netstack.client_mac);
    net_write_be16(frame + 12, NET_ETHERTYPE_IPV4);

    ip[0] = 0x45;
    ip[1] = 0;
    net_write_be16(ip + 2, total_length);
    net_write_be16(ip + 4, g_netstack.next_ip_id++);
    net_write_be16(ip + 6, 0);
    ip[8] = 64;
    ip[9] = NET_IPPROTO_UDP;
    net_write_be32(ip + 12, src_ip);
    net_write_be32(ip + 16, dst_ip);
    net_write_be16(ip + 10, net_checksum(ip, 20));

    net_write_be16(udp + 0, src_port);
    net_write_be16(udp + 2, dst_port);
    net_write_be16(udp + 4, (uint16_t)(8 + payload_len));
    net_write_be16(udp + 6, 0);
    memcpy(udp + 8, payload, payload_len);
    return 14u + (size_t)total_length;
}

static size_t net_build_tcp_frame(uint8_t *frame, const uint8_t dst_mac[6],
                                  uint32_t src_ip, uint32_t dst_ip,
                                  uint16_t src_port, uint16_t dst_port,
                                  uint32_t seq, uint32_t ack, uint8_t flags,
                                  const uint8_t *payload, size_t payload_len) {
    uint8_t *ip = frame + 14;
    uint8_t *tcp = ip + 20;
    uint16_t total_length = (uint16_t)(20 + 20 + payload_len);

    memset(frame, 0, 14 + total_length);
    net_copy_mac(frame, dst_mac);
    net_copy_mac(frame + 6, g_netstack.client_mac);
    net_write_be16(frame + 12, NET_ETHERTYPE_IPV4);

    ip[0] = 0x45;
    ip[1] = 0;
    net_write_be16(ip + 2, total_length);
    net_write_be16(ip + 4, g_netstack.next_ip_id++);
    net_write_be16(ip + 6, 0);
    ip[8] = 64;
    ip[9] = NET_IPPROTO_TCP;
    net_write_be32(ip + 12, src_ip);
    net_write_be32(ip + 16, dst_ip);
    net_write_be16(ip + 10, net_checksum(ip, 20));

    net_write_be16(tcp + 0, src_port);
    net_write_be16(tcp + 2, dst_port);
    net_write_be32(tcp + 4, seq);
    net_write_be32(tcp + 8, ack);
    tcp[12] = 0x50;
    tcp[13] = flags;
    net_write_be16(tcp + 14, 4096);
    net_write_be16(tcp + 16, 0);
    net_write_be16(tcp + 18, 0);
    if (payload_len > 0) {
        memcpy(tcp + 20, payload, payload_len);
    }
    net_write_be16(tcp + 16, net_tcp_checksum(src_ip, dst_ip, tcp, 20 + payload_len));
    return 14u + (size_t)total_length;
}

static bool net_parse_udp_packet(const uint8_t *frame, size_t frame_len,
                                 uint32_t *src_ip, uint32_t *dst_ip,
                                 uint16_t *src_port, uint16_t *dst_port,
                                 const uint8_t **payload, size_t *payload_len) {
    const uint8_t *ip;
    const uint8_t *udp;
    size_t ip_header_len;
    uint16_t total_length;
    size_t udp_length;

    if (frame_len < 42 || net_read_be16(frame + 12) != NET_ETHERTYPE_IPV4) {
        return false;
    }
    ip = frame + 14;
    if ((ip[0] >> 4) != 4 || ip[9] != NET_IPPROTO_UDP) {
        return false;
    }
    ip_header_len = (size_t)(ip[0] & 0x0Fu) * 4u;
    total_length = net_read_be16(ip + 2);
    if (frame_len < 14 + ip_header_len + 8 || total_length < ip_header_len + 8) {
        return false;
    }
    udp = ip + ip_header_len;
    udp_length = (size_t)net_read_be16(udp + 4);
    if (udp_length < 8 || udp_length > total_length - ip_header_len) {
        return false;
    }
    *src_ip = net_read_be32(ip + 12);
    *dst_ip = net_read_be32(ip + 16);
    *src_port = net_read_be16(udp + 0);
    *dst_port = net_read_be16(udp + 2);
    *payload = udp + 8;
    *payload_len = udp_length - 8u;
    return true;
}

static bool net_parse_tcp_packet(const uint8_t *frame, size_t frame_len, tcp_packet_view_t *view) {
    const uint8_t *ip;
    const uint8_t *tcp;
    size_t ip_header_len;
    size_t tcp_header_len;
    uint16_t total_length;

    if (frame_len < 54 || net_read_be16(frame + 12) != NET_ETHERTYPE_IPV4) {
        return false;
    }
    ip = frame + 14;
    if ((ip[0] >> 4) != 4 || ip[9] != NET_IPPROTO_TCP) {
        return false;
    }
    ip_header_len = (size_t)(ip[0] & 0x0Fu) * 4u;
    total_length = net_read_be16(ip + 2);
    if (frame_len < 14 + ip_header_len + 20 || total_length < ip_header_len + 20) {
        return false;
    }
    tcp = ip + ip_header_len;
    tcp_header_len = (size_t)((tcp[12] >> 4) & 0x0Fu) * 4u;
    if (tcp_header_len < 20 || total_length < ip_header_len + tcp_header_len) {
        return false;
    }
    view->src_ip = net_read_be32(ip + 12);
    view->dst_ip = net_read_be32(ip + 16);
    view->src_port = net_read_be16(tcp + 0);
    view->dst_port = net_read_be16(tcp + 2);
    view->seq = net_read_be32(tcp + 4);
    view->ack = net_read_be32(tcp + 8);
    view->flags = tcp[13];
    view->payload = tcp + tcp_header_len;
    view->payload_len = (size_t)total_length - ip_header_len - tcp_header_len;
    return true;
}

static size_t dns_skip_name(const uint8_t *packet, size_t packet_len, size_t offset) {
    while (offset < packet_len) {
        uint8_t length = packet[offset];
        if (length == 0) {
            return offset + 1;
        }
        if ((length & 0xC0u) == 0xC0u) {
            return offset + 2;
        }
        offset += (size_t)length + 1u;
    }
    return packet_len;
}

static bool netstack_arp_resolve(uint32_t target_ip, uint8_t out_mac[6]) {
    size_t frame_len;

    if (!g_netstack.configured || target_ip == 0) {
        return false;
    }
    if (target_ip == g_netstack.client_ip) {
        net_copy_mac(out_mac, g_netstack.client_mac);
        return true;
    }

    memset(g_net_tx_frame, 0, 42);
    memset(g_net_tx_frame, 0xFF, 6);
    net_copy_mac(g_net_tx_frame + 6, g_netstack.client_mac);
    net_write_be16(g_net_tx_frame + 12, NET_ETHERTYPE_ARP);
    net_write_be16(g_net_tx_frame + 14, 1);
    net_write_be16(g_net_tx_frame + 16, NET_ETHERTYPE_IPV4);
    g_net_tx_frame[18] = 6;
    g_net_tx_frame[19] = 4;
    net_write_be16(g_net_tx_frame + 20, 1);
    net_copy_mac(g_net_tx_frame + 22, g_netstack.client_mac);
    net_write_be32(g_net_tx_frame + 28, g_netstack.client_ip);
    net_write_be32(g_net_tx_frame + 38, target_ip);

    for (int attempt = 0; attempt < 3; ++attempt) {
        net_frame_drain();
        if (!net_frame_send(g_net_tx_frame, 42)) {
            continue;
        }
        for (int wait_cycle = 0; wait_cycle < 40; ++wait_cycle) {
            if (!net_frame_receive(g_net_rx_frame, &frame_len, 25)) {
                continue;
            }
            if (frame_len < 42 || net_read_be16(g_net_rx_frame + 12) != NET_ETHERTYPE_ARP) {
                continue;
            }
            if (net_read_be16(g_net_rx_frame + 20) != 2) {
                continue;
            }
            if (net_read_be32(g_net_rx_frame + 28) != target_ip ||
                net_read_be32(g_net_rx_frame + 38) != g_netstack.client_ip) {
                continue;
            }
            net_copy_mac(out_mac, g_net_rx_frame + 22);
            return true;
        }
    }
    return false;
}

static bool netstack_next_hop_mac(uint32_t dest_ip, uint8_t out_mac[6]) {
    if (!g_netstack.configured) {
        return false;
    }
    if (net_same_subnet(g_netstack.client_ip, dest_ip, g_netstack.subnet_mask)) {
        if (netstack_arp_resolve(dest_ip, out_mac)) {
            return true;
        }
        if (g_netstack.gateway_ip != 0 &&
            (dest_ip == g_netstack.dns_ip || dest_ip == g_netstack.gateway_ip)) {
            if (g_netstack.gateway_mac_valid ||
                netstack_arp_resolve(g_netstack.gateway_ip, g_netstack.gateway_mac)) {
                g_netstack.gateway_mac_valid = true;
                net_copy_mac(out_mac, g_netstack.gateway_mac);
                return true;
            }
        }
        return false;
    }
    if (g_netstack.gateway_mac_valid) {
        net_copy_mac(out_mac, g_netstack.gateway_mac);
        return true;
    }
    if (g_netstack.gateway_ip != 0 && netstack_arp_resolve(g_netstack.gateway_ip, g_netstack.gateway_mac)) {
        g_netstack.gateway_mac_valid = true;
        net_copy_mac(out_mac, g_netstack.gateway_mac);
        return true;
    }
    return false;
}

static size_t netstack_build_dhcp_payload(uint8_t *payload, size_t limit,
                                          uint8_t message_type, uint32_t xid,
                                          uint32_t requested_ip, uint32_t server_id) {
    size_t index = 0;
    size_t hostname_len = strnlen(g_desktop.hostname, 31);

    if (limit < 256) {
        return 0;
    }
    memset(payload, 0, limit);
    payload[0] = 1;
    payload[1] = 1;
    payload[2] = 6;
    payload[3] = 0;
    net_write_be32(payload + 4, xid);
    net_write_be16(payload + 10, 0x8000);
    net_copy_mac(payload + 28, g_netstack.client_mac);
    payload[236] = 99;
    payload[237] = 130;
    payload[238] = 83;
    payload[239] = 99;
    index = 240;
    payload[index++] = 53;
    payload[index++] = 1;
    payload[index++] = message_type;
    if (requested_ip != 0) {
        payload[index++] = 50;
        payload[index++] = 4;
        net_write_be32(payload + index, requested_ip);
        index += 4;
    }
    if (server_id != 0) {
        payload[index++] = 54;
        payload[index++] = 4;
        net_write_be32(payload + index, server_id);
        index += 4;
    }
    payload[index++] = 55;
    payload[index++] = 4;
    payload[index++] = 1;
    payload[index++] = 3;
    payload[index++] = 6;
    payload[index++] = 51;
    if (hostname_len > 0) {
        payload[index++] = 12;
        payload[index++] = (uint8_t)hostname_len;
        memcpy(payload + index, g_desktop.hostname, hostname_len);
        index += hostname_len;
    }
    payload[index++] = 255;
    return index;
}

static bool netstack_wait_for_dhcp(uint32_t xid, uint8_t expected_type, dhcp_offer_t *offer) {
    size_t frame_len;

    memset(offer, 0, sizeof(*offer));
    for (int wait_cycle = 0; wait_cycle < 220; ++wait_cycle) {
        const uint8_t *payload = NULL;
        size_t payload_len = 0;
        uint32_t src_ip = 0;
        uint32_t dst_ip = 0;
        uint16_t src_port = 0;
        uint16_t dst_port = 0;

        if (!net_frame_receive(g_net_rx_frame, &frame_len, 25)) {
            continue;
        }
        if (!net_parse_udp_packet(g_net_rx_frame, frame_len, &src_ip, &dst_ip, &src_port, &dst_port,
                                  &payload, &payload_len)) {
            continue;
        }
        if (src_port != NET_PORT_DHCP_SERVER || dst_port != NET_PORT_DHCP_CLIENT || payload_len < 244) {
            continue;
        }
        if (payload[0] != 2 || net_read_be32(payload + 4) != xid) {
            continue;
        }
        if (memcmp(payload + 28, g_netstack.client_mac, 6) != 0) {
            continue;
        }
        if (payload[236] != 99 || payload[237] != 130 || payload[238] != 83 || payload[239] != 99) {
            continue;
        }
        offer->your_ip = net_read_be32(payload + 16);
        for (size_t index = 240; index + 1 < payload_len;) {
            uint8_t option = payload[index++];
            uint8_t option_len;

            if (option == 0) {
                continue;
            }
            if (option == 255) {
                break;
            }
            option_len = payload[index++];
            if (index + option_len > payload_len) {
                break;
            }
            if (option == 53 && option_len >= 1) {
                offer->message_type = payload[index];
            } else if (option == 54 && option_len >= 4) {
                offer->server_id = net_read_be32(payload + index);
            } else if (option == 1 && option_len >= 4) {
                offer->subnet_mask = net_read_be32(payload + index);
            } else if (option == 3 && option_len >= 4) {
                offer->router = net_read_be32(payload + index);
            } else if (option == 6 && option_len >= 4) {
                offer->dns = net_read_be32(payload + index);
            } else if (option == 51 && option_len >= 4) {
                offer->lease_seconds = net_read_be32(payload + index);
            }
            index += option_len;
        }
        if (offer->message_type != expected_type) {
            continue;
        }
        offer->valid = true;
        return true;
    }
    return false;
}

static bool netstack_renew_dhcp(void) {
    uint8_t payload[320];
    dhcp_offer_t offer;
    size_t payload_len;
    size_t frame_len;
    uint32_t xid;

    if (!g_network.initialized) {
        return false;
    }
    if (g_e1000.initialized) {
        net_copy_mac(g_netstack.client_mac, g_e1000.mac);
    } else if (g_network.protocol != NULL && g_network.protocol->mode != NULL) {
        net_copy_mac(g_netstack.client_mac, g_network.protocol->mode->current_address.addr);
    } else {
        return false;
    }
    xid = netstack_random_next();
    payload_len = netstack_build_dhcp_payload(payload, sizeof(payload), 1, xid, 0, 0);
    if (payload_len == 0) {
        return false;
    }
    frame_len = net_build_udp_frame(g_net_tx_frame, NULL, 0, 0xFFFFFFFFu,
                                    NET_PORT_DHCP_CLIENT, NET_PORT_DHCP_SERVER,
                                    payload, payload_len);
    net_frame_drain();
    if (!net_frame_send(g_net_tx_frame, frame_len) || !netstack_wait_for_dhcp(xid, 2, &offer) || !offer.valid) {
        return false;
    }

    payload_len = netstack_build_dhcp_payload(payload, sizeof(payload), 3, xid, offer.your_ip, offer.server_id);
    frame_len = net_build_udp_frame(g_net_tx_frame, NULL, 0, 0xFFFFFFFFu,
                                    NET_PORT_DHCP_CLIENT, NET_PORT_DHCP_SERVER,
                                    payload, payload_len);
    net_frame_drain();
    if (!net_frame_send(g_net_tx_frame, frame_len) || !netstack_wait_for_dhcp(xid, 5, &offer) || !offer.valid) {
        return false;
    }

    g_netstack.configured = true;
    g_netstack.static_profile = false;
    g_netstack.client_ip = offer.your_ip;
    g_netstack.subnet_mask = offer.subnet_mask != 0 ? offer.subnet_mask : net_ip_make(255, 255, 255, 0);
    g_netstack.gateway_ip = offer.router;
    g_netstack.dns_ip = offer.dns;
    g_netstack.lease_seconds = offer.lease_seconds;
    g_netstack.gateway_mac_valid = false;
    strcopy_limit(g_netstack.browser_status, "DHCP lease acquired. SNSX can resolve names and fetch HTTP pages.",
                  sizeof(g_netstack.browser_status));
    netstack_sync_strings();
    return true;
}

static bool netstack_resolve_dns_name(const char *host, uint32_t *out_ip) {
    uint8_t payload[512];
    uint8_t dest_mac[6];
    size_t payload_len = 0;
    size_t frame_len;
    uint16_t query_id;
    uint16_t local_port;

    if (net_parse_ipv4_text(host, out_ip)) {
        return true;
    }
    if (!g_netstack.configured || g_netstack.dns_ip == 0 || !netstack_next_hop_mac(g_netstack.dns_ip, dest_mac)) {
        return false;
    }

    memset(payload, 0, sizeof(payload));
    query_id = (uint16_t)(netstack_random_next() & 0xFFFFu);
    local_port = g_netstack.next_port++;
    net_write_be16(payload + 0, query_id);
    net_write_be16(payload + 2, 0x0100);
    net_write_be16(payload + 4, 1);
    payload_len = 12;

    {
        const char *cursor = host;
        while (*cursor != '\0') {
            const char *segment = cursor;
            size_t length = 0;
            while (segment[length] != '\0' && segment[length] != '.') {
                ++length;
            }
            if (length == 0 || payload_len + length + 2 >= sizeof(payload)) {
                return false;
            }
            payload[payload_len++] = (uint8_t)length;
            memcpy(payload + payload_len, segment, length);
            payload_len += length;
            cursor = segment + length;
            if (*cursor == '.') {
                ++cursor;
            }
        }
    }
    payload[payload_len++] = 0;
    net_write_be16(payload + payload_len, 1);
    payload_len += 2;
    net_write_be16(payload + payload_len, 1);
    payload_len += 2;

    frame_len = net_build_udp_frame(g_net_tx_frame, dest_mac, g_netstack.client_ip, g_netstack.dns_ip,
                                    local_port, NET_PORT_DNS, payload, payload_len);
    net_frame_drain();
    if (!net_frame_send(g_net_tx_frame, frame_len)) {
        return false;
    }

    for (int wait_cycle = 0; wait_cycle < 220; ++wait_cycle) {
        const uint8_t *udp_payload = NULL;
        size_t udp_payload_len = 0;
        uint32_t src_ip = 0;
        uint32_t dst_ip = 0;
        uint16_t src_port = 0;
        uint16_t dst_port = 0;

        if (!net_frame_receive(g_net_rx_frame, &frame_len, 25)) {
            continue;
        }
        if (!net_parse_udp_packet(g_net_rx_frame, frame_len, &src_ip, &dst_ip, &src_port, &dst_port,
                                  &udp_payload, &udp_payload_len)) {
            continue;
        }
        if (src_port != NET_PORT_DNS || dst_port != local_port || udp_payload_len < 12) {
            continue;
        }
        if (net_read_be16(udp_payload + 0) != query_id) {
            continue;
        }
        if ((net_read_be16(udp_payload + 2) & 0x8000u) == 0) {
            continue;
        }
        {
            uint16_t questions = net_read_be16(udp_payload + 4);
            uint16_t answers = net_read_be16(udp_payload + 6);
            size_t offset = 12;
            for (uint16_t i = 0; i < questions; ++i) {
                offset = dns_skip_name(udp_payload, udp_payload_len, offset);
                if (offset + 4 > udp_payload_len) {
                    break;
                }
                offset += 4;
            }
            for (uint16_t i = 0; i < answers && offset + 10 <= udp_payload_len; ++i) {
                uint16_t type;
                uint16_t class_code;
                uint16_t rdlength;

                offset = dns_skip_name(udp_payload, udp_payload_len, offset);
                if (offset + 10 > udp_payload_len) {
                    break;
                }
                type = net_read_be16(udp_payload + offset);
                class_code = net_read_be16(udp_payload + offset + 2);
                rdlength = net_read_be16(udp_payload + offset + 8);
                offset += 10;
                if (offset + rdlength > udp_payload_len) {
                    break;
                }
                if (type == 1 && class_code == 1 && rdlength == 4) {
                    *out_ip = net_read_be32(udp_payload + offset);
                    return true;
                }
                offset += rdlength;
            }
        }
    }
    return false;
}

static void netstack_append_char(char *output, size_t output_size, size_t *index, char ch) {
    if (*index + 1 >= output_size) {
        return;
    }
    output[(*index)++] = ch;
    output[*index] = '\0';
}

static void netstack_append_text(char *output, size_t output_size, size_t *index, const char *text) {
    while (*text != '\0') {
        netstack_append_char(output, output_size, index, *text++);
    }
}

static const char *netstack_find_text(const char *text, const char *needle) {
    size_t needle_len = strlen(needle);
    if (needle_len == 0) {
        return text;
    }
    while (*text != '\0') {
        if (strncmp(text, needle, needle_len) == 0) {
            return text;
        }
        ++text;
    }
    return NULL;
}

static bool netstack_tag_starts(const char *html, const char *tag_name) {
    size_t index = 0;

    if (html == NULL || html[0] != '<') {
        return false;
    }
    ++html;
    if (*html == '/') {
        ++html;
    }
    while (*tag_name != '\0' && html[index] != '\0') {
        if (ascii_lower(html[index]) != ascii_lower(*tag_name)) {
            return false;
        }
        ++index;
        ++tag_name;
    }
    return *tag_name == '\0';
}

static void netstack_render_html_body(const char *html, char *output, size_t output_size) {
    bool in_tag = false;
    bool in_script = false;
    bool in_style = false;
    bool last_space = false;
    size_t index = 0;

    output[0] = '\0';
    while (*html != '\0') {
        if (in_script) {
            const char *end = netstack_find_text(html, "</script");
            if (end == NULL) {
                break;
            }
            html = end;
            in_script = false;
            continue;
        }
        if (in_style) {
            const char *end = netstack_find_text(html, "</style");
            if (end == NULL) {
                break;
            }
            html = end;
            in_style = false;
            continue;
        }
        if (!in_tag && *html == '<') {
            if (strncmp(html, "<!--", 4) == 0) {
                const char *end = netstack_find_text(html, "-->");
                if (end == NULL) {
                    break;
                }
                html = end + 3;
                continue;
            }
            if (netstack_tag_starts(html, "script")) {
                in_tag = true;
                in_script = true;
                ++html;
                continue;
            }
            if (netstack_tag_starts(html, "style")) {
                in_tag = true;
                in_style = true;
                ++html;
                continue;
            }
            if (netstack_tag_starts(html, "br") || netstack_tag_starts(html, "p") || netstack_tag_starts(html, "div") ||
                netstack_tag_starts(html, "section") || netstack_tag_starts(html, "article") ||
                netstack_tag_starts(html, "tr") || netstack_tag_starts(html, "table") ||
                netstack_tag_starts(html, "h1") || netstack_tag_starts(html, "h2") ||
                netstack_tag_starts(html, "h3") || netstack_tag_starts(html, "title")) {
                netstack_append_char(output, output_size, &index, '\n');
                last_space = false;
            }
            if (netstack_tag_starts(html, "li")) {
                netstack_append_char(output, output_size, &index, '\n');
                netstack_append_text(output, output_size, &index, "* ");
                last_space = false;
            }
            if (netstack_tag_starts(html, "td") || netstack_tag_starts(html, "th")) {
                netstack_append_text(output, output_size, &index, " | ");
                last_space = true;
            }
            in_tag = true;
            ++html;
            continue;
        }
        if (in_tag) {
            if (*html == '>') {
                in_tag = false;
            }
            ++html;
            continue;
        }
        if (*html == '&') {
            if (strncmp(html, "&nbsp;", 6) == 0) {
                netstack_append_char(output, output_size, &index, ' ');
                html += 6;
                continue;
            }
            if (strncmp(html, "&amp;", 5) == 0) {
                netstack_append_char(output, output_size, &index, '&');
                html += 5;
                continue;
            }
            if (strncmp(html, "&lt;", 4) == 0) {
                netstack_append_char(output, output_size, &index, '<');
                html += 4;
                continue;
            }
            if (strncmp(html, "&gt;", 4) == 0) {
                netstack_append_char(output, output_size, &index, '>');
                html += 4;
                continue;
            }
            if (strncmp(html, "&quot;", 6) == 0) {
                netstack_append_char(output, output_size, &index, '"');
                html += 6;
                continue;
            }
            if (strncmp(html, "&apos;", 6) == 0) {
                netstack_append_char(output, output_size, &index, '\'');
                html += 6;
                continue;
            }
        }
        if (*html == '\r') {
            ++html;
            continue;
        }
        if (*html == '\n') {
            netstack_append_char(output, output_size, &index, '\n');
            last_space = false;
            ++html;
            continue;
        }
        if (*html == ' ' || *html == '\t') {
            if (!last_space) {
                netstack_append_char(output, output_size, &index, ' ');
                last_space = true;
            }
            ++html;
            continue;
        }
        netstack_append_char(output, output_size, &index, *html++);
        last_space = false;
    }
}

static bool netstack_parse_url(const char *url, char *host, size_t host_size,
                               char *path, size_t path_size, uint16_t *port, bool *https) {
    const char *cursor = url;
    size_t host_len = 0;
    const char *path_start;

    *https = false;
    *port = 80;
    if (strncmp(cursor, "http://", 7) == 0) {
        cursor += 7;
    } else if (strncmp(cursor, "https://", 8) == 0) {
        cursor += 8;
        *https = true;
        *port = 443;
    } else {
        return false;
    }

    path_start = strchr(cursor, '/');
    if (path_start == NULL) {
        path_start = cursor + strlen(cursor);
    }
    host_len = (size_t)(path_start - cursor);
    if (host_len == 0 || host_len >= host_size) {
        return false;
    }
    memcpy(host, cursor, host_len);
    host[host_len] = '\0';

    {
        char *colon = strchr(host, ':');
        if (colon != NULL) {
            *port = (uint16_t)atoi(colon + 1);
            *colon = '\0';
        }
    }

    if (*path_start == '\0') {
        strcopy_limit(path, "/", path_size);
    } else {
        strcopy_limit(path, path_start, path_size);
    }
    return true;
}

static bool netstack_build_https_bridge_url(const char *url, char *bridge_url, size_t bridge_url_size) {
    static const char hex[] = "0123456789ABCDEF";
    size_t out = 0;

    strcopy_limit(bridge_url,
                  "http://" SNSX_HTTPS_BRIDGE_HOST ":" STRINGIFY(SNSX_HTTPS_BRIDGE_PORT) "/fetch?url=",
                  bridge_url_size);
    out = strlen(bridge_url);
    if (out + 1 >= bridge_url_size) {
        return false;
    }

    while (*url != '\0') {
        unsigned char ch = (unsigned char)*url++;
        bool unreserved =
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '_' || ch == '.' || ch == '~';

        if (unreserved) {
            if (out + 1 >= bridge_url_size) {
                return false;
            }
            bridge_url[out++] = (char)ch;
        } else {
            if (out + 3 >= bridge_url_size) {
                return false;
            }
            bridge_url[out++] = '%';
            bridge_url[out++] = hex[(ch >> 4) & 0x0F];
            bridge_url[out++] = hex[ch & 0x0F];
        }
    }

    bridge_url[out] = '\0';
    return true;
}

static bool netstack_build_dev_bridge_url(const char *path, char *bridge_url, size_t bridge_url_size) {
    bridge_url[0] = '\0';
    strcopy_limit(bridge_url,
                  "http://" SNSX_DEV_BRIDGE_HOST ":" STRINGIFY(SNSX_DEV_BRIDGE_PORT),
                  bridge_url_size);
    if (path == NULL || path[0] == '\0') {
        return true;
    }
    if (strlen(bridge_url) + strlen(path) + 2 >= bridge_url_size) {
        return false;
    }
    if (path[0] != '/') {
        strcat(bridge_url, "/");
    }
    strcat(bridge_url, path);
    return true;
}

static uint32_t netstack_parse_status_code(const char *status_line) {
    const char *cursor = strchr(status_line, ' ');
    if (cursor == NULL) {
        return 0;
    }
    while (*cursor == ' ') {
        ++cursor;
    }
    return (uint32_t)atoi(cursor);
}

static const char *netstack_find_header_value(const char *headers, const char *name) {
    const char *line = headers;
    size_t name_len = strlen(name);

    while (line != NULL && *line != '\0') {
        const char *line_end = netstack_find_text(line, "\r\n");
        if (strncmp(line, name, name_len) == 0 && line[name_len] == ':') {
            line += name_len + 1;
            while (*line == ' ') {
                ++line;
            }
            return line;
        }
        if (line_end == NULL) {
            break;
        }
        line = line_end + 2;
    }
    return NULL;
}

static bool netstack_tcp_send_stream(const uint8_t *dest_mac, uint32_t remote_ip,
                                     uint16_t local_port, uint16_t remote_port,
                                     uint32_t *seq, uint32_t remote_seq,
                                     const uint8_t *data, size_t data_len) {
    size_t sent = 0;

    while (sent < data_len) {
        size_t chunk = data_len - sent;
        size_t frame_len;
        if (chunk > NET_TCP_SEGMENT_MAX) {
            chunk = NET_TCP_SEGMENT_MAX;
        }
        frame_len = net_build_tcp_frame(g_net_tx_frame, dest_mac, g_netstack.client_ip, remote_ip,
                                        local_port, remote_port, *seq, remote_seq,
                                        NET_TCP_ACK | NET_TCP_PSH, data + sent, chunk);
        if (!net_frame_send(g_net_tx_frame, frame_len)) {
            return false;
        }
        *seq += (uint32_t)chunk;
        sent += chunk;
    }
    return true;
}

static bool netstack_http_request_text(const char *method, const char *url, const char *content_type,
                                       const uint8_t *body, size_t body_len,
                                       char *output, size_t output_size) {
    char host[64];
    char path[512];
    char rendered[BROWSER_PAGE_MAX];
    char bridge_url[NET_URL_MAX];
    uint8_t dest_mac[6];
    tcp_packet_view_t packet;
    uint32_t remote_ip = 0;
    uint32_t seq;
    uint32_t remote_seq;
    uint16_t local_port;
    uint16_t remote_port;
    size_t frame_len;
    size_t response_len = 0;
    bool https = false;
    bool have_data = false;
    bool handshake_ready = false;

    output[0] = '\0';
    browser_links_reset();
    if (!netstack_parse_url(url, host, sizeof(host), path, sizeof(path), &remote_port, &https)) {
        strcopy_limit(output, "Browser supports snsx:// pages, http:// URLs, and https:// via the SNSX bridge.", output_size);
        return false;
    }
    if (https) {
        if (strcmp(method, "GET") != 0 || body_len != 0) {
            strcopy_limit(output, "HTTPS bridge currently supports GET requests only.", output_size);
            return false;
        }
        if (!netstack_build_https_bridge_url(url, bridge_url, sizeof(bridge_url))) {
            strcopy_limit(output, "HTTPS bridge URL is too long for this SNSX browser build.", output_size);
            return false;
        }
        strcopy_limit(g_netstack.browser_status, "Requesting HTTPS page through the SNSX host bridge.",
                      sizeof(g_netstack.browser_status));
        if (netstack_http_request_text("GET", bridge_url, NULL, NULL, 0, output, output_size)) {
            strcopy_limit(g_netstack.last_host, host, sizeof(g_netstack.last_host));
            strcopy_limit(g_netstack.last_ip, "via bridge", sizeof(g_netstack.last_ip));
            strcopy_limit(g_netstack.browser_status, "Remote page fetched over HTTPS through the SNSX host bridge.",
                          sizeof(g_netstack.browser_status));
            return true;
        }
        strcopy_limit(output,
                      "SNSX could not reach the HTTPS bridge. On the host Mac, run `make https-bridge` and try again.",
                      output_size);
        return false;
    }
    if (!g_netstack.configured) {
        network_refresh(true);
    }
    if (!g_netstack.configured) {
        strcopy_limit(output, "SNSX is offline. Run CONNECT in Network Center first.", output_size);
        return false;
    }
    if (!netstack_resolve_dns_name(host, &remote_ip)) {
        network_refresh(true);
        if (!netstack_resolve_dns_name(host, &remote_ip)) {
            strcopy_limit(output, "DNS lookup failed for that host.", output_size);
            return false;
        }
    }
    if (!netstack_next_hop_mac(remote_ip, dest_mac)) {
        strcopy_limit(output, "SNSX could not resolve a MAC address for that destination.", output_size);
        return false;
    }

    strcopy_limit(g_netstack.last_host, host, sizeof(g_netstack.last_host));
    net_ip_to_string(remote_ip, g_netstack.last_ip, sizeof(g_netstack.last_ip));
    strcopy_limit(g_netstack.browser_status, "Opening TCP session and requesting the remote page.",
                  sizeof(g_netstack.browser_status));

    g_net_http_request[0] = '\0';
    strcopy_limit(g_net_http_request, method, sizeof(g_net_http_request));
    strcat(g_net_http_request, " ");
    strcat(g_net_http_request, path);
    strcat(g_net_http_request, " HTTP/1.0\r\nHost: ");
    strcat(g_net_http_request, host);
    strcat(g_net_http_request, "\r\nUser-Agent: SNSX/0.3\r\nConnection: close\r\nAccept: text/html, text/plain, application/json\r\n");
    if (body != NULL && body_len > 0) {
        char length_text[24];
        strcat(g_net_http_request, "Content-Type: ");
        strcat(g_net_http_request, content_type != NULL ? content_type : "text/plain");
        strcat(g_net_http_request, "\r\nContent-Length: ");
        u32_to_dec((uint32_t)body_len, length_text);
        strcat(g_net_http_request, length_text);
        strcat(g_net_http_request, "\r\n");
    }
    strcat(g_net_http_request, "\r\n");

    local_port = g_netstack.next_port++;
    seq = netstack_random_next();
    net_frame_drain();
    frame_len = net_build_tcp_frame(g_net_tx_frame, dest_mac, g_netstack.client_ip, remote_ip,
                                    local_port, remote_port, seq, 0, NET_TCP_SYN, NULL, 0);
    if (!net_frame_send(g_net_tx_frame, frame_len)) {
        strcopy_limit(output, "TCP SYN transmit failed.", output_size);
        return false;
    }

    for (int wait_cycle = 0; wait_cycle < 220; ++wait_cycle) {
        if (!net_frame_receive(g_net_rx_frame, &frame_len, 25)) {
            continue;
        }
        if (!net_parse_tcp_packet(g_net_rx_frame, frame_len, &packet)) {
            continue;
        }
        if (packet.src_ip != remote_ip || packet.dst_ip != g_netstack.client_ip ||
            packet.src_port != remote_port || packet.dst_port != local_port) {
            continue;
        }
        if ((packet.flags & (NET_TCP_SYN | NET_TCP_ACK)) == (NET_TCP_SYN | NET_TCP_ACK) &&
            packet.ack == seq + 1) {
            remote_seq = packet.seq + 1;
            seq += 1;
            frame_len = net_build_tcp_frame(g_net_tx_frame, dest_mac, g_netstack.client_ip, remote_ip,
                                            local_port, remote_port, seq, remote_seq, NET_TCP_ACK, NULL, 0);
            net_frame_send(g_net_tx_frame, frame_len);
            if (!netstack_tcp_send_stream(dest_mac, remote_ip, local_port, remote_port, &seq, remote_seq,
                                          (const uint8_t *)g_net_http_request, strlen(g_net_http_request))) {
                strcopy_limit(output, "HTTP request transmit failed.", output_size);
                return false;
            }
            if (body != NULL && body_len > 0 &&
                !netstack_tcp_send_stream(dest_mac, remote_ip, local_port, remote_port, &seq, remote_seq, body, body_len)) {
                strcopy_limit(output, "HTTP body transmit failed.", output_size);
                return false;
            }
            handshake_ready = true;
            break;
        }
    }
    if (!handshake_ready) {
        strcopy_limit(output, "TCP handshake failed.", output_size);
        return false;
    }

    memset(g_net_http_buffer, 0, sizeof(g_net_http_buffer));
    for (int wait_cycle = 0; wait_cycle < 520; ++wait_cycle) {
        bool should_ack = false;

        if (!net_frame_receive(g_net_rx_frame, &frame_len, 25)) {
            if (have_data && wait_cycle > 60) {
                break;
            }
            continue;
        }
        if (!net_parse_tcp_packet(g_net_rx_frame, frame_len, &packet)) {
            continue;
        }
        if (packet.src_ip != remote_ip || packet.dst_ip != g_netstack.client_ip ||
            packet.src_port != remote_port || packet.dst_port != local_port) {
            continue;
        }
        if ((packet.flags & NET_TCP_RST) != 0) {
            strcopy_limit(output, "Remote host reset the TCP session.", output_size);
            return false;
        }
        if (packet.payload_len > 0) {
            if (packet.seq == remote_seq) {
                size_t copy = packet.payload_len;
                if (response_len + copy >= sizeof(g_net_http_buffer)) {
                    copy = sizeof(g_net_http_buffer) - response_len - 1;
                }
                memcpy(g_net_http_buffer + response_len, packet.payload, copy);
                response_len += copy;
                g_net_http_buffer[response_len] = '\0';
                remote_seq += (uint32_t)packet.payload_len;
                have_data = true;
            }
            should_ack = true;
        }
        if ((packet.flags & NET_TCP_FIN) != 0) {
            remote_seq += 1;
            should_ack = true;
        }
        if (should_ack) {
            frame_len = net_build_tcp_frame(g_net_tx_frame, dest_mac, g_netstack.client_ip, remote_ip,
                                            local_port, remote_port, seq, remote_seq, NET_TCP_ACK, NULL, 0);
            net_frame_send(g_net_tx_frame, frame_len);
        }
        if ((packet.flags & NET_TCP_FIN) != 0) {
            break;
        }
    }

    if (!have_data) {
        strcopy_limit(output, "The remote host did not return any HTTP payload.", output_size);
        return false;
    }

    {
        char status_line[96] = "HTTP response";
        const char *headers_end = netstack_find_text(g_net_http_buffer, "\r\n\r\n");
        const char *body = headers_end != NULL ? headers_end + 4 : g_net_http_buffer;
        const char *line_end = netstack_find_text(g_net_http_buffer, "\r\n");
        const char *location = headers_end != NULL ? netstack_find_header_value(g_net_http_buffer, "Location") : NULL;
        const char *content_type_header = headers_end != NULL ? netstack_find_header_value(g_net_http_buffer, "Content-Type") : NULL;
        uint32_t status_code = 0;
        size_t out_index = 0;

        if (line_end != NULL) {
            size_t length = (size_t)(line_end - g_net_http_buffer);
            if (length >= sizeof(status_line)) {
                length = sizeof(status_line) - 1;
            }
            memcpy(status_line, g_net_http_buffer, length);
            status_line[length] = '\0';
        }
        status_code = netstack_parse_status_code(status_line);

        if (status_code >= 300 && status_code < 400 && location != NULL && strcmp(method, "GET") == 0) {
            char location_text[NET_URL_MAX];
            size_t location_len = 0;
            while (location[location_len] != '\0' && location[location_len] != '\r' && location[location_len] != '\n' &&
                   location_len + 1 < sizeof(location_text)) {
                location_text[location_len] = location[location_len];
                ++location_len;
            }
            location_text[location_len] = '\0';
            if (location_text[0] != '\0' && strcmp(location_text, url) != 0) {
                return netstack_http_request_text("GET", location_text, NULL, NULL, 0, output, output_size);
            }
        }

        if (headers_end != NULL && content_type_header != NULL &&
            (strncmp(content_type_header, "text/html", 9) == 0 || strncmp(content_type_header, "application/xhtml", 17) == 0)) {
            netstack_render_html_body(body, rendered, sizeof(rendered));
            browser_extract_html_links(body);
            browser_extract_text_links(rendered);
        } else {
            strcopy_limit(rendered, body, sizeof(rendered));
            browser_extract_text_links(rendered);
        }
        netstack_append_text(output, output_size, &out_index, status_line);
        netstack_append_text(output, output_size, &out_index, "\nHost: ");
        netstack_append_text(output, output_size, &out_index, host);
        netstack_append_text(output, output_size, &out_index, " (");
        netstack_append_text(output, output_size, &out_index, g_netstack.last_ip);
        netstack_append_text(output, output_size, &out_index, ")\n\n");
        netstack_append_text(output, output_size, &out_index, rendered);
    }
    browser_append_links_summary(output, output_size);
    strcopy_limit(g_netstack.browser_status, "Remote page fetched over plain HTTP.", sizeof(g_netstack.browser_status));
    return true;
}

static bool netstack_http_fetch_text(const char *url, char *output, size_t output_size) {
    return netstack_http_request_text("GET", url, NULL, NULL, 0, output, output_size);
}

static bool network_connect_wired(void) {
    EFI_SIMPLE_NETWORK_PROTOCOL *network = g_network.protocol;

    if (g_network.native_available) {
        if (!e1000_initialize()) {
            return false;
        }
        g_network.started = true;
        g_network.initialized = true;
        g_network.internet_services_ready = true;
        g_network.media_present_supported = g_e1000.link_up;
        g_network.media_present = g_e1000.link_up || g_e1000.initialized;
        if (!g_netstack.configured) {
            if (!netstack_renew_dhcp()) {
                netstack_apply_virtualbox_nat_profile();
            }
        }
        g_network.dhcp_ready = g_netstack.configured && !g_netstack.static_profile;
        g_network.internet_ready = g_netstack.configured;
        netstack_sync_strings();
        return g_network.internet_ready;
    }

    if (network == NULL || network->mode == NULL) {
        return false;
    }
    if (network->mode->state == 0 && network->start != NULL) {
        if (network->start(network) == EFI_SUCCESS) {
            g_network.started = true;
        }
    } else if (network->mode->state >= 1) {
        g_network.started = true;
    }

    if (network->mode->state < 2 && network->initialize != NULL) {
        if (network->initialize(network, 0, 0) == EFI_SUCCESS) {
            g_network.initialized = true;
        }
    } else if (network->mode->state >= 2) {
        g_network.initialized = true;
    }
    if (g_network.initialized && network->receive_filters != NULL) {
        network->receive_filters(network, 0x00000003u, 0, 0, 0, NULL);
    }

    if (network->get_status != NULL) {
        uint32_t interrupt_status = 0;
        void *tx_buf = NULL;
        network->get_status(network, &interrupt_status, &tx_buf);
        (void)interrupt_status;
        (void)tx_buf;
    }

    if (network->mode != NULL) {
        g_network.mode_state = network->mode->state;
        g_network.media_present = network->mode->media_present;
        g_network.media_present_supported = network->mode->media_present_supported;
    }
    g_network.internet_services_ready = g_network.started && g_network.initialized;
    if (g_network.internet_services_ready &&
        (!g_network.media_present_supported || g_network.media_present)) {
        if (!g_netstack.configured) {
            if (!netstack_renew_dhcp()) {
                netstack_apply_virtualbox_nat_profile();
            }
        }
    }
    g_network.dhcp_ready = g_netstack.configured && !g_netstack.static_profile;
    g_network.internet_ready = g_netstack.configured;
    netstack_sync_strings();
    return g_network.internet_ready;
}

static void network_apply_profile_status(bool connect_attempted) {
    bool wired_fallback =
        (g_desktop.network_preference == NETWORK_PREF_WIFI ||
         g_desktop.network_preference == NETWORK_PREF_HOTSPOT) &&
        g_network.available && !wifi_radio_available();

    g_network.preference = g_desktop.network_preference;
    g_network.wifi_profile_ready = g_desktop.wifi_ssid[0] != '\0';
    g_network.hotspot_profile_ready = g_desktop.hotspot_name[0] != '\0';
    g_network.dhcp_ready = g_netstack.configured && !g_netstack.static_profile;
    strcopy_limit(g_network.hostname, g_desktop.hostname, sizeof(g_network.hostname));
    strcopy_limit(g_network.preferred_transport, desktop_network_mode_name(), sizeof(g_network.preferred_transport));
    netstack_sync_strings();

    if (g_desktop.network_preference == NETWORK_PREF_WIFI) {
        strcopy_limit(g_network.profile, "Wi-Fi profile: ", sizeof(g_network.profile));
        strcat(g_network.profile, g_desktop.wifi_ssid);
        if (wired_fallback) {
            strcat(g_network.profile, " (wired fallback)");
        } else if (!g_desktop.wifi_enabled) {
            strcopy_limit(g_network.status,
                          "Wi-Fi mode is selected, but the SNSX Wi-Fi switch is turned off.",
                          sizeof(g_network.status));
            strcopy_limit(g_network.internet,
                          "Turn Wi-Fi on inside the SNSX Wi-Fi dashboard to activate that profile.",
                          sizeof(g_network.internet));
            g_network.internet_ready = false;
            return;
        }
        if (!wired_fallback) {
            strcopy_limit(g_network.status,
                          "Wi-Fi is selected, but this VirtualBox guest only exposes Ethernet to SNSX.",
                          sizeof(g_network.status));
            strcopy_limit(g_network.internet,
                          "Nearby hotspot discovery needs a guest-visible Wi-Fi adapter. Host Wi-Fi can still back guest Ethernet once SNSX has a native NIC path.",
                          sizeof(g_network.internet));
            g_network.internet_ready = false;
            return;
        }
    }

    if (g_desktop.network_preference == NETWORK_PREF_HOTSPOT) {
        strcopy_limit(g_network.profile, "Hotspot profile: ", sizeof(g_network.profile));
        strcat(g_network.profile, g_desktop.hotspot_name);
        if (wired_fallback) {
            strcat(g_network.profile, " (wired fallback)");
        } else if (!g_desktop.wifi_enabled) {
            strcopy_limit(g_network.status,
                          "Hotspot mode is configured, but the SNSX wireless switch is turned off.",
                          sizeof(g_network.status));
            strcopy_limit(g_network.internet,
                          "Turn Wi-Fi back on to prepare the hotspot profile.",
                          sizeof(g_network.internet));
            g_network.internet_ready = false;
            return;
        }
        if (!wired_fallback) {
            strcopy_limit(g_network.status,
                          "Hotspot mode is configured, but SNSX does not yet provide a software AP backend.",
                          sizeof(g_network.status));
            strcopy_limit(g_network.internet,
                          "Hotspot routing will become available after the SNSX networking stack grows beyond firmware transport.",
                          sizeof(g_network.internet));
            g_network.internet_ready = false;
            return;
        }
    }

    strcopy_limit(g_network.profile, "Host ", sizeof(g_network.profile));
    strcat(g_network.profile, g_desktop.hostname);
    strcat(g_network.profile, " using ");
    strcat(g_network.profile, g_network.available ? g_network.transport : "firmware network discovery");

    if (!g_network.available) {
        if (g_netdiag.pci_network_handles > 0) {
            strcopy_limit(g_network.status,
                          "SNSX found a PCI network controller, but firmware exposed no usable UEFI network protocol.",
                          sizeof(g_network.status));
            strcopy_limit(g_network.internet,
                          "SNSX needs a native NIC driver on this ARM64 VirtualBox target before DHCP, DNS, and the browser can reach the outside network.",
                          sizeof(g_network.internet));
        } else {
            strcopy_limit(g_network.status, "No firmware network protocol or PCI network controller detected.", sizeof(g_network.status));
            strcopy_limit(g_network.internet, "SNSX cannot reach the outside network until the VM exposes network hardware or firmware services.", sizeof(g_network.internet));
        }
        return;
    }

    if (g_netstack.configured) {
        if (g_netstack.static_profile) {
            strcopy_limit(g_network.status,
                          "Wired Ethernet is initialized and SNSX is using the VirtualBox NAT fallback address profile.",
                          sizeof(g_network.status));
            strcopy_limit(g_network.internet,
                          "SNSX can reach host-backed internet services through VirtualBox NAT while the host Mac stays on Wi-Fi or Ethernet.",
                          sizeof(g_network.internet));
        } else {
            strcopy_limit(g_network.status, "Wired Ethernet is initialized and DHCP supplied an IPv4 lease.",
                          sizeof(g_network.status));
            strcopy_limit(g_network.internet,
                          "SNSX can resolve DNS, browse plain HTTP directly, and reach HTTPS plus Code Studio through the host bridges when they are running.",
                          sizeof(g_network.internet));
        }
        return;
    }

    if (g_network.internet_services_ready) {
        strcopy_limit(g_network.status, "The wired adapter is initialized, but SNSX does not have a DHCP lease yet.",
                      sizeof(g_network.status));
        strcopy_limit(g_network.internet,
                      "Run CONNECT to retry DHCP on the wired LAN path.",
                      sizeof(g_network.internet));
        return;
    }

    if (connect_attempted) {
        strcopy_limit(g_network.status, "SNSX attempted to initialize the wired adapter, but the firmware kept it offline.",
                      sizeof(g_network.status));
        strcopy_limit(g_network.internet,
                      "Retry CONNECT after the VM finishes exposing its network device.",
                      sizeof(g_network.internet));
        return;
    }

    if (g_network.media_present) {
        strcopy_limit(g_network.status, "A wired link is visible. Press CONNECT to initialize the firmware adapter.",
                      sizeof(g_network.status));
        strcopy_limit(g_network.internet,
                      "The cable path looks ready, but the adapter has not been brought online yet.",
                      sizeof(g_network.internet));
        return;
    }

    strcopy_limit(g_network.status, "Network adapter detected. Wired link is not active yet.", sizeof(g_network.status));
    strcopy_limit(g_network.internet,
                  "VirtualBox networking is present, but SNSX has not confirmed a live Ethernet path yet.",
                  sizeof(g_network.internet));
}

static void network_refresh(bool try_connect) {
    EFI_SIMPLE_NETWORK_PROTOCOL *network = NULL;
    bool connect_attempted = false;
    EFI_MAC_ADDRESS mac_text;
    uint8_t previous_mac[6];
    bool previous_configured = g_netstack.configured;

    memset(previous_mac, 0, sizeof(previous_mac));
    memcpy(previous_mac, g_netstack.client_mac, sizeof(previous_mac));

    memset(&g_network, 0, sizeof(g_network));
    network_collect_diagnostics();
    strcopy_limit(g_network.mac, "Unavailable", sizeof(g_network.mac));
    strcopy_limit(g_network.permanent_mac, "Unavailable", sizeof(g_network.permanent_mac));
    strcopy_limit(g_network.driver_name, "None", sizeof(g_network.driver_name));
    strcopy_limit(g_network.transport, g_netdiag.pci_network_handles > 0 ?
                  "PCI network controller detected" : "Unavailable",
                  sizeof(g_network.transport));
    strcopy_limit(g_network.preferred_transport, desktop_network_mode_name(), sizeof(g_network.preferred_transport));
    strcopy_limit(g_network.hostname, g_desktop.hostname, sizeof(g_network.hostname));
    strcopy_limit(g_network.profile, "No active network profile.", sizeof(g_network.profile));
    strcopy_limit(g_network.internet, g_netdiag.pci_network_handles > 0 ?
                  "SNSX sees a PCI NIC, but it still needs a native driver before DHCP can run." :
                  "Internet stack is waiting on DHCP and the wired firmware adapter.",
                  sizeof(g_network.internet));
    strcopy_limit(g_network.capabilities,
                  g_netdiag.pci_network_handles > 0 ?
                  "SNSX found PCI network hardware. The current blocker is firmware transport: no usable UEFI network protocol is exposed yet." :
                  "Wired LAN supports DHCP, IPv4, DNS, and plain HTTP once firmware transport is exposed. Wi-Fi and Bluetooth still need device drivers.",
                  sizeof(g_network.capabilities));
    strcopy_limit(g_network.status,
                  g_netdiag.pci_network_handles > 0 ?
                  "PCI network controller detected, but no firmware network protocol is available." :
                  "No firmware network protocol detected.",
                  sizeof(g_network.status));
    netstack_sync_strings();

    network = network_locate_protocol();
    if (network != NULL && network->mode != NULL) {
        g_network.available = true;
        g_network.protocol = network;
        strcopy_limit(g_network.driver_name, "UEFI SNP", sizeof(g_network.driver_name));
        g_network.media_present = network->mode->media_present;
        g_network.media_present_supported = network->mode->media_present_supported;
        g_network.mode_state = network->mode->state;
        g_network.max_packet_size = network->mode->max_packet_size;
        g_network.if_type = network->mode->if_type;
        g_network.hw_address_size = network->mode->hw_address_size;
        if (network->mode->hw_address_size >= 6 &&
            memcmp(g_netstack.client_mac, network->mode->current_address.addr, 6) != 0) {
            netstack_reset();
            net_copy_mac(g_netstack.client_mac, network->mode->current_address.addr);
        }
        format_mac_address(&network->mode->current_address, network->mode->hw_address_size,
                           g_network.mac, sizeof(g_network.mac));
        format_mac_address(&network->mode->permanent_address, network->mode->hw_address_size,
                           g_network.permanent_mac, sizeof(g_network.permanent_mac));

        if (network->mode->if_type == 1) {
            strcopy_limit(g_network.transport, "Ethernet / LAN (VirtualBox NAT)", sizeof(g_network.transport));
        } else {
            strcopy_limit(g_network.transport, "Firmware-managed network adapter", sizeof(g_network.transport));
        }

        if (network->mode->state >= 1) {
            g_network.started = true;
        }
        if (network->mode->state >= 2) {
            g_network.initialized = true;
        }
        g_network.internet_services_ready = g_network.started && g_network.initialized;
        if (try_connect &&
            (g_desktop.network_preference == NETWORK_PREF_AUTO ||
             g_desktop.network_preference == NETWORK_PREF_ETHERNET ||
             ((g_desktop.network_preference == NETWORK_PREF_WIFI ||
               g_desktop.network_preference == NETWORK_PREF_HOTSPOT) &&
              !wifi_radio_available()))) {
            connect_attempted = true;
            network_connect_wired();
        }
    } else {
        if (e1000_probe()) {
            e1000_read_mac(g_e1000.mac);
            if (!e1000_mac_is_valid(g_e1000.mac)) {
                e1000_build_fallback_mac(g_e1000.mac);
            }
            if (!previous_configured || memcmp(previous_mac, g_e1000.mac, 6) != 0) {
                netstack_reset();
            }
            g_network.available = true;
            g_network.native_available = true;
            strcopy_limit(g_network.driver_name, "SNSX 82540EM", sizeof(g_network.driver_name));
            strcopy_limit(g_network.transport, "Native Intel 82540EM Ethernet", sizeof(g_network.transport));
            strcopy_limit(g_network.capabilities,
                          "SNSX native 82540EM Ethernet driver is active on this target. DHCP, IPv4, DNS, and plain HTTP can run without UEFI SNP.",
                          sizeof(g_network.capabilities));
            g_network.media_present_supported = g_e1000.link_up;
            g_network.hw_address_size = 6;
            g_network.max_packet_size = 1518;
            g_network.started = g_e1000.initialized;
            g_network.initialized = g_e1000.initialized;
            g_network.internet_services_ready = g_e1000.initialized;
            if (try_connect &&
                (g_desktop.network_preference == NETWORK_PREF_AUTO ||
                 g_desktop.network_preference == NETWORK_PREF_ETHERNET ||
                 ((g_desktop.network_preference == NETWORK_PREF_WIFI ||
                   g_desktop.network_preference == NETWORK_PREF_HOTSPOT) &&
                  !wifi_radio_available()))) {
                connect_attempted = true;
                network_connect_wired();
            }
            if (g_e1000.present) {
                memset(&mac_text, 0, sizeof(mac_text));
                memcpy(mac_text.addr, g_e1000.mac, 6);
                format_mac_address(&mac_text, 6, g_network.mac, sizeof(g_network.mac));
                format_mac_address(&mac_text, 6, g_network.permanent_mac, sizeof(g_network.permanent_mac));
                net_copy_mac(g_netstack.client_mac, g_e1000.mac);
                g_network.media_present = g_e1000.link_up || g_e1000.initialized;
            }
        } else {
            netstack_reset();
        }
    }

    network_apply_profile_status(connect_attempted);
}

static void network_init(void) {
    network_refresh(true);
}

static uint32_t gfx_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    if (g_gfx.pixel_format == PixelRedGreenBlueReserved8BitPerColor) {
        return (uint32_t)red | ((uint32_t)green << 8) | ((uint32_t)blue << 16);
    }
    return (uint32_t)blue | ((uint32_t)green << 8) | ((uint32_t)red << 16);
}

static uint32_t theme_color(size_t role) {
    size_t theme = g_desktop.theme;
    if (theme >= ARRAY_SIZE(g_theme_palette)) {
        theme = 0;
    }
    if (role >= THEME_ROLE_COUNT) {
        role = 0;
    }
    return gfx_rgb(g_theme_palette[theme][role][0],
                   g_theme_palette[theme][role][1],
                   g_theme_palette[theme][role][2]);
}

static const char *desktop_theme_name(void) {
    size_t theme = g_desktop.theme;
    if (theme >= ARRAY_SIZE(g_theme_names)) {
        theme = 0;
    }
    return g_theme_names[theme];
}

static void gfx_fill_vertical_gradient(int x, int y, int width, int height,
                                       uint32_t top_r, uint32_t top_g, uint32_t top_b,
                                       uint32_t bottom_r, uint32_t bottom_g, uint32_t bottom_b) {
    if (height <= 0) {
        return;
    }
    for (int row = 0; row < height; ++row) {
        int red = (int)top_r + (((int)bottom_r - (int)top_r) * row) / height;
        int green = (int)top_g + (((int)bottom_g - (int)top_g) * row) / height;
        int blue = (int)top_b + (((int)bottom_b - (int)top_b) * row) / height;
        gfx_fill_rect(x, y + row, width, 1, gfx_rgb((uint8_t)red, (uint8_t)green, (uint8_t)blue));
    }
}

static void gfx_fill_theme_gradient(int x, int y, int width, int height, size_t top_role, size_t bottom_role) {
    size_t theme = g_desktop.theme;
    if (theme >= ARRAY_SIZE(g_theme_palette)) {
        theme = 0;
    }
    gfx_fill_vertical_gradient(
        x, y, width, height,
        g_theme_palette[theme][top_role][0], g_theme_palette[theme][top_role][1], g_theme_palette[theme][top_role][2],
        g_theme_palette[theme][bottom_role][0], g_theme_palette[theme][bottom_role][1], g_theme_palette[theme][bottom_role][2]);
}

static void gfx_fill_rect(int x, int y, int width, int height, uint32_t color) {
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + width;
    int y1 = y + height;

    if (!g_gfx.available || width <= 0 || height <= 0) {
        return;
    }
    if (x1 > (int)g_gfx.width) {
        x1 = (int)g_gfx.width;
    }
    if (y1 > (int)g_gfx.height) {
        y1 = (int)g_gfx.height;
    }
    for (int py = y0; py < y1; ++py) {
        uint32_t *row = g_gfx.drawbuffer + (size_t)py * g_gfx.draw_stride;
        for (int px = x0; px < x1; ++px) {
            row[px] = color;
        }
    }
}

static void gfx_present(void) {
    if (!g_gfx.available || g_gfx.framebuffer == NULL || g_gfx.drawbuffer == NULL ||
        g_gfx.drawbuffer == g_gfx.framebuffer) {
        return;
    }

    for (uint32_t py = 0; py < g_gfx.height; ++py) {
        memcpy(g_gfx.framebuffer + (size_t)py * g_gfx.stride,
               g_gfx.drawbuffer + (size_t)py * g_gfx.draw_stride,
               (size_t)g_gfx.width * sizeof(uint32_t));
    }
}

static void gfx_draw_rect(int x, int y, int width, int height, int thickness, uint32_t color) {
    gfx_fill_rect(x, y, width, thickness, color);
    gfx_fill_rect(x, y + height - thickness, width, thickness, color);
    gfx_fill_rect(x, y, thickness, height, color);
    gfx_fill_rect(x + width - thickness, y, thickness, height, color);
}

static void gfx_draw_panel(int x, int y, int width, int height, uint32_t fill, uint32_t border) {
    gfx_fill_rect(x + 14, y + 16, width, height, theme_color(THEME_ROLE_DOCK));
    gfx_fill_rect(x + 6, y + 8, width, height, gfx_rgb(8, 13, 26));
    gfx_fill_rect(x, y, width, height, fill);
    gfx_fill_rect(x + 2, y + 2, width - 4, 3, gfx_rgb(255, 255, 255));
    gfx_draw_rect(x, y, width, height, 2, border);
}

static uint8_t font_row(char ch, int row) {
    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 'a' + 'A');
    }
    switch (ch) {
        case 'A': { static const uint8_t g[7] = {14, 17, 17, 31, 17, 17, 17}; return g[row]; }
        case 'B': { static const uint8_t g[7] = {30, 17, 17, 30, 17, 17, 30}; return g[row]; }
        case 'C': { static const uint8_t g[7] = {14, 17, 16, 16, 16, 17, 14}; return g[row]; }
        case 'D': { static const uint8_t g[7] = {30, 17, 17, 17, 17, 17, 30}; return g[row]; }
        case 'E': { static const uint8_t g[7] = {31, 16, 16, 30, 16, 16, 31}; return g[row]; }
        case 'F': { static const uint8_t g[7] = {31, 16, 16, 30, 16, 16, 16}; return g[row]; }
        case 'G': { static const uint8_t g[7] = {14, 17, 16, 23, 17, 17, 14}; return g[row]; }
        case 'H': { static const uint8_t g[7] = {17, 17, 17, 31, 17, 17, 17}; return g[row]; }
        case 'I': { static const uint8_t g[7] = {14, 4, 4, 4, 4, 4, 14}; return g[row]; }
        case 'J': { static const uint8_t g[7] = {1, 1, 1, 1, 17, 17, 14}; return g[row]; }
        case 'K': { static const uint8_t g[7] = {17, 18, 20, 24, 20, 18, 17}; return g[row]; }
        case 'L': { static const uint8_t g[7] = {16, 16, 16, 16, 16, 16, 31}; return g[row]; }
        case 'M': { static const uint8_t g[7] = {17, 27, 21, 17, 17, 17, 17}; return g[row]; }
        case 'N': { static const uint8_t g[7] = {17, 25, 21, 19, 17, 17, 17}; return g[row]; }
        case 'O': { static const uint8_t g[7] = {14, 17, 17, 17, 17, 17, 14}; return g[row]; }
        case 'P': { static const uint8_t g[7] = {30, 17, 17, 30, 16, 16, 16}; return g[row]; }
        case 'Q': { static const uint8_t g[7] = {14, 17, 17, 17, 21, 18, 13}; return g[row]; }
        case 'R': { static const uint8_t g[7] = {30, 17, 17, 30, 20, 18, 17}; return g[row]; }
        case 'S': { static const uint8_t g[7] = {15, 16, 16, 14, 1, 1, 30}; return g[row]; }
        case 'T': { static const uint8_t g[7] = {31, 4, 4, 4, 4, 4, 4}; return g[row]; }
        case 'U': { static const uint8_t g[7] = {17, 17, 17, 17, 17, 17, 14}; return g[row]; }
        case 'V': { static const uint8_t g[7] = {17, 17, 17, 17, 17, 10, 4}; return g[row]; }
        case 'W': { static const uint8_t g[7] = {17, 17, 17, 17, 21, 27, 17}; return g[row]; }
        case 'X': { static const uint8_t g[7] = {17, 17, 10, 4, 10, 17, 17}; return g[row]; }
        case 'Y': { static const uint8_t g[7] = {17, 17, 10, 4, 4, 4, 4}; return g[row]; }
        case 'Z': { static const uint8_t g[7] = {31, 1, 2, 4, 8, 16, 31}; return g[row]; }
        case '0': { static const uint8_t g[7] = {14, 17, 19, 21, 25, 17, 14}; return g[row]; }
        case '1': { static const uint8_t g[7] = {4, 12, 4, 4, 4, 4, 14}; return g[row]; }
        case '2': { static const uint8_t g[7] = {14, 17, 1, 2, 4, 8, 31}; return g[row]; }
        case '3': { static const uint8_t g[7] = {30, 1, 1, 14, 1, 1, 30}; return g[row]; }
        case '4': { static const uint8_t g[7] = {2, 6, 10, 18, 31, 2, 2}; return g[row]; }
        case '5': { static const uint8_t g[7] = {31, 16, 16, 30, 1, 1, 30}; return g[row]; }
        case '6': { static const uint8_t g[7] = {14, 16, 16, 30, 17, 17, 14}; return g[row]; }
        case '7': { static const uint8_t g[7] = {31, 1, 2, 4, 8, 8, 8}; return g[row]; }
        case '8': { static const uint8_t g[7] = {14, 17, 17, 14, 17, 17, 14}; return g[row]; }
        case '9': { static const uint8_t g[7] = {14, 17, 17, 15, 1, 1, 14}; return g[row]; }
        case '.': { static const uint8_t g[7] = {0, 0, 0, 0, 0, 12, 12}; return g[row]; }
        case ',': { static const uint8_t g[7] = {0, 0, 0, 0, 12, 12, 8}; return g[row]; }
        case ':': { static const uint8_t g[7] = {0, 12, 12, 0, 12, 12, 0}; return g[row]; }
        case ';': { static const uint8_t g[7] = {0, 12, 12, 0, 12, 12, 8}; return g[row]; }
        case '-': { static const uint8_t g[7] = {0, 0, 0, 31, 0, 0, 0}; return g[row]; }
        case '_': { static const uint8_t g[7] = {0, 0, 0, 0, 0, 0, 31}; return g[row]; }
        case '/': { static const uint8_t g[7] = {1, 2, 4, 4, 8, 16, 0}; return g[row]; }
        case '\\': { static const uint8_t g[7] = {16, 8, 4, 4, 2, 1, 0}; return g[row]; }
        case '(': { static const uint8_t g[7] = {2, 4, 8, 8, 8, 4, 2}; return g[row]; }
        case ')': { static const uint8_t g[7] = {8, 4, 2, 2, 2, 4, 8}; return g[row]; }
        case '[': { static const uint8_t g[7] = {14, 8, 8, 8, 8, 8, 14}; return g[row]; }
        case ']': { static const uint8_t g[7] = {14, 2, 2, 2, 2, 2, 14}; return g[row]; }
        case '+': { static const uint8_t g[7] = {0, 4, 4, 31, 4, 4, 0}; return g[row]; }
        case '*': { static const uint8_t g[7] = {0, 10, 4, 31, 4, 10, 0}; return g[row]; }
        case '=': { static const uint8_t g[7] = {0, 31, 0, 31, 0, 0, 0}; return g[row]; }
        case '>': { static const uint8_t g[7] = {16, 8, 4, 2, 4, 8, 16}; return g[row]; }
        case '<': { static const uint8_t g[7] = {1, 2, 4, 8, 4, 2, 1}; return g[row]; }
        case '!': { static const uint8_t g[7] = {4, 4, 4, 4, 4, 0, 4}; return g[row]; }
        case '?': { static const uint8_t g[7] = {14, 17, 1, 2, 4, 0, 4}; return g[row]; }
        case '\'': { static const uint8_t g[7] = {4, 4, 2, 0, 0, 0, 0}; return g[row]; }
        case '"': { static const uint8_t g[7] = {10, 10, 0, 0, 0, 0, 0}; return g[row]; }
        case '#': { static const uint8_t g[7] = {10, 31, 10, 10, 31, 10, 0}; return g[row]; }
        case '&': { static const uint8_t g[7] = {12, 18, 20, 8, 21, 18, 13}; return g[row]; }
        case '|': { static const uint8_t g[7] = {4, 4, 4, 4, 4, 4, 4}; return g[row]; }
        case ' ': { static const uint8_t g[7] = {0, 0, 0, 0, 0, 0, 0}; return g[row]; }
        default: { static const uint8_t g[7] = {14, 17, 1, 2, 4, 0, 4}; return g[row]; }
    }
}

static void gfx_draw_char(int x, int y, char ch, int scale, uint32_t color) {
    for (int row = 0; row < 7; ++row) {
        uint8_t bits = font_row(ch, row);
        for (int col = 0; col < 5; ++col) {
            if ((bits & (1u << (4 - col))) == 0) {
                continue;
            }
            gfx_fill_rect(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

static void gfx_draw_text(int x, int y, const char *text, int scale, uint32_t color) {
    int cx = x;
    int cy = y;
    int advance = 6 * scale;

    while (*text != '\0') {
        char ch = *text++;
        if (ch == '\n') {
            cx = x;
            cy += 8 * scale;
            continue;
        }
        if (ch != '\r') {
            gfx_draw_char(cx, cy, ch, scale, color);
            cx += advance;
        }
    }
}

static void gfx_draw_text_block(int x, int y, int width, int height, const char *text,
                                size_t length, int scale, uint32_t color,
                                int start_line, int cursor_index) {
    int cols = width / (6 * scale);
    int rows = height / (8 * scale);
    int line = 0;
    int col = 0;

    if (cols <= 0 || rows <= 0) {
        return;
    }

    for (size_t i = 0; i <= length; ++i) {
        if (cursor_index >= 0 && (int)i == cursor_index &&
            line >= start_line && line < start_line + rows) {
            gfx_fill_rect(x + col * 6 * scale, y + (line - start_line) * 8 * scale,
                          2, 7 * scale, color);
        }
        if (i == length) {
            break;
        }

        char ch = text[i];
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            line++;
            col = 0;
            continue;
        }
        if (col >= cols) {
            line++;
            col = 0;
        }
        if (line >= start_line && line < start_line + rows) {
            gfx_draw_char(x + col * 6 * scale, y + (line - start_line) * 8 * scale,
                          ch, scale, color);
        }
        col++;
    }
}

static int text_cursor_line(const uint8_t *buffer, size_t length, size_t cursor, int cols) {
    int line = 0;
    int col = 0;
    for (size_t i = 0; i < cursor && i < length; ++i) {
        char ch = (char)buffer[i];
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            line++;
            col = 0;
            continue;
        }
        if (col >= cols) {
            line++;
            col = 0;
        }
        col++;
    }
    return line;
}

static void ui_adjust_scroll(const uint8_t *buffer, size_t length, size_t cursor,
                             int cols, int rows, int *scroll_line) {
    int line = text_cursor_line(buffer, length, cursor, cols);
    if (line < *scroll_line) {
        *scroll_line = line;
    } else if (line >= *scroll_line + rows) {
        *scroll_line = line - rows + 1;
    }
    if (*scroll_line < 0) {
        *scroll_line = 0;
    }
}

static size_t text_index_from_position(const uint8_t *buffer, size_t length,
                                       int cols, int target_line, int target_col) {
    int line = 0;
    int col = 0;

    if (target_line < 0) {
        return 0;
    }
    if (target_col < 0) {
        target_col = 0;
    }

    for (size_t i = 0; i < length; ++i) {
        char ch = (char)buffer[i];
        if (line == target_line && col >= target_col) {
            return i;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            if (line == target_line) {
                return i;
            }
            line++;
            col = 0;
            continue;
        }
        if (col >= cols) {
            if (line == target_line) {
                return i;
            }
            line++;
            col = 0;
        }
        col++;
    }
    return length;
}

static bool point_in_rect(int px, int py, int x, int y, int width, int height) {
    return px >= x && py >= y && px < x + width && py < y + height;
}

static bool ui_event_has_pointer(const ui_event_t *event) {
    return event != NULL && (event->mouse_moved || event->mouse_clicked || event->mouse_released || event->mouse_down);
}

static int ui_event_pointer_x(const ui_event_t *event) {
    return ui_event_has_pointer(event) ? event->mouse_x : g_pointer.x;
}

static int ui_event_pointer_y(const ui_event_t *event) {
    return ui_event_has_pointer(event) ? event->mouse_y : g_pointer.y;
}

static bool ui_pointer_in_rect(const ui_event_t *event, int x, int y, int width, int height) {
    return point_in_rect(ui_event_pointer_x(event), ui_event_pointer_y(event), x, y, width, height);
}

static bool ui_pointer_activated(const ui_event_t *event, int x, int y, int width, int height) {
    if (!g_pointer.available || event == NULL) {
        return false;
    }
    if (!ui_pointer_in_rect(event, x, y, width, height)) {
        return false;
    }
    return event->mouse_clicked || event->mouse_released;
}

static void ui_draw_cursor(void) {
    if (g_pointer.available) {
        if (g_desktop.pointer_trail) {
            gfx_fill_rect(g_pointer.x + 3, g_pointer.y + 3, 3, 22, gfx_rgb(35, 50, 76));
            gfx_fill_rect(g_pointer.x + 3, g_pointer.y + 3, 16, 3, gfx_rgb(35, 50, 76));
        }
        gfx_fill_rect(g_pointer.x, g_pointer.y, 3, 22, gfx_rgb(255, 255, 255));
        gfx_fill_rect(g_pointer.x, g_pointer.y, 16, 3, gfx_rgb(255, 255, 255));
        gfx_fill_rect(g_pointer.x + 2, g_pointer.y + 2, 10, 10,
                      g_pointer.left_down ? theme_color(THEME_ROLE_WARM) : theme_color(THEME_ROLE_ACCENT));
    }
    gfx_present();
}

static void ui_wait_event(ui_event_t *event) {
    size_t idle_loops = 0;

    memset(event, 0, sizeof(*event));
    while (1) {
        EFI_EVENT events[INPUT_PROTOCOL_MAX * 2 + 2];
        enum {
            UI_WAIT_KEY = 1,
            UI_WAIT_KEY_EX = 2,
            UI_WAIT_POINTER = 3
        } sources[INPUT_PROTOCOL_MAX * 2 + 2];
        size_t count = 0;
        size_t ready = 0;

        if (input_try_read_key(&event->key)) {
            event->key_ready = true;
            return;
        }
        if (pointer_poll_event(event)) {
            return;
        }
        if (g_force_text_ui || !boot_diagnostics_confirmed()) {
            if (idle_loops > 0 && (idle_loops % 300) == 0) {
                input_recover_live_devices();
            }
            if (!boot_diagnostics_confirmed() && idle_loops >= 900) {
                g_force_text_ui = true;
                event->key_ready = true;
                event->key.scan_code = EFI_SCAN_ESC;
                return;
            }
            input_pause();
            idle_loops++;
            continue;
        }
        for (size_t i = 0; i < g_input.standard_count && count < ARRAY_SIZE(events); ++i) {
            EFI_SIMPLE_TEXT_INPUT_PROTOCOL *protocol = g_input.standard_protocols[i];
            if (protocol != NULL && protocol->wait_for_key != NULL) {
                events[count] = protocol->wait_for_key;
                sources[count++] = UI_WAIT_KEY;
            }
        }
        for (size_t i = 0; i < g_input.extended_count && count < ARRAY_SIZE(events); ++i) {
            EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *protocol = g_input.extended_protocols[i];
            if (protocol != NULL && protocol->wait_for_key_ex != NULL) {
                events[count] = protocol->wait_for_key_ex;
                sources[count++] = UI_WAIT_KEY_EX;
            }
        }
        if (g_pointer.kind == POINTER_KIND_ABSOLUTE &&
            g_pointer.absolute_protocol != NULL &&
            g_pointer.absolute_protocol->wait_for_input != NULL &&
            count < ARRAY_SIZE(events)) {
            events[count] = g_pointer.absolute_protocol->wait_for_input;
            sources[count++] = UI_WAIT_POINTER;
        } else if (g_pointer.kind == POINTER_KIND_SIMPLE &&
                   g_pointer.simple_protocol != NULL &&
                   g_pointer.simple_protocol->wait_for_input != NULL &&
                   count < ARRAY_SIZE(events)) {
            events[count] = g_pointer.simple_protocol->wait_for_input;
            sources[count++] = UI_WAIT_POINTER;
        }
        if (input_wait_for_events(events, count, &ready) && ready < count) {
            if ((sources[ready] == UI_WAIT_KEY || sources[ready] == UI_WAIT_KEY_EX) &&
                input_try_read_key(&event->key)) {
                event->key_ready = true;
                return;
            }
            if (sources[ready] == UI_WAIT_POINTER && pointer_poll_event(event)) {
                return;
            }
            continue;
        }
        input_pause();
    }
}

static void ui_draw_background(void) {
    gfx_fill_theme_gradient(0, 0, (int)g_gfx.width, (int)g_gfx.height,
                            THEME_ROLE_BG_TOP, THEME_ROLE_BG_BOTTOM);
    gfx_fill_rect(0, 0, (int)g_gfx.width, 58, theme_color(THEME_ROLE_TOPBAR));
    gfx_fill_rect(0, (int)g_gfx.height - 86, (int)g_gfx.width, 86, theme_color(THEME_ROLE_DOCK));
    gfx_fill_rect((int)g_gfx.width - 272, 84, 210, 186, theme_color(THEME_ROLE_ACCENT_STRONG));
    gfx_fill_rect((int)g_gfx.width - 240, 102, 126, 126, theme_color(THEME_ROLE_ACCENT));
    gfx_fill_rect(64, (int)g_gfx.height - 272, 244, 154, theme_color(THEME_ROLE_ACCENT_STRONG));
    gfx_fill_rect(96, (int)g_gfx.height - 250, 160, 76, theme_color(THEME_ROLE_WARM));
}

static void ui_draw_badge(int x, int y, const char *text, uint32_t fill, uint32_t text_color) {
    int width = (int)strlen(text) * 8 + 26;
    gfx_draw_panel(x, y, width, 26, fill, fill);
    gfx_draw_text(x + 12, y + 9, text, 1, text_color);
}

static void ui_draw_toggle(int x, int y, int width, int height, const char *label, bool enabled, bool focused) {
    int knob_w = height - 10;
    int knob_x = enabled ? x + width - knob_w - 6 : x + 6;
    uint32_t fill = enabled ? theme_color(THEME_ROLE_ACCENT) : theme_color(THEME_ROLE_PANEL_ALT);
    uint32_t border = enabled ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_BORDER);

    gfx_draw_text(x, y - 24, label, 2, theme_color(THEME_ROLE_TEXT));
    gfx_draw_panel(x, y, width, height, fill, border);
    gfx_fill_rect(knob_x, y + 5, knob_w, knob_w, theme_color(THEME_ROLE_PANEL));
    gfx_draw_rect(knob_x, y + 5, knob_w, knob_w, 2, theme_color(THEME_ROLE_TEXT_LIGHT));
    gfx_draw_text(x + width + 16, y + 10, enabled ? "ON" : "OFF", 1, theme_color(THEME_ROLE_TEXT));
    if (focused) {
        gfx_draw_rect(x - 6, y - 6, width + 12, height + 12, 3, theme_color(THEME_ROLE_ACCENT_STRONG));
    }
}

static void ui_draw_button(int x, int y, int width, int height, const char *label,
                           uint32_t fill, uint32_t border, uint32_t text_color) {
    int text_width = (int)strlen(label) * 12;
    int text_x = x + (width - text_width) / 2;

    if (text_x < x + 12) {
        text_x = x + 12;
    }
    gfx_draw_panel(x, y, width, height, fill, border);
    gfx_draw_text(text_x, y + height / 2 - 8, label, 2, text_color);
}

static void ui_draw_button_focus(int x, int y, int width, int height, const char *label,
                                 uint32_t fill, uint32_t border, uint32_t text_color, bool focused) {
    ui_draw_button(x, y, width, height, label, fill, border, text_color);
    if (focused) {
        gfx_draw_rect(x - 6, y - 6, width + 12, height + 12, 3, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_rect(x - 2, y - 2, width + 4, height + 4, 1, theme_color(THEME_ROLE_TEXT_LIGHT));
    }
}

static void ui_draw_top_bar(const char *title, const char *subtitle) {
    gfx_draw_text(28, 16, title, 3, theme_color(THEME_ROLE_TEXT_LIGHT));
    gfx_draw_text(30, 40, subtitle, 1, theme_color(THEME_ROLE_TEXT_LIGHT));
    gfx_draw_text((int)g_gfx.width - 280, 18, "SNSX DESKTOP", 2, theme_color(THEME_ROLE_TEXT_LIGHT));
    gfx_draw_text((int)g_gfx.width - 280, 42, "ARM64 VIRTUALBOX EDITION", 1, theme_color(THEME_ROLE_TEXT_LIGHT));
}

static void ui_draw_footer(const char *message) {
    gfx_draw_text(30, (int)g_gfx.height - 58, message, 1, theme_color(THEME_ROLE_TEXT_LIGHT));
    gfx_draw_text(30, (int)g_gfx.height - 36, g_desktop.status_line, 1, theme_color(THEME_ROLE_TEXT_LIGHT));
}

static void ui_draw_window(const char *title, const char *subtitle) {
    ui_draw_background();
    ui_draw_top_bar(title, subtitle);
    gfx_draw_panel(28, 78, (int)g_gfx.width - 56, (int)g_gfx.height - 168,
                   theme_color(THEME_ROLE_PANEL), theme_color(THEME_ROLE_BORDER));
    gfx_fill_rect(28, 78, (int)g_gfx.width - 56, 46, theme_color(THEME_ROLE_ACCENT_STRONG));
    gfx_fill_rect(28, 124, (int)g_gfx.width - 56, 4, theme_color(THEME_ROLE_WARM));
    gfx_draw_text(48, 94, title, 2, theme_color(THEME_ROLE_TEXT_LIGHT));
    gfx_draw_text(48, 114, subtitle, 1, theme_color(THEME_ROLE_TEXT_LIGHT));
    ui_draw_badge((int)g_gfx.width - 274, 90, desktop_theme_name(),
                  theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_TEXT));
    gfx_fill_rect((int)g_gfx.width - 88, 88, 32, 24, theme_color(THEME_ROLE_DANGER));
    gfx_draw_text((int)g_gfx.width - 78, 96, "X", 2, theme_color(THEME_ROLE_TEXT_LIGHT));
    gfx_draw_text(48, (int)g_gfx.height - 90, g_persist.status, 1, theme_color(THEME_ROLE_TEXT_SOFT));
}

static bool __attribute__((unused)) ui_close_button_hit(void) {
    return point_in_rect(g_pointer.x, g_pointer.y, (int)g_gfx.width - 88, 88, 32, 24);
}

static bool ui_close_button_event_hit(const ui_event_t *event) {
    return ui_pointer_activated(event, (int)g_gfx.width - 88, 88, 32, 24);
}

static bool ui_prompt_line(const char *title, const char *message, const char *seed,
                           char *output, size_t limit) {
    ui_event_t event;
    size_t length = 0;

    strcopy_limit(output, seed == NULL ? "" : seed, limit);
    length = strlen(output);

    while (1) {
        ui_draw_background();
        ui_draw_top_bar("SNSX PROMPT", "TYPE A VALUE AND PRESS ENTER");
        gfx_draw_panel((int)g_gfx.width / 2 - 250, (int)g_gfx.height / 2 - 110, 500, 220,
                       gfx_rgb(242, 246, 252), gfx_rgb(34, 54, 81));
        gfx_draw_text((int)g_gfx.width / 2 - 210, (int)g_gfx.height / 2 - 70, title, 2, gfx_rgb(29, 42, 63));
        gfx_draw_text((int)g_gfx.width / 2 - 210, (int)g_gfx.height / 2 - 34, message, 1, gfx_rgb(82, 96, 122));
        gfx_fill_rect((int)g_gfx.width / 2 - 210, (int)g_gfx.height / 2, 420, 56, gfx_rgb(255, 255, 255));
        gfx_draw_rect((int)g_gfx.width / 2 - 210, (int)g_gfx.height / 2, 420, 56, 2, gfx_rgb(58, 84, 120));
        gfx_draw_text((int)g_gfx.width / 2 - 194, (int)g_gfx.height / 2 + 18, output, 2, gfx_rgb(25, 38, 55));
        gfx_fill_rect((int)g_gfx.width / 2 - 194 + (int)length * 12, (int)g_gfx.height / 2 + 14, 3, 18, gfx_rgb(36, 122, 212));
        gfx_draw_text((int)g_gfx.width / 2 - 210, (int)g_gfx.height / 2 + 78,
                      "ENTER ACCEPTS  ESC CANCELS  BACKSPACE DELETES", 1, gfx_rgb(76, 90, 114));
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event)) {
            return false;
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_ESC) {
            return false;
        }
        if (event.key.unicode_char == '\r') {
            return true;
        }
        if (event.key.unicode_char == '\b') {
            if (length > 0) {
                output[--length] = '\0';
            }
            continue;
        }
        if (event.key.unicode_char >= 32 && event.key.unicode_char <= 126 && length + 1 < limit) {
            output[length++] = (char)event.key.unicode_char;
            output[length] = '\0';
        }
    }
}

static void ui_view_text_file(const char *path, const char *window_title) {
    ui_event_t event;
    char title[FS_PATH_MAX];
    int scroll = 0;
    arm_fs_entry_t *entry = fs_find(path);

    strcopy_limit(title, window_title, sizeof(title));
    if (entry == NULL || entry->type != ARM_FS_TEXT) {
        return;
    }
    strcopy_limit(g_last_opened_file, path, sizeof(g_last_opened_file));

    while (1) {
        int text_x = 56;
        int text_y = 146;
        int text_w = (int)g_gfx.width - 112;
        int text_h = (int)g_gfx.height - 246;
        int visible_rows = text_h / 16;
        ui_draw_window(title, path);
        gfx_draw_text_block(text_x, text_y, text_w, text_h, (const char *)entry->data,
                            entry->size, 2, gfx_rgb(28, 41, 61), scroll, -1);
        ui_draw_footer("UP/DOWN SCROLL  E EDIT  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event)) {
            return;
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_ESC) {
            return;
        }
        if (event.key.scan_code == EFI_SCAN_UP && scroll > 0) {
            scroll--;
        } else if (event.key.scan_code == EFI_SCAN_DOWN) {
            scroll++;
        } else if (event.key.scan_code == EFI_SCAN_PAGE_UP) {
            scroll -= visible_rows > 1 ? visible_rows - 1 : 1;
        } else if (event.key.scan_code == EFI_SCAN_PAGE_DOWN) {
            scroll += visible_rows > 1 ? visible_rows - 1 : 1;
        } else if (ascii_lower((char)event.key.unicode_char) == 'e') {
            ui_edit_text_file(path);
        }
        if (scroll < 0) {
            scroll = 0;
        }
    }
}

static void ui_edit_text_file(const char *path) {
    ui_event_t event;
    size_t length = 0;
    size_t cursor = 0;
    int scroll = 0;
    arm_fs_entry_t *entry = fs_find(path);

    memset(g_text_editor_buffer, 0, sizeof(g_text_editor_buffer));
    if (entry != NULL && entry->type == ARM_FS_TEXT) {
        memcpy(g_text_editor_buffer, entry->data, entry->size);
        length = entry->size;
        cursor = length;
    }
    strcopy_limit(g_last_opened_file, path, sizeof(g_last_opened_file));

    while (1) {
        int text_x = 56;
        int text_y = 146;
        int text_w = (int)g_gfx.width - 112;
        int text_h = (int)g_gfx.height - 246;
        int cols = text_w / 12;
        int rows = text_h / 16;
        int save_x = (int)g_gfx.width - 242;
        int save_y = (int)g_gfx.height - 138;
        int save_w = 150;
        int save_h = 44;

        ui_adjust_scroll(g_text_editor_buffer, length, cursor, cols, rows, &scroll);
        ui_draw_window("SNSX EDITOR", path);
        gfx_draw_text_block(text_x, text_y, text_w, text_h, (const char *)g_text_editor_buffer,
                            length, 2, gfx_rgb(27, 40, 58), scroll, (int)cursor);
        ui_draw_button(save_x, save_y, save_w, save_h, "SAVE",
                       theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                       theme_color(THEME_ROLE_TEXT));
        ui_draw_footer("TYPE TO EDIT  F2 SAVE  ESC BACK  ARROWS MOVE  BACKSPACE DELETE");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event)) {
            return;
        }
        if (ui_pointer_activated(&event, save_x, save_y, save_w, save_h)) {
            fs_write(path, g_text_editor_buffer, length, ARM_FS_TEXT);
            desktop_note_launch("EDITOR", "Saved the current text file.");
            persist_flush_state();
            entry = fs_find(path);
            continue;
        }
        if (ui_pointer_activated(&event, text_x, text_y, text_w, text_h)) {
            int click_col = (ui_event_pointer_x(&event) - text_x) / 12;
            int click_line = scroll + (ui_event_pointer_y(&event) - text_y) / 16;
            cursor = text_index_from_position(g_text_editor_buffer, length, cols, click_line, click_col);
            continue;
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_ESC) {
            return;
        }
        if (event.key.scan_code == EFI_SCAN_F2) {
            fs_write(path, g_text_editor_buffer, length, ARM_FS_TEXT);
            desktop_note_launch("EDITOR", "Saved the current text file.");
            persist_flush_state();
            entry = fs_find(path);
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_LEFT) {
            if (cursor > 0) {
                cursor--;
            }
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_RIGHT) {
            if (cursor < length) {
                cursor++;
            }
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_HOME) {
            cursor = 0;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_END) {
            cursor = length;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_DELETE) {
            if (cursor < length) {
                memmove(g_text_editor_buffer + cursor, g_text_editor_buffer + cursor + 1, length - cursor - 1);
                length--;
                g_text_editor_buffer[length] = '\0';
            }
            continue;
        }
        if (event.key.unicode_char == '\b') {
            if (cursor > 0) {
                memmove(g_text_editor_buffer + cursor - 1, g_text_editor_buffer + cursor, length - cursor);
                cursor--;
                length--;
                g_text_editor_buffer[length] = '\0';
            }
            continue;
        }
        if (event.key.unicode_char == '\r') {
            if (length + 1 < sizeof(g_text_editor_buffer)) {
                memmove(g_text_editor_buffer + cursor + 1, g_text_editor_buffer + cursor, length - cursor);
                g_text_editor_buffer[cursor] = '\n';
                cursor++;
                length++;
                g_text_editor_buffer[length] = '\0';
            }
            continue;
        }
        if (event.key.unicode_char >= 32 && event.key.unicode_char <= 126 && length + 1 < sizeof(g_text_editor_buffer)) {
            memmove(g_text_editor_buffer + cursor + 1, g_text_editor_buffer + cursor, length - cursor);
            g_text_editor_buffer[cursor] = (uint8_t)event.key.unicode_char;
            cursor++;
            length++;
            g_text_editor_buffer[length] = '\0';
        }
    }
}

static void ui_run_calc_app(void) {
    ui_event_t event;
    char expr[128];
    char result[64];
    size_t length = 0;

    memset(expr, 0, sizeof(expr));
    strcopy_limit(result, "ENTER AN EXPRESSION", sizeof(result));

    while (1) {
        int32_t value;
        ui_draw_window("SNSX CALCULATOR", "TYPE AN EXPRESSION AND PRESS ENTER");
        gfx_draw_panel(74, 160, (int)g_gfx.width - 148, 120, gfx_rgb(255, 255, 255), gfx_rgb(51, 79, 113));
        gfx_draw_text(96, 186, expr, 3, gfx_rgb(27, 42, 60));
        gfx_draw_panel(74, 310, (int)g_gfx.width - 148, 90, gfx_rgb(224, 240, 255), gfx_rgb(68, 98, 133));
        gfx_draw_text(96, 344, result, 2, gfx_rgb(23, 42, 72));
        gfx_draw_text(96, 438, "EXAMPLES: 7*(8+1)    100/4+3", 1, gfx_rgb(70, 88, 111));
        ui_draw_footer("TYPE DIGITS AND OPERATORS  ENTER EVALUATES  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event)) {
            return;
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_ESC) {
            return;
        }
        if (event.key.unicode_char == '\b') {
            if (length > 0) {
                expr[--length] = '\0';
            }
            continue;
        }
        if (event.key.unicode_char == '\r') {
            if (calc_eval(expr, &value)) {
                strcopy_limit(result, "RESULT ", sizeof(result));
                {
                    char digits[32];
                    u32_to_dec((uint32_t)(value < 0 ? -value : value), digits);
                    if (value < 0) {
                        strcopy_limit(result, "RESULT -", sizeof(result));
                        strcat(result, digits);
                    } else {
                        strcopy_limit(result, "RESULT ", sizeof(result));
                        strcat(result, digits);
                    }
                }
            } else {
                strcopy_limit(result, "INVALID EXPRESSION", sizeof(result));
            }
            continue;
        }
        if (event.key.unicode_char >= 32 && event.key.unicode_char <= 126 && length + 1 < sizeof(expr)) {
            expr[length++] = (char)event.key.unicode_char;
            expr[length] = '\0';
        }
    }
}

static shell_result_t ui_run_terminal_app(void) {
    console_set_color(EFI_WHITE, EFI_BLACK);
    console_clear();
    g_st->con_out->enable_cursor(g_st->con_out, 1);
    {
        shell_result_t result = shell_run(true);
        g_st->con_out->enable_cursor(g_st->con_out, 0);
        return result;
    }
}

static void boot_run_input_diagnostics(void) {
    ui_event_t event;
    size_t ticks = 0;
    size_t recovery_ticks = 0;

    boot_diagnostics_reset();
    if (!g_gfx.available) {
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_clear();
        console_writeline("SNSX Input Diagnostics");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_writeline("Checking firmware keyboard visibility before the desktop starts.");
        console_writeline("Press Enter, or wait for the auto-continue timer.");
        console_writeline("");
        console_write("Keyboard protocol: ");
        console_writeline(g_boot_diag.keyboard_protocol ? "present" : "missing");
        console_write("Extended keyboard protocol: ");
        console_writeline(g_boot_diag.keyboard_extended_protocol ? "present" : "missing");
        console_writeline("");

        while (ticks < 300) {
            memset(&event, 0, sizeof(event));
            if (input_try_read_key(&event.key)) {
                event.key_ready = true;
                boot_diagnostics_note_event(&event);
                if (event.key.unicode_char == '\r' || event.key.scan_code == EFI_SCAN_ESC) {
                    break;
                }
            }
            if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
                g_st->boot_services->stall(20000);
            }
            ticks++;
        }
        g_boot_diag.timed_out = ticks >= 300;
        if (boot_diagnostics_confirmed()) {
            desktop_set_status("Input diagnostics detected a live keyboard event.");
        } else if (g_boot_diag.keyboard_protocol || g_boot_diag.keyboard_extended_protocol) {
            desktop_set_status("Firmware keyboard protocol detected, but no live key event arrived during diagnostics.");
        } else {
            desktop_set_status("Firmware keyboard protocol is missing in text-console mode.");
        }
        return;
    }

    while (ticks < 320) {
        int btn_w = 220;
        int btn_h = 50;
        int btn_x = (int)g_gfx.width / 2 - btn_w / 2;
        int btn_y = (int)g_gfx.height - 146;
        int seconds_left = (int)((320 - ticks + 49) / 50);

        ui_draw_window("SNSX INPUT DIAGNOSTICS", "VERIFYING KEYBOARD, POINTER, AND GRAPHICS BEFORE FIRST RUN");
        gfx_draw_panel(66, 154, (int)g_gfx.width - 132, 132,
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(90, 184, "PRESS A KEY, MOVE THE MOUSE, OR CLICK CONTINUE.", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 208, "This confirms that VirtualBox firmware is handing SNSX live input events.", 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(90, 232, g_persist.status, 1, theme_color(THEME_ROLE_TEXT_SOFT));

        gfx_draw_panel(66, 314, 420, 170, theme_color(THEME_ROLE_PANEL), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(90, 342, "KEYBOARD", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(90, 374, g_boot_diag.keyboard_protocol ? "STANDARD PROTOCOL READY" : "STANDARD PROTOCOL MISSING", 1,
                      theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 396, g_boot_diag.keyboard_extended_protocol ? "EXTENDED PROTOCOL READY" : "EXTENDED PROTOCOL MISSING", 1,
                      theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 430, g_boot_diag.key_seen ? "LIVE KEY EVENT DETECTED" : "WAITING FOR A LIVE KEY EVENT", 1,
                      g_boot_diag.key_seen ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(90, 452, g_boot_diag.enter_seen ? "ENTER CONFIRMED" : "PRESS ENTER TO CONTINUE IMMEDIATELY", 1,
                      g_boot_diag.enter_seen ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_TEXT_SOFT));

        gfx_draw_panel(520, 314, (int)g_gfx.width - 586, 170,
                       theme_color(THEME_ROLE_PANEL), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(544, 342, "POINTER", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(544, 374, g_boot_diag.pointer_protocol ? "POINTER PROTOCOL READY" : "NO POINTER PROTOCOL", 1,
                      theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(544, 408, g_boot_diag.pointer_moved ? "MOUSE MOVEMENT DETECTED" : "MOVE THE MOUSE TO TEST CAPTURE", 1,
                      g_boot_diag.pointer_moved ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(544, 430, g_boot_diag.pointer_clicked ? "MOUSE CLICK DETECTED" : "CLICK TO VERIFY POINTER BUTTONS", 1,
                      g_boot_diag.pointer_clicked ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(544, 452, g_boot_diag.graphics_ready ? "GRAPHICS OUTPUT READY" : "TEXT FALLBACK", 1,
                      theme_color(THEME_ROLE_TEXT));

        if (seconds_left < 0) {
            seconds_left = 0;
        }
        ui_draw_button(btn_x, btn_y, btn_w, btn_h, "CONTINUE",
                       theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                       theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(btn_x + 250, btn_y + 20, "AUTO CONTINUE IN", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        {
            char countdown[8];
            u32_to_dec((uint32_t)seconds_left, countdown);
            gfx_draw_text(btn_x + 384, btn_y + 16, countdown, 2, theme_color(THEME_ROLE_TEXT));
        }
        ui_draw_footer("IF THE POINTER DISAPPEARS AFTER CLICKING, TRY MOVING THE MOUSE AGAIN OR USE HOST KEY RELEASE");
        ui_draw_cursor();

        memset(&event, 0, sizeof(event));
        if (input_try_read_key(&event.key)) {
            event.key_ready = true;
        } else {
            pointer_poll_event(&event);
        }
        boot_diagnostics_note_event(&event);

        if (event.key_ready && (event.key.unicode_char == '\r' || event.key.scan_code == EFI_SCAN_ESC)) {
            break;
        }
        if (ui_pointer_activated(&event, btn_x, btn_y, btn_w, btn_h)) {
            break;
        }
        if (boot_diagnostics_confirmed() && ticks >= 40) {
            break;
        }
        if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
            g_st->boot_services->stall(20000);
        }
        ticks++;
    }

    if (!boot_diagnostics_confirmed() &&
        (g_boot_diag.keyboard_protocol || g_boot_diag.keyboard_extended_protocol || g_boot_diag.pointer_protocol)) {
        input_recover_live_devices();
        while (recovery_ticks < 120 && !boot_diagnostics_confirmed()) {
            memset(&event, 0, sizeof(event));
            if (input_try_read_key(&event.key)) {
                event.key_ready = true;
            } else {
                pointer_poll_event(&event);
            }
            boot_diagnostics_note_event(&event);
            if (boot_diagnostics_confirmed()) {
                break;
            }
            if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
                g_st->boot_services->stall(20000);
            }
            recovery_ticks++;
        }
    }

    g_boot_diag.timed_out = ticks >= 320;
    g_force_text_ui = !boot_diagnostics_confirmed();
    if (boot_diagnostics_confirmed()) {
        desktop_set_status("Input diagnostics confirmed a working key or click event.");
        g_force_text_ui = false;
    } else if (g_boot_diag.keyboard_protocol || g_boot_diag.pointer_protocol) {
        desktop_set_status("Rescue mode enabled: firmware devices exist, but SNSX did not confirm a working key or click event.");
    } else {
        desktop_set_status("Input diagnostics did not find usable firmware keyboard or pointer protocols.");
    }
}

static void ui_run_setup_app(void) {
    ui_event_t event;
    size_t focus = 0;
    size_t idle_ticks = 0;

    if (!g_gfx.available || g_force_text_ui) {
        app_setup_console();
        return;
    }

    while (1) {
        int btn_margin = 42;
        int btn_gap = 10;
        int btn_y = (int)g_gfx.height - 172;
        int btn_h = 48;
        int btn_w = ((int)g_gfx.width - btn_margin * 2 - btn_gap * 4) / 5;
        int btn_x1 = btn_margin;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;
        int btn_x5 = btn_x4 + btn_w + btn_gap;
        int seconds_left = (int)((400 - idle_ticks + 49) / 50);

        ui_draw_window("SNSX FIRST-RUN SETUP", "DESKTOP READINESS, INSTALL, SAVE, GUIDE, AND INPUT RETEST");
        gfx_draw_panel(66, 154, (int)g_gfx.width - 132, 146,
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(90, 184, "SNSX is ready to boot into the full desktop. This setup screen appears first so", 1,
                      theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 206, "you can confirm input, install the VM disk, save the live session, or read the guide.", 1,
                      theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 238, g_persist.status, 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(90, 260, g_boot_diag.key_seen ? "Keyboard input was detected during diagnostics." :
                                                      "Keyboard input was not seen during diagnostics yet.",
                      1, theme_color(THEME_ROLE_TEXT_SOFT));

        gfx_draw_panel(66, 332, 420, 186, theme_color(THEME_ROLE_PANEL), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(90, 360, "READY CHECK", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(90, 392, g_boot_diag.graphics_ready ? "GRAPHICS DESKTOP READY" : "TEXT FALLBACK ACTIVE", 1,
                      theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 414, g_boot_diag.keyboard_protocol ? "FIRMWARE KEYBOARD PRESENT" : "FIRMWARE KEYBOARD MISSING", 1,
                      theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 436, g_boot_diag.pointer_protocol ? "FIRMWARE POINTER PRESENT" : "NO POINTER PROTOCOL", 1,
                      theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 458, g_boot_diag.pointer_clicked ? "MOUSE CLICK VERIFIED" : "MOUSE CLICK NOT YET VERIFIED", 1,
                      theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 480, g_boot_diag.timed_out ? "DIAGNOSTICS AUTO-CONTINUED" : "DIAGNOSTICS COMPLETED INTERACTIVELY", 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));

        gfx_draw_panel(520, 332, (int)g_gfx.width - 586, 186,
                       theme_color(THEME_ROLE_PANEL), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(544, 360, "NEXT STEPS", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(544, 392, "1. INSTALL writes BOOTAA64.EFI to the writable VM disk.", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(544, 414, "2. SAVE writes the live SNSX filesystem snapshot to that disk.", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(544, 436, "3. GUIDE shows the exact ISO and VirtualBox procedure.", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(544, 458, "4. DESKTOP opens the full graphical launcher and apps.", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(544, 480, "5. RETEST reruns the input diagnostics if capture still looks wrong.", 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, "DESKTOP",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "INSTALL",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "SAVE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "GUIDE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 3);
        ui_draw_button_focus(btn_x5, btn_y, btn_w, btn_h, "RETEST",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 4);
        if (seconds_left < 0) {
            seconds_left = 0;
        }
        gfx_draw_text(68, (int)g_gfx.height - 114, "AUTO-OPEN DESKTOP IN", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        {
            char countdown[8];
            u32_to_dec((uint32_t)seconds_left, countdown);
            gfx_draw_text(274, (int)g_gfx.height - 120, countdown, 2, theme_color(THEME_ROLE_TEXT));
        }
        ui_draw_footer("TAB OR ARROWS SELECT  ENTER OPENS  D INSTALL SAVE GUIDE RETEST  AUTO-DESKTOP IF INPUT IS QUIET");
        ui_draw_cursor();

        memset(&event, 0, sizeof(event));
        while (1) {
            if (input_try_read_key(&event.key)) {
                event.key_ready = true;
                break;
            }
            if (pointer_poll_event(&event)) {
                break;
            }
            if (idle_ticks >= 400) {
                break;
            }
            if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
                g_st->boot_services->stall(20000);
            }
            idle_ticks++;
        }
        if (idle_ticks >= 400 && !event.key_ready && !event.mouse_clicked && !event.mouse_moved && !event.mouse_down) {
            desktop_note_launch("SETUP", "Auto-opened the SNSX desktop.");
            break;
        }
        if (event.key_ready || event.mouse_clicked || event.mouse_moved || event.mouse_down) {
            idle_ticks = 0;
        }
        if (ui_close_button_event_hit(&event)) {
            break;
        }
        if (event.mouse_clicked || event.mouse_released) {
            if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                focus = 0;
                break;
            }
            if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                focus = 1;
                persist_install_boot_disk();
                desktop_note_launch("SETUP", g_persist.status);
                continue;
            }
            if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                focus = 2;
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("SETUP", g_persist.status);
                continue;
            }
            if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                focus = 3;
                ui_view_text_file("/docs/INSTALL.TXT", "SNSX GUIDE");
                continue;
            }
            if (ui_pointer_activated(&event, btn_x5, btn_y, btn_w, btn_h)) {
                focus = 4;
                boot_run_input_diagnostics();
                continue;
            }
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_ESC) {
            break;
        }
        if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 5;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 4 : focus - 1;
            continue;
        }
        if (event.key.unicode_char == '\r') {
            if (focus == 0) {
                break;
            }
            if (focus == 1) {
                persist_install_boot_disk();
                desktop_note_launch("SETUP", g_persist.status);
            } else if (focus == 2) {
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("SETUP", g_persist.status);
            } else if (focus == 3) {
                ui_view_text_file("/docs/INSTALL.TXT", "SNSX GUIDE");
            } else {
                boot_run_input_diagnostics();
            }
            continue;
        }
        switch (ascii_lower((char)event.key.unicode_char)) {
            case 'd':
                focus = 0;
                break;
            case 'i':
                focus = 1;
                persist_install_boot_disk();
                desktop_note_launch("SETUP", g_persist.status);
                break;
            case 's':
                focus = 2;
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("SETUP", g_persist.status);
                break;
            case 'g':
                focus = 3;
                ui_view_text_file("/docs/INSTALL.TXT", "SNSX GUIDE");
                break;
            case 'r':
                focus = 4;
                boot_run_input_diagnostics();
                break;
            default:
                break;
        }
    }

    g_desktop.welcome_seen = true;
    desktop_note_launch("SETUP", "First-run setup completed.");
    desktop_commit_preferences();
}

static void __attribute__((unused)) ui_run_about_app(void) {
    ui_event_t event;
    while (1) {
        ui_draw_window("ABOUT SNSX", "STANDALONE ARM64 UI EDITION");
        gfx_draw_text(66, 154, "SNSX now boots into a themed mouse-and-keyboard desktop UI.", 1, gfx_rgb(35, 49, 70));
        gfx_draw_text(66, 176, "Apps: Welcome, Files, Notes, Calc, Guide, Terminal, Installer, Network, Disks, Settings.", 1, gfx_rgb(35, 49, 70));
        gfx_draw_text(66, 198, "Filesystem: live RAM disk with optional disk-backed persistence and install flow.", 1, gfx_rgb(35, 49, 70));
        gfx_draw_text(66, 220, "Desktop preferences survive after Save Session on the writable VM disk.", 1, gfx_rgb(35, 49, 70));
        gfx_draw_text(66, 242, "Target: Oracle VirtualBox ARM64 on Apple Silicon.", 1, gfx_rgb(35, 49, 70));
        gfx_draw_panel(66, 284, (int)g_gfx.width - 132, 170, gfx_rgb(222, 238, 255), gfx_rgb(55, 89, 126));
        gfx_draw_text(88, 314, "KEYBOARD SHORTCUTS", 2, gfx_rgb(32, 55, 84));
        gfx_draw_text(88, 350, "ARROWS/TAB SELECT DESKTOP APPS", 1, gfx_rgb(43, 58, 80));
        gfx_draw_text(88, 372, "ENTER OPENS AN APP  CLICKING DOES TOO", 1, gfx_rgb(43, 58, 80));
        gfx_draw_text(88, 394, "ESC RETURNS TO THE DESKTOP  F2 OR SAVE BUTTON WRITES IN EDITOR", 1, gfx_rgb(43, 58, 80));
        gfx_draw_text(88, 416, g_persist.installed ? "PERSISTENCE: INSTALLED" : "PERSISTENCE: LIVE SESSION ONLY", 1,
                      gfx_rgb(43, 58, 80));
        ui_draw_footer("ESC BACK TO DESKTOP");
        ui_draw_cursor();
        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event) ||
            (event.key_ready && (event.key.scan_code == EFI_SCAN_ESC || event.key.unicode_char == '\r'))) {
            return;
        }
    }
}

static void ui_run_network_app(void) {
    ui_event_t event;
    size_t focus = 0;
    bool auto_connect_checked = false;

    while (1) {
        char snsx_address[96];
        char session_address[96];
        char link_address[96];
        size_t wifi_slots[WIFI_PROFILE_MAX];
        size_t bt_slots[BLUETOOTH_DEVICE_MAX];
        size_t wifi_count = wifi_collect_slots(wifi_slots, ARRAY_SIZE(wifi_slots));
        size_t bt_count = bluetooth_collect_slots(bt_slots, ARRAY_SIZE(bt_slots));
        size_t wifi_connected = 0;
        size_t bt_connected = 0;
        int btn_y = (int)g_gfx.height - 156;
        int btn_w = 154;
        int btn_h = 46;
        int btn_gap = 12;
        int btn_x1 = 72;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;
        int btn_x5 = btn_x4 + btn_w + btn_gap;
        bool wired_live = g_network.available && (!g_network.media_present_supported || g_network.media_present);
        char wifi_count_text[16];
        char bt_count_text[16];

        for (size_t i = 0; i < wifi_count; ++i) {
            if (g_wifi_profiles[wifi_slots[i]].connected) {
                ++wifi_connected;
            }
        }
        for (size_t i = 0; i < bt_count; ++i) {
            if (g_bluetooth_devices[bt_slots[i]].connected) {
                ++bt_connected;
            }
        }
        u32_to_dec((uint32_t)wifi_count, wifi_count_text);
        u32_to_dec((uint32_t)bt_count, bt_count_text);
        network_compose_snsx_address(snsx_address, sizeof(snsx_address));
        network_compose_session_address(session_address, sizeof(session_address));
        network_compose_link_address(link_address, sizeof(link_address));

        if (!auto_connect_checked && g_desktop.network_autoconnect && !g_netstack.configured) {
            network_refresh(true);
            auto_connect_checked = true;
        }

        ui_draw_window("SNSX NETWORK CENTER", "IDENTITIES, WIRED INTERNET, FIRMWARE DIAGNOSTICS, AND BROWSER ROUTES");
        gfx_draw_panel(66, 154, (int)g_gfx.width - 132, 126, gfx_rgb(228, 239, 255), gfx_rgb(62, 93, 131));
        gfx_draw_text(90, 182, g_network.available ? "SNSX NETWORK STACK IS ONLINE WITH A FIRMWARE ADAPTER" :
                                                     (g_netdiag.pci_network_handles > 0 ?
                                                      "SNSX FOUND NETWORK HARDWARE BUT NO UEFI NETWORK STACK" :
                                                      "SNSX HAS NO UEFI NETWORK PROTOCOL ON THIS TARGET"),
                      2, gfx_rgb(32, 53, 81));
        gfx_draw_text(90, 212, g_network.profile, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(90, 234, g_network.status, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(90, 256, g_network.internet, 1, gfx_rgb(57, 73, 95));
        ui_draw_badge(758, 176, g_netstack.configured ? "DHCP READY" : "NO LEASE",
                      g_netstack.configured ? theme_color(THEME_ROLE_WARM) : theme_color(THEME_ROLE_PANEL_ALT),
                      theme_color(THEME_ROLE_TEXT));
        ui_draw_badge(758, 208, g_desktop.wifi_enabled ? "WIFI UI ON" : "WIFI UI OFF",
                      g_desktop.wifi_enabled ? theme_color(THEME_ROLE_ACCENT) : theme_color(THEME_ROLE_PANEL_ALT),
                      theme_color(THEME_ROLE_TEXT_LIGHT));
        ui_draw_badge(758, 240, g_desktop.bluetooth_enabled ? "BT UI ON" : "BT UI OFF",
                      g_desktop.bluetooth_enabled ? theme_color(THEME_ROLE_ACCENT) : theme_color(THEME_ROLE_PANEL_ALT),
                      theme_color(THEME_ROLE_TEXT_LIGHT));

        gfx_draw_panel(66, 306, 288, 226, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(92, 332, "SNSX IDENTITY", 2, gfx_rgb(34, 54, 80));
        gfx_draw_text(92, 366, "Host name", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(188, 366, g_network.hostname, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(92, 392, "SNSX address", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(92, 412, snsx_address, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(92, 438, "Session route", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(92, 458, session_address, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(92, 484, "Link address", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(92, 504, link_address, 1, gfx_rgb(24, 43, 66));

        gfx_draw_panel(376, 306, 288, 226, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(402, 332, "WIRED INTERNET", 2, gfx_rgb(34, 54, 80));
        gfx_draw_text(402, 366, g_network.available ? "Adapter available" : "No wired adapter", 1, gfx_rgb(55, 70, 92));
        gfx_draw_text(402, 388, wired_live ? "Cable / media present" : "Cable / media not reported", 1, gfx_rgb(55, 70, 92));
        gfx_draw_text(402, 410, g_network.internet_services_ready ? "Firmware transport initialized" : "Firmware transport offline", 1,
                      gfx_rgb(55, 70, 92));
        gfx_draw_text(402, 432, g_network.dhcp_ready ? "DHCP lease active" : "DHCP lease not bound", 1, gfx_rgb(55, 70, 92));
        gfx_draw_text(402, 454, "IPv4", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(476, 454, g_network.ipv4, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(402, 476, "Gateway", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(494, 476, g_network.gateway, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(402, 498, "DNS", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(458, 498, g_network.dns_server, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(402, 520, "NIC", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(446, 520, g_netdiag.hardware, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(402, 542, "Driver", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(468, 542, g_network.driver_name, 1, gfx_rgb(24, 43, 66));

        gfx_draw_panel(686, 306, (int)g_gfx.width - 752, 226, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(712, 332, "FIRMWARE / BT", 2, gfx_rgb(34, 54, 80));
        gfx_draw_text(712, 366, g_desktop.wifi_enabled ? "Wi-Fi dashboard enabled" : "Wi-Fi dashboard disabled", 1, gfx_rgb(55, 70, 92));
        gfx_draw_text(712, 388, g_desktop.bluetooth_enabled ? "Bluetooth dashboard enabled" : "Bluetooth dashboard disabled", 1,
                      gfx_rgb(55, 70, 92));
        gfx_draw_text(712, 414, "Firmware stack", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(712, 434, g_netdiag.firmware_stack, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(712, 458, "Saved Wi-Fi / BT", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(858, 458, wifi_count_text, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(878, 458, "/", 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(894, 458, bt_count_text, 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(712, 480, "Active profiles", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(842, 480, wifi_connected > 0 ? "WIFI" : "-", 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(886, 480, bt_connected > 0 ? "BT" : "-", 1, gfx_rgb(24, 43, 66));
        gfx_draw_text(712, 504, "Browser route", 1, gfx_rgb(63, 78, 101));
        gfx_draw_text(712, 522, "snsxdiag://network", 1, gfx_rgb(24, 43, 66));

        gfx_draw_panel(66, 554, (int)g_gfx.width - 132, 84, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(92, 578, "CAPABILITIES", 2, gfx_rgb(34, 54, 80));
        gfx_draw_text(92, 606, g_network.capabilities, 1, gfx_rgb(55, 70, 92));
        gfx_draw_text(92, 626, g_netdiag.wireless, 1, gfx_rgb(55, 70, 92));

        gfx_draw_text(1040, 520, "Packet size", 1, gfx_rgb(63, 78, 101));
        {
            char packet[16];
            u32_to_dec(g_network.max_packet_size, packet);
            gfx_draw_text(1152, 520, g_network.available ? packet : "0", 1, gfx_rgb(24, 43, 66));
        }

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, "CONNECT",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "WI-FI",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "BLUETOOTH",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "BROWSER",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 3);
        ui_draw_button_focus(btn_x5, btn_y, btn_w, btn_h, "GUIDE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 4);
        ui_draw_footer("C CONNECTS  W WI-FI  B BLUETOOTH  O BROWSER  D DIAG  M MODE  A AUTOCONNECT  G GUIDE  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event) ||
            (event.key_ready && event.key.scan_code == EFI_SCAN_ESC)) {
            return;
        }
        if (event.mouse_clicked || event.mouse_released) {
            if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                focus = 0;
                network_refresh(true);
                desktop_note_launch("NETWORK", g_network.status);
                continue;
            }
            if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                focus = 1;
                ui_run_wifi_app();
                continue;
            }
            if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                focus = 2;
                ui_run_bluetooth_app();
                continue;
            }
            if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                focus = 3;
                ui_run_browser_app();
                continue;
            }
            if (ui_pointer_activated(&event, btn_x5, btn_y, btn_w, btn_h)) {
                focus = 4;
                ui_view_text_file("/docs/NETWORK.TXT", "SNSX NETWORK GUIDE");
                continue;
            }
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 5;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 4 : focus - 1;
            continue;
        }
        if (event.key.unicode_char == '\r') {
            if (focus == 0) {
                network_refresh(true);
                desktop_note_launch("NETWORK", g_network.status);
            } else if (focus == 1) {
                ui_run_wifi_app();
            } else if (focus == 2) {
                ui_run_bluetooth_app();
            } else if (focus == 3) {
                ui_run_browser_app();
            } else {
                ui_view_text_file("/docs/NETWORK.TXT", "SNSX NETWORK GUIDE");
            }
            continue;
        }
        switch (ascii_lower((char)event.key.unicode_char)) {
            case 'c':
                focus = 0;
                network_refresh(true);
                desktop_note_launch("NETWORK", g_network.status);
                break;
            case 'w':
                focus = 1;
                ui_run_wifi_app();
                break;
            case 'b':
                focus = 2;
                ui_run_bluetooth_app();
                break;
            case 'o':
                focus = 3;
                ui_run_browser_app();
                break;
            case 'd':
                focus = 3;
                strcopy_limit(g_browser_address, "snsxdiag://network", sizeof(g_browser_address));
                ui_run_browser_app();
                break;
            case 'm':
                desktop_cycle_network_mode();
                desktop_note_launch("NETWORK", "Changed the preferred network mode.");
                desktop_commit_preferences();
                network_refresh(g_desktop.network_autoconnect);
                break;
            case 'a':
                g_desktop.network_autoconnect = !g_desktop.network_autoconnect;
                desktop_note_launch("NETWORK", g_desktop.network_autoconnect ? "Network autoconnect enabled." :
                                                                     "Network autoconnect disabled.");
                desktop_commit_preferences();
                network_refresh(g_desktop.network_autoconnect);
                break;
            case 'g':
                focus = 4;
                ui_view_text_file("/docs/NETWORK.TXT", "SNSX NETWORK GUIDE");
                break;
            default:
                break;
        }
    }
}

static void ui_run_wifi_app(void) {
    ui_event_t event;
    size_t selected = 0;
    size_t focus = 0;
    char input[FS_PATH_MAX];
    char password[FS_PATH_MAX];

    while (1) {
        size_t slots[WIFI_PROFILE_MAX];
        size_t count = wifi_collect_slots(slots, ARRAY_SIZE(slots));
        int btn_y = (int)g_gfx.height - 156;
        int btn_w = 146;
        int btn_h = 46;
        int btn_gap = 12;
        int btn_x1 = 72;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;
        int btn_x5 = btn_x4 + btn_w + btn_gap;
        if (selected >= count && count > 0) {
            selected = count - 1;
        }
        bool selected_connected = count > 0 ? g_wifi_profiles[slots[selected]].connected : false;

        ui_draw_window("SNSX WI-FI", "KNOWN NETWORKS, OTHER NETWORKS, AND SNSX PROFILE MEMORY");
        gfx_draw_panel(66, 154, (int)g_gfx.width - 132, 118, gfx_rgb(228, 239, 255), gfx_rgb(62, 93, 131));
        ui_draw_toggle(90, 182, 92, 34, "WI-FI", g_desktop.wifi_enabled, focus == 0);
        gfx_draw_text(250, 182, g_desktop.wifi_enabled ? "WI-FI PROFILE DASHBOARD IS ACTIVE" :
                                                         "WI-FI PROFILE DASHBOARD IS OFF",
                      2, gfx_rgb(32, 53, 81));
        gfx_draw_text(250, 214, wifi_radio_available() ? "Firmware has exposed a Wi-Fi-capable adapter to SNSX." :
                                                         "This VirtualBox ARM64 target only exposes Ethernet to the guest.",
                      1, gfx_rgb(24, 43, 66));
        gfx_draw_text(250, 236, "SNSX still remembers networks here so a hardware-backed build can reuse them later.", 1,
                      gfx_rgb(57, 73, 95));

        gfx_draw_panel(66, 296, 420, 258, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(90, 322, "KNOWN NETWORKS", 2, gfx_rgb(34, 54, 80));
        if (count == 0) {
            gfx_draw_text(90, 360, "NO SAVED WI-FI PROFILES YET", 1, gfx_rgb(55, 70, 92));
            gfx_draw_text(90, 382, "Use ADD to store a hotspot or access-point name.", 1, gfx_rgb(55, 70, 92));
        } else {
            for (size_t row = 0; row < count && row < 5; ++row) {
                int y = 354 + (int)row * 40;
                wifi_profile_t *profile = &g_wifi_profiles[slots[row]];
                bool active = row == selected;
                gfx_draw_panel(88, y, 360, 28,
                               active ? theme_color(THEME_ROLE_WARM) : gfx_rgb(255, 255, 255),
                               active ? theme_color(THEME_ROLE_ACCENT_STRONG) : gfx_rgb(195, 205, 220));
                gfx_draw_text(104, y + 8, profile->ssid, 1, gfx_rgb(26, 39, 57));
                gfx_draw_text(276, y + 8, profile->password[0] != '\0' ? "LOCKED" : "OPEN", 1,
                              gfx_rgb(63, 78, 101));
                gfx_draw_text(350, y + 8, profile->connected ? "ACTIVE" : "SAVED", 1,
                              gfx_rgb(63, 78, 101));
            }
        }

        gfx_draw_panel(520, 296, (int)g_gfx.width - 586, 258, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(544, 322, "OTHER NETWORKS", 2, gfx_rgb(34, 54, 80));
        gfx_draw_text(544, 360, wifi_radio_available() ? "SCAN SUPPORT CAN LIST NEARBY ACCESS POINTS HERE." :
                                                         "NEARBY NETWORK SCAN IS NOT AVAILABLE ON THIS VIRTUALBOX TARGET.",
                      1, gfx_rgb(55, 70, 92));
        gfx_draw_text(544, 382, "SNSX can still save a network name and password manually.", 1, gfx_rgb(55, 70, 92));
        if (count == 0) {
            gfx_draw_text(544, 420, "ADD A NETWORK TO BEGIN.", 1, gfx_rgb(55, 70, 92));
        } else {
            wifi_profile_t *profile = &g_wifi_profiles[slots[selected]];
            gfx_draw_text(544, 420, profile->ssid, 2, gfx_rgb(25, 43, 66));
            gfx_draw_text(544, 452, selected_connected ? "PROFILE SELECTED" : "NOT SELECTED", 1, gfx_rgb(55, 70, 92));
            gfx_draw_text(544, 474, profile->auto_connect ? "AUTO-CONNECT ENABLED" : "MANUAL CONNECT", 1, gfx_rgb(55, 70, 92));
            gfx_draw_text(544, 496, profile->password[0] != '\0' ? "PASSWORD STORED" : "PASSWORD REQUIRED", 1,
                          gfx_rgb(55, 70, 92));
        }
        gfx_draw_text(544, 530, g_desktop.wifi_enabled ? "Wi-Fi profile mode is on." : "Turn Wi-Fi on to select a profile.",
                      1, gfx_rgb(55, 70, 92));

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, g_desktop.wifi_enabled ? "TURN OFF" : "TURN ON",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "ADD",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "CONNECT",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "FORGET",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 3);
        ui_draw_button_focus(btn_x5, btn_y, btn_w, btn_h, "NETWORK",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 4);
        ui_draw_footer("T TOGGLES  A ADDS  C CONNECTS  F FORGETS  ENTER OPENS  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event) ||
            (event.key_ready && event.key.scan_code == EFI_SCAN_ESC)) {
            return;
        }
        if (event.mouse_clicked || event.mouse_released) {
            if (ui_pointer_activated(&event, 90, 182, 92, 34)) {
                focus = 0;
            } else {
                for (size_t row = 0; row < count && row < 5; ++row) {
                    int y = 354 + (int)row * 40;
                    if (ui_pointer_activated(&event, 88, y, 360, 28)) {
                        selected = row;
                    }
                }
                if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                    focus = 0;
                } else if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                    focus = 1;
                } else if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                    focus = 2;
                } else if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                    focus = 3;
                } else if (ui_pointer_activated(&event, btn_x5, btn_y, btn_w, btn_h)) {
                    focus = 4;
                } else {
                    continue;
                }
            }
        } else if (!event.key_ready) {
            continue;
        } else if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 5;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 4 : focus - 1;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_UP && selected > 0) {
            selected--;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_DOWN && selected + 1 < count) {
            selected++;
            continue;
        } else {
            char shortcut = ascii_lower((char)event.key.unicode_char);
            if (shortcut == 't') {
                focus = 0;
            } else if (shortcut == 'a') {
                focus = 1;
            } else if (shortcut == 'c') {
                focus = 2;
            } else if (shortcut == 'f') {
                focus = 3;
            } else if (shortcut == 'n') {
                focus = 4;
            } else if (event.key.unicode_char != '\r') {
                continue;
            }
        }

        if (focus == 0) {
            g_desktop.wifi_enabled = !g_desktop.wifi_enabled;
            if (!g_desktop.wifi_enabled) {
                for (size_t i = 0; i < WIFI_PROFILE_MAX; ++i) {
                    g_wifi_profiles[i].connected = false;
                }
            }
            desktop_note_launch("WIFI", g_desktop.wifi_enabled ? "Wi-Fi dashboard enabled." : "Wi-Fi dashboard disabled.");
            desktop_commit_preferences();
            network_refresh(g_desktop.network_autoconnect);
            continue;
        }
        if (focus == 1) {
            size_t slot = WIFI_PROFILE_MAX;
            memset(input, 0, sizeof(input));
            memset(password, 0, sizeof(password));
            if (!ui_prompt_line("WI-FI NAME", "ENTER THE NETWORK NAME TO SAVE", "", input, sizeof(input)) ||
                input[0] == '\0') {
                continue;
            }
            if (!ui_prompt_line("WI-FI PASSWORD", "ENTER THE NETWORK PASSWORD", "", password, sizeof(password))) {
                continue;
            }
            for (size_t i = 0; i < WIFI_PROFILE_MAX; ++i) {
                if (g_wifi_profiles[i].used && strcmp(g_wifi_profiles[i].ssid, input) == 0) {
                    slot = i;
                    break;
                }
                if (!g_wifi_profiles[i].used && slot == WIFI_PROFILE_MAX) {
                    slot = i;
                }
            }
            if (slot == WIFI_PROFILE_MAX) {
                desktop_note_launch("WIFI", "No free Wi-Fi profile slots remain.");
                continue;
            }
            memset(&g_wifi_profiles[slot], 0, sizeof(g_wifi_profiles[slot]));
            g_wifi_profiles[slot].used = true;
            g_wifi_profiles[slot].auto_connect = true;
            g_wifi_profiles[slot].remembered = true;
            strcopy_limit(g_wifi_profiles[slot].ssid, input, sizeof(g_wifi_profiles[slot].ssid));
            strcopy_limit(g_wifi_profiles[slot].password, password, sizeof(g_wifi_profiles[slot].password));
            strcopy_limit(g_desktop.wifi_ssid, input, sizeof(g_desktop.wifi_ssid));
            desktop_note_launch("WIFI", "Saved the Wi-Fi profile.");
            desktop_commit_preferences();
            continue;
        }
        if (focus == 2) {
            if (count == 0) {
                desktop_note_launch("WIFI", "No Wi-Fi profile is available to connect.");
                continue;
            }
            if (!g_desktop.wifi_enabled) {
                desktop_note_launch("WIFI", "Turn Wi-Fi on before selecting a network profile.");
                continue;
            }
            wifi_profile_t *profile = &g_wifi_profiles[slots[selected]];
            if (profile->password[0] == '\0') {
                if (!ui_prompt_line("WI-FI PASSWORD", "ENTER THE PASSWORD FOR THIS NETWORK", "", password, sizeof(password))) {
                    continue;
                }
                strcopy_limit(profile->password, password, sizeof(profile->password));
            }
            for (size_t i = 0; i < WIFI_PROFILE_MAX; ++i) {
                g_wifi_profiles[i].connected = false;
            }
            profile->connected = true;
            g_desktop.network_preference = NETWORK_PREF_WIFI;
            desktop_note_launch("WIFI", wifi_radio_available() ? "Connected to the selected Wi-Fi profile." :
                                                             "Selected the Wi-Fi profile. This VM still exposes no guest radio.");
            desktop_commit_preferences();
            network_refresh(g_desktop.network_autoconnect);
            continue;
        }
        if (focus == 3) {
            if (count == 0) {
                continue;
            }
            memset(&g_wifi_profiles[slots[selected]], 0, sizeof(g_wifi_profiles[slots[selected]]));
            if (selected > 0 && selected >= count - 1) {
                selected--;
            }
            desktop_note_launch("WIFI", "Forgot the selected Wi-Fi profile.");
            desktop_commit_preferences();
            network_refresh(g_desktop.network_autoconnect);
            continue;
        }
        return;
    }
}

static void ui_run_bluetooth_app(void) {
    ui_event_t event;
    size_t selected = 0;
    size_t focus = 0;
    char name[FS_PATH_MAX];
    char kind[FS_PATH_MAX];

    while (1) {
        size_t slots[BLUETOOTH_DEVICE_MAX];
        size_t count = bluetooth_collect_slots(slots, ARRAY_SIZE(slots));
        int btn_y = (int)g_gfx.height - 156;
        int btn_w = 146;
        int btn_h = 46;
        int btn_gap = 12;
        int btn_x1 = 72;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;
        int btn_x5 = btn_x4 + btn_w + btn_gap;
        if (selected >= count && count > 0) {
            selected = count - 1;
        }
        bool selected_connected = count > 0 ? g_bluetooth_devices[slots[selected]].connected : false;

        ui_draw_window("SNSX BLUETOOTH", "PAIRED DEVICES, DISCOVERY READINESS, AND SNSX ACCESSORIES");
        gfx_draw_panel(66, 154, (int)g_gfx.width - 132, 118, gfx_rgb(228, 239, 255), gfx_rgb(62, 93, 131));
        ui_draw_toggle(90, 182, 92, 34, "BLUETOOTH", g_desktop.bluetooth_enabled, focus == 0);
        gfx_draw_text(250, 182, g_desktop.bluetooth_enabled ? "BLUETOOTH DASHBOARD IS ACTIVE" :
                                                              "BLUETOOTH DASHBOARD IS OFF",
                      2, gfx_rgb(32, 53, 81));
        gfx_draw_text(250, 214, bluetooth_controller_available() ?
                      "Firmware has exposed a Bluetooth controller to SNSX." :
                      "This VirtualBox ARM64 target does not expose a guest Bluetooth controller.",
                      1, gfx_rgb(24, 43, 66));
        gfx_draw_text(250, 236, "SNSX still stores device pairings so later hardware-backed builds can reuse them.", 1,
                      gfx_rgb(57, 73, 95));

        gfx_draw_panel(66, 296, 420, 258, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(90, 322, "DEVICES", 2, gfx_rgb(34, 54, 80));
        if (count == 0) {
            gfx_draw_text(90, 360, "NO BLUETOOTH DEVICES PAIRED YET", 1, gfx_rgb(55, 70, 92));
            gfx_draw_text(90, 382, "Use PAIR to remember headphones, phones, or controllers.", 1, gfx_rgb(55, 70, 92));
        } else {
            for (size_t row = 0; row < count && row < 5; ++row) {
                int y = 354 + (int)row * 40;
                bluetooth_device_t *device = &g_bluetooth_devices[slots[row]];
                bool active = row == selected;
                gfx_draw_panel(88, y, 360, 28,
                               active ? theme_color(THEME_ROLE_WARM) : gfx_rgb(255, 255, 255),
                               active ? theme_color(THEME_ROLE_ACCENT_STRONG) : gfx_rgb(195, 205, 220));
                gfx_draw_text(104, y + 8, device->name, 1, gfx_rgb(26, 39, 57));
                gfx_draw_text(276, y + 8, device->kind, 1, gfx_rgb(63, 78, 101));
                gfx_draw_text(350, y + 8, device->connected ? "ACTIVE" : "PAIRED", 1, gfx_rgb(63, 78, 101));
            }
        }

        gfx_draw_panel(520, 296, (int)g_gfx.width - 586, 258, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(544, 322, "DISCOVERY / STATUS", 2, gfx_rgb(34, 54, 80));
        gfx_draw_text(544, 360, bluetooth_controller_available() ?
                      "LIVE DISCOVERY CAN APPEAR HERE." :
                      "LIVE DISCOVERY IS NOT AVAILABLE ON THIS VIRTUALBOX TARGET.",
                      1, gfx_rgb(55, 70, 92));
        gfx_draw_text(544, 382, "SNSX can still remember pairings and device kinds.", 1, gfx_rgb(55, 70, 92));
        if (count == 0) {
            gfx_draw_text(544, 420, "PAIR A DEVICE TO BEGIN.", 1, gfx_rgb(55, 70, 92));
        } else {
            bluetooth_device_t *device = &g_bluetooth_devices[slots[selected]];
            gfx_draw_text(544, 420, device->name, 2, gfx_rgb(25, 43, 66));
            gfx_draw_text(544, 452, selected_connected ? "DEVICE SELECTED" : "NOT SELECTED", 1, gfx_rgb(55, 70, 92));
            gfx_draw_text(544, 474, device->paired ? "PAIRING STORED" : "NOT PAIRED", 1, gfx_rgb(55, 70, 92));
            gfx_draw_text(544, 496, device->kind, 1, gfx_rgb(55, 70, 92));
        }
        gfx_draw_text(544, 530, g_desktop.bluetooth_enabled ? "Bluetooth profile mode is on." :
                                                                 "Turn Bluetooth on to select a device profile.",
                      1, gfx_rgb(55, 70, 92));

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, g_desktop.bluetooth_enabled ? "TURN OFF" : "TURN ON",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "PAIR",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "CONNECT",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "UNPAIR",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 3);
        ui_draw_button_focus(btn_x5, btn_y, btn_w, btn_h, "NETWORK",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 4);
        ui_draw_footer("T TOGGLES  P PAIRS  C CONNECTS  U UNPAIRS  ENTER OPENS  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event) ||
            (event.key_ready && event.key.scan_code == EFI_SCAN_ESC)) {
            return;
        }
        if (event.mouse_clicked || event.mouse_released) {
            if (ui_pointer_activated(&event, 90, 182, 92, 34)) {
                focus = 0;
            } else {
                for (size_t row = 0; row < count && row < 5; ++row) {
                    int y = 354 + (int)row * 40;
                    if (ui_pointer_activated(&event, 88, y, 360, 28)) {
                        selected = row;
                    }
                }
                if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                    focus = 0;
                } else if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                    focus = 1;
                } else if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                    focus = 2;
                } else if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                    focus = 3;
                } else if (ui_pointer_activated(&event, btn_x5, btn_y, btn_w, btn_h)) {
                    focus = 4;
                } else {
                    continue;
                }
            }
        } else if (!event.key_ready) {
            continue;
        } else if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 5;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 4 : focus - 1;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_UP && selected > 0) {
            selected--;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_DOWN && selected + 1 < count) {
            selected++;
            continue;
        } else {
            char shortcut = ascii_lower((char)event.key.unicode_char);
            if (shortcut == 't') {
                focus = 0;
            } else if (shortcut == 'p') {
                focus = 1;
            } else if (shortcut == 'c') {
                focus = 2;
            } else if (shortcut == 'u') {
                focus = 3;
            } else if (shortcut == 'n') {
                focus = 4;
            } else if (event.key.unicode_char != '\r') {
                continue;
            }
        }

        if (focus == 0) {
            g_desktop.bluetooth_enabled = !g_desktop.bluetooth_enabled;
            if (!g_desktop.bluetooth_enabled) {
                for (size_t i = 0; i < BLUETOOTH_DEVICE_MAX; ++i) {
                    g_bluetooth_devices[i].connected = false;
                }
            }
            desktop_note_launch("BLUETOOTH", g_desktop.bluetooth_enabled ? "Bluetooth dashboard enabled." :
                                                                      "Bluetooth dashboard disabled.");
            desktop_commit_preferences();
            continue;
        }
        if (focus == 1) {
            size_t slot = BLUETOOTH_DEVICE_MAX;
            memset(name, 0, sizeof(name));
            memset(kind, 0, sizeof(kind));
            if (!ui_prompt_line("DEVICE NAME", "ENTER A DEVICE NAME TO PAIR", "", name, sizeof(name)) ||
                name[0] == '\0') {
                continue;
            }
            if (!ui_prompt_line("DEVICE TYPE", "ENTER A SHORT TYPE (HEADPHONES, PHONE, ETC)", "", kind, sizeof(kind)) ||
                kind[0] == '\0') {
                continue;
            }
            for (size_t i = 0; i < BLUETOOTH_DEVICE_MAX; ++i) {
                if (g_bluetooth_devices[i].used && strcmp(g_bluetooth_devices[i].name, name) == 0) {
                    slot = i;
                    break;
                }
                if (!g_bluetooth_devices[i].used && slot == BLUETOOTH_DEVICE_MAX) {
                    slot = i;
                }
            }
            if (slot == BLUETOOTH_DEVICE_MAX) {
                desktop_note_launch("BLUETOOTH", "No free Bluetooth device slots remain.");
                continue;
            }
            memset(&g_bluetooth_devices[slot], 0, sizeof(g_bluetooth_devices[slot]));
            g_bluetooth_devices[slot].used = true;
            g_bluetooth_devices[slot].paired = true;
            strcopy_limit(g_bluetooth_devices[slot].name, name, sizeof(g_bluetooth_devices[slot].name));
            strcopy_limit(g_bluetooth_devices[slot].kind, kind, sizeof(g_bluetooth_devices[slot].kind));
            desktop_note_launch("BLUETOOTH", "Stored the Bluetooth pairing profile.");
            desktop_commit_preferences();
            continue;
        }
        if (focus == 2) {
            if (count == 0) {
                desktop_note_launch("BLUETOOTH", "No Bluetooth pairing is available to connect.");
                continue;
            }
            if (!g_desktop.bluetooth_enabled) {
                desktop_note_launch("BLUETOOTH", "Turn Bluetooth on before selecting a device profile.");
                continue;
            }
            for (size_t i = 0; i < BLUETOOTH_DEVICE_MAX; ++i) {
                g_bluetooth_devices[i].connected = false;
            }
            g_bluetooth_devices[slots[selected]].connected = true;
            desktop_note_launch("BLUETOOTH", bluetooth_controller_available() ?
                                              "Connected to the selected Bluetooth device." :
                                              "Selected the Bluetooth device. This VM still exposes no guest controller.");
            desktop_commit_preferences();
            continue;
        }
        if (focus == 3) {
            if (count == 0) {
                continue;
            }
            memset(&g_bluetooth_devices[slots[selected]], 0, sizeof(g_bluetooth_devices[slots[selected]]));
            if (selected > 0 && selected >= count - 1) {
                selected--;
            }
            desktop_note_launch("BLUETOOTH", "Removed the selected Bluetooth pairing.");
            desktop_commit_preferences();
            continue;
        }
        return;
    }
}

static void browser_build_page(const char *address) {
    arm_fs_entry_t *entry = NULL;
    char snsx_address[96];
    char session_address[96];
    char link_address[96];

    g_browser_page[0] = '\0';
    browser_links_reset();
    network_compose_snsx_address(snsx_address, sizeof(snsx_address));
    network_compose_session_address(session_address, sizeof(session_address));
    network_compose_link_address(link_address, sizeof(link_address));

    if (address == NULL || address[0] == '\0' || strcmp(address, "snsx://home") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSX Browsing\n"
                      "=============\n"
                      "Bookmarks\n"
                      "snsx://home\n"
                      "snsx://tabs\n"
                      "snsx://bookmarks\n"
                      "snsx://history\n"
                      "snsx://downloads\n"
                      "snsx://studio\n"
                      "snsx://preview\n"
                      "snsx://quickstart\n"
                      "snsx://install\n"
                      "snsx://store\n"
                      "snsx://baremetal\n"
                      "snsx://network\n"
                      "snsx://identity\n"
                      "snsx://wireless\n"
                      "snsx://protocols\n"
                      "snsx://apps\n"
                      "snsx://snake\n"
                      "snsx://bluetooth\n"
                      "snsx://browser\n"
                      "snsxdiag://network\n"
                      "snsxnet://status\n"
                      "snsxnet://identity\n"
                      "\n"
                      "Identity\n",
                      sizeof(g_browser_page));
        strcat(g_browser_page, snsx_address);
        strcat(g_browser_page, "\n");
        strcat(g_browser_page, session_address);
        strcat(g_browser_page, "\n");
        strcat(g_browser_page, link_address);
        strcat(g_browser_page, "\n\nConnection\n");
        strcat(g_browser_page, g_network.status);
        strcat(g_browser_page, "\n");
        strcat(g_browser_page, g_network.internet);
        strcat(g_browser_page, "\n");
        strcat(g_browser_page, "IPv4: ");
        strcat(g_browser_page, g_network.ipv4);
        strcat(g_browser_page, "  Gateway: ");
        strcat(g_browser_page, g_network.gateway);
        strcat(g_browser_page, "  DNS: ");
        strcat(g_browser_page, g_network.dns_server);
        strcat(g_browser_page, "\nBrowser: ");
        strcat(g_browser_page, g_netstack.browser_status);
        strcat(g_browser_page, "\nFirmware: ");
        strcat(g_browser_page, g_netdiag.firmware_stack);
        strcat(g_browser_page, "\nHardware: ");
        strcat(g_browser_page, g_netdiag.hardware);
        strcat(g_browser_page, "\n\nBrowser State\nTabs: ");
        {
            char number[16];
            u32_to_dec((uint32_t)g_browser.tab_count, number);
            strcat(g_browser_page, number);
        }
        strcat(g_browser_page, "  Bookmarks: ");
        {
            char number[16];
            u32_to_dec((uint32_t)g_browser.bookmark_count, number);
            strcat(g_browser_page, number);
        }
        strcat(g_browser_page, "  History: ");
        {
            char number[16];
            u32_to_dec((uint32_t)g_browser.history_count, number);
            strcat(g_browser_page, number);
        }
        strcat(g_browser_page, "\nActive tab: ");
        strcat(g_browser_page, g_browser.tabs[g_browser.active_tab]);
        strcat(g_browser_page, "\n");
        return;
    }

    if (strcmp(address, "snsx://quickstart") == 0) {
        entry = fs_find("/docs/QUICKSTART.TXT");
    } else if (strcmp(address, "snsx://tabs") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSX Browser Tabs\n"
                      "=================\n"
                      "Use 1-4 to jump between tabs.\n"
                      "Press N in the browser to open a new tab with an address.\n\n",
                      sizeof(g_browser_page));
        for (size_t i = 0; i < g_browser.tab_count; ++i) {
            strcat(g_browser_page, i == g_browser.active_tab ? "* " : "- ");
            {
                char number[8];
                u32_to_dec((uint32_t)(i + 1), number);
                strcat(g_browser_page, number);
            }
            strcat(g_browser_page, ". ");
            strcat(g_browser_page, g_browser.tabs[i]);
            strcat(g_browser_page, "\n");
        }
        return;
    } else if (strcmp(address, "snsx://bookmarks") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSX Bookmarks\n"
                      "==============\n"
                      "Press M in the browser to add or remove the current page.\n\n",
                      sizeof(g_browser_page));
        if (g_browser.bookmark_count == 0) {
            strcat(g_browser_page, "No bookmarks saved yet.\n");
        } else {
            for (size_t i = 0; i < g_browser.bookmark_count; ++i) {
                strcat(g_browser_page, "- ");
                strcat(g_browser_page, g_browser.bookmarks[i]);
                strcat(g_browser_page, "\n");
            }
        }
        return;
    } else if (strcmp(address, "snsx://history") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSX History\n"
                      "============\n\n",
                      sizeof(g_browser_page));
        if (g_browser.history_count == 0) {
            strcat(g_browser_page, "No pages have been visited yet.\n");
        } else {
            for (size_t i = 0; i < g_browser.history_count; ++i) {
                strcat(g_browser_page, "- ");
                strcat(g_browser_page, g_browser.history[i]);
                strcat(g_browser_page, "\n");
            }
        }
        return;
    } else if (strcmp(address, "snsx://downloads") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSX Downloads\n"
                      "==============\n"
                      "Page snapshots are stored under /downloads.\n\n",
                      sizeof(g_browser_page));
        if (g_browser.download_count == 0) {
            strcat(g_browser_page, "No saved page snapshots yet.\n");
        } else {
            for (size_t i = 0; i < g_browser.download_count; ++i) {
                strcat(g_browser_page, "- ");
                strcat(g_browser_page, g_browser.downloads[i]);
                strcat(g_browser_page, "\n");
            }
        }
        return;
    } else if (strcmp(address, "snsx://studio") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSX Code Studio\n"
                      "================\n"
                      "Active file: ",
                      sizeof(g_browser_page));
        strcat(g_browser_page, g_studio.active_path);
        strcat(g_browser_page, "\nLanguage: ");
        strcat(g_browser_page, g_studio.language);
        strcat(g_browser_page, "\nStatus: ");
        strcat(g_browser_page, g_studio.status);
        strcat(g_browser_page, "\nLast action: ");
        strcat(g_browser_page, g_studio.last_action);
        strcat(g_browser_page, "\n\nOutput preview\n");
        strcat(g_browser_page, g_studio.output);
        return;
    } else if (strcmp(address, "snsx://preview") == 0) {
        arm_fs_entry_t *html_entry = fs_find("/projects/index.html");
        arm_fs_entry_t *css_entry = fs_find("/projects/style.css");
        strcopy_limit(g_browser_page,
                      "SNSX HTML Preview\n"
                      "=================\n",
                      sizeof(g_browser_page));
        if (html_entry != NULL && html_entry->type == ARM_FS_TEXT) {
            char rendered[BROWSER_PAGE_MAX];
            netstack_render_html_body((const char *)html_entry->data, rendered, sizeof(rendered));
            strcat(g_browser_page, rendered);
        } else {
            strcat(g_browser_page, "No /projects/index.html file is available yet.\n");
        }
        if (css_entry != NULL && css_entry->type == ARM_FS_TEXT) {
            strcat(g_browser_page, "\n\nCSS Source\n----------\n");
            {
                size_t used = strlen(g_browser_page);
                size_t copy = css_entry->size < sizeof(g_browser_page) - used - 1 ?
                              css_entry->size : sizeof(g_browser_page) - used - 1;
                memcpy(g_browser_page + used, css_entry->data, copy);
                g_browser_page[used + copy] = '\0';
            }
        }
        return;
    } else if (strcmp(address, "snsx://install") == 0) {
        entry = fs_find("/docs/INSTALL.TXT");
    } else if (strcmp(address, "snsx://store") == 0) {
        size_t installed_count = 0;
        strcopy_limit(g_browser_page,
                      "SNSX App Store\n"
                      "==============\n"
                      "Catalog\n",
                      sizeof(g_browser_page));
        for (size_t i = 0; i < ARRAY_SIZE(g_store_catalog); ++i) {
            strcat(g_browser_page, g_store_catalog[i].id);
            strcat(g_browser_page, "  [");
            strcat(g_browser_page, store_status_label(i));
            strcat(g_browser_page, "] ");
            strcat(g_browser_page, g_store_catalog[i].category);
            strcat(g_browser_page, "  ");
            strcat(g_browser_page, g_store_catalog[i].name);
            strcat(g_browser_page, "\n");
            if (g_store_installed[i]) {
                installed_count++;
            }
        }
        strcat(g_browser_page, "\nInstalled packages: ");
        {
            char count_text[16];
            u32_to_dec((uint32_t)installed_count, count_text);
            strcat(g_browser_page, count_text);
        }
        strcat(g_browser_page, "\nUse the Store app or shell commands store, pkglist, installpkg ID, and removepkg ID.\n");
        return;
    } else if (strcmp(address, "snsx://baremetal") == 0) {
        entry = fs_find("/docs/BAREMETAL.TXT");
    } else if (strcmp(address, "snsx://network") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSX Network Summary\n"
                      "====================\n"
                      "Preferred mode: ",
                      sizeof(g_browser_page));
        strcat(g_browser_page, g_network.preferred_transport);
        strcat(g_browser_page, "\nTransport: ");
        strcat(g_browser_page, g_network.transport);
        strcat(g_browser_page, "\nIPv4: ");
        strcat(g_browser_page, g_network.ipv4);
        strcat(g_browser_page, "\nSubnet: ");
        strcat(g_browser_page, g_network.subnet_mask);
        strcat(g_browser_page, "\nGateway: ");
        strcat(g_browser_page, g_network.gateway);
        strcat(g_browser_page, "\nDNS: ");
        strcat(g_browser_page, g_network.dns_server);
        strcat(g_browser_page, "\nMAC: ");
        strcat(g_browser_page, g_network.mac);
        strcat(g_browser_page, "\nFirmware: ");
        strcat(g_browser_page, g_netdiag.firmware_stack);
        strcat(g_browser_page, "\nHardware: ");
        strcat(g_browser_page, g_netdiag.hardware);
        strcat(g_browser_page, "\n\n");
        strcat(g_browser_page, g_network.status);
        strcat(g_browser_page, "\n");
        strcat(g_browser_page, g_network.internet);
        return;
    } else if (strcmp(address, "snsx://identity") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSX Identity\n"
                      "=============\n"
                      "Host name: ",
                      sizeof(g_browser_page));
        strcat(g_browser_page, g_network.hostname);
        strcat(g_browser_page, "\nSNSX address: ");
        strcat(g_browser_page, snsx_address);
        strcat(g_browser_page, "\nSession route: ");
        strcat(g_browser_page, session_address);
        strcat(g_browser_page, "\nLink address: ");
        strcat(g_browser_page, link_address);
        strcat(g_browser_page, "\nBrowser status: ");
        strcat(g_browser_page, g_netstack.browser_status);
        return;
    } else if (strcmp(address, "snsx://wireless") == 0) {
        size_t wifi_slots[WIFI_PROFILE_MAX];
        size_t bt_slots[BLUETOOTH_DEVICE_MAX];
        size_t wifi_count = wifi_collect_slots(wifi_slots, ARRAY_SIZE(wifi_slots));
        size_t bt_count = bluetooth_collect_slots(bt_slots, ARRAY_SIZE(bt_slots));
        strcopy_limit(g_browser_page,
                      "SNSX Wireless\n"
                      "=============\n"
                      "Wi-Fi dashboard: ",
                      sizeof(g_browser_page));
        strcat(g_browser_page, g_desktop.wifi_enabled ? "on" : "off");
        strcat(g_browser_page, "\nBluetooth dashboard: ");
        strcat(g_browser_page, g_desktop.bluetooth_enabled ? "on" : "off");
        strcat(g_browser_page, "\nSaved Wi-Fi profiles: ");
        {
            char count_text[16];
            u32_to_dec((uint32_t)wifi_count, count_text);
            strcat(g_browser_page, count_text);
        }
        strcat(g_browser_page, "\nSaved Bluetooth devices: ");
        {
            char count_text[16];
            u32_to_dec((uint32_t)bt_count, count_text);
            strcat(g_browser_page, count_text);
        }
        strcat(g_browser_page, "\nWi-Fi Driver Lab: ");
        strcat(g_browser_page, g_store_installed[STORE_PKG_WIFI_DRIVER_LAB] ? "installed" : "available in Store");
        strcat(g_browser_page, "\nBluetooth Driver Lab: ");
        strcat(g_browser_page, g_store_installed[STORE_PKG_BLUETOOTH_DRIVER_LAB] ? "installed" : "available in Store");
        strcat(g_browser_page, "\n\nVirtualBox note: nearby Wi-Fi and Bluetooth discovery still require guest device drivers.\n");
        return;
    } else if (strcmp(address, "snsx://protocols") == 0) {
        entry = fs_find("/docs/PROTOCOLS.TXT");
    } else if (strcmp(address, "snsx://apps") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSX Apps\n"
                      "=========\n"
                      "Files\n"
                      "Notes\n"
                      "Calculator\n"
                      "Code Studio\n"
                      "Network Center\n"
                      "Browser\n"
                      "Store\n"
                      "Snake\n"
                      "\nOpen snsx://studio for development, snsx://store for packages, or snsx://snake for the arcade page.\n",
                      sizeof(g_browser_page));
        return;
    } else if (strcmp(address, "snsx://snake") == 0) {
        entry = fs_find("/docs/SNAKE.TXT");
    } else if (strcmp(address, "snsx://networkdoc") == 0) {
        entry = fs_find("/docs/NETWORK.TXT");
    } else if (strcmp(address, "snsx://bluetooth") == 0) {
        entry = fs_find("/docs/BLUETOOTH.TXT");
    } else if (strcmp(address, "snsx://browser") == 0) {
        entry = fs_find("/docs/BROWSER.TXT");
    } else if (strcmp(address, "snsxdiag://network") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSX Firmware Network Diagnostics\n"
                      "=================================\n"
                      "Firmware stack: ",
                      sizeof(g_browser_page));
        strcat(g_browser_page, g_netdiag.firmware_stack);
        strcat(g_browser_page, "\nPCI hardware: ");
        strcat(g_browser_page, g_netdiag.hardware);
        strcat(g_browser_page, "\nWireless note: ");
        strcat(g_browser_page, g_netdiag.wireless);
        strcat(g_browser_page, "\n\nStatus: ");
        strcat(g_browser_page, g_network.status);
        strcat(g_browser_page, "\nInternet: ");
        strcat(g_browser_page, g_network.internet);
        strcat(g_browser_page, "\n\nVirtualBox note\n");
        strcat(g_browser_page, "Host Wi-Fi is presented to the guest as Ethernet, so nearby hotspot names do not appear inside SNSX on this target.\n");
        strcat(g_browser_page, "Real nearby Wi-Fi and Bluetooth discovery need passthrough or guest-visible wireless hardware.\n");
        return;
    } else if (strcmp(address, "snsxnet://status") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSXNet Status\n"
                      "==============\n"
                      "Route: ",
                      sizeof(g_browser_page));
        strcat(g_browser_page, session_address);
        strcat(g_browser_page, "\nLast host: ");
        strcat(g_browser_page, g_netstack.last_host);
        strcat(g_browser_page, "\nLast IP: ");
        strcat(g_browser_page, g_netstack.last_ip);
        strcat(g_browser_page, "\nStatus: ");
        strcat(g_browser_page, g_network.status);
        strcat(g_browser_page, "\nInternet: ");
        strcat(g_browser_page, g_network.internet);
        return;
    } else if (strcmp(address, "snsxnet://identity") == 0) {
        strcopy_limit(g_browser_page,
                      "SNSXNet Identity\n"
                      "================\n",
                      sizeof(g_browser_page));
        strcat(g_browser_page, snsx_address);
        strcat(g_browser_page, "\n");
        strcat(g_browser_page, session_address);
        strcat(g_browser_page, "\n");
        strcat(g_browser_page, link_address);
        return;
    }

    if (entry != NULL && entry->type == ARM_FS_TEXT) {
        size_t copy = entry->size < sizeof(g_browser_page) - 1 ? entry->size : sizeof(g_browser_page) - 1;
        memcpy(g_browser_page, entry->data, copy);
        g_browser_page[copy] = '\0';
        return;
    }

    if (strncmp(address, "http://", 7) == 0 || strncmp(address, "https://", 8) == 0) {
        if (!netstack_http_fetch_text(address, g_browser_page, sizeof(g_browser_page))) {
            if (g_browser_page[0] == '\0') {
                strcopy_limit(g_browser_page,
                              "SNSX Browser could not fetch that remote page.\n"
                              "Open Network Center and run CONNECT to renew DHCP, then try the URL again.\n",
                              sizeof(g_browser_page));
            }
        }
        return;
    }

    strcopy_limit(g_browser_page,
                  "Unknown SNSX page.\n"
                  "Try snsx://home, snsx://tabs, snsx://bookmarks, snsx://history, snsx://downloads, snsx://studio,\n"
                  "snsx://preview, snsx://identity, snsx://store, snsx://baremetal, snsx://network, snsx://wireless,\n"
                  "snsx://apps, snsx://snake, snsx://browser, snsxdiag://network, snsxnet://status, or an http:// / https:// URL.\n",
                  sizeof(g_browser_page));
}

static void ui_run_browser_app(void) {
    ui_event_t event;
    int scroll = 0;
    size_t focus = 0;
    char input[BROWSER_ADDRESS_MAX];
    bool auto_connect_checked = false;

    while (1) {
        int text_x = 56;
        int text_y = 188;
        int link_panel_w = g_browser_link_count > 0 ? 290 : 0;
        int link_x = (int)g_gfx.width - 56 - link_panel_w;
        int link_y = 188;
        int link_h = (int)g_gfx.height - 300;
        int text_w = (int)g_gfx.width - 112 - (link_panel_w > 0 ? (link_panel_w + 20) : 0);
        int text_h = (int)g_gfx.height - 300;
        int btn_y = (int)g_gfx.height - 156;
        int btn_gap = 10;
        int btn_w = ((int)g_gfx.width - 144 - btn_gap * 5) / 6;
        int btn_h = 46;
        int btn_x1 = 72;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;
        int btn_x5 = btn_x4 + btn_w + btn_gap;
        int btn_x6 = btn_x5 + btn_w + btn_gap;
        char tab_text[32];
        char bookmark_text[32];
        char save_label[32];

        if (!auto_connect_checked && g_desktop.network_autoconnect && !g_netstack.configured) {
            network_refresh(true);
            auto_connect_checked = true;
        }
        browser_build_page(g_browser_address);
        browser_finalize_page(g_browser_page, sizeof(g_browser_page));
        link_panel_w = g_browser_link_count > 0 ? 290 : 0;
        link_x = (int)g_gfx.width - 56 - link_panel_w;
        text_w = (int)g_gfx.width - 112 - (link_panel_w > 0 ? (link_panel_w + 20) : 0);
        ui_draw_window("SNSX BROWSER", "LOCAL DOCS, SNSX PROTOCOLS, HTTP, HTTPS BRIDGE, AND LIVE STATUS PAGES");
        gfx_draw_panel(56, 144, (int)g_gfx.width - 112, 30, gfx_rgb(255, 255, 255), gfx_rgb(68, 98, 133));
        gfx_draw_text(70, 154, g_browser_address, 1, gfx_rgb(25, 43, 66));
        strcopy_limit(tab_text, "TAB ", sizeof(tab_text));
        {
            char number[8];
            u32_to_dec((uint32_t)(g_browser.active_tab + 1), number);
            strcat(tab_text, number);
            strcat(tab_text, "/");
            u32_to_dec((uint32_t)g_browser.tab_count, number);
            strcat(tab_text, number);
        }
        strcopy_limit(bookmark_text, browser_is_bookmarked(g_browser_address) ? "BOOKMARKED" : "NOT BOOKMARKED",
                      sizeof(bookmark_text));
        strcopy_limit(save_label, "SAVE PAGE", sizeof(save_label));
        ui_draw_badge((int)g_gfx.width - 320, 144, tab_text,
                      theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_TEXT));
        ui_draw_badge((int)g_gfx.width - 192, 144, bookmark_text,
                      theme_color(THEME_ROLE_ACCENT), theme_color(THEME_ROLE_TEXT_LIGHT));
        gfx_draw_text_block(text_x, text_y, text_w, text_h, g_browser_page, strlen(g_browser_page),
                            2, gfx_rgb(27, 40, 58), scroll, -1);
        if (link_panel_w > 0) {
            int panel_y = link_y;
            int panel_h = link_h;
            int item_y = panel_y + 54;
            int item_h = 40;
            int visible = (panel_h - 96) / (item_h + 8);
            if (visible > (int)g_browser_link_count) {
                visible = (int)g_browser_link_count;
            }
            if (visible > 6) {
                visible = 6;
            }
            gfx_draw_panel(link_x, panel_y, link_panel_w, panel_h,
                           theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
            gfx_draw_text(link_x + 18, panel_y + 18, "PAGE LINKS", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
            gfx_draw_text(link_x + 18, panel_y + 42, "Click a link tile or press 1-9.", 1, theme_color(THEME_ROLE_TEXT_SOFT));
            for (int i = 0; i < visible; ++i) {
                char label[96];
                bool hover = g_pointer.engaged &&
                             point_in_rect(g_pointer.x, g_pointer.y, link_x + 14, item_y + i * (item_h + 8), link_panel_w - 28, item_h);
                browser_compose_link_label((size_t)i, label, sizeof(label));
                gfx_draw_panel(link_x + 14, item_y + i * (item_h + 8), link_panel_w - 28, item_h,
                               hover ? theme_color(THEME_ROLE_WARM) : theme_color(THEME_ROLE_PANEL),
                               hover ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_BORDER));
                gfx_draw_text(link_x + 28, item_y + 12 + i * (item_h + 8), label, 1, theme_color(THEME_ROLE_TEXT));
            }
            gfx_draw_text(link_x + 18, panel_y + panel_h - 56, g_netstack.configured ? "ONLINE" : "OFFLINE",
                          2, g_netstack.configured ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_DANGER));
            gfx_draw_text(link_x + 18, panel_y + panel_h - 32, g_netstack.configured ? g_network.ipv4 : g_network.status,
                          1, theme_color(THEME_ROLE_TEXT_SOFT));
        }
        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, "HOME",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "ADDRESS",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "TABS",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "BOOKMARKS",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 3);
        ui_draw_button_focus(btn_x5, btn_y, btn_w, btn_h, "HISTORY",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 4);
        ui_draw_button_focus(btn_x6, btn_y, btn_w, btn_h, save_label,
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 5);
        ui_draw_footer("H HOME  A ADDRESS  T TABS  B BOOKMARKS  R HISTORY  M TOGGLE MARK  S SAVE  D DOWNLOADS  N NEW TAB  L OPEN LINK  [ ] TABS  1-9 LINKS  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event) ||
            (event.key_ready && event.key.scan_code == EFI_SCAN_ESC)) {
            return;
        }
        if (event.mouse_clicked || event.mouse_released) {
            if (link_panel_w > 0) {
                int item_y = link_y + 54;
                int item_h = 40;
                int visible = (link_h - 96) / (item_h + 8);
                if (visible > (int)g_browser_link_count) {
                    visible = (int)g_browser_link_count;
                }
                if (visible > 6) {
                    visible = 6;
                }
                for (int i = 0; i < visible; ++i) {
                    if (ui_pointer_activated(&event, link_x + 14, item_y + i * (item_h + 8), link_panel_w - 28, item_h)) {
                        if (browser_open_link_index((size_t)i)) {
                            scroll = 0;
                        }
                        continue;
                    }
                }
            }
            if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                focus = 0;
            } else if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                focus = 1;
            } else if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                focus = 2;
            } else if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                focus = 3;
            } else if (ui_pointer_activated(&event, btn_x5, btn_y, btn_w, btn_h)) {
                focus = 4;
            } else if (ui_pointer_activated(&event, btn_x6, btn_y, btn_w, btn_h)) {
                focus = 5;
            } else {
                continue;
            }
        } else if (!event.key_ready) {
            continue;
        } else if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 6;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 5 : focus - 1;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_UP) {
            if (scroll > 0) {
                scroll--;
            }
            continue;
        } else if (event.key.scan_code == EFI_SCAN_DOWN) {
            scroll++;
            continue;
        } else {
            char shortcut = ascii_lower((char)event.key.unicode_char);
            if (shortcut >= '1' && shortcut <= '9') {
                size_t link = (size_t)(shortcut - '1');
                if (link < g_browser_link_count && browser_open_link_index(link)) {
                    scroll = 0;
                } else {
                    size_t tab = (size_t)(shortcut - '1');
                    if (tab < g_browser.tab_count) {
                        browser_activate_tab(tab);
                        scroll = 0;
                    }
                }
                continue;
            }
            if (shortcut == 'h') {
                focus = 0;
            } else if (shortcut == 'a') {
                focus = 1;
            } else if (shortcut == 't') {
                focus = 2;
            } else if (shortcut == 'b') {
                focus = 3;
            } else if (shortcut == 'r') {
                focus = 4;
            } else if (shortcut == 's') {
                focus = 5;
            } else if (shortcut == 'm') {
                bool added = false;
                browser_toggle_bookmark(g_browser_address, &added);
                desktop_note_launch("BROWSER", added ? "Saved the current page to bookmarks." :
                                                        "Removed the current page from bookmarks.");
                continue;
            } else if (shortcut == 'd') {
                browser_navigate_to("snsx://downloads");
                scroll = 0;
                continue;
            } else if (shortcut == 'p') {
                browser_navigate_to("snsx://preview");
                scroll = 0;
                continue;
            } else if (shortcut == 'n') {
                input[0] = '\0';
                if (ui_prompt_line("NEW TAB", "ENTER AN ADDRESS FOR THE NEW TAB", g_browser_address, input, sizeof(input)) &&
                    input[0] != '\0') {
                    browser_open_tab(input);
                    scroll = 0;
                }
                continue;
            } else if (shortcut == 'l') {
                char link_choice[16];
                link_choice[0] = '\0';
                if (g_browser_link_count == 0) {
                    desktop_note_launch("BROWSER", "There are no extracted links on this page.");
                    continue;
                }
                if (ui_prompt_line("OPEN LINK", "ENTER THE LINK NUMBER SHOWN ON THE PAGE", "", link_choice, sizeof(link_choice))) {
                    int index = atoi(link_choice);
                    if (index > 0 && (size_t)index <= g_browser_link_count && browser_open_link_index((size_t)index - 1)) {
                        scroll = 0;
                    } else {
                        desktop_note_launch("BROWSER", "That link number is not available on this page.");
                    }
                }
                continue;
            } else if (shortcut == '[') {
                if (g_browser.active_tab > 0) {
                    browser_activate_tab(g_browser.active_tab - 1);
                    scroll = 0;
                }
                continue;
            } else if (shortcut == ']') {
                if (g_browser.active_tab + 1 < g_browser.tab_count) {
                    browser_activate_tab(g_browser.active_tab + 1);
                    scroll = 0;
                }
                continue;
            } else if (event.key.unicode_char != '\r') {
                continue;
            }
        }

        if (focus == 0) {
            browser_navigate_to("snsx://home");
            scroll = 0;
        } else if (focus == 1) {
            strcopy_limit(input, g_browser_address, sizeof(input));
            if (ui_prompt_line("ADDRESS", "ENTER SNSX OR WEB ADDRESS", input, input, sizeof(input)) && input[0] != '\0') {
                browser_navigate_to(input);
                scroll = 0;
            }
        } else if (focus == 2) {
            browser_navigate_to("snsx://tabs");
            scroll = 0;
        } else if (focus == 3) {
            browser_navigate_to("snsx://bookmarks");
            scroll = 0;
        } else if (focus == 4) {
            browser_navigate_to("snsx://history");
            scroll = 0;
        } else {
            char saved_path[FS_PATH_MAX];
            if (browser_save_page_snapshot(g_browser_address, g_browser_page, saved_path, sizeof(saved_path))) {
                desktop_note_launch("BROWSER", saved_path);
            } else {
                desktop_note_launch("BROWSER", "SNSX could not save the current page snapshot.");
            }
        }
    }
}

static void ui_run_code_studio_app(void) {
    ui_event_t event;
    size_t focus = 0;
    char input[FS_PATH_MAX];

    while (1) {
        int info_x = 56;
        int info_y = 154;
        int info_w = 340;
        int info_h = (int)g_gfx.height - 308;
        int output_x = info_x + info_w + 20;
        int output_y = info_y;
        int output_w = (int)g_gfx.width - output_x - 56;
        int output_h = info_h;
        int btn_y = (int)g_gfx.height - 142;
        int btn_gap = 12;
        int btn_w = ((int)g_gfx.width - 112 - btn_gap * 4) / 5;
        int btn_h = 46;
        int btn_x1 = 56;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;
        int btn_x5 = btn_x4 + btn_w + btn_gap;

        ui_draw_window("SNSX CODE STUDIO", "HOST-BACKED DEVELOPMENT FOR SNSX, PYTHON, JAVASCRIPT, C, C++, HTML, AND CSS");
        gfx_draw_panel(info_x, info_y, info_w, info_h, theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_panel(output_x, output_y, output_w, output_h, theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(info_x + 18, info_y + 18, "WORKSPACE", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(info_x + 18, info_y + 52, "ACTIVE FILE", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(info_x + 132, info_y + 52, g_studio.active_path, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(info_x + 18, info_y + 78, "LANGUAGE", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(info_x + 132, info_y + 78, g_studio.language, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(info_x + 18, info_y + 104, "STATUS", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text_block(info_x + 18, info_y + 128, info_w - 36, 88, g_studio.status, strlen(g_studio.status),
                            1, theme_color(THEME_ROLE_TEXT), 0, -1);
        gfx_draw_text(info_x + 18, info_y + 232, "QUICK FILES", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(info_x + 18, info_y + 266, "/projects/main.snsx", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(info_x + 18, info_y + 288, "/projects/main.py", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(info_x + 18, info_y + 310, "/projects/main.js", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(info_x + 18, info_y + 332, "/projects/main.c", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(info_x + 18, info_y + 354, "/projects/main.cpp", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(info_x + 18, info_y + 376, "/projects/index.html", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(info_x + 18, info_y + 398, "/projects/style.css", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(info_x + 18, info_y + 434, "Host bridge", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(info_x + 108, info_y + 434, "10.0.2.2:8790", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(info_x + 18, info_y + 458, "If run/build fails, start make dev-bridge on the host.", 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));

        gfx_draw_text(output_x + 18, output_y + 18, "OUTPUT", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text_block(output_x + 18, output_y + 54, output_w - 36, output_h - 72,
                            g_studio.output, strlen(g_studio.output), 1, theme_color(THEME_ROLE_TEXT), 0, -1);

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, "FILE",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "EDIT",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "RUN",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "BUILD",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 3);
        ui_draw_button_focus(btn_x5, btn_y, btn_w, btn_h, "GUIDE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 4);
        ui_draw_footer("F FILE  E EDIT  R RUN  B BUILD  G GUIDE  P PREVIEW  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event) ||
            (event.key_ready && event.key.scan_code == EFI_SCAN_ESC)) {
            return;
        }
        if (event.mouse_clicked || event.mouse_released) {
            if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                focus = 0;
            } else if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                focus = 1;
            } else if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                focus = 2;
            } else if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                focus = 3;
            } else if (ui_pointer_activated(&event, btn_x5, btn_y, btn_w, btn_h)) {
                focus = 4;
            } else {
                continue;
            }
        } else if (!event.key_ready) {
            continue;
        } else if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 5;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 4 : focus - 1;
            continue;
        } else {
            char shortcut = ascii_lower((char)event.key.unicode_char);
            if (shortcut == 'f') {
                focus = 0;
            } else if (shortcut == 'e') {
                focus = 1;
            } else if (shortcut == 'r') {
                focus = 2;
            } else if (shortcut == 'b') {
                focus = 3;
            } else if (shortcut == 'g') {
                focus = 4;
            } else if (shortcut == 'p') {
                browser_navigate_to("snsx://preview");
                ui_run_browser_app();
                continue;
            } else if (event.key.unicode_char != '\r') {
                continue;
            }
        }

        if (focus == 0) {
            strcopy_limit(input, g_studio.active_path, sizeof(input));
            if (ui_prompt_line("WORKSPACE FILE", "ENTER A FILE NAME OR /projects PATH", input, input, sizeof(input)) &&
                input[0] != '\0') {
                char path[FS_PATH_MAX];
                studio_resolve_input_path(input, path);
                studio_ensure_path_exists(path);
                studio_select_path(path);
            }
            continue;
        }
        if (focus == 1) {
            studio_ensure_path_exists(g_studio.active_path);
            ui_edit_text_file(g_studio.active_path);
            studio_select_path(g_studio.active_path);
            continue;
        }
        if (focus == 2) {
            studio_execute_bridge("run");
            continue;
        }
        if (focus == 3) {
            studio_execute_bridge("build");
            continue;
        }
        ui_view_text_file("/docs/STUDIO.TXT", "SNSX CODE STUDIO");
    }
}

static void app_code_studio_console(void) {
    char line[SHELL_INPUT_MAX];
    char *argv[SHELL_ARG_MAX];

    while (1) {
        console_clear();
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_writeline("SNSX Code Studio");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_divider();
        console_write("Active file: ");
        console_writeline(g_studio.active_path);
        console_write("Language: ");
        console_writeline(g_studio.language);
        console_write("Status: ");
        console_writeline(g_studio.status);
        console_divider();
        console_writeline(g_studio.output);
        console_divider();
        console_writeline("Commands: file PATH  edit  run  build  guide  preview  back");
        console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
        console_write("studio");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("> ");
        line_read(line, sizeof(line));

        if (line[0] == '\0') {
            continue;
        }
        if (strcmp(line, "back") == 0 || strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            return;
        }
        if (strcmp(line, "edit") == 0) {
            studio_ensure_path_exists(g_studio.active_path);
            app_edit(g_studio.active_path);
            studio_select_path(g_studio.active_path);
            continue;
        }
        if (strcmp(line, "run") == 0) {
            studio_execute_bridge("run");
            continue;
        }
        if (strcmp(line, "build") == 0) {
            studio_execute_bridge("build");
            continue;
        }
        if (strcmp(line, "guide") == 0) {
            view_text("/docs/STUDIO.TXT");
            continue;
        }
        if (strcmp(line, "preview") == 0) {
            browser_build_page("snsx://preview");
            browser_finalize_page(g_browser_page, sizeof(g_browser_page));
            console_writeline(g_browser_page);
            wait_for_enter(NULL);
            continue;
        }
        {
            size_t argc = shell_tokenize(line, argv, ARRAY_SIZE(argv));
            if (argc >= 2 && strcmp(argv[0], "file") == 0) {
                char path[FS_PATH_MAX];
                studio_resolve_input_path(argv[1], path);
                studio_ensure_path_exists(path);
                studio_select_path(path);
                continue;
            }
        }
        wait_for_enter("Unknown studio command.");
    }
}

static void ui_run_store_app(void) {
    ui_event_t event;
    size_t selected = 0;
    size_t focus = 0;
    char status[128];

    while (1) {
        const store_package_manifest_t *pkg = &g_store_catalog[selected];
        const char *asset_text = g_store_installed[selected] ? pkg->asset_path : "Install the package to create its asset file.";
        bool can_remove = g_store_installed[selected] && !pkg->essential;
        int list_x = 72;
        int list_y = 168;
        int list_w = 360;
        int list_h = 356;
        int details_x = 460;
        int details_y = 168;
        int details_w = (int)g_gfx.width - details_x - 72;
        int details_h = 356;
        int row_h = 36;
        int btn_y = (int)g_gfx.height - 152;
        int btn_w = 158;
        int btn_h = 46;
        int btn_gap = 12;
        int btn_x1 = 72;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;

        ui_draw_window("SNSX APP STORE", "APPS, GAMES, DRIVER PACKS, AND BARE-METAL KITS");
        gfx_draw_panel(list_x, list_y, list_w, list_h, theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(list_x + 18, list_y + 18, "CATALOG", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(list_x + 18, list_y + 44, "Core packages stay installed. Optional kits can be added or removed.", 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));

        for (size_t i = 0; i < ARRAY_SIZE(g_store_catalog); ++i) {
            int y = list_y + 82 + (int)i * row_h;
            bool hover = g_pointer.engaged && point_in_rect(g_pointer.x, g_pointer.y, list_x + 14, y - 6, list_w - 28, 28);
            bool active = i == selected || hover;
            uint32_t fill = active ? theme_color(THEME_ROLE_WARM) : theme_color(THEME_ROLE_PANEL);
            uint32_t border = active ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_BORDER);
            gfx_draw_panel(list_x + 14, y - 6, list_w - 28, 28, fill, border);
            gfx_draw_text(list_x + 28, y, g_store_catalog[i].name, 1, theme_color(THEME_ROLE_TEXT));
            gfx_draw_text(list_x + 236, y, g_store_catalog[i].category, 1, theme_color(THEME_ROLE_TEXT_SOFT));
            gfx_draw_text(list_x + 290, y, store_status_label(i), 1, theme_color(THEME_ROLE_TEXT_SOFT));
            if (hover) {
                selected = i;
            }
        }

        gfx_draw_panel(details_x, details_y, details_w, details_h,
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(details_x + 20, details_y + 18, pkg->name, 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        ui_draw_badge(details_x + 20, details_y + 52, pkg->category,
                      theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_TEXT));
        ui_draw_badge(details_x + 140, details_y + 52, store_status_label(selected),
                      theme_color(THEME_ROLE_ACCENT), theme_color(THEME_ROLE_TEXT_LIGHT));
        gfx_draw_text(details_x + 20, details_y + 102, "PACKAGE ID", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(details_x + 152, details_y + 102, pkg->id, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(details_x + 20, details_y + 130, "ASSET", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(details_x + 152, details_y + 130, asset_text, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(details_x + 20, details_y + 166, "SUMMARY", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text_block(details_x + 20, details_y + 192, details_w - 40, 108, pkg->summary, strlen(pkg->summary),
                            1, theme_color(THEME_ROLE_TEXT), 0, -1);
        gfx_draw_text(details_x + 20, details_y + 320,
                      "Installed package assets appear in /apps, /games, /drivers, or /kits.", 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(details_x + 20, details_y + 342,
                      "Use the Bare-Metal guide before installing SNSX to a USB stick or laptop disk.", 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h,
                             g_store_installed[selected] ? "INSTALLED" : "INSTALL",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, can_remove ? "REMOVE" : "LOCKED",
                             theme_color(THEME_ROLE_PANEL_ALT), can_remove ? theme_color(THEME_ROLE_DANGER) : theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "OPEN FILE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "GUIDE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 3);
        ui_draw_footer("UP OR DOWN SELECT  TAB CHANGES ACTION  I INSTALL  R REMOVE  O OPEN  G GUIDE  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event) ||
            (event.key_ready && event.key.scan_code == EFI_SCAN_ESC)) {
            return;
        }
        if (event.mouse_clicked || event.mouse_released) {
            bool handled = false;
            for (size_t i = 0; i < ARRAY_SIZE(g_store_catalog); ++i) {
                int y = list_y + 82 + (int)i * row_h;
                if (ui_pointer_activated(&event, list_x + 14, y - 6, list_w - 28, 28)) {
                    selected = i;
                    handled = true;
                }
            }
            if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                focus = 0;
                handled = true;
            } else if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                focus = 1;
                handled = true;
            } else if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                focus = 2;
                handled = true;
            } else if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                focus = 3;
                handled = true;
            }
            if (!handled) {
                continue;
            }
        } else if (!event.key_ready) {
            continue;
        } else if (event.key.scan_code == EFI_SCAN_UP && selected > 0) {
            selected--;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_DOWN && selected + 1 < ARRAY_SIZE(g_store_catalog)) {
            selected++;
            continue;
        } else if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 4;
            continue;
        } else if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 3 : focus - 1;
            continue;
        } else {
            char shortcut = ascii_lower((char)event.key.unicode_char);
            if (shortcut == 'i') {
                focus = 0;
            } else if (shortcut == 'r') {
                focus = 1;
            } else if (shortcut == 'o') {
                focus = 2;
            } else if (shortcut == 'g' || shortcut == 'b') {
                focus = 3;
            } else if (event.key.unicode_char != '\r') {
                continue;
            }
        }

        if (focus == 0) {
            store_install_package_index(selected, status, sizeof(status));
            desktop_note_launch("STORE", status);
            continue;
        }
        if (focus == 1) {
            store_remove_package_index(selected, status, sizeof(status));
            desktop_note_launch("STORE", status);
            continue;
        }
        if (focus == 2) {
            arm_fs_entry_t *entry = fs_find(pkg->asset_path);
            if (entry != NULL) {
                ui_view_text_file(pkg->asset_path, pkg->name);
            } else if (selected == STORE_PKG_BAREMETAL_KIT) {
                ui_view_text_file("/docs/BAREMETAL.TXT", "SNSX BARE-METAL GUIDE");
            } else {
                ui_view_text_file("/docs/STORE.TXT", "SNSX STORE GUIDE");
            }
            continue;
        }
        if (selected == STORE_PKG_BAREMETAL_KIT) {
            ui_view_text_file("/docs/BAREMETAL.TXT", "SNSX BARE-METAL GUIDE");
        } else {
            ui_view_text_file("/docs/STORE.TXT", "SNSX STORE GUIDE");
        }
    }
}

static bool snake_occupied(int x, int y, const int *snake_x, const int *snake_y, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        if (snake_x[i] == x && snake_y[i] == y) {
            return true;
        }
    }
    return false;
}

static void snake_spawn_food(int *food_x, int *food_y,
                             const int *snake_x, const int *snake_y, size_t length,
                             int board_w, int board_h) {
    for (size_t attempt = 0; attempt < 256; ++attempt) {
        int candidate_x = (int)(netstack_random_next() % (uint32_t)board_w);
        int candidate_y = (int)(netstack_random_next() % (uint32_t)board_h);
        if (!snake_occupied(candidate_x, candidate_y, snake_x, snake_y, length)) {
            *food_x = candidate_x;
            *food_y = candidate_y;
            return;
        }
    }
    *food_x = 0;
    *food_y = 0;
}

static void ui_run_snake_app(void) {
    int snake_x[SNAKE_MAX_SEGMENTS];
    int snake_y[SNAKE_MAX_SEGMENTS];
    const int board_x = 96;
    const int board_y = 176;
    const int board_w = 24;
    const int board_h = 16;
    const int cell = 26;
    const int board_px_w = board_w * cell;
    const int board_px_h = board_h * cell;
    const int side_x = board_x + board_px_w + 28;
    const int btn_y = board_y + board_px_h - 24;
    const int btn_w = 128;
    const int btn_h = 42;
    const int btn_gap = 10;
    const int btn_x1 = side_x;
    const int btn_x2 = btn_x1 + btn_w + btn_gap;
    const int btn_x3 = btn_x2 + btn_w + btn_gap;
    size_t length;
    int dir_x;
    int dir_y;
    int next_dir_x;
    int next_dir_y;
    int food_x;
    int food_y;
    bool paused;
    bool game_over;
    char score_text[16];

restart_game:
    length = 5;
    for (size_t i = 0; i < length; ++i) {
        snake_x[i] = 8 - (int)i;
        snake_y[i] = 7;
    }
    dir_x = 1;
    dir_y = 0;
    next_dir_x = 1;
    next_dir_y = 0;
    paused = false;
    game_over = false;
    snake_spawn_food(&food_x, &food_y, snake_x, snake_y, length, board_w, board_h);

    while (1) {
        ui_draw_window("SNSX SNAKE", "ARCADE APP WITH KEYBOARD PLAY AND DESKTOP POINTER SUPPORT");
        gfx_draw_panel(board_x - 14, board_y - 14, board_px_w + 28, board_px_h + 28,
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        for (int y = 0; y < board_h; ++y) {
            for (int x = 0; x < board_w; ++x) {
                uint32_t fill = ((x + y) & 1) == 0 ? gfx_rgb(214, 232, 218) : gfx_rgb(226, 240, 229);
                gfx_fill_rect(board_x + x * cell, board_y + y * cell, cell - 1, cell - 1, fill);
            }
        }
        gfx_fill_rect(board_x + food_x * cell + 5, board_y + food_y * cell + 5, cell - 10, cell - 10,
                      theme_color(THEME_ROLE_DANGER));
        for (size_t i = 0; i < length; ++i) {
            uint32_t fill = i == 0 ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_ACCENT);
            gfx_fill_rect(board_x + snake_x[i] * cell + 3, board_y + snake_y[i] * cell + 3, cell - 6, cell - 6, fill);
        }

        gfx_draw_panel(side_x, board_y, 288, 208, theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(side_x + 24, board_y + 24, "ARCADE STATUS", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        u32_to_dec((uint32_t)(length > 5 ? length - 5 : 0), score_text);
        gfx_draw_text(side_x + 24, board_y + 60, "Score", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(side_x + 110, board_y + 60, score_text, 2, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(side_x + 24, board_y + 92, paused ? "Paused" : (game_over ? "Game over" : "Running"),
                      1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(side_x + 24, board_y + 118, "Arrows or WASD steer the snake.", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(side_x + 24, board_y + 140, "Space pauses. R restarts. ESC exits.", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(side_x + 24, board_y + 162, "Pointer buttons can pause, restart, or leave.", 1, theme_color(THEME_ROLE_TEXT));

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, paused ? "RESUME" : "PAUSE",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), false);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "RESTART",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), false);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "EXIT",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_DANGER),
                             theme_color(THEME_ROLE_TEXT), false);
        if (game_over) {
            gfx_draw_panel(board_x + 122, board_y + 140, 360, 108,
                           theme_color(THEME_ROLE_PANEL), theme_color(THEME_ROLE_DANGER));
            gfx_draw_text(board_x + 156, board_y + 170, "GAME OVER", 3, theme_color(THEME_ROLE_TEXT));
            gfx_draw_text(board_x + 156, board_y + 208, "PRESS R TO RESTART OR ESC TO EXIT", 1, theme_color(THEME_ROLE_TEXT));
        }
        ui_draw_footer("ARROWS OR WASD MOVE  SPACE PAUSES  R RESTARTS  ESC BACK");
        ui_draw_cursor();

        for (int wait_step = 0; wait_step < 8; ++wait_step) {
            ui_event_t event;
            memset(&event, 0, sizeof(event));
            if (input_try_read_key(&event.key)) {
                event.key_ready = true;
            } else if (!pointer_poll_event(&event)) {
                if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->stall != NULL) {
                    g_st->boot_services->stall(20000);
                } else {
                    input_pause();
                }
                continue;
            }

            if (ui_close_button_event_hit(&event)) {
                return;
            }
            if (event.mouse_clicked || event.mouse_released) {
                if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                    paused = !paused;
                } else if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                    goto restart_game;
                } else if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                    return;
                }
            }
            if (!event.key_ready) {
                continue;
            }
            if (event.key.scan_code == EFI_SCAN_ESC) {
                return;
            }
            switch (ascii_lower((char)event.key.unicode_char)) {
                case 'w':
                    if (dir_y != 1) {
                        next_dir_x = 0;
                        next_dir_y = -1;
                    }
                    break;
                case 's':
                    if (dir_y != -1) {
                        next_dir_x = 0;
                        next_dir_y = 1;
                    }
                    break;
                case 'a':
                    if (dir_x != 1) {
                        next_dir_x = -1;
                        next_dir_y = 0;
                    }
                    break;
                case 'd':
                    if (dir_x != -1) {
                        next_dir_x = 1;
                        next_dir_y = 0;
                    }
                    break;
                case 'r':
                    goto restart_game;
                case ' ':
                    paused = !paused;
                    break;
                default:
                    break;
            }
            if (event.key.scan_code == EFI_SCAN_UP && dir_y != 1) {
                next_dir_x = 0;
                next_dir_y = -1;
            } else if (event.key.scan_code == EFI_SCAN_DOWN && dir_y != -1) {
                next_dir_x = 0;
                next_dir_y = 1;
            } else if (event.key.scan_code == EFI_SCAN_LEFT && dir_x != 1) {
                next_dir_x = -1;
                next_dir_y = 0;
            } else if (event.key.scan_code == EFI_SCAN_RIGHT && dir_x != -1) {
                next_dir_x = 1;
                next_dir_y = 0;
            }
        }

        if (paused || game_over) {
            continue;
        }

        dir_x = next_dir_x;
        dir_y = next_dir_y;
        {
            int new_x = snake_x[0] + dir_x;
            int new_y = snake_y[0] + dir_y;
            bool grow = new_x == food_x && new_y == food_y;

            if (new_x < 0 || new_x >= board_w || new_y < 0 || new_y >= board_h ||
                snake_occupied(new_x, new_y, snake_x, snake_y, grow ? length : length - 1)) {
                game_over = true;
                continue;
            }
            if (grow && length + 1 < SNAKE_MAX_SEGMENTS) {
                snake_x[length] = snake_x[length - 1];
                snake_y[length] = snake_y[length - 1];
                ++length;
            }
            for (size_t i = length - 1; i > 0; --i) {
                snake_x[i] = snake_x[i - 1];
                snake_y[i] = snake_y[i - 1];
            }
            snake_x[0] = new_x;
            snake_y[0] = new_y;
            if (grow) {
                snake_spawn_food(&food_x, &food_y, snake_x, snake_y, length, board_w, board_h);
            }
        }
    }
}

static void __attribute__((unused)) ui_run_disk_app_legacy(void) {
    ui_event_t event;
    size_t selected = 0;
    size_t focus = 0;

    while (1) {
        disk_volume_info_t infos[PERSIST_MAX_VOLUMES];
        size_t count = disk_collect_info(infos, ARRAY_SIZE(infos));
        int btn_y = (int)g_gfx.height - 156;
        int btn_w = 166;
        int btn_h = 46;
        int btn_gap = 18;
        int btn_x1 = 72;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;

        if (selected >= count && count > 0) {
            selected = count - 1;
        }

        ui_draw_window("SNSX DISK MANAGER", "UEFI VOLUMES, INSTALL TARGETS, SNAPSHOTS, AND PERSISTENCE STATE");
        gfx_draw_panel(66, 154, 382, 350, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(90, 182, "VOLUMES", 2, gfx_rgb(34, 54, 80));
        if (count == 0) {
            gfx_draw_text(90, 222, "NO UEFI FILE-SYSTEM VOLUMES DETECTED", 1, gfx_rgb(55, 70, 92));
        } else {
            for (size_t i = 0; i < count; ++i) {
                int y = 222 + (int)i * 56;
                bool active = i == selected;
                gfx_draw_panel(88, y, 320, 40,
                               active ? theme_color(THEME_ROLE_WARM) : gfx_rgb(255, 255, 255),
                               active ? theme_color(THEME_ROLE_ACCENT_STRONG) : gfx_rgb(195, 205, 220));
                gfx_draw_text(108, y + 10, infos[i].name, 1, gfx_rgb(28, 42, 61));
                gfx_draw_text(228, y + 10, infos[i].writable ? "WRITABLE" : "READ ONLY", 1, gfx_rgb(63, 78, 101));
                if (infos[i].selected) {
                    gfx_draw_text(318, y + 10, "ACTIVE", 1, gfx_rgb(17, 76, 166));
                }
            }
        }

        gfx_draw_panel(482, 154, (int)g_gfx.width - 548, 350, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(506, 182, "SELECTED VOLUME", 2, gfx_rgb(34, 54, 80));
        if (count == 0) {
            gfx_draw_text(506, 224, "ATTACH A WRITABLE FAT DISK TO ENABLE INSTALL AND SAVE.", 1, gfx_rgb(55, 70, 92));
        } else {
            gfx_draw_text(506, 224, infos[selected].name, 2, gfx_rgb(25, 43, 66));
            gfx_draw_text(506, 258, infos[selected].role, 1, gfx_rgb(55, 70, 92));
            gfx_draw_text(506, 292, infos[selected].has_bootloader ? "BOOTAA64.EFI PRESENT" : "BOOTAA64.EFI NOT PRESENT", 1,
                          gfx_rgb(55, 70, 92));
            gfx_draw_text(506, 314, infos[selected].has_state ? "STATE.BIN PRESENT" : "STATE.BIN NOT PRESENT", 1,
                          gfx_rgb(55, 70, 92));
            gfx_draw_text(506, 336, infos[selected].writable ? "WRITABLE TARGET" : "READ-ONLY TARGET", 1,
                          gfx_rgb(55, 70, 92));
            gfx_draw_text(506, 368, "Snapshot size", 1, gfx_rgb(63, 78, 101));
            {
                char bytes[16];
                u32_to_dec((uint32_t)infos[selected].state_size, bytes);
                gfx_draw_text(626, 368, bytes, 1, gfx_rgb(24, 43, 66));
            }
            gfx_draw_text(506, 400, g_persist.status, 1, gfx_rgb(24, 43, 66));
        }

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, "INSTALL",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "SAVE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "RESCAN",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "GUIDE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 3);
        ui_draw_footer("UP/DOWN SELECT VOLUME  I INSTALL  S SAVE  R RESCAN  G GUIDE  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if ((event.mouse_clicked && ui_close_button_hit()) ||
            (event.key_ready && event.key.scan_code == EFI_SCAN_ESC)) {
            return;
        }
        if (event.mouse_clicked) {
            for (size_t i = 0; i < count; ++i) {
                int y = 222 + (int)i * 56;
                if (point_in_rect(g_pointer.x, g_pointer.y, 88, y, 320, 40)) {
                    selected = i;
                    break;
                }
            }
            if (point_in_rect(g_pointer.x, g_pointer.y, btn_x1, btn_y, btn_w, btn_h)) {
                focus = 0;
                if (count > 0 && infos[selected].writable) {
                    g_persist.available = true;
                    g_persist.volume_index = selected;
                    persist_install_boot_disk();
                    desktop_note_launch("DISKS", g_persist.status);
                }
                continue;
            }
            if (point_in_rect(g_pointer.x, g_pointer.y, btn_x2, btn_y, btn_w, btn_h)) {
                focus = 1;
                if (count > 0 && infos[selected].writable) {
                    g_persist.available = true;
                    g_persist.volume_index = selected;
                    g_persist.dirty = true;
                    persist_flush_state();
                    desktop_note_launch("DISKS", g_persist.status);
                }
                continue;
            }
            if (point_in_rect(g_pointer.x, g_pointer.y, btn_x3, btn_y, btn_w, btn_h)) {
                focus = 2;
                persist_discover();
                desktop_note_launch("DISKS", g_persist.status);
                continue;
            }
            if (point_in_rect(g_pointer.x, g_pointer.y, btn_x4, btn_y, btn_w, btn_h)) {
                focus = 3;
                ui_view_text_file("/docs/DISKS.TXT", "SNSX DISK GUIDE");
                continue;
            }
            continue;
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_UP && selected > 0) {
            selected--;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_DOWN && selected + 1 < count) {
            selected++;
            continue;
        }
        if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 4;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 3 : focus - 1;
            continue;
        }
        if (event.key.unicode_char == '\r') {
            if (focus == 0 && count > 0 && infos[selected].writable) {
                g_persist.available = true;
                g_persist.volume_index = selected;
                persist_install_boot_disk();
                desktop_note_launch("DISKS", g_persist.status);
            } else if (focus == 1 && count > 0 && infos[selected].writable) {
                g_persist.available = true;
                g_persist.volume_index = selected;
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("DISKS", g_persist.status);
            } else if (focus == 2) {
                persist_discover();
                desktop_note_launch("DISKS", g_persist.status);
            } else if (focus == 3) {
                ui_view_text_file("/docs/DISKS.TXT", "SNSX DISK GUIDE");
            }
            continue;
        }
        switch (ascii_lower((char)event.key.unicode_char)) {
            case 'i':
                focus = 0;
                if (count > 0 && infos[selected].writable) {
                    g_persist.available = true;
                    g_persist.volume_index = selected;
                    persist_install_boot_disk();
                    desktop_note_launch("DISKS", g_persist.status);
                }
                break;
            case 's':
                focus = 1;
                if (count > 0 && infos[selected].writable) {
                    g_persist.available = true;
                    g_persist.volume_index = selected;
                    g_persist.dirty = true;
                    persist_flush_state();
                    desktop_note_launch("DISKS", g_persist.status);
                }
                break;
            case 'r':
                focus = 2;
                persist_discover();
                desktop_note_launch("DISKS", g_persist.status);
                break;
            case 'g':
                focus = 3;
                ui_view_text_file("/docs/DISKS.TXT", "SNSX DISK GUIDE");
                break;
            default:
                break;
        }
    }
}

static void ui_run_disk_app(void) {
    ui_event_t event;
    size_t selected = 0;
    size_t focus = 0;

    while (1) {
        disk_volume_info_t infos[PERSIST_MAX_VOLUMES];
        size_t count = disk_collect_info(infos, ARRAY_SIZE(infos));
        int btn_y = (int)g_gfx.height - 156;
        int btn_w = 146;
        int btn_h = 46;
        int btn_gap = 12;
        int btn_x1 = 72;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;
        int btn_x5 = btn_x4 + btn_w + btn_gap;

        if (selected >= count && count > 0) {
            selected = count - 1;
        }

        ui_draw_window("SNSX DISK MANAGER", "UEFI VOLUMES, INSTALL TARGETS, SNAPSHOTS, AND PERSISTENCE STATE");
        gfx_draw_panel(66, 154, 382, 356, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(90, 182, "VOLUMES", 2, gfx_rgb(34, 54, 80));
        if (count == 0) {
            gfx_draw_text(90, 222, "NO UEFI FILE-SYSTEM VOLUMES DETECTED", 1, gfx_rgb(55, 70, 92));
        } else {
            for (size_t i = 0; i < count; ++i) {
                int y = 222 + (int)i * 54;
                bool active = i == selected;
                gfx_draw_panel(88, y, 320, 40,
                               active ? theme_color(THEME_ROLE_WARM) : gfx_rgb(255, 255, 255),
                               active ? theme_color(THEME_ROLE_ACCENT_STRONG) : gfx_rgb(195, 205, 220));
                gfx_draw_text(108, y + 10, infos[i].name, 1, gfx_rgb(28, 42, 61));
                gfx_draw_text(228, y + 10, infos[i].writable ? "WRITABLE" : "READ ONLY", 1, gfx_rgb(63, 78, 101));
                if (infos[i].selected) {
                    gfx_draw_text(318, y + 10, "ACTIVE", 1, gfx_rgb(17, 76, 166));
                }
            }
        }

        gfx_draw_panel(482, 154, (int)g_gfx.width - 548, 356, gfx_rgb(244, 247, 252), gfx_rgb(85, 108, 136));
        gfx_draw_text(506, 182, "SELECTED VOLUME", 2, gfx_rgb(34, 54, 80));
        if (count == 0) {
            gfx_draw_text(506, 224, "ATTACH A WRITABLE FAT DISK TO ENABLE INSTALL AND SAVE.", 1, gfx_rgb(55, 70, 92));
        } else {
            gfx_draw_text(506, 224, infos[selected].name, 2, gfx_rgb(25, 43, 66));
            gfx_draw_text(506, 258, infos[selected].role, 1, gfx_rgb(55, 70, 92));
            gfx_draw_text(506, 292, infos[selected].has_bootloader ? "BOOTAA64.EFI PRESENT" : "BOOTAA64.EFI NOT PRESENT", 1,
                          gfx_rgb(55, 70, 92));
            gfx_draw_text(506, 314, infos[selected].has_state ? "STATE.BIN PRESENT" : "STATE.BIN NOT PRESENT", 1,
                          gfx_rgb(55, 70, 92));
            gfx_draw_text(506, 336, infos[selected].writable ? "WRITABLE TARGET" : "READ-ONLY TARGET", 1,
                          gfx_rgb(55, 70, 92));
            gfx_draw_text(506, 358, infos[selected].selected ? "ACTIVE SNSX TARGET" : "NOT THE ACTIVE SNSX TARGET", 1,
                          gfx_rgb(55, 70, 92));
            gfx_draw_text(506, 388, "Snapshot size", 1, gfx_rgb(63, 78, 101));
            {
                char bytes[16];
                u32_to_dec((uint32_t)infos[selected].state_size, bytes);
                gfx_draw_text(626, 388, bytes, 1, gfx_rgb(24, 43, 66));
            }
            gfx_draw_text(506, 420, g_persist.status, 1, gfx_rgb(24, 43, 66));
            gfx_draw_text(506, 450, "USE selects the active SNSX disk without writing immediately.", 1,
                          gfx_rgb(55, 70, 92));
            gfx_draw_text(506, 472, "INSTALL writes BOOTAA64.EFI. SAVE writes /SNSX/STATE.BIN.", 1,
                          gfx_rgb(55, 70, 92));
        }

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, "USE",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "INSTALL",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "SAVE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "RESCAN",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 3);
        ui_draw_button_focus(btn_x5, btn_y, btn_w, btn_h, "GUIDE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 4);
        ui_draw_footer("UP/DOWN SELECT  U USE  I INSTALL  S SAVE  R RESCAN  G GUIDE  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event) ||
            (event.key_ready && event.key.scan_code == EFI_SCAN_ESC)) {
            return;
        }
        if (event.mouse_clicked || event.mouse_released) {
            for (size_t i = 0; i < count; ++i) {
                int y = 222 + (int)i * 54;
                if (ui_pointer_activated(&event, 88, y, 320, 40)) {
                    selected = i;
                    break;
                }
            }
            if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                focus = 0;
                if (count > 0) {
                    g_persist.available = true;
                    g_persist.volume_index = selected;
                    persist_set_status(infos[selected].writable ?
                        "Selected writable volume as the active SNSX disk." :
                        "Selected read-only volume. Install and Save remain disabled.");
                    desktop_note_launch("DISKS", g_persist.status);
                }
                continue;
            }
            if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                focus = 1;
                if (count > 0 && infos[selected].writable) {
                    g_persist.available = true;
                    g_persist.volume_index = selected;
                    persist_set_status("Selected writable volume as the active SNSX disk.");
                    persist_install_boot_disk();
                    desktop_note_launch("DISKS", g_persist.status);
                }
                continue;
            }
            if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                focus = 2;
                if (count > 0 && infos[selected].writable) {
                    g_persist.available = true;
                    g_persist.volume_index = selected;
                    g_persist.dirty = true;
                    persist_flush_state();
                    desktop_note_launch("DISKS", g_persist.status);
                }
                continue;
            }
            if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                focus = 3;
                persist_discover();
                network_refresh(g_desktop.network_autoconnect);
                desktop_note_launch("DISKS", g_persist.status);
                continue;
            }
            if (ui_pointer_activated(&event, btn_x5, btn_y, btn_w, btn_h)) {
                focus = 4;
                ui_view_text_file("/docs/DISKS.TXT", "SNSX DISK GUIDE");
                continue;
            }
            continue;
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_UP && selected > 0) {
            selected--;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_DOWN && selected + 1 < count) {
            selected++;
            continue;
        }
        if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 5;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 4 : focus - 1;
            continue;
        }
        if (event.key.unicode_char == '\r') {
            if (focus == 0 && count > 0) {
                g_persist.available = true;
                g_persist.volume_index = selected;
                persist_set_status(infos[selected].writable ?
                    "Selected writable volume as the active SNSX disk." :
                    "Selected read-only volume. Install and Save remain disabled.");
                desktop_note_launch("DISKS", g_persist.status);
            } else if (focus == 1 && count > 0 && infos[selected].writable) {
                g_persist.available = true;
                g_persist.volume_index = selected;
                persist_set_status("Selected writable volume as the active SNSX disk.");
                persist_install_boot_disk();
                desktop_note_launch("DISKS", g_persist.status);
            } else if (focus == 2 && count > 0 && infos[selected].writable) {
                g_persist.available = true;
                g_persist.volume_index = selected;
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("DISKS", g_persist.status);
            } else if (focus == 3) {
                persist_discover();
                network_refresh(g_desktop.network_autoconnect);
                desktop_note_launch("DISKS", g_persist.status);
            } else if (focus == 4) {
                ui_view_text_file("/docs/DISKS.TXT", "SNSX DISK GUIDE");
            }
            continue;
        }
        switch (ascii_lower((char)event.key.unicode_char)) {
            case 'u':
                focus = 0;
                if (count > 0) {
                    g_persist.available = true;
                    g_persist.volume_index = selected;
                    persist_set_status(infos[selected].writable ?
                        "Selected writable volume as the active SNSX disk." :
                        "Selected read-only volume. Install and Save remain disabled.");
                    desktop_note_launch("DISKS", g_persist.status);
                }
                break;
            case 'i':
                focus = 1;
                if (count > 0 && infos[selected].writable) {
                    g_persist.available = true;
                    g_persist.volume_index = selected;
                    persist_set_status("Selected writable volume as the active SNSX disk.");
                    persist_install_boot_disk();
                    desktop_note_launch("DISKS", g_persist.status);
                }
                break;
            case 's':
                focus = 2;
                if (count > 0 && infos[selected].writable) {
                    g_persist.available = true;
                    g_persist.volume_index = selected;
                    g_persist.dirty = true;
                    persist_flush_state();
                    desktop_note_launch("DISKS", g_persist.status);
                }
                break;
            case 'r':
                focus = 3;
                persist_discover();
                network_refresh(g_desktop.network_autoconnect);
                desktop_note_launch("DISKS", g_persist.status);
                break;
            case 'g':
                focus = 4;
                ui_view_text_file("/docs/DISKS.TXT", "SNSX DISK GUIDE");
                break;
            default:
                break;
        }
    }
}

static void ui_run_installer_app(void) {
    ui_event_t event;
    size_t focus = 0;

    while (1) {
        bool can_install = g_persist.available;
        int btn_y = 366;
        int btn_w = 160;
        int btn_h = 54;
        int btn_x1 = 78;
        int btn_x2 = btn_x1 + btn_w + 18;
        int btn_x3 = btn_x2 + btn_w + 18;
        int btn_x4 = btn_x3 + btn_w + 18;

        ui_draw_window("SNSX INSTALLER", "PERSISTENT DISK AND BOOT INSTALL");
        gfx_draw_panel(66, 154, (int)g_gfx.width - 132, 154, gfx_rgb(230, 242, 255), gfx_rgb(59, 89, 126));
        gfx_draw_text(90, 184, can_install ? "WRITABLE SNSX DISK AVAILABLE" : "NO WRITABLE SNSX DISK DETECTED",
                      2, gfx_rgb(33, 55, 84));
        gfx_draw_text(90, 220, g_persist.status, 1, gfx_rgb(37, 58, 87));
        gfx_draw_text(90, 242, "Install copies BOOTAA64.EFI and enables state snapshots on the writable disk.", 1,
                      gfx_rgb(37, 58, 87));
        gfx_draw_text(90, 264, "Save writes the live SNSX filesystem to /SNSX/STATE.BIN on that disk.", 1,
                      gfx_rgb(37, 58, 87));
        gfx_draw_text(90, 286, "Bare Metal opens the USB and primary-disk guide for running SNSX outside VirtualBox.", 1,
                      gfx_rgb(37, 58, 87));

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, "INSTALL",
                             gfx_rgb(255, 214, 149), gfx_rgb(149, 95, 36), gfx_rgb(53, 36, 18), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "SAVE",
                             gfx_rgb(221, 239, 255), gfx_rgb(74, 106, 142), gfx_rgb(32, 53, 84), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "BARE METAL",
                             gfx_rgb(235, 244, 252), gfx_rgb(94, 113, 140), gfx_rgb(44, 59, 84), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "RELOAD",
                             gfx_rgb(235, 244, 252), gfx_rgb(94, 113, 140), gfx_rgb(44, 59, 84), focus == 3);

        gfx_draw_panel(66, 452, (int)g_gfx.width - 132, 84, gfx_rgb(244, 247, 252), gfx_rgb(80, 101, 129));
        gfx_draw_text(88, 480, "KEYS: I INSTALL  S SAVE  B BARE METAL  L RELOAD  ESC BACK", 1, gfx_rgb(52, 69, 95));
        gfx_draw_text(88, 502, "Tip: use the updated VM helper to attach a persistence disk automatically.", 1,
                      gfx_rgb(52, 69, 95));
        ui_draw_footer("TAB OR ARROWS SELECT  ENTER OPENS  I / S / B / L SHORTCUTS  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event)) {
            return;
        }
        if (event.mouse_clicked || event.mouse_released) {
            if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                focus = 0;
                persist_install_boot_disk();
                desktop_note_launch("INSTALLER", g_persist.status);
                continue;
            }
            if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                focus = 1;
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("INSTALLER", "Saved the live SNSX session.");
                continue;
            }
            if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                focus = 2;
                ui_view_text_file("/docs/BAREMETAL.TXT", "SNSX BARE-METAL GUIDE");
                continue;
            }
            if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                focus = 3;
                persist_discover();
                desktop_note_launch("INSTALLER", g_persist.status);
                continue;
            }
            if (focus == 0) {
                persist_install_boot_disk();
                desktop_note_launch("INSTALLER", g_persist.status);
            } else if (focus == 1) {
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("INSTALLER", "Saved the live SNSX session.");
            } else if (focus == 2) {
                ui_view_text_file("/docs/BAREMETAL.TXT", "SNSX BARE-METAL GUIDE");
            } else {
                persist_discover();
                desktop_note_launch("INSTALLER", g_persist.status);
            }
            continue;
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_ESC) {
            return;
        }
        if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 4;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 3 : focus - 1;
            continue;
        }
        if (event.key.unicode_char == '\r') {
            if (focus == 0) {
                persist_install_boot_disk();
                desktop_note_launch("INSTALLER", g_persist.status);
            } else if (focus == 1) {
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("INSTALLER", "Saved the live SNSX session.");
            } else if (focus == 2) {
                ui_view_text_file("/docs/BAREMETAL.TXT", "SNSX BARE-METAL GUIDE");
            } else {
                persist_discover();
                desktop_note_launch("INSTALLER", g_persist.status);
            }
            continue;
        }
        switch (ascii_lower((char)event.key.unicode_char)) {
            case 'i':
                focus = 0;
                persist_install_boot_disk();
                desktop_note_launch("INSTALLER", g_persist.status);
                break;
            case 's':
                focus = 1;
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("INSTALLER", "Saved the live SNSX session.");
                break;
            case 'b':
                focus = 2;
                ui_view_text_file("/docs/BAREMETAL.TXT", "SNSX BARE-METAL GUIDE");
                break;
            case 'l':
                focus = 3;
                persist_discover();
                desktop_note_launch("INSTALLER", g_persist.status);
                break;
            default:
                break;
        }
    }
}

static void app_store_console(void) {
    char line[SHELL_INPUT_MAX];
    char *argv[SHELL_ARG_MAX];
    char status[128];

    while (1) {
        console_clear();
        console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
        console_writeline("SNSX App Store");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_divider();
        command_store_list();
        console_divider();
        console_writeline("Commands: list  install ID  remove ID  open ID  guide  baremetal  back");
        console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
        console_write("store");
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_write("> ");
        line_read(line, sizeof(line));

        if (line[0] == '\0') {
            continue;
        }
        if (strcmp(line, "back") == 0 || strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            return;
        }
        if (strcmp(line, "list") == 0) {
            continue;
        }
        if (strcmp(line, "guide") == 0) {
            view_text("/docs/STORE.TXT");
            continue;
        }
        if (strcmp(line, "baremetal") == 0) {
            view_text("/docs/BAREMETAL.TXT");
            continue;
        }
        {
            size_t argc = shell_tokenize(line, argv, ARRAY_SIZE(argv));
            if (argc == 0) {
                continue;
            }
            if (strcmp(argv[0], "install") == 0 && argc > 1) {
                store_install_package_by_id(argv[1], status, sizeof(status));
                desktop_note_launch("STORE", status);
                wait_for_enter(status);
                continue;
            }
            if ((strcmp(argv[0], "remove") == 0 || strcmp(argv[0], "uninstall") == 0) && argc > 1) {
                store_remove_package_by_id(argv[1], status, sizeof(status));
                desktop_note_launch("STORE", status);
                wait_for_enter(status);
                continue;
            }
            if (strcmp(argv[0], "open") == 0 && argc > 1) {
                size_t index = store_find_package_index(argv[1]);
                if (index < ARRAY_SIZE(g_store_catalog) && fs_find(g_store_catalog[index].asset_path) != NULL) {
                    view_text(g_store_catalog[index].asset_path);
                } else {
                    wait_for_enter("Package file is not installed yet.");
                }
                continue;
            }
        }
        wait_for_enter("Unknown store command.");
    }
}

static void ui_run_welcome_app(bool mark_seen_on_exit) {
    ui_event_t event;
    size_t focus = 0;
    while (1) {
        int btn_margin = 46;
        int btn_gap = 12;
        int btn_y = (int)g_gfx.height - 172;
        int btn_w = ((int)g_gfx.width - btn_margin * 2 - btn_gap * 3) / 4;
        int btn_h = 52;
        int btn_x1 = btn_margin;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;
        char launches[16];

        u32_to_dec(g_desktop.launch_count, launches);

        ui_draw_window("WELCOME TO SNSX", "YOUR ARM64 VIRTUALBOX DESKTOP IS READY");
        gfx_draw_panel(66, 154, 456, 236, theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(90, 182, "SNSX is now running as a standalone desktop OS session.", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 204, "Use Files for documents, Notes for writing, Calc for quick math,", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 226, "Terminal for shell work, and Installer to make the VM disk bootable.", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(90, 262, "CURRENT SESSION", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(90, 298, g_persist.status, 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(90, 320, g_pointer.engaged ? "Mouse navigation active." : "Keyboard navigation active.", 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(90, 342, g_network.available ? "UEFI network device discovered." : "No firmware network device reported.", 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));

        gfx_draw_panel(548, 154, (int)g_gfx.width - 614, 236,
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(574, 182, "DESKTOP SNAPSHOT", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        ui_draw_badge(574, 220, desktop_theme_name(), theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_TEXT));
        ui_draw_badge(698, 220, g_desktop.autosave ? "AUTOSAVE ON" : "AUTOSAVE OFF",
                      theme_color(THEME_ROLE_ACCENT), theme_color(THEME_ROLE_TEXT_LIGHT));
        gfx_draw_text(574, 266, "Last app", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(654, 266, g_desktop.last_app, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(574, 288, "Launch count", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(674, 288, launches, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(574, 310, "Last file", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(644, 310, g_last_opened_file, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(574, 346, "Tip: use Settings to change themes, then Save Session", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(574, 368, "to persist the desktop look on the VM disk.", 1, theme_color(THEME_ROLE_TEXT_SOFT));

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, "START",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, "GUIDE",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "INSTALLER",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "SETTINGS",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus == 3);

        ui_draw_footer("TAB OR ARROWS SELECT  ENTER OPEN  G GUIDE  I INSTALLER  S SETTINGS  ESC CLOSES");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event)) {
            break;
        }
        if (event.mouse_clicked || event.mouse_released) {
            if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                focus = 0;
                break;
            }
            if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                focus = 1;
                ui_view_text_file("/docs/DESKTOP.TXT", "SNSX GUIDE");
                continue;
            }
            if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                focus = 2;
                ui_run_installer_app();
                continue;
            }
            if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                focus = 3;
                ui_run_settings_app();
                continue;
            }
            if (focus == 0) {
                break;
            }
            if (focus == 1) {
                ui_view_text_file("/docs/DESKTOP.TXT", "SNSX GUIDE");
                continue;
            }
            if (focus == 2) {
                ui_run_installer_app();
                continue;
            }
            ui_run_settings_app();
            continue;
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_ESC) {
            break;
        }
        if (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT) {
            focus = (focus + 1) % 4;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_LEFT) {
            focus = focus == 0 ? 3 : focus - 1;
            continue;
        }
        if (event.key.unicode_char == '\r') {
            if (focus == 0) {
                break;
            }
            if (focus == 1) {
                ui_view_text_file("/docs/DESKTOP.TXT", "SNSX GUIDE");
                continue;
            }
            if (focus == 2) {
                ui_run_installer_app();
                continue;
            }
            ui_run_settings_app();
            continue;
        }
        switch (ascii_lower((char)event.key.unicode_char)) {
            case 'g':
                focus = 1;
                ui_view_text_file("/docs/DESKTOP.TXT", "SNSX GUIDE");
                break;
            case 'i':
                focus = 2;
                ui_run_installer_app();
                break;
            case 's':
                focus = 3;
                ui_run_settings_app();
                break;
            default:
                break;
        }
    }

    if (mark_seen_on_exit) {
        g_desktop.welcome_seen = true;
        desktop_note_launch("WELCOME", "Welcome center completed.");
        desktop_commit_preferences();
    }
}

static void __attribute__((unused)) ui_run_settings_app_legacy(void) {
    ui_event_t event;
    size_t preview_theme = g_desktop.theme;
    bool focus_buttons = false;
    size_t button_focus = 0;

    while (1) {
        int theme_y = 180;
        int card_w = ((int)g_gfx.width - 180) / 4;
        int btn_margin = 42;
        int btn_gap = 10;
        int btn_y = (int)g_gfx.height - 172;
        int btn_w = ((int)g_gfx.width - btn_margin * 2 - btn_gap * 4) / 5;
        int btn_h = 50;
        int btn_x1 = btn_margin;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;
        int btn_x5 = btn_x4 + btn_w + btn_gap;
        char launches[16];

        u32_to_dec(g_desktop.launch_count, launches);

        ui_draw_window("SNSX SETTINGS", "THEMES, INPUT, SESSION, NETWORK, AND STORAGE CONTROL");
        for (size_t i = 0; i < ARRAY_SIZE(g_theme_names); ++i) {
            int x = 48 + (int)i * card_w;
            bool selected = i == preview_theme;
            uint32_t fill = gfx_rgb(g_theme_palette[i][THEME_ROLE_PANEL][0],
                                    g_theme_palette[i][THEME_ROLE_PANEL][1],
                                    g_theme_palette[i][THEME_ROLE_PANEL][2]);
            uint32_t border = selected ? theme_color(THEME_ROLE_WARM)
                                       : gfx_rgb(g_theme_palette[i][THEME_ROLE_BORDER][0],
                                                 g_theme_palette[i][THEME_ROLE_BORDER][1],
                                                 g_theme_palette[i][THEME_ROLE_BORDER][2]);
            uint32_t accent = gfx_rgb(g_theme_palette[i][THEME_ROLE_ACCENT][0],
                                      g_theme_palette[i][THEME_ROLE_ACCENT][1],
                                      g_theme_palette[i][THEME_ROLE_ACCENT][2]);
            gfx_draw_panel(x, theme_y, card_w - 18, 150, fill, border);
            gfx_fill_theme_gradient(x + 12, theme_y + 14, card_w - 42, 52, THEME_ROLE_BG_TOP, THEME_ROLE_BG_BOTTOM);
            gfx_fill_rect(x + 28, theme_y + 84, 56, 56, accent);
            gfx_draw_text(x + 102, theme_y + 92, g_theme_names[i], 2, gfx_rgb(g_theme_palette[i][THEME_ROLE_TEXT][0],
                                                                               g_theme_palette[i][THEME_ROLE_TEXT][1],
                                                                               g_theme_palette[i][THEME_ROLE_TEXT][2]));
            gfx_draw_text(x + 102, theme_y + 118, selected ? "SELECTED" : "CLICK TO PREVIEW", 1,
                          gfx_rgb(g_theme_palette[i][THEME_ROLE_TEXT_SOFT][0],
                                  g_theme_palette[i][THEME_ROLE_TEXT_SOFT][1],
                                  g_theme_palette[i][THEME_ROLE_TEXT_SOFT][2]));
            if (!focus_buttons && selected) {
                gfx_draw_rect(x - 6, theme_y - 6, card_w - 6, 162, 3, theme_color(THEME_ROLE_ACCENT_STRONG));
                gfx_draw_rect(x - 2, theme_y - 2, card_w - 14, 154, 1, theme_color(THEME_ROLE_TEXT_LIGHT));
            }
        }

        gfx_draw_panel(66, 360, (int)g_gfx.width - 132, 150,
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(88, 388, "SESSION", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(88, 422, g_desktop.autosave ? "Autosave is enabled for desktop preferences." :
                                                    "Autosave is disabled for desktop preferences.",
                      1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(88, 444, "Last app", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(164, 444, g_desktop.last_app, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(314, 444, "Launches", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(396, 444, launches, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(468, 444, "Disk", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(516, 444, g_persist.installed ? "installed" : "live only", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(88, 472, "Pointer", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(154, 472, g_pointer.available ? "ready" : "not detected", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(254, 472, "Network", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(334, 472, g_network.available ? "adapter present" : "offline", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(480, 472, "Theme changes are written to /home/.SNSX_DESKTOP.TXT.", 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, "APPLY",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), focus_buttons && button_focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, g_desktop.autosave ? "AUTOSAVE ON" : "AUTOSAVE OFF",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus_buttons && button_focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, "SAVE NOW",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus_buttons && button_focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "NETWORK",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus_buttons && button_focus == 3);
        ui_draw_button_focus(btn_x5, btn_y, btn_w, btn_h, "DISKS",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), focus_buttons && button_focus == 4);

        ui_draw_footer("LEFT/RIGHT MOVE  TAB OR DOWN TO BUTTONS  T AUTOSAVE  S SAVE  N NETWORK  D DISKS  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (event.mouse_clicked && ui_close_button_hit()) {
            return;
        }
        if (event.mouse_clicked) {
            bool handled = false;
            for (size_t i = 0; i < ARRAY_SIZE(g_theme_names); ++i) {
                int x = 48 + (int)i * card_w;
                if (point_in_rect(g_pointer.x, g_pointer.y, x, theme_y, card_w - 18, 150)) {
                    preview_theme = i;
                    focus_buttons = false;
                    handled = true;
                }
            }
            if (point_in_rect(g_pointer.x, g_pointer.y, btn_x1, btn_y, btn_w, btn_h)) {
                focus_buttons = true;
                button_focus = 0;
                handled = true;
                char message[64];
                g_desktop.theme = (uint32_t)preview_theme;
                strcopy_limit(message, "Theme set to ", sizeof(message));
                strcat(message, desktop_theme_name());
                desktop_note_launch("SETTINGS", message);
                desktop_commit_preferences();
                continue;
            }
            if (point_in_rect(g_pointer.x, g_pointer.y, btn_x2, btn_y, btn_w, btn_h)) {
                focus_buttons = true;
                button_focus = 1;
                handled = true;
                g_desktop.autosave = !g_desktop.autosave;
                desktop_note_launch("SETTINGS", g_desktop.autosave ? "Autosave enabled." : "Autosave disabled.");
                desktop_commit_preferences();
                continue;
            }
            if (point_in_rect(g_pointer.x, g_pointer.y, btn_x3, btn_y, btn_w, btn_h)) {
                focus_buttons = true;
                button_focus = 2;
                handled = true;
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("SETTINGS", "Saved the SNSX session.");
                continue;
            }
            if (point_in_rect(g_pointer.x, g_pointer.y, btn_x4, btn_y, btn_w, btn_h)) {
                focus_buttons = true;
                button_focus = 3;
                handled = true;
                ui_run_network_app();
                continue;
            }
            if (point_in_rect(g_pointer.x, g_pointer.y, btn_x5, btn_y, btn_w, btn_h)) {
                focus_buttons = true;
                button_focus = 4;
                handled = true;
                ui_run_disk_app();
                continue;
            }
            if (!handled) {
                continue;
            }
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_ESC) {
            return;
        }
        if (event.key.scan_code == EFI_SCAN_UP) {
            focus_buttons = false;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_DOWN) {
            focus_buttons = true;
            continue;
        }
        if (event.key.unicode_char == '\t') {
            focus_buttons = !focus_buttons;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_LEFT) {
            if (focus_buttons) {
                button_focus = button_focus == 0 ? 4 : button_focus - 1;
            } else {
                preview_theme = preview_theme == 0 ? ARRAY_SIZE(g_theme_names) - 1 : preview_theme - 1;
            }
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_RIGHT) {
            if (focus_buttons) {
                button_focus = (button_focus + 1) % 5;
            } else {
                preview_theme = (preview_theme + 1) % ARRAY_SIZE(g_theme_names);
            }
            continue;
        }
        if (event.key.unicode_char == '\r' || ascii_lower((char)event.key.unicode_char) == 'a') {
            if (focus_buttons && button_focus != 0) {
                if (button_focus == 1) {
                    g_desktop.autosave = !g_desktop.autosave;
                    desktop_note_launch("SETTINGS", g_desktop.autosave ? "Autosave enabled." : "Autosave disabled.");
                    desktop_commit_preferences();
                } else if (button_focus == 2) {
                    g_persist.dirty = true;
                    persist_flush_state();
                    desktop_note_launch("SETTINGS", "Saved the SNSX session.");
                } else if (button_focus == 3) {
                    ui_run_network_app();
                } else if (button_focus == 4) {
                    ui_run_disk_app();
                }
                continue;
            }
            char message[64];
            g_desktop.theme = (uint32_t)preview_theme;
            strcopy_limit(message, "Theme set to ", sizeof(message));
            strcat(message, desktop_theme_name());
            desktop_note_launch("SETTINGS", message);
            desktop_commit_preferences();
            continue;
        }
        switch (ascii_lower((char)event.key.unicode_char)) {
            case 't':
                g_desktop.autosave = !g_desktop.autosave;
                desktop_note_launch("SETTINGS", g_desktop.autosave ? "Autosave enabled." : "Autosave disabled.");
                desktop_commit_preferences();
                break;
            case 's':
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("SETTINGS", "Saved the SNSX session.");
                break;
            case 'n':
                ui_run_network_app();
                break;
            case 'd':
                ui_run_disk_app();
                break;
            default:
                break;
        }
    }
}

static void ui_run_settings_app(void) {
    ui_event_t event;
    size_t preview_theme = g_desktop.theme;
    uint32_t preview_pointer_speed = g_desktop.pointer_speed;
    uint32_t preview_network_mode = g_desktop.network_preference;
    bool preview_pointer_trail = g_desktop.pointer_trail;
    bool button_focus_enabled = false;
    size_t button_focus = 0;

    while (1) {
        int theme_y = 180;
        int theme_w = ((int)g_gfx.width - 180) / 4;
        int speed_y = 372;
        int speed_x = 88;
        int speed_gap = 10;
        int speed_w = 58;
        int speed_h = 40;
        int mode_y = 372;
        int mode_x = 516;
        int mode_gap = 10;
        int mode_w = 102;
        int mode_h = 40;
        int btn_margin = 42;
        int btn_gap = 10;
        int btn_y = (int)g_gfx.height - 172;
        int btn_w = ((int)g_gfx.width - btn_margin * 2 - btn_gap * 4) / 5;
        int btn_h = 50;
        int btn_x1 = btn_margin;
        int btn_x2 = btn_x1 + btn_w + btn_gap;
        int btn_x3 = btn_x2 + btn_w + btn_gap;
        int btn_x4 = btn_x3 + btn_w + btn_gap;
        int btn_x5 = btn_x4 + btn_w + btn_gap;
        char launches[16];

        u32_to_dec(g_desktop.launch_count, launches);

        ui_draw_window("SNSX SETTINGS", "APPEARANCE, INPUT, NETWORK PROFILES, AND STORAGE CONTROL");
        for (size_t i = 0; i < ARRAY_SIZE(g_theme_names); ++i) {
            int x = 48 + (int)i * theme_w;
            bool selected = i == preview_theme;
            uint32_t fill = gfx_rgb(g_theme_palette[i][THEME_ROLE_PANEL][0],
                                    g_theme_palette[i][THEME_ROLE_PANEL][1],
                                    g_theme_palette[i][THEME_ROLE_PANEL][2]);
            uint32_t border = selected ? theme_color(THEME_ROLE_WARM)
                                       : gfx_rgb(g_theme_palette[i][THEME_ROLE_BORDER][0],
                                                 g_theme_palette[i][THEME_ROLE_BORDER][1],
                                                 g_theme_palette[i][THEME_ROLE_BORDER][2]);
            uint32_t accent = gfx_rgb(g_theme_palette[i][THEME_ROLE_ACCENT][0],
                                      g_theme_palette[i][THEME_ROLE_ACCENT][1],
                                      g_theme_palette[i][THEME_ROLE_ACCENT][2]);
            gfx_draw_panel(x, theme_y, theme_w - 18, 150, fill, border);
            gfx_fill_theme_gradient(x + 12, theme_y + 14, theme_w - 42, 52, THEME_ROLE_BG_TOP, THEME_ROLE_BG_BOTTOM);
            gfx_fill_rect(x + 28, theme_y + 84, 56, 56, accent);
            gfx_draw_text(x + 102, theme_y + 92, g_theme_names[i], 2, gfx_rgb(g_theme_palette[i][THEME_ROLE_TEXT][0],
                                                                               g_theme_palette[i][THEME_ROLE_TEXT][1],
                                                                               g_theme_palette[i][THEME_ROLE_TEXT][2]));
            gfx_draw_text(x + 102, theme_y + 118, selected ? "PREVIEW" : "CLICK TO PREVIEW", 1,
                          gfx_rgb(g_theme_palette[i][THEME_ROLE_TEXT_SOFT][0],
                                  g_theme_palette[i][THEME_ROLE_TEXT_SOFT][1],
                                  g_theme_palette[i][THEME_ROLE_TEXT_SOFT][2]));
            if (!button_focus_enabled && selected) {
                gfx_draw_rect(x - 6, theme_y - 6, theme_w - 6, 162, 3, theme_color(THEME_ROLE_ACCENT_STRONG));
            }
        }

        gfx_draw_panel(66, 344, 392, 162, theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(88, 368, "INPUT / SESSION", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(88, 404, g_desktop.autosave ? "Autosave is enabled for desktop preferences." :
                                                    "Autosave is disabled for desktop preferences.",
                      1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(88, 426, preview_pointer_trail ? "Pointer trail preview is enabled." :
                                                       "Pointer trail preview is disabled.",
                      1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(88, 448, "Pointer speed", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(208, 448, desktop_pointer_speed_name_for(preview_pointer_speed), 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(88, 470, "Launches", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(176, 470, launches, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(248, 470, "Last app", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(330, 470, g_desktop.last_app, 1, theme_color(THEME_ROLE_TEXT));

        for (uint32_t speed = 1; speed <= 5; ++speed) {
            int x = speed_x + (int)(speed - 1) * (speed_w + speed_gap);
            bool active = speed == preview_pointer_speed;
            char label[8];
            u32_to_dec(speed, label);
            gfx_draw_panel(x, speed_y, speed_w, speed_h,
                           active ? theme_color(THEME_ROLE_WARM) : theme_color(THEME_ROLE_PANEL),
                           active ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_BORDER));
            gfx_draw_text(x + 20, speed_y + 12, label, 2, theme_color(THEME_ROLE_TEXT));
        }

        gfx_draw_panel(490, 344, (int)g_gfx.width - 556, 162,
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(516, 368, "NETWORK / STORAGE", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(516, 404, g_desktop.network_autoconnect ? "Autoconnect will refresh the active network profile." :
                                                                "Autoconnect is off. SNSX will stay manual until Connect is pressed.",
                      1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(516, 426, "Host name", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(610, 426, g_desktop.hostname, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(516, 448, "Wi-Fi", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(570, 448, g_desktop.wifi_ssid, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(516, 470, "Hotspot", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(598, 470, g_desktop.hotspot_name, 1, theme_color(THEME_ROLE_TEXT));

        for (uint32_t mode = NETWORK_PREF_AUTO; mode <= NETWORK_PREF_HOTSPOT; ++mode) {
            int x = mode_x + (int)mode * (mode_w + mode_gap);
            bool active = mode == preview_network_mode;
            const char *label = mode == NETWORK_PREF_AUTO ? "AUTO" :
                                mode == NETWORK_PREF_ETHERNET ? "ETHERNET" :
                                mode == NETWORK_PREF_WIFI ? "WIFI" : "HOTSPOT";
            gfx_draw_panel(x, mode_y, mode_w, mode_h,
                           active ? theme_color(THEME_ROLE_WARM) : theme_color(THEME_ROLE_PANEL),
                           active ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_BORDER));
            gfx_draw_text(x + 14, mode_y + 14, label, 1, theme_color(THEME_ROLE_TEXT));
        }

        ui_draw_button_focus(btn_x1, btn_y, btn_w, btn_h, "APPLY",
                             theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG),
                             theme_color(THEME_ROLE_TEXT), button_focus_enabled && button_focus == 0);
        ui_draw_button_focus(btn_x2, btn_y, btn_w, btn_h, g_desktop.autosave ? "AUTOSAVE ON" : "AUTOSAVE OFF",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), button_focus_enabled && button_focus == 1);
        ui_draw_button_focus(btn_x3, btn_y, btn_w, btn_h, preview_pointer_trail ? "TRAIL ON" : "TRAIL OFF",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), button_focus_enabled && button_focus == 2);
        ui_draw_button_focus(btn_x4, btn_y, btn_w, btn_h, "NETWORK",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), button_focus_enabled && button_focus == 3);
        ui_draw_button_focus(btn_x5, btn_y, btn_w, btn_h, "DISKS",
                             theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER),
                             theme_color(THEME_ROLE_TEXT), button_focus_enabled && button_focus == 4);

        ui_draw_footer("CLICK OR ARROWS PREVIEW  P CYCLES POINTER SPEED  M CYCLES NETWORK MODE  A APPLIES  ESC BACK");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event)) {
            return;
        }
        if (event.mouse_clicked || event.mouse_released) {
            bool handled = false;

            for (size_t i = 0; i < ARRAY_SIZE(g_theme_names); ++i) {
                int x = 48 + (int)i * theme_w;
                if (ui_pointer_activated(&event, x, theme_y, theme_w - 18, 150)) {
                    preview_theme = i;
                    button_focus_enabled = false;
                    handled = true;
                }
            }
            for (uint32_t speed = 1; speed <= 5; ++speed) {
                int x = speed_x + (int)(speed - 1) * (speed_w + speed_gap);
                if (ui_pointer_activated(&event, x, speed_y, speed_w, speed_h)) {
                    preview_pointer_speed = speed;
                    handled = true;
                }
            }
            for (uint32_t mode = NETWORK_PREF_AUTO; mode <= NETWORK_PREF_HOTSPOT; ++mode) {
                int x = mode_x + (int)mode * (mode_w + mode_gap);
                if (ui_pointer_activated(&event, x, mode_y, mode_w, mode_h)) {
                    preview_network_mode = mode;
                    handled = true;
                }
            }
            if (ui_pointer_activated(&event, btn_x1, btn_y, btn_w, btn_h)) {
                char message[64];
                button_focus_enabled = true;
                button_focus = 0;
                g_desktop.theme = (uint32_t)preview_theme;
                g_desktop.pointer_speed = preview_pointer_speed;
                g_desktop.network_preference = preview_network_mode;
                g_desktop.pointer_trail = preview_pointer_trail;
                strcopy_limit(message, "Applied theme ", sizeof(message));
                strcat(message, desktop_theme_name());
                desktop_note_launch("SETTINGS", message);
                desktop_commit_preferences();
                network_refresh(g_desktop.network_autoconnect);
                continue;
            }
            if (ui_pointer_activated(&event, btn_x2, btn_y, btn_w, btn_h)) {
                button_focus_enabled = true;
                button_focus = 1;
                g_desktop.autosave = !g_desktop.autosave;
                desktop_note_launch("SETTINGS", g_desktop.autosave ? "Autosave enabled." : "Autosave disabled.");
                desktop_commit_preferences();
                continue;
            }
            if (ui_pointer_activated(&event, btn_x3, btn_y, btn_w, btn_h)) {
                button_focus_enabled = true;
                button_focus = 2;
                preview_pointer_trail = !preview_pointer_trail;
                handled = true;
            }
            if (ui_pointer_activated(&event, btn_x4, btn_y, btn_w, btn_h)) {
                button_focus_enabled = true;
                button_focus = 3;
                ui_run_network_app();
                continue;
            }
            if (ui_pointer_activated(&event, btn_x5, btn_y, btn_w, btn_h)) {
                button_focus_enabled = true;
                button_focus = 4;
                ui_run_disk_app();
                continue;
            }
            if (handled) {
                continue;
            }
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_ESC) {
            return;
        }
        if (event.key.scan_code == EFI_SCAN_UP) {
            button_focus_enabled = false;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_DOWN) {
            button_focus_enabled = true;
            continue;
        }
        if (event.key.unicode_char == '\t') {
            button_focus_enabled = !button_focus_enabled;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_LEFT) {
            if (button_focus_enabled) {
                button_focus = button_focus == 0 ? 4 : button_focus - 1;
            } else {
                preview_theme = preview_theme == 0 ? ARRAY_SIZE(g_theme_names) - 1 : preview_theme - 1;
            }
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_RIGHT) {
            if (button_focus_enabled) {
                button_focus = (button_focus + 1) % 5;
            } else {
                preview_theme = (preview_theme + 1) % ARRAY_SIZE(g_theme_names);
            }
            continue;
        }
        if (event.key.unicode_char == '\r' || ascii_lower((char)event.key.unicode_char) == 'a') {
            if (button_focus_enabled) {
                if (button_focus == 1) {
                    g_desktop.autosave = !g_desktop.autosave;
                    desktop_note_launch("SETTINGS", g_desktop.autosave ? "Autosave enabled." : "Autosave disabled.");
                    desktop_commit_preferences();
                } else if (button_focus == 2) {
                    preview_pointer_trail = !preview_pointer_trail;
                } else if (button_focus == 3) {
                    ui_run_network_app();
                } else if (button_focus == 4) {
                    ui_run_disk_app();
                } else {
                    char message[64];
                    g_desktop.theme = (uint32_t)preview_theme;
                    g_desktop.pointer_speed = preview_pointer_speed;
                    g_desktop.network_preference = preview_network_mode;
                    g_desktop.pointer_trail = preview_pointer_trail;
                    strcopy_limit(message, "Applied theme ", sizeof(message));
                    strcat(message, desktop_theme_name());
                    desktop_note_launch("SETTINGS", message);
                    desktop_commit_preferences();
                    network_refresh(g_desktop.network_autoconnect);
                }
                continue;
            }
            char message[64];
            g_desktop.theme = (uint32_t)preview_theme;
            g_desktop.pointer_speed = preview_pointer_speed;
            g_desktop.network_preference = preview_network_mode;
            g_desktop.pointer_trail = preview_pointer_trail;
            strcopy_limit(message, "Applied theme ", sizeof(message));
            strcat(message, desktop_theme_name());
            desktop_note_launch("SETTINGS", message);
            desktop_commit_preferences();
            network_refresh(g_desktop.network_autoconnect);
            continue;
        }
        switch (ascii_lower((char)event.key.unicode_char)) {
            case 'p':
                preview_pointer_speed = preview_pointer_speed >= 5 ? 1 : preview_pointer_speed + 1;
                break;
            case 'm':
                preview_network_mode = preview_network_mode >= NETWORK_PREF_HOTSPOT ? NETWORK_PREF_AUTO :
                                        preview_network_mode + 1;
                break;
            case 't':
                preview_pointer_trail = !preview_pointer_trail;
                break;
            case 's':
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("SETTINGS", "Saved the SNSX session.");
                break;
            case 'n':
                ui_run_network_app();
                break;
            case 'd':
                ui_run_disk_app();
                break;
            default:
                break;
        }
    }
}

static void ui_run_files_app(void) {
    ui_event_t event;
    arm_fs_entry_t *entries[FILE_MANAGER_VIEW_MAX];
    char path[FS_PATH_MAX];
    char input[FS_PATH_MAX];

    strcopy_limit(g_file_view.cwd, g_cwd, sizeof(g_file_view.cwd));

    while (1) {
        size_t count = fs_list(g_file_view.cwd, entries, ARRAY_SIZE(entries));
        int visible = 12;
        int start = 0;
        if (g_file_view.selected >= count && count > 0) {
            g_file_view.selected = count - 1;
        }
        if ((int)g_file_view.selected >= visible) {
            start = (int)g_file_view.selected - visible + 1;
        }

        ui_draw_window("SNSX FILES", g_file_view.cwd);
        gfx_draw_text(56, 140, "ENTER OPEN  E EDIT  N NEW  M MKDIR  R RENAME  C COPY  DEL REMOVE  BACKSPACE UP", 1, gfx_rgb(60, 82, 112));
        for (int row = 0; row < visible && start + row < (int)count; ++row) {
            int y = 178 + row * 40;
            bool selected = (size_t)(start + row) == g_file_view.selected;
            uint32_t fill = selected ? gfx_rgb(255, 223, 170) : gfx_rgb(247, 250, 255);
            uint32_t border = selected ? gfx_rgb(142, 86, 25) : gfx_rgb(199, 210, 226);
            gfx_draw_panel(56, y, (int)g_gfx.width - 112, 30, fill, border);
            gfx_draw_text(72, y + 10, entries[start + row]->type == ARM_FS_DIR ? "[DIR]" : "[TXT]", 1,
                          entries[start + row]->type == ARM_FS_DIR ? gfx_rgb(34, 96, 144) : gfx_rgb(58, 65, 82));
            gfx_draw_text(132, y + 10, path_basename(entries[start + row]->path), 1, gfx_rgb(26, 39, 57));
        }
        ui_draw_footer("ESC BACK TO DESKTOP");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (ui_close_button_event_hit(&event)) {
            strcopy_limit(g_cwd, g_file_view.cwd, sizeof(g_cwd));
            return;
        }
        if (event.mouse_clicked || event.mouse_released) {
            for (int row = 0; row < visible && start + row < (int)count; ++row) {
                int y = 178 + row * 40;
                if (ui_pointer_activated(&event, 56, y, (int)g_gfx.width - 112, 30)) {
                    arm_fs_entry_t *clicked = entries[start + row];
                    g_file_view.selected = (size_t)(start + row);
                    if (clicked->type == ARM_FS_DIR) {
                        strcopy_limit(g_file_view.cwd, clicked->path, sizeof(g_file_view.cwd));
                        g_file_view.selected = 0;
                    } else {
                        ui_view_text_file(clicked->path, "SNSX VIEWER");
                    }
                    break;
                }
            }
            continue;
        }
        if (!event.key_ready) {
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_ESC) {
            strcopy_limit(g_cwd, g_file_view.cwd, sizeof(g_cwd));
            return;
        }
        if (event.key.scan_code == EFI_SCAN_UP && g_file_view.selected > 0) {
            g_file_view.selected--;
            continue;
        }
        if (event.key.scan_code == EFI_SCAN_DOWN && g_file_view.selected + 1 < count) {
            g_file_view.selected++;
            continue;
        }
        if (event.key.unicode_char == '\b') {
            parent_directory(g_file_view.cwd);
            g_file_view.selected = 0;
            continue;
        }
        if (count == 0) {
            if (ascii_lower((char)event.key.unicode_char) == 'n' &&
                ui_prompt_line("NEW FILE", "ENTER FILE NAME", "", input, sizeof(input))) {
                resolve_path_from(g_file_view.cwd, input, path);
                if (fs_write(path, (const uint8_t *)"", 0, ARM_FS_TEXT)) {
                    persist_flush_state();
                    ui_edit_text_file(path);
                }
            } else if (ascii_lower((char)event.key.unicode_char) == 'm' &&
                       ui_prompt_line("NEW DIRECTORY", "ENTER DIRECTORY NAME", "", input, sizeof(input))) {
                resolve_path_from(g_file_view.cwd, input, path);
                fs_mkdir(path);
                persist_flush_state();
            }
            continue;
        }

        arm_fs_entry_t *entry = entries[g_file_view.selected];
        if (event.key.unicode_char == '\r') {
            if (entry->type == ARM_FS_DIR) {
                strcopy_limit(g_file_view.cwd, entry->path, sizeof(g_file_view.cwd));
                g_file_view.selected = 0;
            } else {
                ui_view_text_file(entry->path, "SNSX VIEWER");
            }
            continue;
        }

        switch (ascii_lower((char)event.key.unicode_char)) {
            case 'e':
                if (entry->type != ARM_FS_DIR) {
                    ui_edit_text_file(entry->path);
                }
                break;
            case 'n':
                if (ui_prompt_line("NEW FILE", "ENTER FILE NAME", "", input, sizeof(input))) {
                    resolve_path_from(g_file_view.cwd, input, path);
                    if (fs_write(path, (const uint8_t *)"", 0, ARM_FS_TEXT)) {
                        persist_flush_state();
                        ui_edit_text_file(path);
                    }
                }
                break;
            case 'm':
                if (ui_prompt_line("NEW DIRECTORY", "ENTER DIRECTORY NAME", "", input, sizeof(input))) {
                    resolve_path_from(g_file_view.cwd, input, path);
                    fs_mkdir(path);
                    persist_flush_state();
                }
                break;
            case 'r':
                if (ui_prompt_line("RENAME ENTRY", "NEW NAME", path_basename(entry->path), input, sizeof(input))) {
                    char parent[FS_PATH_MAX];
                    strcopy_limit(parent, entry->path, sizeof(parent));
                    parent_directory(parent);
                    if (strcmp(parent, "/") == 0) {
                        strcopy_limit(path, "/", sizeof(path));
                        strcat(path, input);
                    } else {
                        strcopy_limit(path, parent, sizeof(path));
                        strcat(path, "/");
                        strcat(path, input);
                    }
                    fs_move(entry->path, path);
                    persist_flush_state();
                }
                break;
            case 'c':
                if (entry->type != ARM_FS_DIR &&
                    ui_prompt_line("COPY FILE", "COPY TO NEW NAME", path_basename(entry->path), input, sizeof(input))) {
                    char parent[FS_PATH_MAX];
                    strcopy_limit(parent, entry->path, sizeof(parent));
                    parent_directory(parent);
                    if (strcmp(parent, "/") == 0) {
                        strcopy_limit(path, "/", sizeof(path));
                        strcat(path, input);
                    } else {
                        strcopy_limit(path, parent, sizeof(path));
                        strcat(path, "/");
                        strcat(path, input);
                    }
                    fs_copy(entry->path, path);
                    persist_flush_state();
                }
                break;
            case 'd':
                fs_remove(entry->path);
                persist_flush_state();
                break;
            default:
                if (event.key.scan_code == EFI_SCAN_DELETE) {
                    fs_remove(entry->path);
                    persist_flush_state();
                }
                break;
        }
    }
}

static void app_launcher(void) {
    bool desktop_auto_connect_checked = false;
    static const char *titles[] = {
        "WELCOME", "FILES", "NOTES", "CALC",
        "GUIDE", "INSTALLER", "NETWORK", "DISKS",
        "SETTINGS", "BROWSER", "STORE", "SNAKE", "STUDIO"
    };
    static const char *subtitles[] = {
        "FIRST-RUN TOUR AND STARTUP HELP",
        "BROWSE AND MANAGE FILES",
        "OPEN THE SNSX TEXT EDITOR",
        "RUN EXPRESSIONS",
        "READ THE INSTALL DOCS",
        "INSTALL TO DISK",
        "ADAPTER STATUS",
        "MANAGE SNSX VOLUMES",
        "DESKTOP CONTROL CENTER",
        "LOCAL DOCS AND FUTURE WEB",
        "APPS, GAMES, AND DRIVER PACKS",
        "RETRO ARCADE",
        "HOST-BACKED CODE WORKSPACE"
    };
    static const char *descriptions[] = {
        "Open the welcome center for a guided tour, install flow, and desktop quick start.",
        "Browse directories, create text files, rename documents, and inspect the live SNSX filesystem.",
        "Jump straight into the built-in editor for journaling, notes, or quick configuration changes.",
        "Evaluate expressions inside the graphical calculator.",
        "Read the bundled VirtualBox, desktop, and install documentation without leaving SNSX.",
        "Install BOOTAA64.EFI onto the writable VM disk and save session data.",
        "Inspect firmware-level adapter discovery and media visibility.",
        "Inspect writable disks, boot files, and live state snapshots in one place.",
        "Adjust appearance, autosave, and launch the network and disk control tools.",
        "Browse bundled SNSX pages, bookmarks, and future network-backed destinations.",
        "Manage the SNSX App Store catalog, optional kits, and driver-pack scaffolding.",
        "Play the built-in Snake arcade game with keyboard controls and score tracking.",
        "Edit project files and run or build SNSX, Python, JavaScript, C, C++, HTML, and CSS workspaces."
    };
    size_t selected = 0;
    char line[SHELL_INPUT_MAX];

    if (!g_gfx.available || g_force_text_ui) {
        while (1) {
            desktop_draw_text_ui();
            line_read(line, sizeof(line));

            if (line[0] == '\0' || strcmp(line, "shell") == 0 ||
                strcmp(line, "terminal") == 0 || strcmp(line, "5") == 0) {
                return;
            }
            if (strcmp(line, "setup") == 0 || strcmp(line, "0") == 0) {
                desktop_note_launch("SETUP", "Opened the first-run setup app.");
                app_setup_console();
                continue;
            }
            if (strcmp(line, "1") == 0 || strcmp(line, "files") == 0) {
                desktop_note_launch("FILES", "Opened the file manager.");
                app_files();
                continue;
            }
            if (strcmp(line, "2") == 0 || strcmp(line, "notes") == 0 || strcmp(line, "edit") == 0) {
                desktop_note_launch("NOTES", "Opened /home/NOTES.TXT.");
                app_edit("/home/NOTES.TXT");
                continue;
            }
            if (strcmp(line, "3") == 0 || strcmp(line, "journal") == 0) {
                desktop_note_launch("JOURNAL", "Opened /home/JOURNAL.TXT.");
                app_edit("/home/JOURNAL.TXT");
                continue;
            }
            if (strcmp(line, "4") == 0 || strcmp(line, "calc") == 0 || strcmp(line, "calculator") == 0) {
                desktop_note_launch("CALC", "Opened the calculator.");
                app_calc(0, NULL);
                continue;
            }
            if (strcmp(line, "6") == 0 || strcmp(line, "guide") == 0 || strcmp(line, "desktop") == 0) {
                desktop_note_launch("GUIDE", "Opened the desktop guide.");
                view_text("/docs/DESKTOP.TXT");
                continue;
            }
            if (strcmp(line, "7") == 0 || strcmp(line, "quickstart") == 0) {
                desktop_note_launch("QUICKSTART", "Opened the quickstart guide.");
                view_text("/docs/QUICKSTART.TXT");
                continue;
            }
            if (strcmp(line, "8") == 0 || strcmp(line, "welcome") == 0) {
                desktop_note_launch("WELCOME", "Opened the welcome guide.");
                view_text("/home/WELCOME.TXT");
                continue;
            }
            if (strcmp(line, "13") == 0 || strcmp(line, "snake") == 0 || strcmp(line, "game") == 0) {
                desktop_note_launch("SNAKE", "Opened SNSX Snake.");
                if (g_gfx.available) {
                    ui_run_snake_app();
                } else {
                    view_text("/docs/SNAKE.TXT");
                }
                continue;
            }
            if (strcmp(line, "14") == 0 || strcmp(line, "store") == 0 || strcmp(line, "appstore") == 0) {
                desktop_note_launch("STORE", "Opened the SNSX App Store.");
                app_store_console();
                continue;
            }
            if (strcmp(line, "15") == 0 || strcmp(line, "studio") == 0 || strcmp(line, "code") == 0) {
                desktop_note_launch("STUDIO", "Opened SNSX Code Studio.");
                app_code_studio_console();
                continue;
            }
            if (strcmp(line, "9") == 0 || strcmp(line, "installer") == 0 || strcmp(line, "install") == 0) {
                desktop_note_launch("INSTALLER", "Opened the installer.");
                app_installer_console();
                continue;
            }
            if (strcmp(line, "10") == 0 || strcmp(line, "settings") == 0) {
                desktop_note_launch("SETTINGS", "Opened SNSX Settings.");
                app_settings_console();
                continue;
            }
            if (strcmp(line, "browser") == 0 || strcmp(line, "browse") == 0) {
                desktop_note_launch("BROWSER", "Opened the SNSX Browser guide.");
                view_text("/docs/BROWSER.TXT");
                continue;
            }
            if (strcmp(line, "baremetal") == 0 || strcmp(line, "primary") == 0) {
                desktop_note_launch("INSTALLER", "Opened the bare-metal install guide.");
                view_text("/docs/BAREMETAL.TXT");
                continue;
            }
            if (strcmp(line, "bluetooth") == 0 || strcmp(line, "bt") == 0) {
                desktop_note_launch("BLUETOOTH", "Opened the Bluetooth guide.");
                view_text("/docs/BLUETOOTH.TXT");
                continue;
            }
            if (strcmp(line, "wireless") == 0 || strcmp(line, "wifiapp") == 0) {
                desktop_note_launch("NETWORK", "Opened the wireless guide.");
                view_text("/docs/NETWORK.TXT");
                continue;
            }
            if (strcmp(line, "11") == 0 || strcmp(line, "system") == 0 ||
                strcmp(line, "devices") == 0 || strcmp(line, "network") == 0 || strcmp(line, "about") == 0) {
                desktop_note_launch("SYSTEM", "Opened the System Center.");
                app_system_console();
                continue;
            }
            if (strcmp(line, "disks") == 0 || strcmp(line, "storage") == 0) {
                desktop_note_launch("DISKS", "Opened the Disk Manager.");
                app_disk_console();
                continue;
            }
            if (strcmp(line, "12") == 0 || strcmp(line, "save") == 0) {
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("SAVE", g_persist.status);
                wait_for_enter(g_persist.status);
                continue;
            }
            if (strcmp(line, "reboot") == 0 || strcmp(line, "power") == 0) {
                g_desktop_request_firmware = true;
                return;
            }
            if (strcmp(line, "help") == 0 || strcmp(line, "apps") == 0) {
                console_clear();
                command_help();
                console_writeline("");
                command_apps();
                wait_for_enter(NULL);
                continue;
            }
            wait_for_enter("Unknown desktop option.");
        }
    }

    while (1) {
        ui_event_t event;
        bool launch = false;
        size_t installed_count = 0;
        char installed_text[16];
        char package_text[32];
        int hero_x = 36;
        int hero_y = 124;
        int hero_w = (int)g_gfx.width - 346;
        int hero_h = 118;
        int side_x = (int)g_gfx.width - 272;
        int side_y = 124;
        int side_w = 236;
        int side_h = (int)g_gfx.height - 256;
        int strip_y = 72;
        int strip_h = 34;
        int strip_gap = 10;
        int strip_w = 112;
        int strip_x1 = 36;
        int strip_x2 = strip_x1 + strip_w + strip_gap;
        int strip_x3 = strip_x2 + strip_w + strip_gap;
        int strip_x4 = strip_x3 + strip_w + strip_gap;
        int strip_x5 = strip_x4 + strip_w + strip_gap;
        int strip_x6 = strip_x5 + strip_w + strip_gap;
        int grid_x = 36;
        int grid_y = g_gfx.width >= 960 ? 280 : 250;
        int cols = g_gfx.width >= 960 ? 4 : 3;
        int card_gap_x = g_gfx.width >= 960 ? 18 : 12;
        int card_gap_y = g_gfx.width >= 960 ? 14 : 8;
        int card_w = (hero_w - (cols - 1) * card_gap_x) / cols;
        int card_h = g_gfx.width >= 960 ? 96 : 72;
        int icon_size = g_gfx.width >= 960 ? 42 : 28;
        int icon_y = g_gfx.width >= 960 ? 18 : 14;
        int title_y = g_gfx.width >= 960 ? 26 : 18;
        int subtitle_y = g_gfx.width >= 960 ? 76 : 52;
        int dock_y = (int)g_gfx.height - (g_gfx.width >= 960 ? 156 : 116);
        int quick_gap = 12;
        int quick_w = ((int)g_gfx.width - 92 - quick_gap * 4) / 5;
        int quick_h = g_gfx.width >= 960 ? 48 : 40;
        int quick_x1 = 46;
        int quick_x2 = quick_x1 + quick_w + quick_gap;
        int quick_x3 = quick_x2 + quick_w + quick_gap;
        int quick_x4 = quick_x3 + quick_w + quick_gap;
        int quick_x5 = quick_x4 + quick_w + quick_gap;

        if (!desktop_auto_connect_checked && g_desktop.network_autoconnect && !g_netstack.configured) {
            network_refresh(true);
            desktop_auto_connect_checked = true;
        }

        for (size_t i = 0; i < ARRAY_SIZE(g_store_catalog); ++i) {
            if (g_store_installed[i]) {
                installed_count++;
            }
        }
        u32_to_dec((uint32_t)installed_count, installed_text);
        strcopy_limit(package_text, installed_text, sizeof(package_text));
        strcat(package_text, "/");
        {
            char total_text[16];
            u32_to_dec((uint32_t)ARRAY_SIZE(g_store_catalog), total_text);
            strcat(package_text, total_text);
        }

        ui_draw_background();
        ui_draw_top_bar("SNSX OS", "RICH DESKTOP DASHBOARD WITH APPS, STATUS, THEMES, INSTALLER, AND TERMINAL");
        ui_draw_button(strip_x1, strip_y, strip_w, strip_h, "FILES",
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER), theme_color(THEME_ROLE_TEXT));
        ui_draw_button(strip_x2, strip_y, strip_w, strip_h, "NETWORK",
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER), theme_color(THEME_ROLE_TEXT));
        ui_draw_button(strip_x3, strip_y, strip_w, strip_h, "BROWSER",
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER), theme_color(THEME_ROLE_TEXT));
        ui_draw_button(strip_x4, strip_y, strip_w, strip_h, "STORE",
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER), theme_color(THEME_ROLE_TEXT));
        ui_draw_button(strip_x5, strip_y, strip_w, strip_h, "SETTINGS",
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER), theme_color(THEME_ROLE_TEXT));
        ui_draw_button(strip_x6, strip_y, strip_w, strip_h, "INSTALL",
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER), theme_color(THEME_ROLE_TEXT));
        gfx_draw_panel(hero_x, hero_y, hero_w, hero_h, theme_color(THEME_ROLE_PANEL), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(hero_x + 22, hero_y + 22, titles[selected], 3, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(hero_x + 24, hero_y + 56, subtitles[selected], 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(hero_x + 24, hero_y + 80, descriptions[selected], 1, theme_color(THEME_ROLE_TEXT));
        ui_draw_badge(hero_x + hero_w - 196, hero_y + 20, g_persist.installed ? "INSTALLED" : "LIVE SESSION",
                      theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_TEXT));
        ui_draw_badge(hero_x + hero_w - 196, hero_y + 56, g_pointer.engaged ? "MOUSE ACTIVE" : "KEYBOARD MODE",
                      theme_color(THEME_ROLE_ACCENT), theme_color(THEME_ROLE_TEXT_LIGHT));

        gfx_draw_panel(side_x, side_y, side_w, side_h, theme_color(THEME_ROLE_PANEL), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(side_x + 20, side_y + 20, "SYSTEM", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        ui_draw_badge(side_x + 20, side_y + 56, desktop_theme_name(),
                      theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_TEXT));
        ui_draw_badge(side_x + 128, side_y + 56,
                      g_netstack.configured ? "ONLINE" : (g_network.available ? "CONNECTING" : "OFFLINE"),
                      g_netstack.configured ? theme_color(THEME_ROLE_ACCENT) :
                      (g_network.available ? theme_color(THEME_ROLE_WARM) : theme_color(THEME_ROLE_DANGER)),
                      theme_color(THEME_ROLE_TEXT_LIGHT));
        gfx_draw_text(side_x + 20, side_y + 104, "Session", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(side_x + 20, side_y + 130, g_persist.status, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(side_x + 20, side_y + 154, "Last app", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(side_x + 96, side_y + 154, g_desktop.last_app, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(side_x + 20, side_y + 176, "Last file", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(side_x + 96, side_y + 176, path_basename(g_last_opened_file), 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(side_x + 20, side_y + 210, "Network", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(side_x + 20, side_y + 236, g_network.driver_name, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(side_x + 20, side_y + 258, g_netstack.configured ? g_network.ipv4 : g_network.status, 1,
                      theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(side_x + 20, side_y + 286, "Store", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(side_x + 20, side_y + 312, package_text, 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(side_x + 78, side_y + 312, "packages installed", 1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(side_x + 20, side_y + 334,
                      g_store_installed[STORE_PKG_BAREMETAL_KIT] ? "Bare-metal kit installed" : "Bare-metal kit available in Store",
                      1, theme_color(THEME_ROLE_TEXT_SOFT));
        gfx_draw_text(side_x + 20, side_y + 360, "Workspace", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        gfx_draw_text(side_x + 20, side_y + 386, "Click cards, use the top bar, or press shortcuts.", 1, theme_color(THEME_ROLE_TEXT));
        gfx_draw_text(side_x + 20, side_y + 408, "Files F  Network K  Browser O  Store A  Studio V  Snake X", 1, theme_color(THEME_ROLE_TEXT_SOFT));

        for (size_t i = 0; i < ARRAY_SIZE(titles); ++i) {
            int col = (int)(i % cols);
            int row = (int)(i / cols);
            int x = grid_x + col * (card_w + card_gap_x);
            int y = grid_y + row * (card_h + card_gap_y);
            bool hover = g_pointer.engaged && point_in_rect(g_pointer.x, g_pointer.y, x, y, card_w, card_h);
            bool active = i == selected || hover;
            uint32_t fill = active ? theme_color(THEME_ROLE_WARM) : theme_color(THEME_ROLE_PANEL);
            uint32_t border = active ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_BORDER);
            uint32_t text = active ? theme_color(THEME_ROLE_TEXT) : theme_color(THEME_ROLE_TEXT);
            gfx_draw_panel(x, y, card_w, card_h, fill, border);
            gfx_fill_rect(x + 16, y + icon_y, icon_size, icon_size,
                          active ? theme_color(THEME_ROLE_ACCENT_STRONG) : theme_color(THEME_ROLE_ACCENT));
            gfx_draw_text(x + 58, y + title_y, titles[i], 2, text);
            gfx_draw_text(x + 16, y + subtitle_y, subtitles[i], 1,
                          active ? theme_color(THEME_ROLE_TEXT) : theme_color(THEME_ROLE_TEXT_SOFT));
            if (hover) {
                selected = i;
            }
        }

        gfx_draw_panel(36, dock_y, (int)g_gfx.width - 72, g_gfx.width >= 960 ? 88 : 74,
                       theme_color(THEME_ROLE_PANEL), theme_color(THEME_ROLE_BORDER));
        gfx_draw_text(58, dock_y + 18, "QUICK ACTIONS", 2, theme_color(THEME_ROLE_ACCENT_STRONG));
        ui_draw_button(quick_x1, dock_y + 24, quick_w, quick_h, "SAVE",
                       theme_color(THEME_ROLE_WARM), theme_color(THEME_ROLE_ACCENT_STRONG), theme_color(THEME_ROLE_TEXT));
        ui_draw_button(quick_x2, dock_y + 24, quick_w, quick_h, "WELCOME",
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER), theme_color(THEME_ROLE_TEXT));
        ui_draw_button(quick_x3, dock_y + 24, quick_w, quick_h, "SETTINGS",
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER), theme_color(THEME_ROLE_TEXT));
        ui_draw_button(quick_x4, dock_y + 24, quick_w, quick_h, "TERMINAL",
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_BORDER), theme_color(THEME_ROLE_TEXT));
        ui_draw_button(quick_x5, dock_y + 24, quick_w, quick_h, "POWER",
                       theme_color(THEME_ROLE_PANEL_ALT), theme_color(THEME_ROLE_DANGER), theme_color(THEME_ROLE_TEXT));
        ui_draw_footer("TOP BAR, CARDS, AND QUICK ACTIONS ARE CLICKABLE  ENTER OPENS  SHORTCUTS: F K O A V X T");
        ui_draw_cursor();

        ui_wait_event(&event);
        if (event.mouse_clicked || event.mouse_released) {
            bool handled = false;
            if (ui_pointer_activated(&event, hero_x, hero_y, hero_w, hero_h)) {
                launch = true;
                handled = true;
            }
            if (ui_pointer_activated(&event, strip_x1, strip_y, strip_w, strip_h)) {
                selected = 1;
                launch = true;
                handled = true;
            }
            if (ui_pointer_activated(&event, strip_x2, strip_y, strip_w, strip_h)) {
                selected = 6;
                launch = true;
                handled = true;
            }
            if (ui_pointer_activated(&event, strip_x3, strip_y, strip_w, strip_h)) {
                selected = 9;
                launch = true;
                handled = true;
            }
            if (ui_pointer_activated(&event, strip_x4, strip_y, strip_w, strip_h)) {
                selected = 10;
                launch = true;
                handled = true;
            }
            if (ui_pointer_activated(&event, strip_x5, strip_y, strip_w, strip_h)) {
                selected = 8;
                launch = true;
                handled = true;
            }
            if (ui_pointer_activated(&event, strip_x6, strip_y, strip_w, strip_h)) {
                selected = 5;
                launch = true;
                handled = true;
            }
            if (ui_pointer_activated(&event, quick_x1, dock_y + 24, quick_w, quick_h)) {
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("DESKTOP", "Saved the SNSX session.");
                continue;
            }
            if (ui_pointer_activated(&event, quick_x2, dock_y + 24, quick_w, quick_h)) {
                desktop_note_launch("WELCOME", "Opened the welcome center.");
                ui_run_welcome_app(false);
                continue;
            }
            if (ui_pointer_activated(&event, quick_x3, dock_y + 24, quick_w, quick_h)) {
                desktop_note_launch("SETTINGS", "Opened desktop settings.");
                ui_run_settings_app();
                continue;
            }
            if (ui_pointer_activated(&event, quick_x4, dock_y + 24, quick_w, quick_h)) {
                desktop_note_launch("TERMINAL", "Opened the SNSX terminal.");
                if (ui_run_terminal_app() == SHELL_RESULT_FIRMWARE) {
                    return;
                }
                continue;
            }
            if (ui_pointer_activated(&event, quick_x5, dock_y + 24, quick_w, quick_h)) {
                desktop_note_launch("POWER", "Leaving the SNSX desktop.");
                return;
            }
            for (size_t i = 0; i < ARRAY_SIZE(titles); ++i) {
                int col = (int)(i % cols);
                int row = (int)(i / cols);
                int x = grid_x + col * (card_w + card_gap_x);
                int y = grid_y + row * (card_h + card_gap_y);
                if (ui_pointer_activated(&event, x, y, card_w, card_h)) {
                    selected = i;
                    launch = true;
                    handled = true;
                    break;
                }
            }
            if (!handled) {
                continue;
            }
        }
        if (event.key_ready && event.key.scan_code == EFI_SCAN_ESC) {
            return;
        }
        if (event.key_ready && (event.key.unicode_char == '\t' || event.key.scan_code == EFI_SCAN_RIGHT)) {
            selected = (selected + 1) % ARRAY_SIZE(titles);
            continue;
        }
        if (event.key_ready && event.key.scan_code == EFI_SCAN_LEFT) {
            selected = selected == 0 ? ARRAY_SIZE(titles) - 1 : selected - 1;
            continue;
        }
        if (event.key_ready && event.key.scan_code == EFI_SCAN_DOWN) {
            if (selected + (size_t)cols < ARRAY_SIZE(titles)) {
                selected += (size_t)cols;
            }
            continue;
        }
        if (event.key_ready && event.key.scan_code == EFI_SCAN_UP) {
            if (selected >= (size_t)cols) {
                selected -= (size_t)cols;
            }
            continue;
        }
        if (event.key_ready) {
            char shortcut = ascii_lower((char)event.key.unicode_char);
            if (shortcut >= '1' && shortcut <= '9') {
                selected = (size_t)(shortcut - '1');
                launch = true;
            } else if (shortcut == '0') {
                selected = 9;
                launch = true;
            } else if (shortcut == 's') {
                g_persist.dirty = true;
                persist_flush_state();
                desktop_note_launch("DESKTOP", "Saved the SNSX session.");
                continue;
            } else if (shortcut == 'w') {
                selected = 0;
                launch = true;
            } else if (shortcut == 'f') {
                selected = 1;
                launch = true;
            } else if (shortcut == 'n') {
                selected = 2;
                launch = true;
            } else if (shortcut == 'c') {
                selected = 3;
                launch = true;
            } else if (shortcut == 'g') {
                selected = 4;
                launch = true;
            } else if (shortcut == 'i') {
                selected = 5;
                launch = true;
            } else if (shortcut == 'k') {
                selected = 6;
                launch = true;
            } else if (shortcut == 'd') {
                selected = 7;
                launch = true;
            } else if (shortcut == 'y') {
                selected = 8;
                launch = true;
            } else if (shortcut == 'o') {
                selected = 9;
                launch = true;
            } else if (shortcut == 'a') {
                selected = 10;
                launch = true;
            } else if (shortcut == 'v' || shortcut == 'u') {
                selected = 12;
                launch = true;
            } else if (shortcut == 'x') {
                selected = 11;
                launch = true;
            } else if (shortcut == 't') {
                desktop_note_launch("TERMINAL", "Opened the SNSX terminal.");
                if (ui_run_terminal_app() == SHELL_RESULT_FIRMWARE) {
                    return;
                }
                continue;
            } else if (shortcut == 'p') {
                desktop_note_launch("POWER", "Leaving the SNSX desktop.");
                return;
            }
        }
        if (!launch && (!event.key_ready || event.key.unicode_char != '\r')) {
            continue;
        }

        switch (selected) {
            case 0:
                desktop_note_launch("WELCOME", "Opened the welcome center.");
                ui_run_welcome_app(false);
                break;
            case 1:
                desktop_note_launch("FILES", "Opened the file manager.");
                ui_run_files_app();
                break;
            case 2:
                desktop_note_launch("NOTES", "Opened the Notes editor.");
                ui_edit_text_file("/home/NOTES.TXT");
                break;
            case 3:
                desktop_note_launch("CALC", "Opened the calculator.");
                ui_run_calc_app();
                break;
            case 4:
                desktop_note_launch("GUIDE", "Opened the install and desktop guides.");
                ui_view_text_file("/docs/INSTALL.TXT", "SNSX GUIDE");
                break;
            case 5:
                desktop_note_launch("INSTALLER", "Opened the installer.");
                ui_run_installer_app();
                break;
            case 6:
                desktop_note_launch("NETWORK", "Opened network status.");
                ui_run_network_app();
                break;
            case 7:
                desktop_note_launch("DISKS", "Opened disk management.");
                ui_run_disk_app();
                break;
            case 8:
                desktop_note_launch("SETTINGS", "Opened desktop settings.");
                ui_run_settings_app();
                break;
            case 9:
                desktop_note_launch("BROWSER", "Opened the SNSX Browser.");
                ui_run_browser_app();
                break;
            case 10:
                desktop_note_launch("STORE", "Opened the SNSX App Store.");
                ui_run_store_app();
                break;
            case 11:
                desktop_note_launch("SNAKE", "Opened SNSX Snake.");
                ui_run_snake_app();
                break;
            case 12:
                desktop_note_launch("STUDIO", "Opened SNSX Code Studio.");
                ui_run_code_studio_app();
                break;
            default:
                break;
        }
    }
}

static void command_open(const char *path) {
    char absolute[FS_PATH_MAX];
    arm_fs_entry_t *entry;
    resolve_path(path, absolute);
    if (strcmp(absolute, "/") == 0) {
        command_ls("/");
        return;
    }
    entry = fs_find(absolute);
    if (entry == NULL) {
        console_writeline("Path not found.");
        return;
    }
    if (entry->type == ARM_FS_DIR) {
        command_ls(absolute);
        return;
    }
    view_text(absolute);
}

static void command_find(const char *needle) {
    size_t matches = 0;
    for (size_t i = 0; i < g_fs_count; ++i) {
        if (!text_contains(g_fs[i].path, needle) && !text_contains(path_basename(g_fs[i].path), needle)) {
            continue;
        }
        shell_print_entry(&g_fs[i]);
        matches++;
    }
    if (matches == 0) {
        console_writeline("No matching files or directories.");
    }
}

static void command_write(const char *path, const char *text) {
    char absolute[FS_PATH_MAX];
    resolve_path(path, absolute);
    if (!fs_write(absolute, (const uint8_t *)text, strlen(text), ARM_FS_TEXT)) {
        console_writeline("Write failed.");
    }
}

static void command_append(const char *path, const char *text) {
    char absolute[FS_PATH_MAX];
    resolve_path(path, absolute);
    if (!fs_append_line(absolute, text)) {
        console_writeline("Append failed.");
    }
}

static void command_new(const char *path) {
    char absolute[FS_PATH_MAX];
    resolve_path(path, absolute);
    if (!fs_write(absolute, (const uint8_t *)"", 0, ARM_FS_TEXT)) {
        console_writeline("File creation failed.");
        return;
    }
    app_edit(absolute);
}

static void show_boot_dashboard(void) {
    if (g_gfx.available) {
        ui_draw_background();
        ui_draw_top_bar("SNSX OS", "BOOTING DIAGNOSTICS, FIRST-RUN SETUP, AND THE DESKTOP EXPERIENCE");
        gfx_draw_panel(88, 190, (int)g_gfx.width - 176, 190, gfx_rgb(238, 243, 249), gfx_rgb(36, 56, 84));
        gfx_draw_text(118, 232, "SNSX GRAPHICAL DESKTOP", 3, gfx_rgb(30, 47, 70));
        gfx_draw_text(118, 278, "ARM64 UEFI BUILD FOR APPLE SILICON VIRTUALBOX", 1, gfx_rgb(71, 92, 118));
        gfx_draw_text(118, 302, "APPS: SETUP, FILES, NOTES, CALC, GUIDE, TERMINAL, INSTALLER, NETWORK, SETTINGS, BROWSER, STORE",
                      1, gfx_rgb(71, 92, 118));
        gfx_draw_text(118, 326, g_persist.status, 1, gfx_rgb(71, 92, 118));
        gfx_draw_text(118, 350, g_pointer.engaged ? "MOUSE INPUT ACTIVE" : "KEYBOARD NAVIGATION ACTIVE", 1,
                      gfx_rgb(71, 92, 118));
        gfx_draw_text(118, 374, desktop_theme_name(), 1, gfx_rgb(71, 92, 118));
        gfx_present();
        return;
    }
    console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
    console_writeline("SNSX ARM64 for VirtualBox");
    console_set_color(EFI_WHITE, EFI_BLACK);
    console_writeline("Boot path: UEFI AArch64");
    console_writeline("Target host: Apple Silicon VirtualBox");
    console_writeline("");
    console_writeline("Included UI: diagnostics, first-run setup, desktop launcher, terminal, installer, settings");
    console_writeline("Bundled docs: /docs/QUICKSTART.TXT and /docs/INSTALL.TXT");
    console_writeline("");
}

static void command_help(void) {
    console_writeline("help             Show SNSX commands");
    console_writeline("apps             Show built-in apps");
    console_writeline("setup            Reopen the first-run setup app");
    console_writeline("desktop          Return to the SNSX desktop");
    console_writeline("launcher         Open the SNSX launcher menu");
    console_writeline("theme [NAME]     Show or set the desktop theme");
    console_writeline("pointer [1-5]    Show or set mouse speed");
    console_writeline("trail [on|off]   Toggle the mouse trail");
    console_writeline("save             Save the current SNSX session to disk");
    console_writeline("install          Install SNSX boot files onto the writable disk");
    console_writeline("net              Show detected network adapter info");
    console_writeline("network          Open the Network Center");
    console_writeline("netrefresh       Refresh firmware network status");
    console_writeline("netconnect       Initialize the wired firmware adapter");
    console_writeline("netdiag          Show firmware-handle and PCI NIC diagnostics");
    console_writeline("ip               Show current IPv4, gateway, and DNS state");
    console_writeline("dhcp             Renew the wired DHCP lease");
    console_writeline("dns HOST         Resolve a host name");
    console_writeline("http URL         Fetch an http:// page or https:// page via bridge");
    console_writeline("netmode MODE     Set auto|ethernet|wifi|hotspot");
    console_writeline("autonet [on|off] Toggle network autoconnect");
    console_writeline("hostname NAME    Set the SNSX host name");
    console_writeline("wifi NAME        Store the preferred Wi-Fi profile name");
    console_writeline("hotspot NAME     Store a hotspot profile name");
    console_writeline("wireless         Open the Wi-Fi profile dashboard");
    console_writeline("bluetooth        Open the Bluetooth dashboard");
    console_writeline("browser [ADDR]   Open SNSX Browser");
    console_writeline("studio           Open SNSX Code Studio");
    console_writeline("run [FILE]       Run the active or named source file");
    console_writeline("build [FILE]     Build the active or named source file");
    console_writeline("store            Open the SNSX App Store");
    console_writeline("pkglist          Show App Store package catalog");
    console_writeline("installpkg ID    Install an App Store package");
    console_writeline("removepkg ID     Remove an optional App Store package");
    console_writeline("baremetal        View the USB and primary-disk install guide");
    console_writeline("snsxaddr         Show SNSX protocol identities");
    console_writeline("devices          Show keyboard, mouse, graphics, and disk status");
    console_writeline("clear            Clear the screen");
    console_writeline("pwd              Show current directory");
    console_writeline("ls [PATH]        List files or directories");
    console_writeline("tree [PATH]      Show a simple directory tree");
    console_writeline("cd DIR           Change directory");
    console_writeline("open PATH        Open a file or inspect a directory");
    console_writeline("find NAME        Search files and directories by name");
    console_writeline("cat FILE         Show a text file");
    console_writeline("stat PATH        Show file metadata");
    console_writeline("touch FILE       Create an empty file");
    console_writeline("new FILE         Create a file and open the editor");
    console_writeline("write FILE TEXT  Overwrite a text file from the shell");
    console_writeline("append FILE TXT  Append a line of text to a file");
    console_writeline("mkdir DIR        Create a directory");
    console_writeline("rm PATH          Remove a file or empty directory");
    console_writeline("cp SRC DST       Copy a file");
    console_writeline("mv SRC DST       Move or rename a file or directory");
    console_writeline("welcome          Read the welcome guide");
    console_writeline("quickstart       Open the quickstart guide");
    console_writeline("settings         Open the Settings app");
    console_writeline("system           Open the System Center");
    console_writeline("installer        Open the Installer app");
    console_writeline("disks            Open the Disk Manager");
    console_writeline("files            Launch the file manager");
    console_writeline("journal          Open the journal editor");
    console_writeline("calc [EXPR]      Calculator app");
    console_writeline("snake            Snake arcade app");
    console_writeline("edit FILE        Text editor app");
    console_writeline("guide            View the VirtualBox install guide");
    console_writeline("browser          Browser app for local pages and future web access");
    console_writeline("about            Show system information");
    console_writeline("reboot           Reboot by returning to firmware");
}

static void command_apps(void) {
    console_writeline("SNSX built-in apps:");
    console_writeline("setup");
    console_writeline("files");
    console_writeline("notes");
    console_writeline("journal");
    console_writeline("calc");
    console_writeline("terminal");
    console_writeline("guide");
    console_writeline("quickstart");
    console_writeline("welcome");
    console_writeline("installer");
    console_writeline("disks");
    console_writeline("settings");
    console_writeline("system");
    console_writeline("wireless");
    console_writeline("bluetooth");
    console_writeline("browser");
    console_writeline("studio");
    console_writeline("store");
    console_writeline("snake");
    console_writeline("launcher");
    console_writeline("edit /home/NOTES.TXT");
    console_writeline("network");
    console_writeline("ip");
    console_writeline("dns example.com");
    console_writeline("http http://example.com/");
    console_writeline("run /projects/main.snsx");
}

static void command_ls(const char *path) {
    char absolute[FS_PATH_MAX];
    arm_fs_entry_t *entry;
    arm_fs_entry_t *entries[FILE_MANAGER_VIEW_MAX];
    resolve_path(path, absolute);
    entry = fs_find(absolute);
    if (entry != NULL && entry->type != ARM_FS_DIR) {
        shell_print_entry(entry);
        return;
    }
    if (entry == NULL && strcmp(absolute, "/") != 0) {
        console_writeline("Path not found.");
        return;
    }
    size_t count = fs_list(absolute, entries, ARRAY_SIZE(entries));
    if (count == 0) {
        console_writeline("<empty>");
        return;
    }
    for (size_t i = 0; i < count; ++i) {
        shell_print_entry(entries[i]);
    }
}

static void tree_walk(const char *path, int depth) {
    arm_fs_entry_t *entries[FILE_MANAGER_VIEW_MAX];
    size_t count = fs_list(path, entries, ARRAY_SIZE(entries));
    for (size_t i = 0; i < count; ++i) {
        for (int indent = 0; indent < depth; ++indent) {
            console_write("  ");
        }
        shell_print_entry(entries[i]);
        if (entries[i]->type == ARM_FS_DIR) {
            tree_walk(entries[i]->path, depth + 1);
        }
    }
}

static void command_tree(const char *path) {
    char absolute[FS_PATH_MAX];
    arm_fs_entry_t *entry;
    resolve_path(path, absolute);
    entry = fs_find(absolute);
    if (entry == NULL && strcmp(absolute, "/") != 0) {
        console_writeline("Path not found.");
        return;
    }
    console_writeline(absolute);
    tree_walk(absolute, 1);
}

static void command_cat(const char *path) {
    char absolute[FS_PATH_MAX];
    arm_fs_entry_t *entry;
    resolve_path(path, absolute);
    entry = fs_find(absolute);
    if (entry == NULL) {
        console_writeline("File not found.");
        return;
    }
    if (entry->type != ARM_FS_TEXT) {
        console_writeline("That path is not a text file.");
        return;
    }
    for (size_t i = 0; i < entry->size; ++i) {
        console_putchar((char)entry->data[i]);
    }
    if (entry->size == 0 || entry->data[entry->size - 1] != '\n') {
        console_putchar('\n');
    }
}

static void command_stat(const char *path) {
    char absolute[FS_PATH_MAX];
    arm_fs_entry_t *entry;
    resolve_path(path, absolute);
    entry = fs_find(absolute);
    if (entry == NULL) {
        console_writeline("Path not found.");
        return;
    }
    console_write("Path: ");
    console_writeline(entry->path);
    console_write("Type: ");
    console_writeline(entry->type == ARM_FS_DIR ? "directory" : "text");
    console_write("Size: ");
    console_write_dec((uint32_t)entry->size);
    console_writeline(" bytes");
    if (entry->type == ARM_FS_DIR) {
        console_write("Children: ");
        console_write_dec((uint32_t)fs_count_children(entry->path));
        console_writeline("");
    }
}

static void command_theme(const char *value) {
    if (value == NULL || value[0] == '\0') {
        console_write("Current theme: ");
        console_writeline(desktop_theme_name());
        console_writeline("Available themes: 0 OCEAN, 1 SUNRISE, 2 SLATE, 3 EMERALD");
        return;
    }
    {
        int index = desktop_find_theme(value);
        if (index < 0) {
            console_writeline("Unknown theme. Use OCEAN, SUNRISE, SLATE, or EMERALD.");
            return;
        }
        g_desktop.theme = (uint32_t)index;
        desktop_note_launch("SETTINGS", "Theme changed from the shell.");
        desktop_commit_preferences();
        console_write("Theme set to ");
        console_writeline(desktop_theme_name());
    }
}

static void command_devices(void) {
    console_writeline("SNSX device status");
    console_writeline("------------------");
    console_write("Graphics: ");
    console_writeline(g_gfx.available ? "active framebuffer desktop" : "text console fallback");
    console_write("Keyboard: ");
    if (g_input.standard_available && g_input.extended_available) {
        console_writeline("UEFI standard + extended keyboard events available");
    } else if (g_input.extended_available) {
        console_writeline("UEFI extended keyboard event available");
    } else if (g_input.standard_available) {
        console_writeline("UEFI standard keyboard event available");
    } else {
        console_writeline("UEFI keyboard events missing");
    }
    console_write("Mouse: ");
    if (!g_pointer.available) {
        console_writeline("no pointer protocol");
    } else if (!g_pointer.engaged) {
        console_writeline("pointer protocol present, keyboard-first mode active");
    } else if (g_pointer.kind == POINTER_KIND_ABSOLUTE) {
        console_writeline("UEFI absolute pointer available");
    } else {
        console_writeline("UEFI simple pointer available");
    }
    console_write("Network: ");
    console_writeline(g_network.available ? g_network.transport : "no network protocol");
    console_write("Internet: ");
    console_writeline(g_network.internet);
    console_write("Persistence: ");
    console_writeline(g_persist.status);
    console_write("Volumes: ");
    console_write_dec((uint32_t)g_persist_volume_count);
    console_writeline("");
    console_write("Diagnostics: ");
    if (!g_boot_diag.ran) {
        console_writeline("not run yet");
    } else if (g_boot_diag.key_seen || g_boot_diag.pointer_moved || g_boot_diag.pointer_clicked) {
        console_writeline("live input detected");
    } else if (g_boot_diag.timed_out) {
        console_writeline("timed out without live input");
    } else {
        console_writeline("protocols detected, waiting for live input");
    }
    console_write("Theme: ");
    console_writeline(desktop_theme_name());
}

static void command_about(void) {
    console_writeline("SNSX ARM64 VirtualBox Edition");
    console_writeline("UEFI-booted standalone operating environment for Apple Silicon VirtualBox.");
    console_writeline("Features: boot diagnostics, first-run setup, graphical desktop launcher, terminal,");
    console_writeline("disk-backed persistence, installer, settings, system center, network center,");
    console_writeline("Wi-Fi profile manager, Bluetooth dashboard, browser shell, file manager, notes,");
    console_writeline("journal, calculator, mouse tuning, and remembered connection profiles.");
    console_writeline("Networking: DHCP, IPv4, DNS, and plain HTTP over the firmware Ethernet adapter.");
    console_writeline("Host workflow: build SNSX-VBox-ARM64.iso, create an ARM64 EFI VM, attach the ISO, and boot.");
    console_writeline("Current hardware note: Apple Silicon VirtualBox exposes a virtual wired LAN path.");
    console_writeline("Nearby Wi-Fi scanning, Bluetooth radio access, and live web rendering need future SNSX");
    console_writeline("device drivers plus TLS and a much larger browser engine.");
    console_write("Theme: ");
    console_writeline(desktop_theme_name());
    console_write("Persistence: ");
    console_writeline(g_persist.status);
    console_write("Network: ");
    console_writeline(g_network.transport);
    console_write("Network mode: ");
    console_writeline(desktop_network_mode_name());
}

static void shell_prompt(void) {
    console_set_color(EFI_LIGHTGREEN, EFI_BLACK);
    console_write("SNSX");
    console_set_color(EFI_WHITE, EFI_BLACK);
    console_write(":");
    console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
    console_write(g_cwd);
    console_set_color(EFI_WHITE, EFI_BLACK);
    console_write("> ");
}

static shell_result_t shell_run(bool allow_desktop_return) {
    char line[SHELL_INPUT_MAX];
    char *argv[SHELL_ARG_MAX];
    char browser_target[BROWSER_ADDRESS_MAX];

    console_set_color(EFI_LIGHTCYAN, EFI_BLACK);
    console_writeline("SNSX ARM64 shell ready.");
    console_set_color(EFI_WHITE, EFI_BLACK);
    console_writeline("Apps: setup, desktop, files, notes, journal, calc, snake, settings, installer, network, wireless, bluetooth, browser, studio, store, disks. Type help.");
    if (allow_desktop_return) {
        console_writeline("Type desktop to return to the SNSX desktop.");
    }

    while (1) {
        shell_prompt();
        line_read(line, sizeof(line));
        size_t argc = shell_tokenize(line, argv, ARRAY_SIZE(argv));
        if (argc == 0) {
            continue;
        }
        if (strcmp(argv[0], "help") == 0) {
            command_help();
        } else if (strcmp(argv[0], "apps") == 0) {
            command_apps();
        } else if (strcmp(argv[0], "setup") == 0) {
            if (g_gfx.available) {
                ui_run_setup_app();
                console_clear();
            } else {
                app_setup_console();
                console_clear();
            }
        } else if ((strcmp(argv[0], "desktop") == 0 || strcmp(argv[0], "exit") == 0) && allow_desktop_return) {
            return SHELL_RESULT_DESKTOP;
        } else if (strcmp(argv[0], "theme") == 0) {
            command_theme(argc > 1 ? argv[1] : NULL);
        } else if (strcmp(argv[0], "pointer") == 0) {
            if (argc < 2) {
                console_writeline(desktop_pointer_speed_name());
            } else {
                int speed = atoi(argv[1]);
                if (speed < 1 || speed > 5) {
                    console_writeline("Usage: pointer 1|2|3|4|5");
                } else {
                    g_desktop.pointer_speed = (uint32_t)speed;
                    desktop_commit_preferences();
                    console_writeline("Pointer speed updated.");
                }
            }
        } else if (strcmp(argv[0], "trail") == 0) {
            if (argc < 2) {
                console_writeline(g_desktop.pointer_trail ? "on" : "off");
            } else if (strcmp(argv[1], "on") == 0) {
                g_desktop.pointer_trail = true;
                desktop_commit_preferences();
            } else if (strcmp(argv[1], "off") == 0) {
                g_desktop.pointer_trail = false;
                desktop_commit_preferences();
            } else {
                console_writeline("Usage: trail on|off");
            }
        } else if (strcmp(argv[0], "save") == 0) {
            g_persist.dirty = true;
            persist_flush_state();
            console_writeline(g_persist.status);
        } else if (strcmp(argv[0], "install") == 0) {
            persist_install_boot_disk();
            console_writeline(g_persist.status);
        } else if (strcmp(argv[0], "net") == 0) {
            console_write("MAC: ");
            console_writeline(g_network.mac);
            console_write("IPv4: ");
            console_writeline(g_network.ipv4);
            console_write("Media: ");
            console_writeline(g_network.media_present ? "present" : "not reported");
            console_write("Mode: ");
            console_writeline(desktop_network_mode_name());
        } else if (strcmp(argv[0], "ip") == 0) {
            show_network_status();
        } else if (strcmp(argv[0], "dhcp") == 0) {
            if (network_connect_wired() || netstack_renew_dhcp()) {
                network_refresh(false);
            }
            console_writeline(g_network.status);
        } else if (strcmp(argv[0], "dns") == 0) {
            uint32_t resolved_ip = 0;
            char ip_text[24];
            if (argc < 2) {
                console_writeline("Usage: dns HOST");
            } else if (netstack_resolve_dns_name(argv[1], &resolved_ip)) {
                net_ip_to_string(resolved_ip, ip_text, sizeof(ip_text));
                console_writeline(ip_text);
            } else {
                console_writeline("DNS lookup failed.");
            }
        } else if (strcmp(argv[0], "http") == 0) {
            char url[SHELL_INPUT_MAX];
            if (argc < 2) {
                console_writeline("Usage: http http://host/path | https://host/path");
            } else {
                shell_join_args(1, argc, argv, url, sizeof(url));
                browser_links_reset();
                if (netstack_http_fetch_text(url, g_browser_page, sizeof(g_browser_page))) {
                    console_writeline(g_browser_page);
                } else {
                    console_writeline(g_browser_page[0] != '\0' ? g_browser_page : "HTTP fetch failed.");
                }
            }
        } else if (strcmp(argv[0], "netrefresh") == 0) {
            network_refresh(false);
            console_writeline(g_network.status);
        } else if (strcmp(argv[0], "netconnect") == 0) {
            network_refresh(true);
            console_writeline(g_network.status);
        } else if (strcmp(argv[0], "netdiag") == 0) {
            network_refresh(false);
            show_network_diagnostics();
        } else if (strcmp(argv[0], "netmode") == 0) {
            if (argc < 2) {
                console_writeline(desktop_network_mode_name());
            } else if (strcmp(argv[1], "auto") == 0) {
                g_desktop.network_preference = NETWORK_PREF_AUTO;
                desktop_commit_preferences();
                network_refresh(g_desktop.network_autoconnect);
            } else if (strcmp(argv[1], "ethernet") == 0 || strcmp(argv[1], "lan") == 0) {
                g_desktop.network_preference = NETWORK_PREF_ETHERNET;
                desktop_commit_preferences();
                network_refresh(g_desktop.network_autoconnect);
            } else if (strcmp(argv[1], "wifi") == 0) {
                g_desktop.network_preference = NETWORK_PREF_WIFI;
                desktop_commit_preferences();
                network_refresh(g_desktop.network_autoconnect);
            } else if (strcmp(argv[1], "hotspot") == 0) {
                g_desktop.network_preference = NETWORK_PREF_HOTSPOT;
                desktop_commit_preferences();
                network_refresh(g_desktop.network_autoconnect);
            } else {
                console_writeline("Usage: netmode auto|ethernet|wifi|hotspot");
            }
        } else if (strcmp(argv[0], "autonet") == 0) {
            if (argc < 2) {
                g_desktop.network_autoconnect = !g_desktop.network_autoconnect;
            } else if (strcmp(argv[1], "on") == 0) {
                g_desktop.network_autoconnect = true;
            } else if (strcmp(argv[1], "off") == 0) {
                g_desktop.network_autoconnect = false;
            } else {
                console_writeline("Usage: autonet [on|off]");
                continue;
            }
            desktop_commit_preferences();
            network_refresh(g_desktop.network_autoconnect);
            console_writeline(g_desktop.network_autoconnect ? "Network autoconnect enabled." :
                                                               "Network autoconnect disabled.");
        } else if (strcmp(argv[0], "hostname") == 0) {
            char value[SHELL_INPUT_MAX];
            if (argc < 2) {
                console_writeline(g_desktop.hostname);
            } else {
                shell_join_args(1, argc, argv, value, sizeof(value));
                strcopy_limit(g_desktop.hostname, value, sizeof(g_desktop.hostname));
                desktop_commit_preferences();
                network_refresh(g_desktop.network_autoconnect);
                console_writeline("Host name updated.");
            }
        } else if (strcmp(argv[0], "wifi") == 0) {
            char value[SHELL_INPUT_MAX];
            if (argc < 2) {
                console_writeline(g_desktop.wifi_ssid);
            } else {
                shell_join_args(1, argc, argv, value, sizeof(value));
                strcopy_limit(g_desktop.wifi_ssid, value, sizeof(g_desktop.wifi_ssid));
                desktop_commit_preferences();
                network_refresh(g_desktop.network_autoconnect);
                console_writeline("Wi-Fi profile updated.");
            }
        } else if (strcmp(argv[0], "hotspot") == 0) {
            char value[SHELL_INPUT_MAX];
            if (argc < 2) {
                console_writeline(g_desktop.hotspot_name);
            } else {
                shell_join_args(1, argc, argv, value, sizeof(value));
                strcopy_limit(g_desktop.hotspot_name, value, sizeof(g_desktop.hotspot_name));
                desktop_commit_preferences();
                network_refresh(g_desktop.network_autoconnect);
                console_writeline("Hotspot profile updated.");
            }
        } else if (strcmp(argv[0], "network") == 0) {
            if (g_gfx.available) {
                ui_run_network_app();
                console_clear();
            } else {
                show_network_status();
            }
        } else if (strcmp(argv[0], "wireless") == 0 || strcmp(argv[0], "wifiapp") == 0) {
            if (g_gfx.available) {
                ui_run_wifi_app();
                console_clear();
            } else {
                view_text("/docs/NETWORK.TXT");
            }
        } else if (strcmp(argv[0], "bluetooth") == 0 || strcmp(argv[0], "bt") == 0) {
            if (g_gfx.available) {
                ui_run_bluetooth_app();
                console_clear();
            } else {
                view_text("/docs/BLUETOOTH.TXT");
            }
        } else if (strcmp(argv[0], "browser") == 0 || strcmp(argv[0], "browse") == 0) {
            if (argc > 1) {
                shell_join_args(1, argc, argv, browser_target, sizeof(browser_target));
                browser_navigate_to(browser_target);
            }
            if (g_gfx.available) {
                ui_run_browser_app();
                console_clear();
            } else {
                browser_build_page(g_browser_address);
                browser_finalize_page(g_browser_page, sizeof(g_browser_page));
                console_writeline(g_browser_page);
            }
        } else if (strcmp(argv[0], "studio") == 0 || strcmp(argv[0], "code") == 0) {
            if (argc > 1) {
                char path[FS_PATH_MAX];
                studio_resolve_input_path(argv[1], path);
                studio_ensure_path_exists(path);
                studio_select_path(path);
            }
            if (g_gfx.available) {
                ui_run_code_studio_app();
                console_clear();
            } else {
                app_code_studio_console();
                console_clear();
            }
        } else if (strcmp(argv[0], "run") == 0 || strcmp(argv[0], "build") == 0) {
            if (argc > 1) {
                char path[FS_PATH_MAX];
                studio_resolve_input_path(argv[1], path);
                studio_select_path(path);
            }
            if (strcmp(argv[0], "run") == 0) {
                studio_execute_bridge("run");
            } else {
                studio_execute_bridge("build");
            }
            console_writeline(g_studio.output);
        } else if (strcmp(argv[0], "store") == 0 || strcmp(argv[0], "appstore") == 0) {
            if (argc == 1) {
                if (g_gfx.available) {
                    ui_run_store_app();
                    console_clear();
                } else {
                    app_store_console();
                    console_clear();
                }
            } else if (strcmp(argv[1], "list") == 0) {
                command_store_list();
            } else if (strcmp(argv[1], "install") == 0 && argc > 2) {
                char status[128];
                store_install_package_by_id(argv[2], status, sizeof(status));
                console_writeline(status);
            } else if ((strcmp(argv[1], "remove") == 0 || strcmp(argv[1], "uninstall") == 0) && argc > 2) {
                char status[128];
                store_remove_package_by_id(argv[2], status, sizeof(status));
                console_writeline(status);
            } else {
                console_writeline("Usage: store [list|install ID|remove ID]");
            }
        } else if (strcmp(argv[0], "pkglist") == 0) {
            command_store_list();
        } else if (strcmp(argv[0], "installpkg") == 0) {
            char status[128];
            if (argc < 2) {
                console_writeline("Usage: installpkg ID");
            } else {
                store_install_package_by_id(argv[1], status, sizeof(status));
                console_writeline(status);
            }
        } else if (strcmp(argv[0], "removepkg") == 0 || strcmp(argv[0], "uninstallpkg") == 0) {
            char status[128];
            if (argc < 2) {
                console_writeline("Usage: removepkg ID");
            } else {
                store_remove_package_by_id(argv[1], status, sizeof(status));
                console_writeline(status);
            }
        } else if (strcmp(argv[0], "baremetal") == 0 || strcmp(argv[0], "primary") == 0) {
            view_text("/docs/BAREMETAL.TXT");
        } else if (strcmp(argv[0], "snsxaddr") == 0 || strcmp(argv[0], "identity") == 0) {
            char snsx_address[96];
            char session_address[96];
            char link_address[96];
            network_compose_snsx_address(snsx_address, sizeof(snsx_address));
            network_compose_session_address(session_address, sizeof(session_address));
            network_compose_link_address(link_address, sizeof(link_address));
            console_writeline(snsx_address);
            console_writeline(session_address);
            console_writeline(link_address);
        } else if (strcmp(argv[0], "devices") == 0) {
            command_devices();
        } else if (strcmp(argv[0], "clear") == 0) {
            console_clear();
        } else if (strcmp(argv[0], "pwd") == 0) {
            console_writeline(g_cwd);
        } else if (strcmp(argv[0], "ls") == 0) {
            command_ls(argc > 1 ? argv[1] : g_cwd);
        } else if (strcmp(argv[0], "tree") == 0) {
            command_tree(argc > 1 ? argv[1] : g_cwd);
        } else if (strcmp(argv[0], "open") == 0) {
            if (argc < 2) {
                console_writeline("Usage: open PATH");
            } else {
                command_open(argv[1]);
            }
        } else if (strcmp(argv[0], "find") == 0) {
            char query[SHELL_INPUT_MAX];
            if (argc < 2) {
                console_writeline("Usage: find NAME");
            } else {
                shell_join_args(1, argc, argv, query, sizeof(query));
                command_find(query);
            }
        } else if (strcmp(argv[0], "cd") == 0) {
            char absolute[FS_PATH_MAX];
            if (argc < 2) {
                console_writeline("Usage: cd DIR");
            } else {
                resolve_path(argv[1], absolute);
                arm_fs_entry_t *entry = fs_find(absolute);
                if ((strcmp(absolute, "/") == 0) || (entry != NULL && entry->type == ARM_FS_DIR)) {
                    strcopy_limit(g_cwd, absolute, sizeof(g_cwd));
                } else {
                    console_writeline("Directory not found.");
                }
            }
        } else if (strcmp(argv[0], "cat") == 0) {
            if (argc < 2) {
                console_writeline("Usage: cat FILE");
            } else {
                command_cat(argv[1]);
            }
        } else if (strcmp(argv[0], "stat") == 0) {
            if (argc < 2) {
                console_writeline("Usage: stat PATH");
            } else {
                command_stat(argv[1]);
            }
        } else if (strcmp(argv[0], "touch") == 0) {
            char absolute[FS_PATH_MAX];
            if (argc < 2) {
                console_writeline("Usage: touch FILE");
            } else {
                resolve_path(argv[1], absolute);
                if (!fs_write(absolute, (const uint8_t *)"", 0, ARM_FS_TEXT)) {
                    console_writeline("File creation failed.");
                }
            }
        } else if (strcmp(argv[0], "new") == 0) {
            if (argc < 2) {
                console_writeline("Usage: new FILE");
            } else {
                command_new(argv[1]);
            }
        } else if (strcmp(argv[0], "write") == 0) {
            char text[SHELL_INPUT_MAX];
            if (argc < 3) {
                console_writeline("Usage: write FILE TEXT");
            } else {
                shell_join_args(2, argc, argv, text, sizeof(text));
                command_write(argv[1], text);
            }
        } else if (strcmp(argv[0], "append") == 0) {
            char text[SHELL_INPUT_MAX];
            if (argc < 3) {
                console_writeline("Usage: append FILE TEXT");
            } else {
                shell_join_args(2, argc, argv, text, sizeof(text));
                command_append(argv[1], text);
            }
        } else if (strcmp(argv[0], "mkdir") == 0) {
            char absolute[FS_PATH_MAX];
            if (argc < 2) {
                console_writeline("Usage: mkdir DIR");
            } else {
                resolve_path(argv[1], absolute);
                if (!fs_mkdir(absolute)) {
                    console_writeline("Directory creation failed.");
                }
            }
        } else if (strcmp(argv[0], "rm") == 0) {
            char absolute[FS_PATH_MAX];
            if (argc < 2) {
                console_writeline("Usage: rm PATH");
            } else {
                resolve_path(argv[1], absolute);
                if (!fs_remove(absolute)) {
                    console_writeline("Remove failed.");
                }
            }
        } else if (strcmp(argv[0], "cp") == 0) {
            char src[FS_PATH_MAX];
            char dst[FS_PATH_MAX];
            if (argc < 3) {
                console_writeline("Usage: cp SRC DST");
            } else {
                resolve_path(argv[1], src);
                resolve_path(argv[2], dst);
                if (!fs_copy(src, dst)) {
                    console_writeline("Copy failed.");
                }
            }
        } else if (strcmp(argv[0], "mv") == 0) {
            char src[FS_PATH_MAX];
            char dst[FS_PATH_MAX];
            if (argc < 3) {
                console_writeline("Usage: mv SRC DST");
            } else {
                resolve_path(argv[1], src);
                resolve_path(argv[2], dst);
                if (!fs_move(src, dst)) {
                    console_writeline("Move failed.");
                }
            }
        } else if (strcmp(argv[0], "launcher") == 0 || strcmp(argv[0], "menu") == 0) {
            if (allow_desktop_return) {
                return SHELL_RESULT_DESKTOP;
            }
            app_launcher();
            console_clear();
        } else if (strcmp(argv[0], "welcome") == 0) {
            view_text("/home/WELCOME.TXT");
        } else if (strcmp(argv[0], "quickstart") == 0) {
            view_text("/docs/QUICKSTART.TXT");
        } else if (strcmp(argv[0], "settings") == 0) {
            app_settings_console();
        } else if (strcmp(argv[0], "system") == 0) {
            app_system_console();
        } else if (strcmp(argv[0], "installer") == 0) {
            app_installer_console();
        } else if (strcmp(argv[0], "disks") == 0 || strcmp(argv[0], "storage") == 0) {
            if (g_gfx.available) {
                ui_run_disk_app();
                console_clear();
            } else {
                app_disk_console();
            }
        } else if (strcmp(argv[0], "files") == 0) {
            app_files();
            console_clear();
        } else if (strcmp(argv[0], "notes") == 0) {
            app_edit("/home/NOTES.TXT");
        } else if (strcmp(argv[0], "journal") == 0) {
            app_edit("/home/JOURNAL.TXT");
        } else if (strcmp(argv[0], "calc") == 0) {
            app_calc(argc, argv);
        } else if (strcmp(argv[0], "snake") == 0 || strcmp(argv[0], "game") == 0) {
            if (g_gfx.available) {
                ui_run_snake_app();
                console_clear();
            } else {
                view_text("/docs/SNAKE.TXT");
            }
        } else if (strcmp(argv[0], "edit") == 0) {
            char absolute[FS_PATH_MAX];
            if (argc < 2) {
                console_writeline("Usage: edit FILE");
            } else {
                resolve_path(argv[1], absolute);
                app_edit(absolute);
            }
        } else if (strcmp(argv[0], "guide") == 0) {
            view_text("/docs/INSTALL.TXT");
        } else if (strcmp(argv[0], "about") == 0) {
            command_about();
        } else if (strcmp(argv[0], "reboot") == 0) {
            console_writeline("Exit SNSX and use the VirtualBox reset button to reboot.");
            return SHELL_RESULT_FIRMWARE;
        } else {
            console_writeline("Unknown command. Type help.");
        }
    }
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    shell_result_t result;

    g_image_handle = image_handle;
    g_st = system_table;
    g_desktop_request_firmware = false;
    if (g_st != NULL && g_st->boot_services != NULL && g_st->boot_services->set_watchdog_timer != NULL) {
        g_st->boot_services->set_watchdog_timer(0, 0, 0, NULL);
    }
    firmware_connect_all_controllers();
    input_init();
    fs_seed();
    memset(&g_gfx, 0, sizeof(g_gfx));
    memset(&g_pointer, 0, sizeof(g_pointer));
    if (g_st->con_out != NULL) {
        if (g_st->con_out->reset != NULL) {
            g_st->con_out->reset(g_st->con_out, 1);
        }
        if (g_st->con_out->enable_cursor != NULL) {
            g_st->con_out->enable_cursor(g_st->con_out, 0);
        }
        console_set_color(EFI_WHITE, EFI_BLACK);
        console_clear();
    }
    firmware_connect_all_controllers();
    input_init();
    gfx_init();
    pointer_init();
    netstack_reset();
    network_init();
    persist_discover();
    desktop_load_config();
    wireless_profiles_load();
    store_load_state();
    browser_state_load();
    studio_seed_workspace();
    studio_state_load();
    network_refresh(g_desktop.network_autoconnect);
    show_boot_dashboard();
    boot_run_input_diagnostics();
    if (!g_desktop.welcome_seen) {
        ui_run_setup_app();
    }
    while (1) {
        app_launcher();
        if (g_desktop_request_firmware) {
            return EFI_SUCCESS;
        }
        desktop_note_launch("TERMINAL", "Opened the SNSX terminal.");
        console_clear();
        result = shell_run(true);
        if (result == SHELL_RESULT_FIRMWARE) {
            return EFI_SUCCESS;
        }
    }
    return EFI_SUCCESS;
}
