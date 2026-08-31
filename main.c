/******************************************************************************
* File Name: main.c
*
* Description: This is the source code for the LIN slave code example 
* for ModusToolbox.
*
* Related Document: See README.md 
*
*******************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/


#include "cy_pdl.h"

/* Override LED active state for active-high LEDs */
#define CYBSP_LED_STATE_ON          (1U)
#define CYBSP_LED_STATE_OFF         (0U)

#include "cybsp.h"
#include "mtbcfg_lin.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
/* LIN Instance number */
#define LIN_IFC_HANDLE                  (0u)

/* Define start position and number of bytes for input & output signals */
/* One command byte received from Master */
#define LIN_SIGNALINPUT_START_BYTE      (0u)
#define LIN_SIGNALINPUT_NUM_OF_BYTES    (1u)

/* 2 status bytes from Slave to Master - previous command and LED status */
#define LIN_SIGNALOUTPUT_START_BYTE     (0u)
#define LIN_SIGNALOUTPUT_NUM_OF_BYTES   (2u)

/* Commands sent from LIN master to set LED */
#define CMD_SET_LED2                    (0x11u)
#define CMD_SET_LED3                    (0x22u)
#define CMD_SET_ON                      (0x33u)
#define CMD_SET_OFF                     (0x00u)

/* Commands sent back to LIN master by slave */
#define CMD_SENT_LED2                   (0xAAu)
#define CMD_SENT_LED3                   (0xBBu)
#define CMD_SENT_ON                     (0xCCu)
#define CMD_SENT_OFF                    (0xDDu)

/* Turn ON CYBSP_LED2 - USER LED2 on board */
#define LED2_ON \
    {   \
        Cy_GPIO_Write(CYBSP_LED1_PORT, CYBSP_LED1_PIN, CYBSP_LED_STATE_OFF); \
        Cy_GPIO_Write(CYBSP_LED2_PORT, CYBSP_LED2_PIN, CYBSP_LED_STATE_ON); \
    }

/* Turn ON CYBSP_LED1 - USER LED3 on board */
#define LED3_ON \
    {   \
        Cy_GPIO_Write(CYBSP_LED1_PORT, CYBSP_LED1_PIN, CYBSP_LED_STATE_ON); \
        Cy_GPIO_Write(CYBSP_LED2_PORT, CYBSP_LED2_PIN, CYBSP_LED_STATE_OFF); \
    }

/* Turn OFF all LEDs */
#define ALL_LEDS_OFF \
    {   \
        Cy_GPIO_Write(CYBSP_LED1_PORT, CYBSP_LED1_PIN, CYBSP_LED_STATE_OFF); \
        Cy_GPIO_Write(CYBSP_LED2_PORT, CYBSP_LED2_PIN, CYBSP_LED_STATE_OFF); \
    }

/* Turn ON all LEDs */
#define ALL_LEDS_ON \
    {   \
        Cy_GPIO_Write(CYBSP_LED1_PORT, CYBSP_LED1_PIN, CYBSP_LED_STATE_ON); \
        Cy_GPIO_Write(CYBSP_LED2_PORT, CYBSP_LED2_PIN, CYBSP_LED_STATE_ON); \
    }
#define CY_ASSERT_FAILED      (0u)

/*******************************************************************************
 * Structures
 ******************************************************************************/

/*******************************************************************************
 * Global variable
 ******************************************************************************/
/* Allocate context for LIN operation */
mtb_stc_lin_context_t gl_lin_context;

/*******************************************************************************
 * Function Name: LIN_Isr
 *******************************************************************************
 * Summary:
 *  Implement SCB ISR for LIN
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void LIN_Isr(void)
{
    l_ifc_rx(LIN_IFC_HANDLE, &gl_lin_context);
}

/*******************************************************************************
 * Function Name: LIN_InactivityIsr
 *******************************************************************************
 * Summary:
 *  Implement Inactivity ISR for LIN
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void LIN_InactivityIsr(void)
{
    l_ifc_aux(LIN_IFC_HANDLE, &gl_lin_context);
}

/*******************************************************************************
 * Function Name: handle_error
 *******************************************************************************
 * Summary:
 *  User defined error handling function
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void handle_error(void)
{
    /* Disable all interrupts. */
    __disable_irq();

    /* Stop program execution if any unexpected error happened*/
    CY_ASSERT(CY_ASSERT_FAILED);
}

