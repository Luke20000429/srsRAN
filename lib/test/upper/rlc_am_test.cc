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

#include <iostream>
#include "srslte/common/log_filter.h"
#include "srslte/common/logger_stdout.h"
#include "srslte/upper/rlc_am.h"
#include <assert.h>
#define NBUFS 5

using namespace srsue;
using namespace srslte;

class mac_dummy_timers
    :public srslte::mac_interface_timers
{
public:
  srslte::timers::timer* timer_get(uint32_t timer_id)
  {
    return &t;
  }
  uint32_t timer_get_unique_id(){return 0;}
  void timer_release_id(uint32_t id){}

private:
  srslte::timers::timer t;
};

class rlc_am_tester
    :public pdcp_interface_rlc
    ,public rrc_interface_rlc
{
public:
  rlc_am_tester(){
    bzero(sdus, sizeof(sdus));
    n_sdus = 0;
  }

  // PDCP interface
  void write_pdu(uint32_t lcid, byte_buffer_t *sdu)
  {
    assert(lcid == 1);
    sdus[n_sdus++] = sdu;
  }
  void write_pdu_bcch_bch(byte_buffer_t *sdu) {}
  void write_pdu_bcch_dlsch(byte_buffer_t *sdu) {}
  void write_pdu_pcch(byte_buffer_t *sdu) {}
  
  // RRC interface
  void max_retx_attempted(){}
  std::string get_rb_name(uint32_t lcid) { return std::string(""); }

  byte_buffer_t *sdus[10];
  int n_sdus;
};

void basic_test()
{
  srslte::log_filter log1("RLC_AM_1");
  srslte::log_filter log2("RLC_AM_2");
  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);
  log1.set_hex_limit(-1);
  log2.set_hex_limit(-1);
  rlc_am_tester     tester;
  mac_dummy_timers  timers;

  rlc_am rlc1;
  rlc_am rlc2;

  int len;

  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);

  rlc1.init(&log1, 1, &tester, &tester, &timers);
  rlc2.init(&log2, 1, &tester, &tester, &timers);

  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  cnfg.rlc_mode = LIBLTE_RRC_RLC_MODE_AM;
  cnfg.dl_am_rlc.t_reordering = LIBLTE_RRC_T_REORDERING_MS5;
  cnfg.dl_am_rlc.t_status_prohibit = LIBLTE_RRC_T_STATUS_PROHIBIT_MS5;
  cnfg.ul_am_rlc.max_retx_thresh = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
  cnfg.ul_am_rlc.poll_byte = LIBLTE_RRC_POLL_BYTE_KB25;
  cnfg.ul_am_rlc.poll_pdu = LIBLTE_RRC_POLL_PDU_P4;
  cnfg.ul_am_rlc.t_poll_retx = LIBLTE_RRC_T_POLL_RETRANSMIT_MS5;

  rlc1.configure(&cnfg);
  rlc2.configure(&cnfg);

  // Push 5 SDUs into RLC1
  byte_buffer_t sdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    *sdu_bufs[i].msg    = i; // Write the index into the buffer
    sdu_bufs[i].N_bytes = 1; // Give each buffer a size of 1 byte
    rlc1.write_sdu(&sdu_bufs[i]);
  }

  assert(14 == rlc1.get_buffer_state());

  // Read 5 PDUs from RLC1 (1 byte each)
  byte_buffer_t pdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    len = rlc1.read_pdu(pdu_bufs[i].msg, 4); // 3 bytes for header + payload
    pdu_bufs[i].N_bytes = len;
  }

  assert(0 == rlc1.get_buffer_state());

  // Write 5 PDUs into RLC2
  for(int i=0;i<NBUFS;i++)
  {
    rlc2.write_pdu(pdu_bufs[i].msg, pdu_bufs[i].N_bytes);
  }

  assert(2 == rlc2.get_buffer_state());

  // Read status PDU from RLC2
  byte_buffer_t status_buf;
  len = rlc2.read_pdu(status_buf.msg, 2);
  status_buf.N_bytes = len;

  assert(0 == rlc2.get_buffer_state());

  // Write status PDU to RLC1
  rlc1.write_pdu(status_buf.msg, status_buf.N_bytes);

  for(int i=0; i<tester.n_sdus; i++)
  {
    assert(tester.sdus[i]->N_bytes == 1);
    assert(*(tester.sdus[i]->msg)  == i);
  }
}

