/******************************************************************************
 *
 *  Copyright 1999-2013 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <bluetooth/log.h>

#include "gatt_api.h"
#include "osi/include/allocator.h"
#include "osi/include/osi.h"
#include "srvc_dis_int.h"
#include "srvc_eng_int.h"
#include "stack/include/bt_uuid16.h"
#include "types/bluetooth/uuid.h"
#include "types/raw_address.h"

using base::StringPrintf;
using namespace bluetooth;

static void srvc_eng_s_request_cback(tCONN_ID conn_id, uint32_t trans_id, tGATTS_REQ_TYPE type,
                                     tGATTS_DATA* p_data);
static void srvc_eng_connect_cback(tGATT_IF /* gatt_if */, const RawAddress& bda, tCONN_ID conn_id,
                                   bool connected, tGATT_DISCONN_REASON reason,
                                   tBT_TRANSPORT transport);
static void srvc_eng_c_cmpl_cback(tCONN_ID conn_id, tGATTC_OPTYPE op, tGATT_STATUS status,
                                  tGATT_CL_COMPLETE* p_data);

static tGATT_CBACK srvc_gatt_cback = {
        .p_conn_cb = srvc_eng_connect_cback,
        .p_cmpl_cb = srvc_eng_c_cmpl_cback,
        .p_disc_res_cb = nullptr,
        .p_disc_cmpl_cb = nullptr,
        .p_req_cb = srvc_eng_s_request_cback,
        .p_enc_cmpl_cb = nullptr,
        .p_congestion_cb = nullptr,
        .p_phy_update_cb = nullptr,
        .p_conn_update_cb = nullptr,
        .p_subrate_chg_cb = nullptr,
};

/* type for action functions */
typedef void (*tSRVC_ENG_C_CMPL_ACTION)(tSRVC_CLCB* p_clcb, tGATTC_OPTYPE op, tGATT_STATUS status,
                                        tGATT_CL_COMPLETE* p_data);

static const tSRVC_ENG_C_CMPL_ACTION srvc_eng_c_cmpl_act[SRVC_ID_MAX] = {
        dis_c_cmpl_cback,
};

tSRVC_ENG_CB srvc_eng_cb;

/*******************************************************************************
 *
 * Function         srvc_eng_find_clcb_by_bd_addr
 *
 * Description      The function searches all LCBs with macthing bd address.
 *
 * Returns          Pointer to the found link conenction control block.
 *
 ******************************************************************************/
static tSRVC_CLCB* srvc_eng_find_clcb_by_bd_addr(const RawAddress& bda) {
  uint8_t i_clcb;
  tSRVC_CLCB* p_clcb = NULL;

  for (i_clcb = 0, p_clcb = srvc_eng_cb.clcb; i_clcb < SRVC_MAX_APPS; i_clcb++, p_clcb++) {
    if (p_clcb->in_use && p_clcb->connected && p_clcb->bda == bda) {
      return p_clcb;
    }
  }

  return NULL;
}
/*******************************************************************************
 *
 * Function         srvc_eng_find_clcb_by_conn_id
 *
 * Description      The function searches all LCBs with macthing connection ID.
 *
 * Returns          Pointer to the found link conenction control block.
 *
 ******************************************************************************/
tSRVC_CLCB* srvc_eng_find_clcb_by_conn_id(tCONN_ID conn_id) {
  uint8_t i_clcb;
  tSRVC_CLCB* p_clcb = NULL;

  for (i_clcb = 0, p_clcb = srvc_eng_cb.clcb; i_clcb < SRVC_MAX_APPS; i_clcb++, p_clcb++) {
    if (p_clcb->in_use && p_clcb->connected && p_clcb->conn_id == conn_id) {
      return p_clcb;
    }
  }

  return NULL;
}
/*******************************************************************************
 *
 * Function         srvc_eng_find_clcb_by_conn_id
 *
 * Description      The function searches all LCBs with macthing connection ID.
 *
 * Returns          Pointer to the found link conenction control block.
 *
 ******************************************************************************/
static uint8_t srvc_eng_find_clcb_idx_by_conn_id(tCONN_ID conn_id) {
  uint8_t i_clcb;
  tSRVC_CLCB* p_clcb = NULL;

  for (i_clcb = 0, p_clcb = srvc_eng_cb.clcb; i_clcb < SRVC_MAX_APPS; i_clcb++, p_clcb++) {
    if (p_clcb->in_use && p_clcb->connected && p_clcb->conn_id == conn_id) {
      return i_clcb;
    }
  }

  return SRVC_MAX_APPS;
}
/*******************************************************************************
 *
 * Function         srvc_eng_clcb_alloc
 *
 * Description      Allocate a GATT profile connection link control block
 *
 * Returns          NULL if not found. Otherwise pointer to the connection link
 *                  block.
 *
 ******************************************************************************/