/*******************************************************************************
 * Function Name: main
 *******************************************************************************
 * Summary:
 *  This is the main function.
 *  Initialize LIN slave. If the LIN slave receives an unconditional frame from
 *  master with Frame ID 0x10, then the first byte of data (command to control
 *  the LED) is written to the other unconditional frame (OutFrame). Based on
 *  the received command slave will control three LEDs on the kit.
 *
 *  The LIN Master can read the status of the LEDs by sending the Frame ID 0x11
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/
int main(void)
{
    /* Local variables */
    uint8_t dataReceived = 0u;
    uint8_t dataArray[2] = { 0u, 0u };
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    
    Cy_GPIO_Write(CYBSP_LED1_PORT, CYBSP_LED1_PIN, CYBSP_LED_STATE_OFF); \
    Cy_GPIO_Write(CYBSP_LED2_PORT, CYBSP_LED2_PIN, CYBSP_LED_STATE_OFF); \
  
    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize the LIN core that is specified by the context structure */
    if (MTB_LIN_STATUS_SUCCESS != l_sys_init(&mtb_lin_0_config, &gl_lin_context, &LIN_Isr, &LIN_InactivityIsr))
    {
        handle_error();
    }

    /* Initializes the LIN instance that is specified by the context structure. */
    if (l_ifc_init(LIN_IFC_HANDLE, &CYBSP_LIN_ifc_config, &gl_lin_context))
    {
        handle_error();
    }

    while (1)
    {
        /***********************************************************************
         * Check if "InFrame" frame is received from LIN Master
         **********************************************************************/
        if (true == l_flg_tst(MTB_LIN_0_FLAG_HANDLE_InFrame, &gl_lin_context))
        {
            /* Read the 1st byte command received from the LIN Master */
            l_bytes_rd(MTB_LIN_0_SIGNAL_HANDLE_SignalInput, \
                    LIN_SIGNALINPUT_START_BYTE, \
                    LIN_SIGNALINPUT_NUM_OF_BYTES, \
                    &dataReceived, &gl_lin_context);

            /* Clear frame flag */
            l_flg_clr(MTB_LIN_0_FLAG_HANDLE_InFrame, &gl_lin_context);

            /* Store the received command in dataArray */
            dataArray[0] = dataReceived;

            /* Turn on the LED2 corresponding to the command received */
            if (CMD_SET_LED2 == dataReceived)
            {
                LED2_ON;
                dataArray[1] = CMD_SENT_LED2;
            }

            /* Turn on the LED3 corresponding to the command received */
            else if (CMD_SET_LED3 == dataReceived)
            {
                LED3_ON;
                dataArray[1] = CMD_SENT_LED3;
            }

            /* Turn on all the LED corresponding to the command received */
            else if (CMD_SET_ON == dataReceived)
            {
                ALL_LEDS_ON;
                dataArray[1] = CMD_SENT_ON;
            }

            /* Turn off all the LED corresponding to the command received */
            else if (CMD_SET_OFF == dataReceived)
            {
                ALL_LEDS_OFF;
                dataArray[1] = CMD_SENT_OFF;
            }

            /* Send the previous command and the status of LEDs to LIN Master */
            l_bytes_wr(MTB_LIN_0_SIGNAL_HANDLE_SignalOutput, \
                    LIN_SIGNALOUTPUT_START_BYTE, \
                    LIN_SIGNALOUTPUT_NUM_OF_BYTES, \
                    dataArray, &gl_lin_context);
        }

        /***********************************************************************
         * Check if the data in "OutFrame" frame is sent to LIN Master
         **********************************************************************/
        if (true == l_flg_tst(MTB_LIN_0_FLAG_HANDLE_OutFrame, &gl_lin_context))
        {
            /* Clear frame flag */
            l_flg_clr(MTB_LIN_0_FLAG_HANDLE_OutFrame, &gl_lin_context);
        }
    }
}

/* [] END OF FILE */
