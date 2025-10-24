/*******************************************************************************
* File Name: DAC_PM.c  
* Version 1.90
*
* Description:
*  This file provides the power management source code to API for the
*  VDAC8.  
*
* Note:
*  None
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "DAC.h"

static DAC_backupStruct DAC_backup;


/*******************************************************************************
* Function Name: DAC_SaveConfig
********************************************************************************
* Summary:
*  Save the current user configuration
*
* Parameters:  
*  void  
*
* Return: 
*  void
*
*******************************************************************************/
void DAC_SaveConfig(void) 
{
    if (!((DAC_CR1 & DAC_SRC_MASK) == DAC_SRC_UDB))
    {
        DAC_backup.data_value = DAC_Data;
    }
}


/*******************************************************************************
* Function Name: DAC_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration.
*
* Parameters:  
*  void
*
* Return: 
*  void
*
*******************************************************************************/
void DAC_RestoreConfig(void) 
{
    if (!((DAC_CR1 & DAC_SRC_MASK) == DAC_SRC_UDB))
    {
        if((DAC_Strobe & DAC_STRB_MASK) == DAC_STRB_EN)
        {
            DAC_Strobe &= (uint8)(~DAC_STRB_MASK);
            DAC_Data = DAC_backup.data_value;
            DAC_Strobe |= DAC_STRB_EN;
        }
        else
        {
            DAC_Data = DAC_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: DAC_Sleep
********************************************************************************
* Summary:
*  Stop and Save the user configuration
*
* Parameters:  
*  void:  
*
* Return: 
*  void
*
* Global variables:
*  DAC_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void DAC_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(DAC_ACT_PWR_EN == (DAC_PWRMGR & DAC_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        DAC_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        DAC_backup.enableState = 0u;
    }
    
    DAC_Stop();
    DAC_SaveConfig();
}


/*******************************************************************************
* Function Name: DAC_Wakeup
********************************************************************************
*
* Summary:
*  Restores and enables the user configuration
*  
* Parameters:  
*  void
*
* Return: 
*  void
*
* Global variables:
*  DAC_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void DAC_Wakeup(void) 
{
    DAC_RestoreConfig();
    
    if(DAC_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        DAC_Enable();

        /* Restore the data register */
        DAC_SetValue(DAC_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
