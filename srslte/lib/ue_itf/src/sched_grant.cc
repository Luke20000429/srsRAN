/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2014 The srsLTE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the srsLTE library.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <string.h>
#include <strings.h>
#include <pthread.h>
#include "srslte/srslte.h"

#include "srslte/ue_itf/sched_grant.h"
    
namespace srslte {
namespace ue {
  
  sched_grant::sched_grant(direction_t direction, uint16_t rnti_)
  {
    rnti = rnti_; 
    dir = direction; 
  }
  
  /* Returns the RNTI associated with the UL/DL scheduling grant */
  uint16_t sched_grant::get_rnti() {
    return rnti; 
  }

  uint32_t sched_grant::get_rv() {
    if (dir == UPLINK) {
      return ul_grant.rv_idx;
    } else {
      return dl_grant.rv_idx;    
    }
  }
  
  uint32_t sched_grant::get_tbs() {
    if (dir == UPLINK) {
      return ul_grant.mcs.tbs;
    } else {
      return dl_grant.mcs.tbs;    
    }
  }
  
  uint32_t sched_grant::get_ncce()
  {
    return ncce; 
  }
  
  bool srslte::ue::sched_grant::is_sps_release()
  {
    return false; 
  }
  
  void sched_grant::set_ncce(uint32_t ncce_)
  {
    ncce = ncce_;
  }

  uint32_t sched_grant::get_current_tx_nb()
  {
    return current_tx_nb; 
  }

  void sched_grant::set_current_tx_nb(uint32_t current_tx_nb_)
  {
    current_tx_nb = current_tx_nb_; 
  }


  void sched_grant::set_rv(uint32_t rv) {
    if (dir == UPLINK) {
      ul_grant.rv_idx = rv;
    } else {
      dl_grant.rv_idx = rv;    
    }
  }

  bool sched_grant::get_ndi() {
    if (dir == UPLINK) {
      return ul_grant.ndi;
    } else {
      return dl_grant.ndi;    
    }
  }

  void sched_grant::set_ndi(bool value) {
    if (dir == UPLINK) {
      ul_grant.ndi = value;
    } else {
      dl_grant.ndi = value;    
    }
  }

  bool sched_grant::get_cqi_request() {
    if (dir == UPLINK) {
      return ul_grant.ndi;
    } else {
      return dl_grant.ndi;    
    }  
  }

  int sched_grant::get_harq_process() {
    if (dir == UPLINK) {
      return -1;
    } else {
      return dl_grant.harq_process;    
    }
  }

  bool sched_grant::is_uplink() {
    return dir == UPLINK;
  }

  bool sched_grant::is_downlink() {
    return dir == DOWNLINK;
  }
  void* sched_grant::get_grant_ptr() {
    if (is_uplink()) {
      return (void*) &ul_grant;
    } else {
      return (void*) &dl_grant;    
    }
  }
} // namespace ue
} // namespace srslte