void concat_test()
{
  srslte::log_filter log1("RLC_AM_1");
  srslte::log_filter log2("RLC_AM_2");
  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);
  log1.set_hex_limit(-1);
  log2.set_hex_limit(-1);
  rlc_am_tester     tester;
  mac_dummy_timers  timers;

  rlc_am rlc1;
  rlc_am rlc2;

  int len;

  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);

  rlc1.init(&log1, 1, &tester, &tester, &timers);
  rlc2.init(&log2, 1, &tester, &tester, &timers);

  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  cnfg.rlc_mode = LIBLTE_RRC_RLC_MODE_AM;
  cnfg.dl_am_rlc.t_reordering = LIBLTE_RRC_T_REORDERING_MS5;
  cnfg.dl_am_rlc.t_status_prohibit = LIBLTE_RRC_T_STATUS_PROHIBIT_MS5;
  cnfg.ul_am_rlc.max_retx_thresh = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
  cnfg.ul_am_rlc.poll_byte = LIBLTE_RRC_POLL_BYTE_KB25;
  cnfg.ul_am_rlc.poll_pdu = LIBLTE_RRC_POLL_PDU_P4;
  cnfg.ul_am_rlc.t_poll_retx = LIBLTE_RRC_T_POLL_RETRANSMIT_MS5;

  rlc1.configure(&cnfg);
  rlc2.configure(&cnfg);

  // Push 5 SDUs into RLC1
  byte_buffer_t sdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    *sdu_bufs[i].msg    = i; // Write the index into the buffer
    sdu_bufs[i].N_bytes = 1; // Give each buffer a size of 1 byte
    rlc1.write_sdu(&sdu_bufs[i]);
  }

  assert(14 == rlc1.get_buffer_state());

  // Read 1 PDUs from RLC1 containing all 5 SDUs
  byte_buffer_t pdu_buf;
  len = rlc1.read_pdu(pdu_buf.msg, 13); // 8 bytes for header + payload
  pdu_buf.N_bytes = len;

  assert(0 == rlc1.get_buffer_state());

  // Write PDU into RLC2
  rlc2.write_pdu(pdu_buf.msg, pdu_buf.N_bytes);

  // No status report as we haven't crossed polling thresholds

  assert(tester.n_sdus == 5);
  for(int i=0; i<tester.n_sdus; i++)
  {
    assert(tester.sdus[i]->N_bytes == 1);
    assert(*(tester.sdus[i]->msg)  == i);
  }
}

void segment_test()
{
  srslte::log_filter log1("RLC_AM_1");
  srslte::log_filter log2("RLC_AM_2");
  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);
  log1.set_hex_limit(-1);
  log2.set_hex_limit(-1);
  rlc_am_tester     tester;
  mac_dummy_timers  timers;

  rlc_am rlc1;
  rlc_am rlc2;

  int len;

  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);

  rlc1.init(&log1, 1, &tester, &tester, &timers);
  rlc2.init(&log2, 1, &tester, &tester, &timers);

  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  cnfg.rlc_mode = LIBLTE_RRC_RLC_MODE_AM;
  cnfg.dl_am_rlc.t_reordering = LIBLTE_RRC_T_REORDERING_MS5;
  cnfg.dl_am_rlc.t_status_prohibit = LIBLTE_RRC_T_STATUS_PROHIBIT_MS5;
  cnfg.ul_am_rlc.max_retx_thresh = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
  cnfg.ul_am_rlc.poll_byte = LIBLTE_RRC_POLL_BYTE_KB25;
  cnfg.ul_am_rlc.poll_pdu = LIBLTE_RRC_POLL_PDU_P4;
  cnfg.ul_am_rlc.t_poll_retx = LIBLTE_RRC_T_POLL_RETRANSMIT_MS5;

  rlc1.configure(&cnfg);
  rlc2.configure(&cnfg);

  // Push 5 SDUs into RLC1
  byte_buffer_t sdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    for(int j=0;j<10;j++)
      sdu_bufs[i].msg[j] = j;
    sdu_bufs[i].N_bytes = 10; // Give each buffer a size of 10 bytes
    rlc1.write_sdu(&sdu_bufs[i]);
  }

  assert(59 == rlc1.get_buffer_state());

  // Read PDUs from RLC1 (force segmentation)
  byte_buffer_t pdu_bufs[20];
  int n_pdus = 0;
  while(rlc1.get_buffer_state() > 0){
    len = rlc1.read_pdu(pdu_bufs[n_pdus].msg, 10); // 2 header + payload
    pdu_bufs[n_pdus++].N_bytes = len;
  }

  assert(0 == rlc1.get_buffer_state());

  // Write PDUs into RLC2
  for(int i=0;i<n_pdus;i++)
  {
    rlc2.write_pdu(pdu_bufs[i].msg, pdu_bufs[i].N_bytes);
  }

  assert(2 == rlc2.get_buffer_state());

  // Read status PDU from RLC2
  byte_buffer_t status_buf;
  len = rlc2.read_pdu(status_buf.msg, 10); // 10 bytes is enough to hold the status
  status_buf.N_bytes = len;

  assert(0 == rlc2.get_buffer_state());

  // Write status PDU to RLC1
  rlc1.write_pdu(status_buf.msg, status_buf.N_bytes);

  assert(tester.n_sdus == 5);
  for(int i=0; i<tester.n_sdus; i++)
  {
    assert(tester.sdus[i]->N_bytes == 10);
    for(int j=0;j<10;j++)
      assert(tester.sdus[i]->msg[j]  == j);
  }
}

