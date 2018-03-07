/**************************************************************************/
/*!
    @file     cdc_device_app.c
    @author   hathach (tinyusb.org)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2013, hathach (tinyusb.org)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    This file is part of the tinyusb stack.
*/
/**************************************************************************/

#include "cdc_device_app.h"

#if TUSB_CFG_DEVICE_CDC

#include "common/fifo.h" // TODO refractor
#include "app_os_prio.h"

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
enum { SERIAL_BUFFER_SIZE = 64 };

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
static osal_semaphore_t sem_hdl;

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
TUSB_CFG_ATTR_USBRAM static uint8_t serial_rx_buffer[SERIAL_BUFFER_SIZE];
TUSB_CFG_ATTR_USBRAM static uint8_t serial_tx_buffer[SERIAL_BUFFER_SIZE];

FIFO_DEF(fifo_serial, SERIAL_BUFFER_SIZE, uint8_t, true);

//--------------------------------------------------------------------+
// tinyusb callbacks
//--------------------------------------------------------------------+
void cdc_serial_app_mount(uint8_t coreid)
{
  osal_semaphore_reset(sem_hdl);

  tud_cdc_receive(coreid, serial_rx_buffer, SERIAL_BUFFER_SIZE, true);
}

void cdc_serial_app_umount(uint8_t coreid)
{

}

void tud_cdc_rx_cb(uint8_t coreid, uint32_t xferred_bytes)
{
  fifo_write_n(&fifo_serial, serial_rx_buffer, xferred_bytes);
  osal_semaphore_post(sem_hdl);  // notify main task
}

//--------------------------------------------------------------------+
// APPLICATION CODE
//--------------------------------------------------------------------+
void cdc_serial_app_init(void)
{
  sem_hdl = osal_semaphore_create(1, 0);
  VERIFY(sem_hdl, );

  osal_task_create(cdc_serial_app_task, "cdc", 128, NULL, CDC_SERIAL_APP_TASK_PRIO);
}

tusb_error_t cdc_serial_subtask(void);

void cdc_serial_app_task(void* param)
{
  (void) param;

  OSAL_TASK_BEGIN
  cdc_serial_subtask();
  OSAL_TASK_END
}

tusb_error_t cdc_serial_subtask(void)
{
  OSAL_SUBTASK_BEGIN

  tusb_error_t error;
  (void) error; // suppress compiler's warnings

  osal_semaphore_wait(sem_hdl, OSAL_TIMEOUT_WAIT_FOREVER, &error);

  if ( tud_mounted(0) )
  {
    // echo back data in the fifo
    if ( !tud_cdc_busy(0, CDC_PIPE_DATA_IN) )
    {
      uint16_t count=0;
      while( fifo_read(&fifo_serial, &serial_tx_buffer[count]) )
      {
        count++;
      }

      if (count)
      {
        tud_cdc_send(0, serial_tx_buffer, count, false);
      }
    }

    // getting more data from host
    tud_cdc_receive(0, serial_rx_buffer, SERIAL_BUFFER_SIZE, true);
  }

  OSAL_SUBTASK_END
}

#endif
