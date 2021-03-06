#include <stdio.h>

#include "gui/imports_gui.h"
#include "osal/imports_osal.h"
#include "imports_m64p.h"
#include "imports_qt.h"
#include "common.h"
#include "interface.h"
#include "version.h"

using pfn_ml64_InputReset = m64p_error(CALL*)();
pfn_ml64_InputReset fn_ml64_InputReset;

QString qtCoreDirPath;
QString qtConfigDir;
int g_CoreCapabilities;
int g_CoreAPIVersion;
int coreStarted = 0;

m64p_video_extension_functions vidExtFunctions = {12,
                                                 qtVidExtFuncInit,
                                                 qtVidExtFuncQuit,
                                                 qtVidExtFuncListModes,
                                                 qtVidExtFuncSetMode,
                                                 qtVidExtFuncGLGetProc,
                                                 qtVidExtFuncGLSetAttr,
                                                 qtVidExtFuncGLGetAttr,
                                                 qtVidExtFuncGLSwapBuf,
                                                 qtVidExtFuncSetCaption,
                                                 qtVidExtFuncToggleFS,
                                                 qtVidExtFuncResizeWindow,
                                                 qtVidExtFuncGLGetDefaultFramebuffer};


m64p_dynlib_handle      CoreHandle = NULL;
ptr_CoreErrorMessage    CoreErrorMessage = NULL;

ptr_CoreStartup         CoreStartup = NULL;
ptr_CoreShutdown        CoreShutdown = NULL;
ptr_CoreAttachPlugin    CoreAttachPlugin = NULL;
ptr_CoreDetachPlugin    CoreDetachPlugin = NULL;
ptr_CoreDoCommand       CoreDoCommand = NULL;
ptr_CoreOverrideVidExt  CoreOverrideVidExt = NULL;
ptr_CoreAddCheat        CoreAddCheat = NULL;
ptr_CoreCheatEnabled    CoreCheatEnabled = NULL;

ptr_ConfigListSections     ConfigListSections = NULL;
ptr_ConfigOpenSection      ConfigOpenSection = NULL;
ptr_ConfigDeleteSection    ConfigDeleteSection = NULL;
ptr_ConfigSaveSection      ConfigSaveSection = NULL;
ptr_ConfigListParameters   ConfigListParameters = NULL;
ptr_ConfigSaveFile         ConfigSaveFile = NULL;
ptr_ConfigSetParameter     ConfigSetParameter = NULL;
ptr_ConfigGetParameter     ConfigGetParameter = NULL;
ptr_ConfigGetParameterType ConfigGetParameterType = NULL;
ptr_ConfigGetParameterHelp ConfigGetParameterHelp = NULL;
ptr_ConfigSetDefaultInt    ConfigSetDefaultInt = NULL;
ptr_ConfigSetDefaultFloat  ConfigSetDefaultFloat = NULL;
ptr_ConfigSetDefaultBool   ConfigSetDefaultBool = NULL;
ptr_ConfigSetDefaultString ConfigSetDefaultString = NULL;
ptr_ConfigGetParamInt      ConfigGetParamInt = NULL;
ptr_ConfigGetParamFloat    ConfigGetParamFloat = NULL;
ptr_ConfigGetParamBool     ConfigGetParamBool = NULL;
ptr_ConfigGetParamString   ConfigGetParamString = NULL;

ptr_ConfigGetSharedDataFilepath ConfigGetSharedDataFilepath = NULL;
ptr_ConfigGetUserConfigPath     ConfigGetUserConfigPath = NULL;
ptr_ConfigGetUserDataPath       ConfigGetUserDataPath = NULL;
ptr_ConfigGetUserCachePath      ConfigGetUserCachePath = NULL;

ptr_DebugSetCallbacks      DebugSetCallbacks = NULL;
ptr_DebugSetCoreCompare    DebugSetCoreCompare = NULL;
ptr_DebugSetRunState       DebugSetRunState = NULL;
ptr_DebugGetState          DebugGetState = NULL;
ptr_DebugStep              DebugStep = NULL;
ptr_DebugDecodeOp          DebugDecodeOp = NULL;
ptr_DebugMemGetRecompInfo  DebugMemGetRecompInfo = NULL;
ptr_DebugMemGetMemInfo     DebugMemGetMemInfo = NULL;
ptr_DebugMemGetPointer     DebugMemGetPointer = NULL;