void retx_test()
{
  srslte::log_filter log1("RLC_AM_1");
  srslte::log_filter log2("RLC_AM_2");
  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);
  log1.set_hex_limit(-1);
  log2.set_hex_limit(-1);
  rlc_am_tester     tester;
  mac_dummy_timers  timers;

  rlc_am rlc1;
  rlc_am rlc2;

  int len;

  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);

  rlc1.init(&log1, 1, &tester, &tester, &timers);
  rlc2.init(&log2, 1, &tester, &tester, &timers);

  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  cnfg.rlc_mode = LIBLTE_RRC_RLC_MODE_AM;
  cnfg.dl_am_rlc.t_reordering = LIBLTE_RRC_T_REORDERING_MS5;
  cnfg.dl_am_rlc.t_status_prohibit = LIBLTE_RRC_T_STATUS_PROHIBIT_MS5;
  cnfg.ul_am_rlc.max_retx_thresh = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
  cnfg.ul_am_rlc.poll_byte = LIBLTE_RRC_POLL_BYTE_KB25;
  cnfg.ul_am_rlc.poll_pdu = LIBLTE_RRC_POLL_PDU_P4;
  cnfg.ul_am_rlc.t_poll_retx = LIBLTE_RRC_T_POLL_RETRANSMIT_MS5;

  rlc1.configure(&cnfg);
  rlc2.configure(&cnfg);

  // Push 5 SDUs into RLC1
  byte_buffer_t sdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    *sdu_bufs[i].msg    = i; // Write the index into the buffer
    sdu_bufs[i].N_bytes = 1; // Give each buffer a size of 1 byte
    rlc1.write_sdu(&sdu_bufs[i]);
  }

  assert(14 == rlc1.get_buffer_state());

  // Read 5 PDUs from RLC1 (1 byte each)
  byte_buffer_t pdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    len = rlc1.read_pdu(pdu_bufs[i].msg, 4); // 2 byte header + 1 byte payload
    pdu_bufs[i].N_bytes = len;
  }

  assert(0 == rlc1.get_buffer_state());

  // Write PDUs into RLC2 (skip SN 1)
  for(int i=0;i<NBUFS;i++)
  {
    if(i != 1)
      rlc2.write_pdu(pdu_bufs[i].msg, pdu_bufs[i].N_bytes);
  }

  // Sleep to let reordering timeout expire
  usleep(10000);

  assert(4 == rlc2.get_buffer_state());

  // Read status PDU from RLC2
  byte_buffer_t status_buf;
  len = rlc2.read_pdu(status_buf.msg, 10); // 10 bytes is enough to hold the status
  status_buf.N_bytes = len;

  // Write status PDU to RLC1
  rlc1.write_pdu(status_buf.msg, status_buf.N_bytes);

  assert(3 == rlc1.get_buffer_state()); // 2 byte header + 1 byte payload

  // Read the retx PDU from RLC1
  byte_buffer_t retx;
  len = rlc1.read_pdu(retx.msg, 3); // 2 byte header + 1 byte payload
  retx.N_bytes = len;

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx.msg, retx.N_bytes);

  assert(tester.n_sdus == 5);
  for(int i=0; i<tester.n_sdus; i++)
  {
    assert(tester.sdus[i]->N_bytes == 1);
    assert(*(tester.sdus[i]->msg)  == i);
  }
}

void resegment_test_1()
{
  // SDUs:                |  10  |  10  |  10  |  10  |  10  |
  // PDUs:                |  10  |  10  |  10  |  10  |  10  |
  // Retx PDU segments:                 | 5 | 5|

  srslte::log_filter log1("RLC_AM_1");
  srslte::log_filter log2("RLC_AM_2");
  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);
  log1.set_hex_limit(-1);
  log2.set_hex_limit(-1);
  rlc_am_tester     tester;
  mac_dummy_timers  timers;

  rlc_am rlc1;
  rlc_am rlc2;

  int len;

  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);

  rlc1.init(&log1, 1, &tester, &tester, &timers);
  rlc2.init(&log2, 1, &tester, &tester, &timers);

  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  cnfg.rlc_mode = LIBLTE_RRC_RLC_MODE_AM;
  cnfg.dl_am_rlc.t_reordering = LIBLTE_RRC_T_REORDERING_MS5;
  cnfg.dl_am_rlc.t_status_prohibit = LIBLTE_RRC_T_STATUS_PROHIBIT_MS5;
  cnfg.ul_am_rlc.max_retx_thresh = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
  cnfg.ul_am_rlc.poll_byte = LIBLTE_RRC_POLL_BYTE_KB25;
  cnfg.ul_am_rlc.poll_pdu = LIBLTE_RRC_POLL_PDU_P4;
  cnfg.ul_am_rlc.t_poll_retx = LIBLTE_RRC_T_POLL_RETRANSMIT_MS5;

  rlc1.configure(&cnfg);
  rlc2.configure(&cnfg);

  // Push 5 SDUs into RLC1
  byte_buffer_t sdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    for(int j=0;j<10;j++)
      sdu_bufs[i].msg[j] = j;
    sdu_bufs[i].N_bytes = 10; // Give each buffer a size of 10 bytes
    rlc1.write_sdu(&sdu_bufs[i]);
  }

  assert(59 == rlc1.get_buffer_state());

  // Read 5 PDUs from RLC1 (10 bytes each)
  byte_buffer_t pdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    len = rlc1.read_pdu(pdu_bufs[i].msg, 12); // 12 bytes for header + payload
    pdu_bufs[i].N_bytes = len;
  }

  assert(0 == rlc1.get_buffer_state());

  // Write PDUs into RLC2 (skip SN 1)
  for(int i=0;i<NBUFS;i++)
  {
    if(i != 1)
      rlc2.write_pdu(pdu_bufs[i].msg, pdu_bufs[i].N_bytes);
  }

  // Sleep to let reordering timeout expire
  usleep(10000);

  assert(4 == rlc2.get_buffer_state());

  // Read status PDU from RLC2
  byte_buffer_t status_buf;
  len = rlc2.read_pdu(status_buf.msg, 10); // 10 bytes is enough to hold the status
  status_buf.N_bytes = len;

  // Write status PDU to RLC1
  rlc1.write_pdu(status_buf.msg, status_buf.N_bytes);

  assert(12 == rlc1.get_buffer_state()); // 2 byte header + 10 data

  // Read the retx PDU from RLC1 and force resegmentation
  byte_buffer_t retx1;
  len = rlc1.read_pdu(retx1.msg, 11); // 4 byte header + 5 data
  retx1.N_bytes = len;

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx1.msg, retx1.N_bytes);

  assert(9 == rlc1.get_buffer_state()); // 4 byte header + 5 data

  // Read the remaining segment
  byte_buffer_t retx2;
  len = rlc1.read_pdu(retx2.msg, 11); // 4 byte header + 5 data
  retx2.N_bytes = len;

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx2.msg, retx2.N_bytes);

  assert(tester.n_sdus == 5);
  for(int i=0; i<tester.n_sdus; i++)
  {
    assert(tester.sdus[i]->N_bytes == 10);
    for(int j=0;j<10;j++)
      assert(tester.sdus[i]->msg[j]  == j);
  }
}

