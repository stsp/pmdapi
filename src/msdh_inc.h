static void handler(struct sigcontext *scp, int n)
{
    struct RealModeCallStructure *rmreg;
    unsigned long base;
    __dpmi_get_segment_base_address(_es, &base);
    rmreg = (struct RealModeCallStructure *)(base + _edi);
    msdos.rmcb_handler[n](scp, rmreg, clnt_is_32, msdos.rmcb_arg[n]);
    do_pm_call(scp);
    msdos.rmcb_ret_handler[n](scp, rmreg, clnt_is_32);
}

#define HNDL(n) \
void MSDOS_rmcb_call##n(struct sigcontext *scp) \
{ \
    handler(scp, n); \
}
HNDL(0)
HNDL(1)
HNDL(2)

void MSDOS_API_call(struct sigcontext *scp)
{
    msdos.api_call(scp, msdos.api_arg);
}

void MSDOS_API_WINOS2_call(struct sigcontext *scp)
{
    msdos.api_winos2_call(scp, msdos.api_winos2_arg);
}

void MSDOS_XMS_call(struct sigcontext *scp)
{
    struct RealModeCallStructure rmreg = {};
    msdos.xms_call(scp, &rmreg, msdos.xms_arg);
    __dpmi_simulate_real_mode_procedure_retf(&rmreg);
    msdos.xms_ret(scp, &rmreg);
}

#define DOS_LONG_WRITE_SEG 0
#define DOS_LONG_WRITE_OFF 0
#define DOS_LONG_READ_SEG 0
#define DOS_LONG_READ_OFF 0