ptr_DebugMemRead64         DebugMemRead64 = NULL;
ptr_DebugMemRead32         DebugMemRead32 = NULL;
ptr_DebugMemRead16         DebugMemRead16 = NULL;
ptr_DebugMemRead8          DebugMemRead8 = NULL;

ptr_DebugMemWrite64        DebugMemWrite64 = NULL;
ptr_DebugMemWrite32        DebugMemWrite32 = NULL;
ptr_DebugMemWrite16        DebugMemWrite16 = NULL;
ptr_DebugMemWrite8         DebugMemWrite8 = NULL;

ptr_DebugGetCPUDataPtr     DebugGetCPUDataPtr = NULL;
ptr_DebugBreakpointLookup  DebugBreakpointLookup = NULL;
ptr_DebugBreakpointCommand DebugBreakpointCommand = NULL;

ptr_DebugBreakpointTriggeredBy DebugBreakpointTriggeredBy = NULL;
ptr_DebugVirtualToPhysical     DebugVirtualToPhysical = NULL;

ptr_read_rdram_buffer		 RdRamReadBuffer = NULL;
ptr_read_rdram_32			 RdRamRead32 = NULL;
ptr_read_rdram_16			 RdRamRead16 = NULL;
ptr_read_rdram_8			 RdRamRead8 = NULL;

ptr_write_rdram_32			 RdRamWrite32 = NULL;
ptr_write_rdram_16			 RdRamWrite16 = NULL;
ptr_write_rdram_8			 RdRamWrite8 = NULL;

ptr_read_rom_buffer			 RomReadBuffer = NULL;
ptr_read_rom_32				 RomRead32 = NULL;
ptr_read_rom_16				 RomRead16 = NULL;
ptr_read_rom_8				 RomRead8 = NULL;

ptr_write_rom_32			 RomWrite32 = NULL;
ptr_write_rom_16			 RomWrite16 = NULL;
ptr_write_rom_8				 RomWrite8 = NULL;

ptr_get_cart_clock_rate		 CartGetClockRate = NULL;
ptr_get_cart_pc				 CartGetPC = NULL;
ptr_get_cart_release		 CartGetRelease = NULL;

ptr_get_cart_name			 CartGetName = NULL;
ptr_get_cart_manufacturer_id CartGetManufacturerID = NULL;
ptr_get_cart_serial			 CartGetSerial = NULL;
ptr_get_cart_country_code	 CartGetCountryCode = NULL;

ptr_refresh_memory_cache	 MemoryCacheRefresh = NULL;

ptr_CoreSaveOverride         CoreSaveOverride = NULL;
ptr_IsMupenReady             IsMupenReady = NULL;

bool QtAttachCoreLib()
{
    if (AttachCoreLib(qtCoreDirPath.toLatin1().data()) != M64ERR_SUCCESS && CoreHandle == NULL) {
        QMessageBox msgBox;
        msgBox.setText("Couldn't load core library, please set the path in settings.");
        msgBox.exec();
        return false;
    }
    if (!coreStarted) {
        /* start the Mupen64Plus core library, load the configuration file */
        m64p_error rval;
        if (!qtConfigDir.isEmpty())
            rval = (*CoreStartup)(CORE_API_VERSION, qtConfigDir.toLatin1().data() /*Config dir*/, QString::fromStdString(GetAppDir()).toLatin1().data(), (char*)"Core", DebugCallback, NULL, NULL);
        else
            rval = (*CoreStartup)(CORE_API_VERSION, NULL /*Config dir*/, QString::fromStdString(GetAppDir()).toLatin1().data(), (char*)"Core", DebugCallback, NULL, NULL);
        if (rval != M64ERR_SUCCESS)
        {
            DebugMessage(M64MSG_ERROR, "couldn't start Mupen64Plus core library.");
            DetachCoreLib();
            return false;
        }

        rval = CoreOverrideVidExt(&vidExtFunctions);
        if (rval != M64ERR_SUCCESS)
        {
            DebugMessage(M64MSG_ERROR, "couldn't start VidExt library.");
            DetachCoreLib();
            return false;
        }
        coreStarted = 1;
    }
    return true;
}

