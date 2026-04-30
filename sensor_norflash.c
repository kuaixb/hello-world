/* =====================================================================================
 *
 *  Copyright (C) 2020. Huami Ltd, unpublished work. This computer program includes
 *  Confidential, Proprietary Information and is a Trade Secret of Huami Ltd.
 *  All use, disclosure, and/or reproduction is prohibited unless authorized in writing.
 *  All Rights Reserved.
 *
 *  Author:  yangkunzhen@huami.com
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "mhs_hal.h"

static qspi_dev_t sens_qspidev;
static volatile bool _is_sensor_qspi_inited = false;

static void _sensor_qspi_dev_init(void)
{
    if (true == _is_sensor_qspi_inited)
    {
        return;
    }

    sens_qspidev.clock_mode = QSPI_CLOCK_MODE0;
    sens_qspidev.reg = SENS_QSPI0_BASE;
    hal_qspi_init(&sens_qspidev);
    _is_sensor_qspi_inited = true;
}

static void sensor_norflash_exit_dpd(void)
{
    _sensor_qspi_dev_init();
    qspi_command_t cmd;
    cmd.Address = 0x0;
    cmd.AddressMode = HAL_QSPI_ADDRESS_NONE;
    cmd.AddressSize = 0;
    cmd.DataMode = HAL_QSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.FlashId = 0;
    cmd.Instruction = 0xAB;
    cmd.InstructionMode = HAL_QSPI_INSTRUCTION_1_LINE;
    cmd.InstructionSize = 1;
    cmd.NbData = 0;
    cmd.OperationMode = HAL_QSPI_OPMODE_WRITE_CFG;
    hal_qspi_command(&sens_qspidev, &cmd, 1000);
}

static void sensor_norflash_enter_dpd(void)
{
    _sensor_qspi_dev_init();
    qspi_command_t cmd;
    cmd.Address = 0x0;
    cmd.AddressMode = HAL_QSPI_ADDRESS_NONE;
    cmd.AddressSize = 0;
    cmd.DataMode = HAL_QSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.FlashId = 0;
    cmd.Instruction = 0xB9;
    cmd.InstructionMode = HAL_QSPI_INSTRUCTION_1_LINE;
    cmd.InstructionSize = 1;
    cmd.NbData = 0;
    cmd.OperationMode = HAL_QSPI_OPMODE_WRITE_CFG;
    hal_qspi_command(&sens_qspidev, &cmd, 1000);
}

#if HMI_LCPU_ENABLE
void sensor_flash_init(void)
{
    sensor_norflash_exit_dpd();
    hal_mdelay(10);
}

void sensor_flash_shutdown(void)
{
    gpio_config_t config = {
        .mux = MUX_FUNC_3,
        .mode = HAL_GPIO_MODE_FUNCTION,
        .driving = HAL_GPIO_DRIVING_20MA,
    };
    config.pin = GPIO_PIN_18 | GPIO_PIN_19 | GPIO_PIN_20 | GPIO_PIN_21 | GPIO_PIN_22 | GPIO_PIN_23;
    hal_gpio_init(GPIOC, &config);
    __HAL_SCU_SENS_QSPI0_CLK_ENABLE();

    __HAL_SCU_SEN_QSPI_FORCE_RESET();

    sensor_norflash_enter_dpd();
    __HAL_PWR_SET_SENSQSPI_IO_RETENTION();
}
#endif

#if MCU_POWER_MGMT_ENABLE && APP_SENSORHUB

#include "power_mgmt.h"

static void senscore_norflash_io_resume(void)
{
    // gpio_config_t config = {
    //     .mux = MUX_FUNC_3,
    //     .mode = HAL_GPIO_MODE_FUNCTION,
    //     .driving = HAL_GPIO_DRIVING_20MA,
    // };
    // config.pin = GPIO_PIN_18 | GPIO_PIN_19 | GPIO_PIN_20 | GPIO_PIN_21 | GPIO_PIN_22 | GPIO_PIN_23;
    // hal_gpio_init(GPIOC, &config);
    #if 1
    // pc18-23 mux func register 0x660000A8
    uint32_t func_val = inw(AON_SYSC_BASE + 0xA8);
    func_val |= 0x33333300;
    outw(AON_SYSC_BASE + 0xA8, func_val);
    #endif

    __HAL_PWR_RELEASE_SENSQSPI_IO_RETENTION();
}

static void senscore_norflash_io_suspend(void)
{
    // gpio_config_t gpio = {
    //     .mode   = HAL_GPIO_MODE_FUNCTION,
    //     .mux    = MUX_FUNC_0,
    //     .pull   = HAL_GPIO_PULL_DOWN,
    // };
    // gpio.pin = GPIO_PIN_18 | GPIO_PIN_19 | GPIO_PIN_20 |
    //            GPIO_PIN_21 | GPIO_PIN_22;
    // hal_gpio_init(GPIOC, &gpio);
    // gpio.pull = HAL_GPIO_PULL_UP;
    // gpio.pin = GPIO_PIN_23;
    // hal_gpio_init(GPIOC, &gpio);
    #if 1
    // cs pullup?
    uint32_t pad_val = inw(AON_SYSC_BASE + 0x1AC);
    // set pullup PC23
    pad_val &= ~0xC0000;
    pad_val |= 0x40000;
    outw(AON_SYSC_BASE + 0x1AC, pad_val);

    // pc18-23 function配置为0
    uint32_t func_val = inw(AON_SYSC_BASE + 0xA8);
    func_val &= ~0xFFFFFF00;
    outw(AON_SYSC_BASE + 0xA8, func_val);

    // set pulldown PC18-19
    pad_val = inw(AON_SYSC_BASE + 0x1A4);
    pad_val &= ~0xC000C;
    pad_val |= 0x80008;
    outw(AON_SYSC_BASE + 0x1A4, pad_val);
    // set pulldown PC20-21
    pad_val = inw(AON_SYSC_BASE + 0x1A8);
    pad_val &= ~0xC000C;
    pad_val |= 0x80008;
    outw(AON_SYSC_BASE + 0x1A8, pad_val);
    // set pulldown PC22
    pad_val = inw(AON_SYSC_BASE + 0x1AC);
    pad_val &= ~0x0C;
    pad_val |= 0x08;
    outw(AON_SYSC_BASE + 0x1AC, pad_val);
    #endif

    __HAL_PWR_SET_SENSQSPI_IO_RETENTION();
}


void sensor_nor_test_dpd(void)
{
    uint32_t sr;
    osEnterCritical(&sr);
    sensor_norflash_enter_dpd();
    senscore_norflash_io_suspend();
    hal_qspi_suspend(&sens_qspidev);

    hal_mdelay(100);
    hal_qspi_resume(&sens_qspidev);
    senscore_norflash_io_resume();
    sensor_norflash_exit_dpd();
    hal_udelay(30);
    osExitCritical(sr);
}


static __RETAINED_SRAM void mhs_xip_power_evt_handler(mhs_power_evt_t const *p_power_evt, void *p_context)
{
    switch (p_power_evt->header.evt_id)
    {
        case MHS_POWER_EVT_ENTER_MODE:
        {
            if (MHS_POWER_OBS_MODE_DL == p_power_evt->header.evt_mode)
            {
                sensor_norflash_enter_dpd();
                senscore_norflash_io_suspend();
                hal_qspi_suspend((qspi_dev_t *)p_context);
            }
            break;
        }
        case MHS_POWER_EVT_EXIT_MODE:
        {
            if (MHS_POWER_OBS_MODE_DL == p_power_evt->header.evt_mode)
            {
                senscore_norflash_io_resume();
                hal_qspi_resume((qspi_dev_t *)p_context);
                sensor_norflash_exit_dpd();
                hal_udelay(30);
            }
        }
        break;
        case MHS_POWER_EVT_STANDBY:
        {
            sensor_norflash_enter_dpd();
            senscore_norflash_io_suspend();
            hal_qspi_suspend((qspi_dev_t *)p_context);
        }
        break;
        case MHS_POWER_EVT_DVFS:
        {
        }
        break;
        default:
        break;
    }
}

HAL_POWER_MGMT_OBSERVER(xip_observer, HAL_POWER_OBSERVER_PRIOR_XIP, mhs_xip_power_evt_handler, &sens_qspidev);

#endif