void resegment_test_2()
{

  // SDUs:              |  10  |  10  |  10  |  10  |  10  |
  // PDUs:              | 5 |  10  |     20     |  10  | 5 |
  // Retx PDU segments:            |  10  |  10 |

  srslte::log_filter log1("RLC_AM_1");
  srslte::log_filter log2("RLC_AM_2");
  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);
  log1.set_hex_limit(-1);
  log2.set_hex_limit(-1);
  rlc_am_tester     tester;
  mac_dummy_timers  timers;

  rlc_am rlc1;
  rlc_am rlc2;

  int len;

  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);

  rlc1.init(&log1, 1, &tester, &tester, &timers);
  rlc2.init(&log2, 1, &tester, &tester, &timers);

  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  cnfg.rlc_mode = LIBLTE_RRC_RLC_MODE_AM;
  cnfg.dl_am_rlc.t_reordering = LIBLTE_RRC_T_REORDERING_MS5;
  cnfg.dl_am_rlc.t_status_prohibit = LIBLTE_RRC_T_STATUS_PROHIBIT_MS5;
  cnfg.ul_am_rlc.max_retx_thresh = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
  cnfg.ul_am_rlc.poll_byte = LIBLTE_RRC_POLL_BYTE_KB25;
  cnfg.ul_am_rlc.poll_pdu = LIBLTE_RRC_POLL_PDU_P4;
  cnfg.ul_am_rlc.t_poll_retx = LIBLTE_RRC_T_POLL_RETRANSMIT_MS5;

  rlc1.configure(&cnfg);
  rlc2.configure(&cnfg);

  // Push 5 SDUs into RLC1
  byte_buffer_t sdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    for(int j=0;j<10;j++)
      sdu_bufs[i].msg[j] = j;
    sdu_bufs[i].N_bytes = 10; // Give each buffer a size of 10 bytes
    rlc1.write_sdu(&sdu_bufs[i]);
  }

  assert(59 == rlc1.get_buffer_state());

  // Read 5 PDUs from RLC1 (5 bytes, 10 bytes, 20 bytes, 10 bytes, 5 bytes)
  byte_buffer_t pdu_bufs[NBUFS];
  pdu_bufs[0].N_bytes = rlc1.read_pdu(pdu_bufs[0].msg, 7);  // 2 byte header +  5 byte payload
  pdu_bufs[1].N_bytes = rlc1.read_pdu(pdu_bufs[1].msg, 14); // 4 byte header + 10 byte payload
  pdu_bufs[2].N_bytes = rlc1.read_pdu(pdu_bufs[2].msg, 25); // 5 byte header + 20 byte payload
  pdu_bufs[3].N_bytes = rlc1.read_pdu(pdu_bufs[3].msg, 14); // 4 byte header + 10 byte payload
  pdu_bufs[4].N_bytes = rlc1.read_pdu(pdu_bufs[4].msg, 7);  // 2 byte header +  5 byte payload

  assert(0 == rlc1.get_buffer_state());

  // Write PDUs into RLC2 (skip SN 2)
  for(int i=0;i<NBUFS;i++)
  {
    if(i != 2)
      rlc2.write_pdu(pdu_bufs[i].msg, pdu_bufs[i].N_bytes);
  }

  // Sleep to let reordering timeout expire
  usleep(10000);

  assert(4 == rlc2.get_buffer_state());

  // Read status PDU from RLC2
  byte_buffer_t status_buf;
  status_buf.N_bytes = rlc2.read_pdu(status_buf.msg, 10); // 10 bytes is enough to hold the status

  // Write status PDU to RLC1
  rlc1.write_pdu(status_buf.msg, status_buf.N_bytes);

  assert(25 == rlc1.get_buffer_state()); // 4 byte header + 20 data

  // Read the retx PDU from RLC1 and force resegmentation
  byte_buffer_t retx1;
  retx1.N_bytes = rlc1.read_pdu(retx1.msg, 18); // 6 byte header + 10 data

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx1.msg, retx1.N_bytes);

  assert(16 == rlc1.get_buffer_state()); // 6 byte header + 10 data

  // Read the remaining segment
  byte_buffer_t retx2;
  retx2.N_bytes = rlc1.read_pdu(retx2.msg, 18); // 6 byte header + 10 data

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx2.msg, retx2.N_bytes);

  assert(tester.n_sdus == 5);
  for(int i=0; i<tester.n_sdus; i++)
  {
    assert(tester.sdus[i]->N_bytes == 10);
    for(int j=0;j<10;j++)
      assert(tester.sdus[i]->msg[j]  == j);
  }
}