/* functions */
m64p_error AttachCoreLib(const char *CoreLibFilepath)
{
    /* check if Core DLL is already attached */
    if (CoreHandle != NULL)
        return M64ERR_INVALID_STATE;

    /* load the DLL */
    m64p_error rval = M64ERR_INTERNAL;
    /* first, try a library path+name that was given on the command-line */
    if (CoreLibFilepath != NULL)
    {
        rval = osal_dynlib_open(&CoreHandle, CoreLibFilepath);
    }
    /* then try a library path that was given at compile time */
#if defined(COREDIR)
    if (rval != M64ERR_SUCCESS || CoreHandle == NULL)
    {
        rval = osal_dynlib_open(&CoreHandle, COREDIR OSAL_DEFAULT_DYNLIB_FILENAME);
    }
#endif
    /* then try just the filename of the shared library, to let dlopen() look through the system lib dirs */
    if (rval != M64ERR_SUCCESS || CoreHandle == NULL)
    {
        rval = osal_dynlib_open(&CoreHandle, OSAL_DEFAULT_DYNLIB_FILENAME);
    }
    /* as a last-ditch effort, try loading library in current directory */
    if (rval != M64ERR_SUCCESS || CoreHandle == NULL)
    {
        rval = osal_dynlib_open(&CoreHandle, OSAL_CURRENT_DIR OSAL_DEFAULT_DYNLIB_FILENAME);
    }
    if (rval != M64ERR_SUCCESS || CoreHandle == NULL)
    {
        QString corePath = QDir(QString::fromStdString(GetAppDir())).filePath(OSAL_DEFAULT_DYNLIB_FILENAME);
        rval = osal_dynlib_open(&CoreHandle, corePath.toLatin1().data());
    }
    /* if we haven't found a good core library by now, then we're screwed */
    if (rval != M64ERR_SUCCESS || CoreHandle == NULL)
    {
        DebugMessage(M64MSG_ERROR, "AttachCoreLib() Error: failed to find Mupen64Plus Core library");
        CoreHandle = NULL;
        return M64ERR_INPUT_NOT_FOUND;
    }

    /* attach and call the PluginGetVersion function, check the Core and API versions for compatibility with this front-end */
    ptr_PluginGetVersion CoreVersionFunc;
    CoreVersionFunc = (ptr_PluginGetVersion) osal_dynlib_getproc(CoreHandle, "PluginGetVersion");
    if (CoreVersionFunc == NULL)
    {
        DebugMessage(M64MSG_ERROR, "AttachCoreLib() Error: Shared library '%s' invalid; no PluginGetVersion() function found.", CoreLibFilepath);
        osal_dynlib_close(CoreHandle);
        CoreHandle = NULL;
        return M64ERR_INPUT_INVALID;
    }
    m64p_plugin_type PluginType = (m64p_plugin_type) 0;
    int Compatible = 0;
    int CoreVersion = 0;
    const char *CoreName = NULL;
    (*CoreVersionFunc)(&PluginType, &CoreVersion, &g_CoreAPIVersion, &CoreName, &g_CoreCapabilities);
    if (PluginType != M64PLUGIN_CORE)
        DebugMessage(M64MSG_ERROR, "AttachCoreLib() Error: Shared library '%s' invalid; this is not the emulator core.", CoreLibFilepath);
    else if (CoreVersion < MINIMUM_CORE_VERSION)
        DebugMessage(M64MSG_ERROR, "AttachCoreLib() Error: Shared library '%s' incompatible; core version %i.%i.%i is below minimum supported %i.%i.%i",
                CoreLibFilepath, VERSION_PRINTF_SPLIT(CoreVersion), VERSION_PRINTF_SPLIT(MINIMUM_CORE_VERSION));
    else if ((g_CoreAPIVersion & 0xffff0000) != (CORE_API_VERSION & 0xffff0000))
        DebugMessage(M64MSG_ERROR, "AttachCoreLib() Error: Shared library '%s' incompatible; core API major version %i.%i.%i doesn't match with this application (%i.%i.%i)",
                CoreLibFilepath, VERSION_PRINTF_SPLIT(g_CoreAPIVersion), VERSION_PRINTF_SPLIT(CORE_API_VERSION));
    else
        Compatible = 1;
    /* exit if not compatible */
    if (Compatible == 0)
    {
        osal_dynlib_close(CoreHandle);
        CoreHandle = NULL;
        return M64ERR_INCOMPATIBLE;
    }

    /* attach and call the CoreGetAPIVersion function, check Config API version for compatibility */
    ptr_CoreGetAPIVersions CoreAPIVersionFunc;
    CoreAPIVersionFunc = (ptr_CoreGetAPIVersions) osal_dynlib_getproc(CoreHandle, "CoreGetAPIVersions");
    if (CoreAPIVersionFunc == NULL)
    {
        DebugMessage(M64MSG_ERROR, "AttachCoreLib() Error: Library '%s' broken; no CoreAPIVersionFunc() function found.", CoreLibFilepath);
        osal_dynlib_close(CoreHandle);
        CoreHandle = NULL;
        return M64ERR_INPUT_INVALID;
    }
    int ConfigAPIVersion, DebugAPIVersion, VidextAPIVersion;
    (*CoreAPIVersionFunc)(&ConfigAPIVersion, &DebugAPIVersion, &VidextAPIVersion, NULL);
    if ((ConfigAPIVersion & 0xffff0000) != (CONFIG_API_VERSION & 0xffff0000))
    {
        DebugMessage(M64MSG_ERROR, "AttachCoreLib() Error: Emulator core '%s' incompatible; Config API major version %i.%i.%i doesn't match application: %i.%i.%i",
                CoreLibFilepath, VERSION_PRINTF_SPLIT(ConfigAPIVersion), VERSION_PRINTF_SPLIT(CONFIG_API_VERSION));
        osal_dynlib_close(CoreHandle);
        CoreHandle = NULL;
        return M64ERR_INCOMPATIBLE;
    }

    /* print some information about the core library */
    DebugMessage(M64MSG_INFO, "attached to core library '%s' version %i.%i.%i", CoreName, VERSION_PRINTF_SPLIT(CoreVersion));
    if (g_CoreCapabilities & M64CAPS_DYNAREC)
        DebugMessage(M64MSG_INFO, "            Includes support for Dynamic Recompiler.");
    if (g_CoreCapabilities & M64CAPS_DEBUGGER)
        DebugMessage(M64MSG_INFO, "            Includes support for MIPS r4300 Debugger.");
    if (g_CoreCapabilities & M64CAPS_CORE_COMPARE)
        DebugMessage(M64MSG_INFO, "            Includes support for r4300 Core Comparison.");

    CoreErrorMessage = (ptr_CoreErrorMessage) osal_dynlib_getproc(CoreHandle, "CoreErrorMessage");
    CoreStartup = (ptr_CoreStartup) osal_dynlib_getproc(CoreHandle, "CoreStartup");
    CoreShutdown = (ptr_CoreShutdown) osal_dynlib_getproc(CoreHandle, "CoreShutdown");
    CoreAttachPlugin = (ptr_CoreAttachPlugin) osal_dynlib_getproc(CoreHandle, "CoreAttachPlugin");
    CoreDetachPlugin = (ptr_CoreDetachPlugin) osal_dynlib_getproc(CoreHandle, "CoreDetachPlugin");
    CoreDoCommand = (ptr_CoreDoCommand) osal_dynlib_getproc(CoreHandle, "CoreDoCommand");
    CoreOverrideVidExt = (ptr_CoreOverrideVidExt) osal_dynlib_getproc(CoreHandle, "CoreOverrideVidExt");
    CoreAddCheat = (ptr_CoreAddCheat) osal_dynlib_getproc(CoreHandle, "CoreAddCheat");
    CoreCheatEnabled = (ptr_CoreCheatEnabled) osal_dynlib_getproc(CoreHandle, "CoreCheatEnabled");

    ConfigListSections = (ptr_ConfigListSections) osal_dynlib_getproc(CoreHandle, "ConfigListSections");
    ConfigOpenSection = (ptr_ConfigOpenSection) osal_dynlib_getproc(CoreHandle, "ConfigOpenSection");
    ConfigDeleteSection = (ptr_ConfigDeleteSection) osal_dynlib_getproc(CoreHandle, "ConfigDeleteSection");
    ConfigSaveSection = (ptr_ConfigSaveSection) osal_dynlib_getproc(CoreHandle, "ConfigSaveSection");
    ConfigListParameters = (ptr_ConfigListParameters) osal_dynlib_getproc(CoreHandle, "ConfigListParameters");
    ConfigSaveFile = (ptr_ConfigSaveFile) osal_dynlib_getproc(CoreHandle, "ConfigSaveFile");
    ConfigSetParameter = (ptr_ConfigSetParameter) osal_dynlib_getproc(CoreHandle, "ConfigSetParameter");
    ConfigGetParameter = (ptr_ConfigGetParameter) osal_dynlib_getproc(CoreHandle, "ConfigGetParameter");
    ConfigGetParameterType = (ptr_ConfigGetParameterType) osal_dynlib_getproc(CoreHandle, "ConfigGetParameterType");
    ConfigGetParameterHelp = (ptr_ConfigGetParameterHelp) osal_dynlib_getproc(CoreHandle, "ConfigGetParameterHelp");
    ConfigSetDefaultInt = (ptr_ConfigSetDefaultInt) osal_dynlib_getproc(CoreHandle, "ConfigSetDefaultInt");
    ConfigSetDefaultFloat = (ptr_ConfigSetDefaultFloat) osal_dynlib_getproc(CoreHandle, "ConfigSetDefaultFloat");
    ConfigSetDefaultBool = (ptr_ConfigSetDefaultBool) osal_dynlib_getproc(CoreHandle, "ConfigSetDefaultBool");
    ConfigSetDefaultString = (ptr_ConfigSetDefaultString) osal_dynlib_getproc(CoreHandle, "ConfigSetDefaultString");
    ConfigGetParamInt = (ptr_ConfigGetParamInt) osal_dynlib_getproc(CoreHandle, "ConfigGetParamInt");
    ConfigGetParamFloat = (ptr_ConfigGetParamFloat) osal_dynlib_getproc(CoreHandle, "ConfigGetParamFloat");
    ConfigGetParamBool = (ptr_ConfigGetParamBool) osal_dynlib_getproc(CoreHandle, "ConfigGetParamBool");
    ConfigGetParamString = (ptr_ConfigGetParamString) osal_dynlib_getproc(CoreHandle, "ConfigGetParamString");

    ConfigGetSharedDataFilepath = (ptr_ConfigGetSharedDataFilepath) osal_dynlib_getproc(CoreHandle, "ConfigGetSharedDataFilepath");
    ConfigGetUserConfigPath = (ptr_ConfigGetUserConfigPath) osal_dynlib_getproc(CoreHandle, "ConfigGetUserConfigPath");
    ConfigGetUserDataPath = (ptr_ConfigGetUserDataPath) osal_dynlib_getproc(CoreHandle, "ConfigGetUserDataPath");
    ConfigGetUserCachePath = (ptr_ConfigGetUserCachePath) osal_dynlib_getproc(CoreHandle, "ConfigGetUserCachePath");

    DebugSetCallbacks = (ptr_DebugSetCallbacks) osal_dynlib_getproc(CoreHandle, "DebugSetCallbacks");
    DebugSetCoreCompare = (ptr_DebugSetCoreCompare) osal_dynlib_getproc(CoreHandle, "DebugSetCoreCompare");
    DebugSetRunState = (ptr_DebugSetRunState) osal_dynlib_getproc(CoreHandle, "DebugSetRunState");
    DebugGetState = (ptr_DebugGetState) osal_dynlib_getproc(CoreHandle, "DebugGetState");
    DebugStep = (ptr_DebugStep) osal_dynlib_getproc(CoreHandle, "DebugStep");
    DebugDecodeOp = (ptr_DebugDecodeOp) osal_dynlib_getproc(CoreHandle, "DebugDecodeOp");
    DebugMemGetRecompInfo = (ptr_DebugMemGetRecompInfo) osal_dynlib_getproc(CoreHandle, "DebugMemGetRecompInfo");
    DebugMemGetMemInfo = (ptr_DebugMemGetMemInfo) osal_dynlib_getproc(CoreHandle, "DebugMemGetMemInfo");
    DebugMemGetPointer = (ptr_DebugMemGetPointer) osal_dynlib_getproc(CoreHandle, "DebugMemGetPointer");

    DebugMemRead64 = (ptr_DebugMemRead64) osal_dynlib_getproc(CoreHandle, "DebugMemRead64");
    DebugMemRead32 = (ptr_DebugMemRead32) osal_dynlib_getproc(CoreHandle, "DebugMemRead32");
    DebugMemRead16 = (ptr_DebugMemRead16) osal_dynlib_getproc(CoreHandle, "DebugMemRead16");
    DebugMemRead8 = (ptr_DebugMemRead8) osal_dynlib_getproc(CoreHandle, "DebugMemRead8");

    DebugMemWrite64 = (ptr_DebugMemWrite64) osal_dynlib_getproc(CoreHandle, "DebugMemRead64");
    DebugMemWrite32 = (ptr_DebugMemWrite32) osal_dynlib_getproc(CoreHandle, "DebugMemRead32");
    DebugMemWrite16 = (ptr_DebugMemWrite16) osal_dynlib_getproc(CoreHandle, "DebugMemRead16");
    DebugMemWrite8 = (ptr_DebugMemWrite8) osal_dynlib_getproc(CoreHandle, "DebugMemRead8");

    DebugGetCPUDataPtr = (ptr_DebugGetCPUDataPtr) osal_dynlib_getproc(CoreHandle, "DebugGetCPUDataPtr");
    DebugBreakpointLookup = (ptr_DebugBreakpointLookup) osal_dynlib_getproc(CoreHandle, "DebugBreakpointLookup");
    DebugBreakpointCommand = (ptr_DebugBreakpointCommand) osal_dynlib_getproc(CoreHandle, "DebugBreakpointCommand");

    DebugBreakpointTriggeredBy = (ptr_DebugBreakpointTriggeredBy) osal_dynlib_getproc(CoreHandle, "DebugBreakpointTriggeredBy");
    DebugVirtualToPhysical = (ptr_DebugVirtualToPhysical) osal_dynlib_getproc(CoreHandle, "DebugVirtualToPhysical");
	
    if (isModLoader) {
        RdRamReadBuffer = (ptr_read_rdram_buffer)osal_dynlib_getproc(CoreHandle, "read_rdram_buffer");
        RdRamRead32 = (ptr_read_rdram_32)osal_dynlib_getproc(CoreHandle, "read_rdram_32");
        RdRamRead16 = (ptr_read_rdram_16)osal_dynlib_getproc(CoreHandle, "read_rdram_16");
        RdRamRead8 = (ptr_read_rdram_8)osal_dynlib_getproc(CoreHandle, "read_rdram_8");

        RdRamWrite32 = (ptr_write_rdram_32)osal_dynlib_getproc(CoreHandle, "write_rdram_32");
        RdRamWrite16 = (ptr_write_rdram_16)osal_dynlib_getproc(CoreHandle, "write_rdram_16");
        RdRamWrite8 = (ptr_write_rdram_8)osal_dynlib_getproc(CoreHandle, "write_rdram_8");

        RomReadBuffer = (ptr_read_rom_buffer)osal_dynlib_getproc(CoreHandle, "read_rom_buffer");
        RomRead32 = (ptr_read_rom_32)osal_dynlib_getproc(CoreHandle, "read_rom_32");
        RomRead16 = (ptr_read_rom_16)osal_dynlib_getproc(CoreHandle, "read_rom_16");
        RomRead8 = (ptr_read_rom_8)osal_dynlib_getproc(CoreHandle, "read_rom_8");

        RomWrite32 = (ptr_write_rom_32)osal_dynlib_getproc(CoreHandle, "write_rom_32");
        RomWrite16 = (ptr_write_rom_16)osal_dynlib_getproc(CoreHandle, "write_rom_16");
        RomWrite8 = (ptr_write_rom_8)osal_dynlib_getproc(CoreHandle, "write_rom_8");
        
        CartGetClockRate = (ptr_get_cart_clock_rate)osal_dynlib_getproc(CoreHandle, "get_cart_clock_rate");
        CartGetPC = (ptr_get_cart_pc)osal_dynlib_getproc(CoreHandle, "get_cart_pc");
        CartGetRelease = (ptr_get_cart_release)osal_dynlib_getproc(CoreHandle, "get_cart_release");
        
        CartGetName = (ptr_get_cart_name)osal_dynlib_getproc(CoreHandle, "get_cart_name");
        CartGetManufacturerID = (ptr_get_cart_manufacturer_id)osal_dynlib_getproc(CoreHandle, "get_cart_manufacturer_id");
        CartGetSerial = (ptr_get_cart_serial)osal_dynlib_getproc(CoreHandle, "get_cart_serial");
        CartGetCountryCode = (ptr_get_cart_country_code)osal_dynlib_getproc(CoreHandle, "get_cart_country_code");

        MemoryCacheRefresh = (ptr_refresh_memory_cache)osal_dynlib_getproc(CoreHandle, "refresh_memory_cache");

        CoreSaveOverride = (ptr_CoreSaveOverride)osal_dynlib_getproc(CoreHandle, "CoreSaveOverride");
    	IsMupenReady = (ptr_IsMupenReady)osal_dynlib_getproc(CoreHandle, "IsMupenReady");
        fn_ml64_InputReset = (pfn_ml64_InputReset)osal_dynlib_getproc(CoreHandle, "ml64_InputReset");
    }

    return M64ERR_SUCCESS;
}

