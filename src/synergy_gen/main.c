/* generated main source file - do not edit */
#include "bsp_api.h"
#include "tx_api.h"

extern void main_thread_create(void);

uint32_t g_ssp_common_thread_count;
bool g_ssp_common_initialized;
TX_SEMAPHORE g_ssp_common_initialized_semaphore;

#if defined(__ICCARM__)
#define WEAK_REF_ATTRIBUTE
#pragma weak tx_application_define_user               = tx_application_define_internal
#elif defined(__GNUC__)
#define WEAK_REF_ATTRIBUTE      __attribute__ ((weak, alias("tx_application_define_internal")))
#endif

#ifdef TX_USER_TRACE_BUFFER_DECLARE
TX_USER_TRACE_BUFFER_DECLARE;
#endif

void tx_application_define_internal(void *first_unused_memory);

void tx_application_define_internal(void *first_unused_memory)
{
    /* Does nothing. Default implementation of tx_application_define_user(). */
    SSP_PARAMETER_NOT_USED (first_unused_memory);
}

void tx_application_define_internal(void *first_unused_memory);
void tx_application_define_user(void *first_unused_memory)
WEAK_REF_ATTRIBUTE;

void tx_application_define(void *first_unused_memory)
{
    g_ssp_common_thread_count = 0;
    g_ssp_common_initialized = false;

    /* Create semaphore to make sure common init is done before threads start running. */
    tx_semaphore_create (&g_ssp_common_initialized_semaphore, "SSP Common Init Sem", 1);

    main_thread_create ();

#ifdef TX_USER_ENABLE_TRACE
    TX_USER_ENABLE_TRACE;
#endif				

    tx_application_define_user (first_unused_memory);
}

int main(void)
{
    __disable_irq ();
    tx_kernel_enter ();

    return 0;
}