void resegment_test_3()
{

  // SDUs:              |  10  |  10  |  10  |  10  |  10  |
  // PDUs:              | 5 | 5|      20     |  10  |  10  |
  // Retx PDU segments:        |  10  |  10  |

  srslte::log_filter log1("RLC_AM_1");
  srslte::log_filter log2("RLC_AM_2");
  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);
  log1.set_hex_limit(-1);
  log2.set_hex_limit(-1);
  rlc_am_tester     tester;
  mac_dummy_timers  timers;

  rlc_am rlc1;
  rlc_am rlc2;

  int len;

  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);

  rlc1.init(&log1, 1, &tester, &tester, &timers);
  rlc2.init(&log2, 1, &tester, &tester, &timers);

  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  cnfg.rlc_mode = LIBLTE_RRC_RLC_MODE_AM;
  cnfg.dl_am_rlc.t_reordering = LIBLTE_RRC_T_REORDERING_MS5;
  cnfg.dl_am_rlc.t_status_prohibit = LIBLTE_RRC_T_STATUS_PROHIBIT_MS5;
  cnfg.ul_am_rlc.max_retx_thresh = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
  cnfg.ul_am_rlc.poll_byte = LIBLTE_RRC_POLL_BYTE_KB25;
  cnfg.ul_am_rlc.poll_pdu = LIBLTE_RRC_POLL_PDU_P4;
  cnfg.ul_am_rlc.t_poll_retx = LIBLTE_RRC_T_POLL_RETRANSMIT_MS5;

  rlc1.configure(&cnfg);
  rlc2.configure(&cnfg);

  // Push 5 SDUs into RLC1
  byte_buffer_t sdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    for(int j=0;j<10;j++)
      sdu_bufs[i].msg[j] = j;
    sdu_bufs[i].N_bytes = 10; // Give each buffer a size of 10 bytes
    rlc1.write_sdu(&sdu_bufs[i]);
  }

  assert(59 == rlc1.get_buffer_state());

  // Read 5 PDUs from RLC1 (5 bytes, 5 bytes, 20 bytes, 10 bytes, 10 bytes)
  byte_buffer_t pdu_bufs[NBUFS];
  pdu_bufs[0].N_bytes = rlc1.read_pdu(pdu_bufs[0].msg, 7);  // 2 byte header +  5 byte payload
  pdu_bufs[1].N_bytes = rlc1.read_pdu(pdu_bufs[1].msg, 7);  // 2 byte header +  5 byte payload
  pdu_bufs[2].N_bytes = rlc1.read_pdu(pdu_bufs[2].msg, 24); // 4 byte header + 20 byte payload
  pdu_bufs[3].N_bytes = rlc1.read_pdu(pdu_bufs[3].msg, 12); // 2 byte header + 10 byte payload
  pdu_bufs[4].N_bytes = rlc1.read_pdu(pdu_bufs[4].msg, 12); // 2 byte header + 10 byte payload

  assert(0 == rlc1.get_buffer_state());

  // Write PDUs into RLC2 (skip SN 2)
  for(int i=0;i<NBUFS;i++)
  {
    if(i != 2)
      rlc2.write_pdu(pdu_bufs[i].msg, pdu_bufs[i].N_bytes);
  }

  // Sleep to let reordering timeout expire
  usleep(10000);

  assert(4 == rlc2.get_buffer_state());

  // Read status PDU from RLC2
  byte_buffer_t status_buf;
  status_buf.N_bytes = rlc2.read_pdu(status_buf.msg, 10); // 10 bytes is enough to hold the status

  // Write status PDU to RLC1
  rlc1.write_pdu(status_buf.msg, status_buf.N_bytes);

  // Read the retx PDU from RLC1 and force resegmentation
  byte_buffer_t retx1;
  retx1.N_bytes = rlc1.read_pdu(retx1.msg, 16); // 4 byte header + 10 data

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx1.msg, retx1.N_bytes);

  // Read the remaining segment
  byte_buffer_t retx2;
  retx2.N_bytes = rlc1.read_pdu(retx2.msg, 16); // 4 byte header + 10 data

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx2.msg, retx2.N_bytes);

  assert(tester.n_sdus == 5);
  for(int i=0; i<tester.n_sdus; i++)
  {
    assert(tester.sdus[i]->N_bytes == 10);
    for(int j=0;j<10;j++)
      assert(tester.sdus[i]->msg[j]  == j);
  }
}

