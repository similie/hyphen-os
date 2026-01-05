#pragma once
#include <Arduino.h>

struct SocketConfig
{
        String camHost;
        uint16_t camPort;

        String wssHost;
        uint16_t wssPort;
        String wssPath;
        bool tlsInsecure; // <-- ADD THIS
        uint32_t tunnelTimeoutMs;

        static SocketConfig fromBuildFlags()
        {
                SocketConfig c;

#ifdef HYPHEN_CAM_HOST
                c.camHost = HYPHEN_CAM_HOST;
#else
                c.camHost = "192.168.4.216";
#endif

#ifdef HYPHEN_CAM_PORT
                c.camPort = (uint16_t)HYPHEN_CAM_PORT;
#else
                c.camPort = 554;
#endif

#ifdef HYPHEN_WSS_HOST
                c.wssHost = HYPHEN_WSS_HOST;
#else
                c.wssHost = "127.0.0.1";
#endif

#ifdef HYPHEN_WSS_PORT
                c.wssPort = (uint16_t)HYPHEN_WSS_PORT;
#else
                c.wssPort = 7443;
#endif

#ifdef HYPHEN_WSS_PATH
                c.wssPath = HYPHEN_WSS_PATH;
#else
                c.wssPath = "/";
#endif

#ifdef HYPHEN_IPCAM_TUNNEL_TIMEOUT_MS
                c.tunnelTimeoutMs = (uint32_t)HYPHEN_IPCAM_TUNNEL_TIMEOUT_MS;
#else
                c.tunnelTimeoutMs = 45000;
#endif

#ifdef HYPHEN_WSS_INSECURE
                c.tlsInsecure = (HYPHEN_WSS_INSECURE == 1);
#else
                c.tlsInsecure = true; // DEV DEFAULT
#endif

                return c;
        }
};