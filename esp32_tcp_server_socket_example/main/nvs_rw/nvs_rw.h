/* Header file: NVS (Non-Volatile Storage */

#ifndef HEADER_MOD_NVS
#define HEADER_MOD_NVS

/* Defines - NVS test */
#define TEST_NVS_STRING                "testnvs"
#define KEY_TEST_NVS                   "test"

#endif

/* Prototypes */
void init_nvs(void);
esp_err_t store_string_nvs(char * pt_key, char * pt_string);
esp_err_t read_string_nvs(char * pt_key, char * pt_string, size_t tam_str);
esp_err_t test_nvs_write_and_read(void);
esp_err_t clean_NVS_partition(void);