void resegment_test_4()
{

  // SDUs:              |  10  |  10  |  10  |  10  |  10  |
  // PDUs:              | 5 | 5|         30         | 5 | 5|
  // Retx PDU segments:        |    15    |    15   |

  srslte::log_filter log1("RLC_AM_1");
  srslte::log_filter log2("RLC_AM_2");
  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);
  log1.set_hex_limit(-1);
  log2.set_hex_limit(-1);
  rlc_am_tester     tester;
  mac_dummy_timers  timers;

  rlc_am rlc1;
  rlc_am rlc2;

  int len;

  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);

  rlc1.init(&log1, 1, &tester, &tester, &timers);
  rlc2.init(&log2, 1, &tester, &tester, &timers);

  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  cnfg.rlc_mode = LIBLTE_RRC_RLC_MODE_AM;
  cnfg.dl_am_rlc.t_reordering = LIBLTE_RRC_T_REORDERING_MS5;
  cnfg.dl_am_rlc.t_status_prohibit = LIBLTE_RRC_T_STATUS_PROHIBIT_MS5;
  cnfg.ul_am_rlc.max_retx_thresh = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
  cnfg.ul_am_rlc.poll_byte = LIBLTE_RRC_POLL_BYTE_KB25;
  cnfg.ul_am_rlc.poll_pdu = LIBLTE_RRC_POLL_PDU_P4;
  cnfg.ul_am_rlc.t_poll_retx = LIBLTE_RRC_T_POLL_RETRANSMIT_MS5;

  rlc1.configure(&cnfg);
  rlc2.configure(&cnfg);

  // Push 5 SDUs into RLC1
  byte_buffer_t sdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    for(int j=0;j<10;j++)
      sdu_bufs[i].msg[j] = j;
    sdu_bufs[i].N_bytes = 10; // Give each buffer a size of 10 bytes
    rlc1.write_sdu(&sdu_bufs[i]);
  }

  assert(59 == rlc1.get_buffer_state());

  // Read 5 PDUs from RLC1 (5 bytes, 5 bytes, 30 bytes, 5 bytes, 5 bytes)
  byte_buffer_t pdu_bufs[NBUFS];
  pdu_bufs[0].N_bytes = rlc1.read_pdu(pdu_bufs[0].msg, 7);  // 2 byte header +  5 byte payload
  pdu_bufs[1].N_bytes = rlc1.read_pdu(pdu_bufs[1].msg, 7);  // 2 byte header +  5 byte payload
  pdu_bufs[2].N_bytes = rlc1.read_pdu(pdu_bufs[2].msg, 35); // 5 byte header + 30 byte payload
  pdu_bufs[3].N_bytes = rlc1.read_pdu(pdu_bufs[3].msg, 7);  // 2 byte header +  5 byte payload
  pdu_bufs[4].N_bytes = rlc1.read_pdu(pdu_bufs[4].msg, 7);  // 2 byte header +  5 byte payload

  assert(0 == rlc1.get_buffer_state());

  // Write PDUs into RLC2 (skip SN 2)
  for(int i=0;i<NBUFS;i++)
  {
    if(i != 2)
      rlc2.write_pdu(pdu_bufs[i].msg, pdu_bufs[i].N_bytes);
  }

  // Sleep to let reordering timeout expire
  usleep(10000);

  assert(4 == rlc2.get_buffer_state());

  // Read status PDU from RLC2
  byte_buffer_t status_buf;
  status_buf.N_bytes = rlc2.read_pdu(status_buf.msg, 10); // 10 bytes is enough to hold the status

  // Write status PDU to RLC1
  rlc1.write_pdu(status_buf.msg, status_buf.N_bytes);

  // Read the retx PDU from RLC1 and force resegmentation
  byte_buffer_t retx1;
  retx1.N_bytes = rlc1.read_pdu(retx1.msg, 23); // 6 byte header + 15 data

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx1.msg, retx1.N_bytes);

  // Read the remaining segment
  byte_buffer_t retx2;
  retx2.N_bytes = rlc1.read_pdu(retx2.msg, 23); // 6 byte header + 15 data

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx2.msg, retx2.N_bytes);

  assert(tester.n_sdus == 5);
  for(int i=0; i<tester.n_sdus; i++)
  {
    assert(tester.sdus[i]->N_bytes == 10);
    for(int j=0;j<10;j++)
      assert(tester.sdus[i]->msg[j]  == j);
  }
}