m64p_error DetachCoreLib(void)
{
    if (CoreHandle == NULL)
        return M64ERR_INVALID_STATE;

    (*CoreShutdown)();

    /* set the core function pointers to NULL */
    CoreErrorMessage = NULL;
    CoreStartup = NULL;
    CoreShutdown = NULL;
    CoreAttachPlugin = NULL;
    CoreDetachPlugin = NULL;
    CoreDoCommand = NULL;
    CoreOverrideVidExt = NULL;
    CoreAddCheat = NULL;
    CoreCheatEnabled = NULL;

    ConfigListSections = NULL;
    ConfigOpenSection = NULL;
    ConfigDeleteSection = NULL;
    ConfigSaveSection = NULL;
    ConfigListParameters = NULL;
    ConfigSetParameter = NULL;
    ConfigGetParameter = NULL;
    ConfigGetParameterType = NULL;
    ConfigGetParameterHelp = NULL;
    ConfigSetDefaultInt = NULL;
    ConfigSetDefaultBool = NULL;
    ConfigSetDefaultString = NULL;
    ConfigGetParamInt = NULL;
    ConfigGetParamBool = NULL;
    ConfigGetParamString = NULL;

    ConfigGetSharedDataFilepath = NULL;
    ConfigGetUserDataPath = NULL;
    ConfigGetUserCachePath = NULL;

    DebugSetCallbacks = NULL;
    DebugSetCoreCompare = NULL;
    DebugSetRunState = NULL;
    DebugGetState = NULL;
    DebugStep = NULL;
    DebugDecodeOp = NULL;
    DebugMemGetRecompInfo = NULL;
    DebugMemGetMemInfo = NULL;
    DebugMemGetPointer = NULL;

    DebugMemRead64 = NULL;
    DebugMemRead32 = NULL;
    DebugMemRead16 = NULL;
    DebugMemRead8 = NULL;

    DebugMemWrite64 = NULL;
    DebugMemWrite32 = NULL;
    DebugMemWrite16 = NULL;
    DebugMemWrite8 = NULL;

    DebugGetCPUDataPtr = NULL;
    DebugBreakpointLookup = NULL;
    DebugBreakpointCommand = NULL;

    DebugBreakpointTriggeredBy = NULL;
    DebugVirtualToPhysical = NULL;

	RdRamReadBuffer = NULL;
	RdRamRead32 = NULL;
	RdRamRead16 = NULL;
	RdRamRead8 = NULL;
	RdRamWrite32 = NULL;
	RdRamWrite16 = NULL;
	RdRamWrite8 = NULL;

	RomReadBuffer = NULL;
	RomRead32 = NULL;
	RomRead16 = NULL;
	RomRead8 = NULL;
	RomWrite32 = NULL;
	RomWrite16 = NULL;
	RomWrite8 = NULL;

	CartGetClockRate = NULL;
	CartGetPC = NULL;
	CartGetRelease = NULL;	
	CartGetName = NULL;
	CartGetManufacturerID = NULL;
	CartGetSerial = NULL;
	CartGetCountryCode = NULL;

	MemoryCacheRefresh = NULL;
	
	CoreSaveOverride = NULL;
	IsMupenReady = NULL;

    /* detach the shared library */
    osal_dynlib_close(CoreHandle);
    CoreHandle = NULL;
    coreStarted = 0;

    return M64ERR_SUCCESS;
}
