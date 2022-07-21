/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 Software Radio Systems Limited
 *
 * \section LICENSE
 *
 * This file is part of the srsUE library.
 *
 * srsUE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsUE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#ifndef NAS_H
#define NAS_H

#include "srslte/common/buffer_pool.h"
#include "srslte/common/log.h"
#include "srslte/common/common.h"
#include "srslte/interfaces/ue_interfaces.h"
#include "srslte/common/security.h"
#include "srslte/asn1/liblte_mme.h"
#include "srslte/common/nas_pcap.h"

using srslte::byte_buffer_t;

namespace srsue {

// EMM states (3GPP 24.302 v10.0.0)
typedef enum {
  EMM_STATE_NULL = 0,
  EMM_STATE_DEREGISTERED,
  EMM_STATE_REGISTERED_INITIATED,
  EMM_STATE_REGISTERED,
  EMM_STATE_SERVICE_REQUEST_INITIATED,
  EMM_STATE_DEREGISTERED_INITIATED,
  EMM_STATE_TAU_INITIATED,
  EMM_STATE_N_ITEMS,
} emm_state_t;
static const char emm_state_text[EMM_STATE_N_ITEMS][100] = {"NULL",
                                                            "DEREGISTERED",
                                                            "REGISTERED INITIATED",
                                                            "REGISTERED",
                                                            "SERVICE REQUEST INITIATED",
                                                            "DEREGISTERED INITIATED",
                                                            "TRACKING AREA UPDATE INITIATED"};

static const bool eia_caps[8] = {false, true, true, false, false, false, false, false};
static const bool eea_caps[8] = {true,  true, true, false, false, false, false, false};

typedef enum {
  PLMN_NOT_SELECTED = 0,
  PLMN_SELECTED
} plmn_selection_state_t;

class nas
  : public nas_interface_rrc,
    public nas_interface_ue,
    public nas_interface_gw
{
public:
  nas();
  void init(usim_interface_nas  *usim_,
            rrc_interface_nas   *rrc_,
            gw_interface_nas    *gw_,
            srslte::log         *nas_log_,
            srslte::srslte_nas_config_t cfg_);
  void stop();

  emm_state_t get_state();

  // RRC interface
  void notify_connection_setup();
  void write_pdu(uint32_t lcid, byte_buffer_t *pdu);
  uint32_t get_ul_count();
  bool is_attached();
  bool is_attaching();
  bool get_s_tmsi(LIBLTE_RRC_S_TMSI_STRUCT *s_tmsi);
  bool get_k_asme(uint8_t *k_asme_, uint32_t n);
  void plmn_found(LIBLTE_RRC_PLMN_IDENTITY_STRUCT plmn_id, uint16_t tracking_area_code);
  void plmn_search_end();

  // UE interface
  void attach_request();
  void deattach_request();

  // PCAP
  void start_pcap(srslte::nas_pcap *pcap_);

private:
  srslte::byte_buffer_pool *pool;
  srslte::log *nas_log;
  rrc_interface_nas *rrc;
  usim_interface_nas *usim;
  gw_interface_nas *gw;

  srslte::srslte_nas_config_t cfg;

  emm_state_t state;

  plmn_selection_state_t plmn_selection;
  LIBLTE_RRC_PLMN_IDENTITY_STRUCT current_plmn;
  LIBLTE_RRC_PLMN_IDENTITY_STRUCT selecting_plmn;
  LIBLTE_RRC_PLMN_IDENTITY_STRUCT home_plmn;

  std::vector<LIBLTE_RRC_PLMN_IDENTITY_STRUCT > known_plmns;

  LIBLTE_MME_EMM_INFORMATION_MSG_STRUCT emm_info;

  // Security context
  struct nas_sec_ctxt{
    uint8_t  ksi;
    uint8_t  k_asme[32];
    uint32_t tx_count;
    uint32_t rx_count;
    srslte::CIPHERING_ALGORITHM_ID_ENUM  cipher_algo;
    srslte::INTEGRITY_ALGORITHM_ID_ENUM  integ_algo;
    LIBLTE_MME_EPS_MOBILE_ID_GUTI_STRUCT guti;
  };

  bool have_guti;
  bool have_ctxt;
  nas_sec_ctxt ctxt;

  uint32_t ip_addr;
  uint8_t eps_bearer_id;

  uint8_t transaction_id;

  // Security
  uint8_t k_nas_enc[32];
  uint8_t k_nas_int[32];

  // PCAP
  srslte::nas_pcap *pcap = NULL;

  void integrity_generate(uint8_t *key_128,
                          uint32_t count,
                          uint8_t direction,
                          uint8_t *msg,
                          uint32_t msg_len,
                          uint8_t *mac);
  bool integrity_check(byte_buffer_t *pdu);
  void cipher_encrypt(byte_buffer_t *pdu);
  void cipher_decrypt(byte_buffer_t *pdu);

  bool check_cap_replay(LIBLTE_MME_UE_SECURITY_CAPABILITIES_STRUCT *caps);

  // Parsers
  void parse_attach_accept(uint32_t lcid, byte_buffer_t *pdu);
  void parse_attach_reject(uint32_t lcid, byte_buffer_t *pdu);
  void parse_authentication_request(uint32_t lcid, byte_buffer_t *pdu);
  void parse_authentication_reject(uint32_t lcid, byte_buffer_t *pdu);
  void parse_identity_request(uint32_t lcid, byte_buffer_t *pdu);
  void parse_security_mode_command(uint32_t lcid, byte_buffer_t *pdu);
  void parse_service_reject(uint32_t lcid, byte_buffer_t *pdu);
  void parse_esm_information_request(uint32_t lcid, byte_buffer_t *pdu);
  void parse_emm_information(uint32_t lcid, byte_buffer_t *pdu);

  // Senders
  void send_attach_request();
  void send_identity_response();
  void send_service_request();
  void send_esm_information_response();
  void gen_pdn_connectivity_request(LIBLTE_BYTE_MSG_STRUCT *msg);
  void send_security_mode_reject(uint8_t cause);

  // security context persistence file
  bool read_ctxt_file(nas_sec_ctxt *ctxt);
  bool write_ctxt_file(nas_sec_ctxt ctxt);

  // ctxt file helpers
  std::string hex_to_string(uint8_t *hex, int size);
  bool        string_to_hex(std::string hex_str, uint8_t *hex, uint32_t len);
  std::string emm_info_str(LIBLTE_MME_EMM_INFORMATION_MSG_STRUCT *info);

  template <class T>
  bool readvar(std::istream &file, const char *key, T *var)
  {
    std::string line;
    size_t len = strlen(key);
    std::getline(file, line);
    if(line.substr(0,len).compare(key)) {
      return false;
    }
    *var = (T)atoi(line.substr(len).c_str());
    return true;
  }

  bool readvar(std::istream &file, const char *key, uint8_t *var, int varlen)
  {
    std::string line;
    size_t len = strlen(key);
    std::getline(file, line);
    if(line.substr(0,len).compare(key)) {
      return false;
    }
    std::string tmp = line.substr(len);
    if(!string_to_hex(tmp, var, varlen)) {
      return false;
    }
    return true;
  }
};

} // namespace srsue


#endif // NAS_H