void resegment_test_5()
{

  // SDUs:              |  10  |  10  |  10  |  10  |  10  |
  // PDUs:              |2|3|            40            |3|2|
  // Retx PDU segments:     |     20      |     20     |

  srslte::log_filter log1("RLC_AM_1");
  srslte::log_filter log2("RLC_AM_2");
  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);
  log1.set_hex_limit(-1);
  log2.set_hex_limit(-1);
  rlc_am_tester     tester;
  mac_dummy_timers  timers;

  rlc_am rlc1;
  rlc_am rlc2;

  int len;

  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);

  rlc1.init(&log1, 1, &tester, &tester, &timers);
  rlc2.init(&log2, 1, &tester, &tester, &timers);

  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  cnfg.rlc_mode = LIBLTE_RRC_RLC_MODE_AM;
  cnfg.dl_am_rlc.t_reordering = LIBLTE_RRC_T_REORDERING_MS5;
  cnfg.dl_am_rlc.t_status_prohibit = LIBLTE_RRC_T_STATUS_PROHIBIT_MS5;
  cnfg.ul_am_rlc.max_retx_thresh = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
  cnfg.ul_am_rlc.poll_byte = LIBLTE_RRC_POLL_BYTE_KB25;
  cnfg.ul_am_rlc.poll_pdu = LIBLTE_RRC_POLL_PDU_P4;
  cnfg.ul_am_rlc.t_poll_retx = LIBLTE_RRC_T_POLL_RETRANSMIT_MS5;

  rlc1.configure(&cnfg);
  rlc2.configure(&cnfg);

  // Push 5 SDUs into RLC1
  byte_buffer_t sdu_bufs[NBUFS];
  for(int i=0;i<NBUFS;i++)
  {
    for(int j=0;j<10;j++)
      sdu_bufs[i].msg[j] = j;
    sdu_bufs[i].N_bytes = 10; // Give each buffer a size of 10 bytes
    rlc1.write_sdu(&sdu_bufs[i]);
  }

  assert(59 == rlc1.get_buffer_state());

  // Read 5 PDUs from RLC1 (2 bytes, 3 bytes, 40 bytes, 3 bytes, 2 bytes)
  byte_buffer_t pdu_bufs[NBUFS];
  pdu_bufs[0].N_bytes = rlc1.read_pdu(pdu_bufs[0].msg, 4);  // 2 byte header +  2 byte payload
  pdu_bufs[1].N_bytes = rlc1.read_pdu(pdu_bufs[1].msg, 5);  // 2 byte header +  3 byte payload
  pdu_bufs[2].N_bytes = rlc1.read_pdu(pdu_bufs[2].msg, 48); // 8 byte header + 40 byte payload
  pdu_bufs[3].N_bytes = rlc1.read_pdu(pdu_bufs[3].msg, 5);  // 2 byte header +  3 byte payload
  pdu_bufs[4].N_bytes = rlc1.read_pdu(pdu_bufs[4].msg, 4);  // 2 byte header +  2 byte payload

  assert(0 == rlc1.get_buffer_state());

  // Write PDUs into RLC2 (skip SN 2)
  for(int i=0;i<NBUFS;i++)
  {
    if(i != 2)
      rlc2.write_pdu(pdu_bufs[i].msg, pdu_bufs[i].N_bytes);
  }

  // Sleep to let reordering timeout expire
  usleep(10000);

  assert(4 == rlc2.get_buffer_state());

  // Read status PDU from RLC2
  byte_buffer_t status_buf;
  status_buf.N_bytes = rlc2.read_pdu(status_buf.msg, 10); // 10 bytes is enough to hold the status

  // Write status PDU to RLC1
  rlc1.write_pdu(status_buf.msg, status_buf.N_bytes);

  // Read the retx PDU from RLC1 and force resegmentation
  byte_buffer_t retx1;
  retx1.N_bytes = rlc1.read_pdu(retx1.msg, 29); // 7 byte header + 20 data

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx1.msg, retx1.N_bytes);

  // Read the remaining segment
  byte_buffer_t retx2;
  retx2.N_bytes = rlc1.read_pdu(retx2.msg, 29); // 7 byte header + 20 data

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx2.msg, retx2.N_bytes);

  assert(tester.n_sdus == 5);
  for(int i=0; i<tester.n_sdus; i++)
  {
    assert(tester.sdus[i]->N_bytes == 10);
    for(int j=0;j<10;j++)
      assert(tester.sdus[i]->msg[j]  == j);
  }
}

