void WatchdogStart() {
#ifndef DISABLE_WATCHDOG
#error Implement Watchdog init in WatchdogStart()
#endif
}

void WatchdogReset() {
#ifndef DISABLE_WATCHDOG
#error Implement Watchdog reset in WatchdogReset()
#endif
}

extern "C" void CopService() {
    WatchdogReset();
}