static tSRVC_CLCB* srvc_eng_clcb_alloc(tCONN_ID conn_id, const RawAddress& bda) {
  uint8_t i_clcb = 0;
  tSRVC_CLCB* p_clcb = NULL;

  for (i_clcb = 0, p_clcb = srvc_eng_cb.clcb; i_clcb < SRVC_MAX_APPS; i_clcb++, p_clcb++) {
    if (!p_clcb->in_use) {
      p_clcb->in_use = true;
      p_clcb->conn_id = conn_id;
      p_clcb->connected = true;
      p_clcb->bda = bda;
      break;
    }
  }
  return p_clcb;
}
/*******************************************************************************
 *
 * Function         srvc_eng_clcb_dealloc
 *
 * Description      De-allocate a GATT profile connection link control block
 *
 * Returns          True the deallocation is successful
 *
 ******************************************************************************/
static bool srvc_eng_clcb_dealloc(tCONN_ID conn_id) {
  uint8_t i_clcb = 0;
  tSRVC_CLCB* p_clcb = NULL;

  for (i_clcb = 0, p_clcb = srvc_eng_cb.clcb; i_clcb < SRVC_MAX_APPS; i_clcb++, p_clcb++) {
    if (p_clcb->in_use && p_clcb->connected && (p_clcb->conn_id == conn_id)) {
      unsigned j;
      for (j = 0; j < ARRAY_SIZE(p_clcb->dis_value.data_string); j++) {
        osi_free(p_clcb->dis_value.data_string[j]);
      }

      memset(p_clcb, 0, sizeof(tSRVC_CLCB));
      return true;
    }
  }
  return false;
}
/*******************************************************************************
 *   Service Engine Server Attributes Database Read/Read Blob Request process
 ******************************************************************************/
static uint8_t srvc_eng_process_read_req(uint8_t clcb_idx, tGATT_READ_REQ* p_data,
                                         tGATTS_RSP* p_rsp, tGATT_STATUS* p_status) {
  tGATT_STATUS status = GATT_NOT_FOUND;
  uint8_t act = SRVC_ACT_RSP;

  if (p_data->is_long) {
    p_rsp->attr_value.offset = p_data->offset;
  }

  p_rsp->attr_value.handle = p_data->handle;

  if (dis_valid_handle_range(p_data->handle)) {
    act = dis_read_attr_value(clcb_idx, p_data->handle, &p_rsp->attr_value, p_data->is_long,
                              p_status);
  } else {
    *p_status = status;
  }
  return act;
}
/*******************************************************************************
 *   Service Engine Server Attributes Database write Request process
 ******************************************************************************/
static uint8_t srvc_eng_process_write_req(uint8_t /* clcb_idx */, tGATT_WRITE_REQ* p_data,
                                          tGATTS_RSP* /* p_rsp */, tGATT_STATUS* p_status) {
  uint8_t act = SRVC_ACT_RSP;

  if (dis_valid_handle_range(p_data->handle)) {
    act = dis_write_attr_value(p_data, p_status);
  } else {
    *p_status = GATT_NOT_FOUND;
  }

  return act;
}

/*******************************************************************************
 *
 * Function         srvc_eng_s_request_cback
 *
 * Description      GATT DIS attribute access request callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void srvc_eng_s_request_cback(tCONN_ID conn_id, uint32_t trans_id, tGATTS_REQ_TYPE type,
                                     tGATTS_DATA* p_data) {
  tGATT_STATUS status = GATT_INVALID_PDU;
  tGATTS_RSP rsp_msg;
  uint8_t act = SRVC_ACT_IGNORE;
  uint8_t clcb_idx = srvc_eng_find_clcb_idx_by_conn_id(conn_id);
  if (clcb_idx == SRVC_MAX_APPS) {
    log::error("Can't find clcb, id:{}", conn_id);
    return;
  }

  log::verbose("srvc_eng_s_request_cback : recv type (0x{:02x})", type);

  memset(&rsp_msg, 0, sizeof(tGATTS_RSP));

  srvc_eng_cb.clcb[clcb_idx].trans_id = trans_id;

  switch (type) {
    case GATTS_REQ_TYPE_READ_CHARACTERISTIC:
    case GATTS_REQ_TYPE_READ_DESCRIPTOR:
      act = srvc_eng_process_read_req(clcb_idx, &p_data->read_req, &rsp_msg, &status);
      break;

    case GATTS_REQ_TYPE_WRITE_CHARACTERISTIC:
    case GATTS_REQ_TYPE_WRITE_DESCRIPTOR:
      act = srvc_eng_process_write_req(clcb_idx, &p_data->write_req, &rsp_msg, &status);
      if (!p_data->write_req.need_rsp) {
        act = SRVC_ACT_IGNORE;
      }
      break;

    case GATTS_REQ_TYPE_WRITE_EXEC:
      log::verbose("Ignore GATT_REQ_EXEC_WRITE/WRITE_CMD");
      break;

    case GATTS_REQ_TYPE_MTU:
      log::verbose("Get MTU exchange new mtu size: {}", p_data->mtu);
      break;

    default:
      log::verbose("Unknown/unexpected LE GAP ATT request: 0x{:02x}", type);
      break;
  }

  srvc_eng_cb.clcb[clcb_idx].trans_id = 0;

  if (act == SRVC_ACT_RSP) {
    if (GATTS_SendRsp(conn_id, trans_id, status, &rsp_msg) != GATT_SUCCESS) {
      log::warn("Unable to send GATT server respond conn_id:{}", conn_id);
    }
  }
}

/*******************************************************************************
 *
 * Function         srvc_eng_c_cmpl_cback
 *
 * Description      Client operation complete callback.
 *
 * Returns          void
 *
 ******************************************************************************/