void resegment_test_6()
{
  // SDUs:                |10|10|10|  54  |  54  |  54  |  54  |  54  | 54 |
  // PDUs:                |10|10|10|                270               | 54 |
  // Retx PDU segments:            |  120           |      150        |

  srslte::log_filter log1("RLC_AM_1");
  srslte::log_filter log2("RLC_AM_2");
  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);
  log1.set_hex_limit(-1);
  log2.set_hex_limit(-1);
  rlc_am_tester     tester;
  mac_dummy_timers  timers;

  rlc_am rlc1;
  rlc_am rlc2;

  int len;

  log1.set_level(srslte::LOG_LEVEL_DEBUG);
  log2.set_level(srslte::LOG_LEVEL_DEBUG);

  rlc1.init(&log1, 1, &tester, &tester, &timers);
  rlc2.init(&log2, 1, &tester, &tester, &timers);

  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  cnfg.rlc_mode = LIBLTE_RRC_RLC_MODE_AM;
  cnfg.dl_am_rlc.t_reordering = LIBLTE_RRC_T_REORDERING_MS5;
  cnfg.dl_am_rlc.t_status_prohibit = LIBLTE_RRC_T_STATUS_PROHIBIT_MS5;
  cnfg.ul_am_rlc.max_retx_thresh = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
  cnfg.ul_am_rlc.poll_byte = LIBLTE_RRC_POLL_BYTE_KB25;
  cnfg.ul_am_rlc.poll_pdu = LIBLTE_RRC_POLL_PDU_P4;
  cnfg.ul_am_rlc.t_poll_retx = LIBLTE_RRC_T_POLL_RETRANSMIT_MS5;

  rlc1.configure(&cnfg);
  rlc2.configure(&cnfg);

  // Push SDUs into RLC1
  byte_buffer_t sdu_bufs[9];
  for(int i=0;i<3;i++)
  {
    for(int j=0;j<10;j++)
      sdu_bufs[i].msg[j] = j;
    sdu_bufs[i].N_bytes = 10;
    rlc1.write_sdu(&sdu_bufs[i]);
  }
  for(int i=3;i<9;i++)
  {
    for(int j=0;j<54;j++)
      sdu_bufs[i].msg[j] = j;
    sdu_bufs[i].N_bytes = 54;
    rlc1.write_sdu(&sdu_bufs[i]);
  }

  assert(369 == rlc1.get_buffer_state());

  // Read PDUs from RLC1 (10, 10, 10, 270, 54)
  byte_buffer_t pdu_bufs[5];
  for(int i=0;i<3;i++) {
    len = rlc1.read_pdu(pdu_bufs[i].msg, 12);
    pdu_bufs[i].N_bytes = len;
  }
  len = rlc1.read_pdu(pdu_bufs[3].msg, 278);
  pdu_bufs[3].N_bytes = len;
  len = rlc1.read_pdu(pdu_bufs[4].msg, 56);
  pdu_bufs[4].N_bytes = len;

  assert(0 == rlc1.get_buffer_state());

  // Write PDUs into RLC2 (skip SN 3)
  for(int i=0;i<5;i++)
  {
    if(i != 3)
      rlc2.write_pdu(pdu_bufs[i].msg, pdu_bufs[i].N_bytes);
  }

  // Sleep to let reordering timeout expire
  usleep(10000);

  assert(4 == rlc2.get_buffer_state());

  // Read status PDU from RLC2
  byte_buffer_t status_buf;
  len = rlc2.read_pdu(status_buf.msg, 10); // 10 bytes is enough to hold the status
  status_buf.N_bytes = len;

  // Write status PDU to RLC1
  rlc1.write_pdu(status_buf.msg, status_buf.N_bytes);

  assert(278 == rlc1.get_buffer_state());

  // Read the retx PDU from RLC1 and force resegmentation
  byte_buffer_t retx1;
  len = rlc1.read_pdu(retx1.msg, 129);
  retx1.N_bytes = len;

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx1.msg, retx1.N_bytes);

  assert(157 == rlc1.get_buffer_state());

  // Read the remaining segment
  byte_buffer_t retx2;
  len = rlc1.read_pdu(retx2.msg, 159);
  retx2.N_bytes = len;

  // Write the retx PDU to RLC2
  rlc2.write_pdu(retx2.msg, retx2.N_bytes);

  assert(tester.n_sdus == 9);
  for(int i=0;i<3;i++)
  {
    assert(tester.sdus[i]->N_bytes == 10);
    for(int j=0;j<10;j++)
      assert(tester.sdus[i]->msg[j]  == j);
  }
  for(int i=3;i<9;i++)
  {
    assert(tester.sdus[i]->N_bytes == 54);
    for(int j=0;j<54;j++)
      assert(tester.sdus[i]->msg[j]  == j);
  }
}

int main(int argc, char **argv) {
  basic_test();
  byte_buffer_pool::get_instance()->cleanup();
  concat_test();
  byte_buffer_pool::get_instance()->cleanup();
  segment_test();
  byte_buffer_pool::get_instance()->cleanup();
  retx_test();
  byte_buffer_pool::get_instance()->cleanup();
  resegment_test_1();
  byte_buffer_pool::get_instance()->cleanup();
  resegment_test_2();
  byte_buffer_pool::get_instance()->cleanup();
  resegment_test_3();
  byte_buffer_pool::get_instance()->cleanup();
  resegment_test_4();
  byte_buffer_pool::get_instance()->cleanup();
  resegment_test_5();
  byte_buffer_pool::get_instance()->cleanup();
  resegment_test_6();
}
