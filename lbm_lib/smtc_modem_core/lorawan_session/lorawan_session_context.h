/**
 * @file      lorawan_session_context.h
 *
 * @brief     LoRaWAN session context persistence definitions
 *
 * @remark    Universal Firmware - ESP32 IoT Device Project
 *            Session persistence extension for LoRa Basic Modem
 */

#ifndef LORAWAN_SESSION_CONTEXT_H
#define LORAWAN_SESSION_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "lr1mac_defs.h"

/**
 * @brief LoRaWAN session context structure for persistence
 * @remark This structure contains critical session parameters that need
 *         to be preserved across device resets/deep sleep cycles
 */
typedef struct {
    // Header and version
    uint32_t magic;                          // Magic number for validation (0x4C424D53 = "LBMS")
    uint32_t version;                        // Structure version for future compatibility
    
    // Device Identity & Addressing
    uint32_t dev_addr;                       // Device address assigned during join
    lr1mac_activation_mode_t activation_mode; // OTAA or ABP mode
    
    // Security Counters (Critical for preventing replay attacks)
    uint32_t fcnt_up;                        // Uplink frame counter
    uint32_t fcnt_dwn;                       // Downlink frame counter
    
    // MAC Command Parameters (Negotiated via ADR and MAC commands)
    uint8_t tx_data_rate;                    // Current transmission data rate
    uint8_t tx_data_rate_adr;                // Data rate set by ADR
    int8_t  tx_power;                        // TX power level
    uint8_t nb_trans;                        // Number of transmissions
    uint8_t nb_available_tx_channel;         // Available TX channels
    
    // RX Parameter Setup (From RxParamSetupReq)
    uint8_t  rx2_data_rate;                  // RX2 window data rate
    uint32_t rx2_frequency;                  // RX2 window frequency
    uint8_t  rx1_dr_offset;                  // RX1 data rate offset
    uint8_t  rx1_delay_s;                    // RX1 delay in seconds
    
    // Regional & Power Parameters
    uint8_t  max_erp_dbm;                    // Maximum ERP from TxParamSetup
    uint32_t max_duty_cycle_index;           // Duty cycle limitations
    
    // ADR Configuration
    int      adr_ack_cnt;                    // ADR acknowledgment counter
    uint8_t  adr_ack_delay;                  // ADR acknowledgment delay
    uint8_t  adr_ack_limit;                  // ADR acknowledgment limit
    bool     adr_enable;                     // ADR enable status
    
    // Join and Network Information
    join_status_t join_status;               // Current join status
    uint16_t dev_nonce;                      // Device nonce (last used)
    uint8_t  join_nonce[6];                  // Join nonce + NetID from last join
    
    // Network Time
    uint32_t seconds_since_epoch;            // Network time seconds
    uint32_t fractional_second;              // Network time fractional part
    
    // Class B (optional)
    uint32_t beacon_freq_hz;                 // Beacon frequency
    uint32_t ping_slot_freq_hz;              // Ping slot frequency
    uint8_t  ping_slot_dr;                   // Ping slot data rate
    uint8_t  ping_slot_periodicity_ans;      // Acknowledged ping slot periodicity
    
    // Validation and Metadata
    uint32_t last_save_timestamp;            // Timestamp of last save (for aging)
    uint32_t save_counter;                   // Number of times context has been saved
    uint32_t crc32;                          // CRC32 checksum for integrity validation
} lorawan_session_context_t;

/**
 * @brief Magic number for LoRaWAN session context validation
 */
#define LORAWAN_SESSION_MAGIC 0x4C424D53  // "LBMS" - LoRa Basic Modem Session

/**
 * @brief Current version of the session context structure
 */
#define LORAWAN_SESSION_VERSION 1

/**
 * @brief Maximum age of session context in seconds (24 hours)
 * @remark After this time, the session context should be considered stale
 *         and a rejoin procedure should be initiated
 */
#define LORAWAN_SESSION_MAX_AGE_SECONDS (24 * 60 * 60)

/**
 * @brief Validation result for session context
 */
typedef enum {
    LORAWAN_SESSION_VALID,        // Context is valid and can be used
    LORAWAN_SESSION_INVALID_CRC,  // CRC validation failed
    LORAWAN_SESSION_INVALID_MAGIC,// Magic number validation failed
    LORAWAN_SESSION_TOO_OLD,      // Context is too old (stale)
    LORAWAN_SESSION_WRONG_VERSION,// Version mismatch
    LORAWAN_SESSION_NOT_JOINED,   // Device not in joined state
} lorawan_session_validation_t;

/**
 * @brief Validate a LoRaWAN session context
 *
 * @param [in] ctx Pointer to session context structure
 * @param [in] current_time Current system time in seconds
 * @return Validation result
 */
lorawan_session_validation_t lorawan_session_validate_context(const lorawan_session_context_t* ctx, uint32_t current_time);

/**
 * @brief Calculate CRC32 checksum for session context
 *
 * @param [in] ctx Pointer to session context structure
 * @return CRC32 checksum
 */
uint32_t lorawan_session_calculate_crc(const lorawan_session_context_t* ctx);

/**
 * @brief Initialize session context with default values
 *
 * @param [out] ctx Pointer to session context structure to initialize
 */
void lorawan_session_init_context(lorawan_session_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // LORAWAN_SESSION_CONTEXT_H