static void srvc_eng_c_cmpl_cback(tCONN_ID conn_id, tGATTC_OPTYPE op, tGATT_STATUS status,
                                  tGATT_CL_COMPLETE* p_data) {
  tSRVC_CLCB* p_clcb = srvc_eng_find_clcb_by_conn_id(conn_id);

  log::verbose("srvc_eng_c_cmpl_cback() - op_code: 0x{:02x}  status: 0x{:02x}", op, status);

  if (p_clcb == NULL) {
    log::error("received for unknown connection");
    return;
  }

  if (p_clcb->cur_srvc_id != SRVC_ID_NONE && p_clcb->cur_srvc_id <= SRVC_ID_MAX) {
    srvc_eng_c_cmpl_act[p_clcb->cur_srvc_id - 1](p_clcb, op, status, p_data);
  }
}

/*******************************************************************************
 *
 * Function         srvc_eng_connect_cback
 *
 * Description      Gatt profile connection callback.
 *
 * Returns          void
 *
 ******************************************************************************/
static void srvc_eng_connect_cback(tGATT_IF /* gatt_if */, const RawAddress& bda, tCONN_ID conn_id,
                                   bool connected, tGATT_DISCONN_REASON /* reason */,
                                   tBT_TRANSPORT /* transport */) {
  log::verbose("from {} connected:{} conn_id={}", bda, connected, conn_id);

  if (connected) {
    if (srvc_eng_clcb_alloc(conn_id, bda) == NULL) {
      log::error("srvc_eng_connect_cback: no_resource");
      return;
    }
  } else {
    srvc_eng_clcb_dealloc(conn_id);
  }
}
/*******************************************************************************
 *
 * Function         srvc_eng_c_cmpl_cback
 *
 * Description      Client operation complete callback.
 *
 * Returns          void
 *
 ******************************************************************************/
bool srvc_eng_request_channel(const RawAddress& remote_bda, uint8_t srvc_id) {
  bool set = true;
  tSRVC_CLCB* p_clcb = srvc_eng_find_clcb_by_bd_addr(remote_bda);

  if (p_clcb == NULL) {
    p_clcb = srvc_eng_clcb_alloc(0, remote_bda);
  }

  if (p_clcb && p_clcb->cur_srvc_id == SRVC_ID_NONE) {
    p_clcb->cur_srvc_id = srvc_id;
  } else {
    set = false;
  }

  return set;
}
/*******************************************************************************
 *
 * Function         srvc_eng_release_channel
 *
 * Description      Client operation complete callback.
 *
 * Returns          void
 *
 ******************************************************************************/
void srvc_eng_release_channel(tCONN_ID conn_id) {
  tSRVC_CLCB* p_clcb = srvc_eng_find_clcb_by_conn_id(conn_id);

  if (p_clcb == NULL) {
    log::error("invalid connection id {}", conn_id);
    return;
  }

  p_clcb->cur_srvc_id = SRVC_ID_NONE;

  /* check pending request */
  if (GATT_Disconnect(p_clcb->conn_id) != GATT_SUCCESS) {
    log::warn("Unable to disconnect GATT conn_id:{}", p_clcb->conn_id);
  }
}
/*******************************************************************************
 *
 * Function         srvc_eng_init
 *
 * Description      Initialize the GATT Service engine.
 *
 ******************************************************************************/
tGATT_STATUS srvc_eng_init(void) {
  if (srvc_eng_cb.enabled) {
    log::error("DIS already initialized");
  } else {
    memset(&srvc_eng_cb, 0, sizeof(tSRVC_ENG_CB));

    /* Create a GATT profile service */
    bluetooth::Uuid app_uuid = bluetooth::Uuid::From16Bit(UUID_SERVCLASS_DEVICE_INFO);
    srvc_eng_cb.gatt_if = GATT_Register(app_uuid, "GattServiceEngine", &srvc_gatt_cback, false);
    GATT_StartIf(srvc_eng_cb.gatt_if);

    log::verbose("Srvc_Init:  gatt_if={}", srvc_eng_cb.gatt_if);

    srvc_eng_cb.enabled = true;
    dis_cb.dis_read_uuid_idx = 0xff;
  }
  return GATT_SUCCESS;
}
