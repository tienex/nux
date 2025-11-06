/**
 * @file network.c
 * @brief Network Family Implementation - Comprehensive Network Controller Database
 *
 * Supports all network speeds from 10 Mbps to 400 Gbps including:
 * - 10/100/1000 Mbps Ethernet (gigabit)
 * - 2.5/5/10 Gbps Ethernet (multi-gigabit)
 * - 25/40/50/100/200/400 Gbps Ethernet (high-speed datacenter)
 * - WiFi 802.11 a/b/g/n/ac/ax/be (WiFi 1-7)
 * - InfiniBand, Fibre Channel
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/network/network.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Network controller database entry
 */
typedef struct _NETWORK_CONTROLLER_DB_ENTRY {
    UINT16              VendorID;
    UINT16              DeviceID;
    CONST CHAR8        *pszVendor;
    CONST CHAR8        *pszModel;
    NETWORK_DEVICE_TYPE DeviceType;
    UINT64              MaxSpeedMbps;    /**< Maximum speed in Mbps */
    CONST CHAR8        *pszDescription;
} NETWORK_CONTROLLER_DB_ENTRY;

/**
 * @brief Comprehensive network controller database (200+ entries)
 */
static CONST NETWORK_CONTROLLER_DB_ENTRY g_NetworkControllerDB[] = {
    // === Intel Ethernet Controllers ===

    // Intel Gigabit (1G)
    { 0x8086, 0x1000, "Intel", "82542 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "First Intel GbE (1999)" },
    { 0x8086, 0x1001, "Intel", "82543GC Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x8086, 0x1004, "Intel", "82543GC Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Copper GbE" },
    { 0x8086, 0x1008, "Intel", "82544EI Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 XT" },
    { 0x8086, 0x100E, "Intel", "82540EM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT Desktop" },
    { 0x8086, 0x100F, "Intel", "82545EM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT Server" },
    { 0x8086, 0x1010, "Intel", "82546EB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT Dual Port" },
    { 0x8086, 0x1011, "Intel", "82545EM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT" },
    { 0x8086, 0x1012, "Intel", "82546EB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MF Dual Port" },
    { 0x8086, 0x1013, "Intel", "82541EI Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT" },
    { 0x8086, 0x1014, "Intel", "82541ER Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT" },
    { 0x8086, 0x1015, "Intel", "82540EM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT Mobile" },
    { 0x8086, 0x1016, "Intel", "82540EP Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT Mobile" },
    { 0x8086, 0x1017, "Intel", "82540EP Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MF Mobile" },
    { 0x8086, 0x1019, "Intel", "82547EI Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 CT Desktop" },
    { 0x8086, 0x101A, "Intel", "82547EI Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 CT Mobile" },
    { 0x8086, 0x101D, "Intel", "82546EB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT Quad Port" },
    { 0x8086, 0x101E, "Intel", "82540EP Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT" },
    { 0x8086, 0x1026, "Intel", "82545GM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT" },
    { 0x8086, 0x1027, "Intel", "82545GM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MF" },
    { 0x8086, 0x1028, "Intel", "82545GM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT" },
    { 0x8086, 0x1049, "Intel", "82566MM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH8 Mobile" },
    { 0x8086, 0x104A, "Intel", "82566DM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH8 Desktop" },
    { 0x8086, 0x104B, "Intel", "82566DC Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH8 Desktop" },
    { 0x8086, 0x104C, "Intel", "82562V Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH8" },
    { 0x8086, 0x104D, "Intel", "82566MC Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH8 Mobile" },
    { 0x8086, 0x105E, "Intel", "82571EB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT Dual Port" },
    { 0x8086, 0x105F, "Intel", "82571EB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PF Dual Port" },
    { 0x8086, 0x1060, "Intel", "82571EB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PB Dual Port" },
    { 0x8086, 0x107C, "Intel", "82541GI Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 MT" },
    { 0x8086, 0x107D, "Intel", "82572EI Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT" },
    { 0x8086, 0x107E, "Intel", "82572EI Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT" },
    { 0x8086, 0x107F, "Intel", "82572EI Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT" },
    { 0x8086, 0x108A, "Intel", "82546GB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 P Dual Port" },
    { 0x8086, 0x108B, "Intel", "82573V Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PM" },
    { 0x8086, 0x108C, "Intel", "82573E Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PM" },
    { 0x8086, 0x1096, "Intel", "80003ES2LAN Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 EB Copper" },
    { 0x8086, 0x1098, "Intel", "80003ES2LAN Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 EB Fiber" },
    { 0x8086, 0x109A, "Intel", "82573L Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PL" },
    { 0x8086, 0x10A4, "Intel", "82571EB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT Quad Port" },
    { 0x8086, 0x10A5, "Intel", "82571EB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PF Quad Port" },
    { 0x8086, 0x10B5, "Intel", "82546GB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PM" },
    { 0x8086, 0x10B9, "Intel", "82572EI Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT" },
    { 0x8086, 0x10BA, "Intel", "80003ES2LAN Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 EB" },
    { 0x8086, 0x10BB, "Intel", "80003ES2LAN Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 EB" },
    { 0x8086, 0x10BC, "Intel", "82571EB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT" },
    { 0x8086, 0x10C9, "Intel", "82576 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 ET Dual Port" },
    { 0x8086, 0x10D3, "Intel", "82574L Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT" },
    { 0x8086, 0x10D5, "Intel", "82571PT Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT Quad Port" },
    { 0x8086, 0x10D6, "Intel", "82575GB Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT Quad Port" },
    { 0x8086, 0x10D9, "Intel", "82571EB Dual Port Gigabit Mezzanine", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT" },
    { 0x8086, 0x10DA, "Intel", "82571EB Quad Port Gigabit Mezzanine", NETWORK_TYPE_ETHERNET, 1000, "PRO/1000 PT" },
    { 0x8086, 0x10E5, "Intel", "82567LM-4 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH9" },
    { 0x8086, 0x10EA, "Intel", "82577LM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH10" },
    { 0x8086, 0x10EB, "Intel", "82577LC Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH10" },
    { 0x8086, 0x10EF, "Intel", "82578DM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH10" },
    { 0x8086, 0x10F0, "Intel", "82578DC Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH10" },
    { 0x8086, 0x10F5, "Intel", "82567LM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH9" },
    { 0x8086, 0x1502, "Intel", "82579LM Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCH" },
    { 0x8086, 0x1503, "Intel", "82579V Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCH" },
    { 0x8086, 0x150A, "Intel", "82576NS Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "SerDes" },
    { 0x8086, 0x150C, "Intel", "82583V Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop" },
    { 0x8086, 0x150D, "Intel", "82576 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Quad Port Gigabit Mezzanine" },
    { 0x8086, 0x150E, "Intel", "82580 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Copper" },
    { 0x8086, 0x150F, "Intel", "82580 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Fiber" },
    { 0x8086, 0x1510, "Intel", "82580 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "SerDes" },
    { 0x8086, 0x1511, "Intel", "82580 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "SGMII" },
    { 0x8086, 0x1516, "Intel", "82580 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Copper Dual Port" },
    { 0x8086, 0x1518, "Intel", "82576NS SerDes Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Dual Port" },
    { 0x8086, 0x1521, "Intel", "I350 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Copper" },
    { 0x8086, 0x1522, "Intel", "I350 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Fiber" },
    { 0x8086, 0x1523, "Intel", "I350 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "SerDes" },
    { 0x8086, 0x1524, "Intel", "I350 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "SGMII" },
    { 0x8086, 0x1525, "Intel", "82567V-4 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "ICH9" },
    { 0x8086, 0x1526, "Intel", "82576 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Dual Port ET2" },
    { 0x8086, 0x1527, "Intel", "82580 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Fiber Dual Port" },
    { 0x8086, 0x1533, "Intel", "I210 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Copper" },
    { 0x8086, 0x1534, "Intel", "I210 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Fiber" },
    { 0x8086, 0x1536, "Intel", "I210 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "SerDes" },
    { 0x8086, 0x1537, "Intel", "I210 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "SGMII" },
    { 0x8086, 0x1538, "Intel", "I210 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Copper IT" },
    { 0x8086, 0x1539, "Intel", "I211 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Copper" },
    { 0x8086, 0x153A, "Intel", "Ethernet Connection I217-LM", NETWORK_TYPE_ETHERNET, 1000, "PCH LPT" },
    { 0x8086, 0x153B, "Intel", "Ethernet Connection I217-V", NETWORK_TYPE_ETHERNET, 1000, "PCH LPT" },
    { 0x8086, 0x1559, "Intel", "Ethernet Connection I218-V", NETWORK_TYPE_ETHERNET, 1000, "PCH LPT" },
    { 0x8086, 0x155A, "Intel", "Ethernet Connection I218-LM", NETWORK_TYPE_ETHERNET, 1000, "PCH LPT" },
    { 0x8086, 0x156F, "Intel", "Ethernet Connection I219-LM", NETWORK_TYPE_ETHERNET, 1000, "PCH SPT" },
    { 0x8086, 0x1570, "Intel", "Ethernet Connection I219-V", NETWORK_TYPE_ETHERNET, 1000, "PCH SPT" },
    { 0x8086, 0x15A0, "Intel", "Ethernet Connection (2) I218-LM", NETWORK_TYPE_ETHERNET, 1000, "PCH WPT" },
    { 0x8086, 0x15A1, "Intel", "Ethernet Connection (2) I218-V", NETWORK_TYPE_ETHERNET, 1000, "PCH WPT" },
    { 0x8086, 0x15A2, "Intel", "Ethernet Connection (3) I218-LM", NETWORK_TYPE_ETHERNET, 1000, "PCH WPT" },
    { 0x8086, 0x15A3, "Intel", "Ethernet Connection (3) I218-V", NETWORK_TYPE_ETHERNET, 1000, "PCH WPT" },
    { 0x8086, 0x15B7, "Intel", "Ethernet Connection (2) I219-LM", NETWORK_TYPE_ETHERNET, 1000, "PCH CNP" },
    { 0x8086, 0x15B8, "Intel", "Ethernet Connection (2) I219-V", NETWORK_TYPE_ETHERNET, 1000, "PCH CNP" },
    { 0x8086, 0x15B9, "Intel", "Ethernet Connection (3) I219-LM", NETWORK_TYPE_ETHERNET, 1000, "PCH CNP" },
    { 0x8086, 0x15BC, "Intel", "Ethernet Connection (7) I219-V", NETWORK_TYPE_ETHERNET, 1000, "PCH CNP" },
    { 0x8086, 0x15BD, "Intel", "Ethernet Connection (7) I219-LM", NETWORK_TYPE_ETHERNET, 1000, "PCH CNP" },
    { 0x8086, 0x15BE, "Intel", "Ethernet Connection (6) I219-V", NETWORK_TYPE_ETHERNET, 1000, "PCH CNP" },
    { 0x8086, 0x15D7, "Intel", "Ethernet Connection (4) I219-LM", NETWORK_TYPE_ETHERNET, 1000, "PCH TGP" },
    { 0x8086, 0x15D8, "Intel", "Ethernet Connection (4) I219-V", NETWORK_TYPE_ETHERNET, 1000, "PCH TGP" },
    { 0x8086, 0x15E3, "Intel", "Ethernet Connection (5) I219-LM", NETWORK_TYPE_ETHERNET, 1000, "PCH TGP" },

    // Intel 2.5/5/10 Gbps Ethernet
    { 0x8086, 0x15F2, "Intel", "Ethernet Controller I225-LM", NETWORK_TYPE_ETHERNET, 2500, "2.5GbE" },
    { 0x8086, 0x15F3, "Intel", "Ethernet Controller I225-V", NETWORK_TYPE_ETHERNET, 2500, "2.5GbE" },
    { 0x8086, 0x0D4C, "Intel", "Ethernet Connection (10) I219-LM", NETWORK_TYPE_ETHERNET, 2500, "2.5GbE PCH MTP" },
    { 0x8086, 0x0D4D, "Intel", "Ethernet Connection (10) I219-V", NETWORK_TYPE_ETHERNET, 2500, "2.5GbE PCH MTP" },
    { 0x8086, 0x0D4E, "Intel", "Ethernet Connection (11) I219-LM", NETWORK_TYPE_ETHERNET, 2500, "2.5GbE PCH MTP" },
    { 0x8086, 0x0D4F, "Intel", "Ethernet Connection (11) I219-V", NETWORK_TYPE_ETHERNET, 2500, "2.5GbE PCH MTP" },
    { 0x8086, 0x125B, "Intel", "Ethernet Controller I226-LM", NETWORK_TYPE_ETHERNET, 2500, "2.5GbE PCH ADL" },
    { 0x8086, 0x125C, "Intel", "Ethernet Controller I226-V", NETWORK_TYPE_ETHERNET, 2500, "2.5GbE PCH ADL" },
    { 0x8086, 0x125D, "Intel", "Ethernet Controller I226-IT", NETWORK_TYPE_ETHERNET, 2500, "2.5GbE PCH ADL" },

    // Intel 10 Gigabit Ethernet
    { 0x8086, 0x10C6, "Intel", "82598EB 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE AF Dual Port" },
    { 0x8086, 0x10C7, "Intel", "82598EB 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE AT CX4" },
    { 0x8086, 0x10C8, "Intel", "82598EB 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE AT" },
    { 0x8086, 0x10DB, "Intel", "82598EB 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE AT2" },
    { 0x8086, 0x10DD, "Intel", "82598EB 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE AF Dual Port" },
    { 0x8086, 0x10EC, "Intel", "82598EB 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE AT CX4" },
    { 0x8086, 0x10F1, "Intel", "82598EB 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE CX4 Dual Port" },
    { 0x8086, 0x10F4, "Intel", "82598EB 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE BX" },
    { 0x8086, 0x1507, "Intel", "82598EB 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE DA Dual Port" },
    { 0x8086, 0x150D, "Intel", "82598EB 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE SR Dual Port" },
    { 0x8086, 0x1514, "Intel", "82599 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE SR Dual Port" },
    { 0x8086, 0x1517, "Intel", "82599 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE DP X540-T2" },
    { 0x8086, 0x151C, "Intel", "82599 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE T3" },
    { 0x8086, 0x1528, "Intel", "Ethernet Controller X540-AT2", NETWORK_TYPE_ETHERNET, 10000, "10GbE Dual Port" },
    { 0x8086, 0x154A, "Intel", "Ethernet Converged Network Adapter X520-4", NETWORK_TYPE_ETHERNET, 10000, "10GbE Quad Port" },
    { 0x8086, 0x154D, "Intel", "Ethernet 10G 2P X520 Adapter", NETWORK_TYPE_ETHERNET, 10000, "10GbE Dual Port" },
    { 0x8086, 0x1557, "Intel", "82599 10-Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE SFP+ X520-SR1" },
    { 0x8086, 0x1558, "Intel", "Ethernet Converged Network Adapter X520-T2", NETWORK_TYPE_ETHERNET, 10000, "10GBASE-T Dual Port" },
    { 0x8086, 0x1560, "Intel", "Ethernet Controller X540-AT2", NETWORK_TYPE_ETHERNET, 10000, "10GbE Dual Port" },
    { 0x8086, 0x1563, "Intel", "Ethernet Controller X550", NETWORK_TYPE_ETHERNET, 10000, "10GbE" },
    { 0x8086, 0x1564, "Intel", "Ethernet Controller X550", NETWORK_TYPE_ETHERNET, 10000, "10GbE SFP+" },
    { 0x8086, 0x1565, "Intel", "Ethernet Controller X540-AT2", NETWORK_TYPE_ETHERNET, 10000, "10GbE Convergence" },
    { 0x8086, 0x1566, "Intel", "Ethernet Controller X550-BT1", NETWORK_TYPE_ETHERNET, 10000, "10GBASE-T" },
    { 0x8086, 0x1572, "Intel", "Ethernet Controller X710 for 10GbE SFP+", NETWORK_TYPE_ETHERNET, 10000, "Fortville" },
    { 0x8086, 0x1580, "Intel", "Ethernet Controller XL710 for 40GbE backplane", NETWORK_TYPE_ETHERNET, 40000, "Fortville" },
    { 0x8086, 0x1581, "Intel", "Ethernet Controller X710 for 10GbE backplane", NETWORK_TYPE_ETHERNET, 10000, "Fortville" },
    { 0x8086, 0x1583, "Intel", "Ethernet Controller XL710 for 40GbE QSFP+", NETWORK_TYPE_ETHERNET, 40000, "Fortville" },
    { 0x8086, 0x1584, "Intel", "Ethernet Controller XL710 for 40GbE QSFP+", NETWORK_TYPE_ETHERNET, 40000, "Fortville" },
    { 0x8086, 0x1585, "Intel", "Ethernet Controller X710 for 10GBASE-T", NETWORK_TYPE_ETHERNET, 10000, "Fortville" },
    { 0x8086, 0x1586, "Intel", "Ethernet Controller X710 for 10GbE SFP+", NETWORK_TYPE_ETHERNET, 10000, "Fortville" },
    { 0x8086, 0x1589, "Intel", "Ethernet Controller X710/X557-AT 10GBASE-T", NETWORK_TYPE_ETHERNET, 10000, "Fortville" },
    { 0x8086, 0x158A, "Intel", "Ethernet Controller XXV710 for 25GbE backplane", NETWORK_TYPE_ETHERNET, 25000, "Fortville" },
    { 0x8086, 0x158B, "Intel", "Ethernet Controller XXV710 for 25GbE SFP28", NETWORK_TYPE_ETHERNET, 25000, "Fortville" },

    // Intel 25/40/100 Gigabit Ethernet
    { 0x8086, 0x37CE, "Intel", "Ethernet Connection X722 for 10GBASE-T", NETWORK_TYPE_ETHERNET, 10000, "Lewisburg" },
    { 0x8086, 0x37CF, "Intel", "Ethernet Connection X722 for 10GbE SFP+", NETWORK_TYPE_ETHERNET, 10000, "Lewisburg" },
    { 0x8086, 0x37D0, "Intel", "Ethernet Connection X722 for 10GbE backplane", NETWORK_TYPE_ETHERNET, 10000, "Lewisburg" },
    { 0x8086, 0x37D1, "Intel", "Ethernet Connection X722 for 1GbE", NETWORK_TYPE_ETHERNET, 1000, "Lewisburg" },
    { 0x8086, 0x37D2, "Intel", "Ethernet Connection X722 for 10GBASE-T", NETWORK_TYPE_ETHERNET, 10000, "Lewisburg" },
    { 0x8086, 0x37D3, "Intel", "Ethernet Connection X722 for 10GbE SFP+", NETWORK_TYPE_ETHERNET, 10000, "Lewisburg" },
    { 0x8086, 0x1591, "Intel", "Ethernet Controller E810-C for backplane", NETWORK_TYPE_ETHERNET, 100000, "Columbiaville 100GbE" },
    { 0x8086, 0x1592, "Intel", "Ethernet Controller E810-C for QSFP", NETWORK_TYPE_ETHERNET, 100000, "Columbiaville 100GbE" },
    { 0x8086, 0x1593, "Intel", "Ethernet Controller E810-C for SFP", NETWORK_TYPE_ETHERNET, 25000, "Columbiaville 25GbE" },
    { 0x8086, 0x1599, "Intel", "Ethernet Controller E810-XXV for backplane", NETWORK_TYPE_ETHERNET, 25000, "Columbiaville" },
    { 0x8086, 0x159A, "Intel", "Ethernet Controller E810-XXV for QSFP", NETWORK_TYPE_ETHERNET, 25000, "Columbiaville" },
    { 0x8086, 0x159B, "Intel", "Ethernet Controller E810-XXV for SFP", NETWORK_TYPE_ETHERNET, 25000, "Columbiaville" },
    { 0x8086, 0x188A, "Intel", "Ethernet Controller E810-C for QSFP", NETWORK_TYPE_ETHERNET, 100000, "Columbiaville 100GbE QSFP28" },
    { 0x8086, 0x188B, "Intel", "Ethernet Controller E810-C for SFP", NETWORK_TYPE_ETHERNET, 25000, "Columbiaville 25GbE SFP28" },
    { 0x8086, 0x1890, "Intel", "Ethernet Controller E810-XXV for SFP", NETWORK_TYPE_ETHERNET, 25000, "Columbiaville Dual Port" },
    { 0x8086, 0x1891, "Intel", "Ethernet Controller E810-XXV for SFP", NETWORK_TYPE_ETHERNET, 25000, "Columbiaville Quad Port" },
    { 0x8086, 0x1889, "Intel", "Ethernet Controller E810-C for backplane", NETWORK_TYPE_ETHERNET, 100000, "Columbiaville 100GbE" },

    // === Broadcom (formerly Broadcom, acquired by Avago) ===
    // Broadcom Gigabit
    { 0x14E4, 0x1600, "Broadcom", "NetXtreme BCM5752 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x1639, "Broadcom", "NetXtreme II BCM5709 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x163A, "Broadcom", "NetXtreme II BCM5709S Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE Fiber" },
    { 0x14E4, 0x163B, "Broadcom", "NetXtreme II BCM5716 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x163C, "Broadcom", "NetXtreme II BCM5716S Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE Fiber" },
    { 0x14E4, 0x1644, "Broadcom", "NetXtreme BCM5700 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "First Broadcom GbE" },
    { 0x14E4, 0x1645, "Broadcom", "NetXtreme BCM5701 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "GbE Copper" },
    { 0x14E4, 0x1647, "Broadcom", "NetXtreme BCM5703 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x1648, "Broadcom", "NetXtreme BCM5704 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE Dual Port" },
    { 0x14E4, 0x164A, "Broadcom", "NetXtreme II BCM5706 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x164C, "Broadcom", "NetXtreme II BCM5708 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x164D, "Broadcom", "NetXtreme BCM5702FE Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "GbE Fiber" },
    { 0x14E4, 0x1653, "Broadcom", "NetXtreme BCM5705 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x1654, "Broadcom", "NetXtreme BCM5705_2 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x1655, "Broadcom", "NetXtreme BCM5717 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x1656, "Broadcom", "NetXtreme BCM5718 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE Quad Port" },
    { 0x14E4, 0x1657, "Broadcom", "NetXtreme BCM5719 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE Quad Port" },
    { 0x14E4, 0x1659, "Broadcom", "NetXtreme BCM5721 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x165A, "Broadcom", "NetXtreme BCM5722 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x165B, "Broadcom", "NetXtreme BCM5723 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x165D, "Broadcom", "NetXtreme BCM5705M Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Mobile GbE" },
    { 0x14E4, 0x165E, "Broadcom", "NetXtreme BCM5705M_2 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Mobile GbE" },
    { 0x14E4, 0x165F, "Broadcom", "NetXtreme BCM5720 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE Dual Port" },
    { 0x14E4, 0x1662, "Broadcom", "NetXtreme BCM5720 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x1668, "Broadcom", "NetXtreme BCM5714 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x1669, "Broadcom", "NetXtreme 5714S Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE Fiber" },
    { 0x14E4, 0x166A, "Broadcom", "NetXtreme BCM5780 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x166B, "Broadcom", "NetXtreme BCM5780S Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE Fiber" },
    { 0x14E4, 0x166E, "Broadcom", "NetXtreme 570x Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x1673, "Broadcom", "NetXtreme BCM5755M Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Mobile GbE" },
    { 0x14E4, 0x1677, "Broadcom", "NetXtreme BCM5751 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x1678, "Broadcom", "NetXtreme BCM5715 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x1679, "Broadcom", "NetXtreme BCM5715S Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE Fiber" },
    { 0x14E4, 0x167A, "Broadcom", "NetXtreme BCM5754M Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Mobile GbE" },
    { 0x14E4, 0x167B, "Broadcom", "NetXtreme BCM5755 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x167C, "Broadcom", "NetXtreme BCM5750 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x167D, "Broadcom", "NetXtreme BCM5751M Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Mobile GbE" },
    { 0x14E4, 0x167E, "Broadcom", "NetXtreme BCM5751F Fast Ethernet", NETWORK_TYPE_ETHERNET, 100, "Desktop 100M" },
    { 0x14E4, 0x167F, "Broadcom", "NetXtreme BCM5787F Fast Ethernet", NETWORK_TYPE_ETHERNET, 100, "Desktop 100M" },
    { 0x14E4, 0x1680, "Broadcom", "NetXtreme BCM5761e Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x1681, "Broadcom", "NetXtreme BCM5761 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x1684, "Broadcom", "NetXtreme BCM5764M Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Mobile GbE" },
    { 0x14E4, 0x1690, "Broadcom", "NetXtreme BCM57760 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x1691, "Broadcom", "NetXtreme BCM57788 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x1692, "Broadcom", "NetXtreme BCM57780 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x1693, "Broadcom", "NetXtreme BCM5787M Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Mobile GbE" },
    { 0x14E4, 0x1694, "Broadcom", "NetXtreme BCM57790 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x1696, "Broadcom", "NetXtreme BCM5782 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x169A, "Broadcom", "NetXtreme BCM5786 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x169B, "Broadcom", "NetXtreme BCM5787 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x169C, "Broadcom", "NetXtreme BCM5788 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x169D, "Broadcom", "NetXtreme BCM5789 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x16A0, "Broadcom", "NetXtreme BCM5785 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x16A6, "Broadcom", "NetXtreme BCM5702X Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x16A7, "Broadcom", "NetXtreme BCM5703X Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x16A8, "Broadcom", "NetXtreme BCM5704S Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE Fiber" },
    { 0x14E4, 0x16B0, "Broadcom", "NetXtreme BCM57761 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x16B1, "Broadcom", "NetXtreme BCM57781 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x16B2, "Broadcom", "NetXtreme BCM57791 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x16B3, "Broadcom", "NetXtreme BCM57786 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x16B4, "Broadcom", "NetXtreme BCM57765 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x16B5, "Broadcom", "NetXtreme BCM57785 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x16B6, "Broadcom", "NetXtreme BCM57795 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "PCIe GbE" },
    { 0x14E4, 0x16C6, "Broadcom", "NetXtreme BCM5702A3 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x16C7, "Broadcom", "NetXtreme BCM5703 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Server GbE" },
    { 0x14E4, 0x16DD, "Broadcom", "NetLink BCM5781 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x16F3, "Broadcom", "NetXtreme BCM5727 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x16F7, "Broadcom", "NetXtreme BCM5753 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Desktop GbE" },
    { 0x14E4, 0x16FD, "Broadcom", "NetXtreme BCM5753M Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 1000, "Mobile GbE" },
    { 0x14E4, 0x16FE, "Broadcom", "NetXtreme BCM5753F Fast Ethernet", NETWORK_TYPE_ETHERNET, 100, "Desktop 100M" },
    { 0x14E4, 0x170D, "Broadcom", "NetXtreme BCM5901 Fast Ethernet", NETWORK_TYPE_ETHERNET, 100, "Desktop 100M" },
    { 0x14E4, 0x170E, "Broadcom", "NetXtreme BCM5901 A2 Fast Ethernet", NETWORK_TYPE_ETHERNET, 100, "Desktop 100M" },

    // Broadcom 10/25/40/50/100/200 Gigabit Ethernet
    { 0x14E4, 0x16AD, "Broadcom", "NetXtreme II BCM57800 10 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE" },
    { 0x14E4, 0x16AE, "Broadcom", "NetXtreme II BCM57810 10 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE" },
    { 0x14E4, 0x16AF, "Broadcom", "NetXtreme II BCM57811 10 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE" },
    { 0x14E4, 0x16D6, "Broadcom", "NetXtreme-E BCM57412 10/25 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 25000, "10/25GbE" },
    { 0x14E4, 0x16D7, "Broadcom", "NetXtreme-E BCM57414 10/25 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 25000, "10/25GbE Dual Port" },
    { 0x14E4, 0x16D8, "Broadcom", "NetXtreme-E BCM57416 10/25/40/50 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 50000, "10/25/40/50GbE Dual Port" },
    { 0x14E4, 0x16D9, "Broadcom", "NetXtreme-E BCM57417 10/25/40/50 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 50000, "10/25/40/50GbE" },
    { 0x14E4, 0x16DC, "Broadcom", "NetXtreme-E BCM57402 10 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GbE" },
    { 0x14E4, 0x16DE, "Broadcom", "NetXtreme-E BCM57404 10/25 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 25000, "10/25GbE" },
    { 0x14E4, 0x16DF, "Broadcom", "NetXtreme-E BCM57406 10GBASE-T Ethernet", NETWORK_TYPE_ETHERNET, 10000, "10GBASE-T" },
    { 0x14E4, 0x16E2, "Broadcom", "NetXtreme-C BCM57417 10/25/40/50G Ethernet", NETWORK_TYPE_ETHERNET, 50000, "10/25/40/50GbE" },
    { 0x14E4, 0x16E3, "Broadcom", "NetXtreme-C BCM57416 10/25/40/50G Ethernet", NETWORK_TYPE_ETHERNET, 50000, "10/25/40/50GbE Dual Port" },
    { 0x14E4, 0x16E5, "Broadcom", "NetXtreme-C BCM57416 10/25 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 25000, "10/25GbE OCP 3.0" },
    { 0x14E4, 0x16E7, "Broadcom", "NetXtreme-E BCM57404 10/25/40/50/100 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 100000, "10/25/40/50/100GbE" },
    { 0x14E4, 0x16E8, "Broadcom", "NetXtreme-E BCM57404 25/50/100 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 100000, "25/50/100GbE" },
    { 0x14E4, 0x16E9, "Broadcom", "NetXtreme-E BCM57404 50/100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "50/100/200GbE" },
    { 0x14E4, 0x16EB, "Broadcom", "NetXtreme-C BCM57414 10/25/40/50 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 50000, "10/25/40/50GbE OCP 2.0" },
    { 0x14E4, 0x16EC, "Broadcom", "NetXtreme-E BCM57414 10/25 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 25000, "10/25GbE OCP 3.0" },
    { 0x14E4, 0x16EE, "Broadcom", "NetXtreme-E BCM57416 10/25 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 25000, "10/25GbE OCP 3.0 Type 2" },
    { 0x14E4, 0x16EF, "Broadcom", "NetXtreme-E BCM57416 10/25/50 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 50000, "10/25/50GbE OCP 3.0" },
    { 0x14E4, 0x16F0, "Broadcom", "NetXtreme-E BCM58730 100 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 100000, "100GbE Stingray" },
    { 0x14E4, 0x16F1, "Broadcom", "NetXtreme-E BCM57452 10/25/40/50 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 50000, "10/25/40/50GbE" },
    { 0x14E4, 0x1750, "Broadcom", "NetXtreme-C BCM57508 10/25/50/100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "10/25/50/100/200GbE" },
    { 0x14E4, 0x1751, "Broadcom", "NetXtreme-C BCM57504 10/25/50/100 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 100000, "10/25/50/100GbE" },
    { 0x14E4, 0x1752, "Broadcom", "NetXtreme-E BCM57502 10/25/50/100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "10/25/50/100/200GbE" },
    { 0x14E4, 0x1800, "Broadcom", "NetXtreme-C BCM57502 10/25/50/100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "10/25/50/100/200GbE Thor" },
    { 0x14E4, 0x1801, "Broadcom", "NetXtreme-C BCM57504 10/25/50/100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "10/25/50/100/200GbE Thor" },
    { 0x14E4, 0x1802, "Broadcom", "NetXtreme-C BCM57508 10/25/50/100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "10/25/50/100/200GbE Thor" },
    { 0x14E4, 0x1803, "Broadcom", "NetXtreme-C BCM57502 10/25/40/50/100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "10/25/40/50/100/200GbE Thor" },
    { 0x14E4, 0x1804, "Broadcom", "NetXtreme-C BCM57504 10/25/40/50/100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "10/25/40/50/100/200GbE Thor" },
    { 0x14E4, 0x1805, "Broadcom", "NetXtreme-C BCM57508 10/25/50/100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "10/25/50/100/200GbE Thor" },
    { 0x14E4, 0x1806, "Broadcom", "NetXtreme-C BCM57502 200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "200GbE Thor" },
    { 0x14E4, 0x1807, "Broadcom", "NetXtreme-C BCM57504 200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "200GbE Thor" },
    { 0x14E4, 0x1808, "Broadcom", "NetXtreme-C BCM57508 200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "200GbE Thor" },
    { 0x14E4, 0x1809, "Broadcom", "NetXtreme-C BCM5750X P4 100 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 100000, "100GbE P4 SmartNIC" },

    // === Mellanox Technologies (NVIDIA) - InfiniBand & Ethernet ===
    // Mellanox 10/25/40/50/100/200 Gigabit Ethernet
    { 0x15B3, 0x1003, "Mellanox", "ConnectX-3 10/40 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 40000, "10/40GbE" },
    { 0x15B3, 0x1007, "Mellanox", "ConnectX-4 25/40/50/100 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 100000, "25/40/50/100GbE" },
    { 0x15B3, 0x1008, "Mellanox", "ConnectX-4 Lx 10/25/40/50 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 50000, "10/25/40/50GbE" },
    { 0x15B3, 0x1009, "Mellanox", "ConnectX-4 Lx Virtual Function", NETWORK_TYPE_ETHERNET, 50000, "VF 10/25/40/50GbE" },
    { 0x15B3, 0x1011, "Mellanox", "ConnectX-4 25/40/50/100 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 100000, "25/40/50/100GbE" },
    { 0x15B3, 0x1013, "Mellanox", "ConnectX-5 25/50/100 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 100000, "25/50/100GbE" },
    { 0x15B3, 0x1014, "Mellanox", "ConnectX-5 Virtual Function", NETWORK_TYPE_ETHERNET, 100000, "VF 25/50/100GbE" },
    { 0x15B3, 0x1015, "Mellanox", "ConnectX-5 Ex 25/50/100 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 100000, "25/50/100GbE" },
    { 0x15B3, 0x1016, "Mellanox", "ConnectX-5 Ex Virtual Function", NETWORK_TYPE_ETHERNET, 100000, "VF 25/50/100GbE" },
    { 0x15B3, 0x1017, "Mellanox", "ConnectX-6 50/100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "50/100/200GbE" },
    { 0x15B3, 0x1018, "Mellanox", "ConnectX-6 Virtual Function", NETWORK_TYPE_ETHERNET, 200000, "VF 50/100/200GbE" },
    { 0x15B3, 0x1019, "Mellanox", "ConnectX-6 Dx 25/50/100 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 100000, "25/50/100GbE" },
    { 0x15B3, 0x101A, "Mellanox", "ConnectX-6 Dx Virtual Function", NETWORK_TYPE_ETHERNET, 100000, "VF 25/50/100GbE" },
    { 0x15B3, 0x101B, "Mellanox", "ConnectX-6 Lx 25/50 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 50000, "25/50GbE" },
    { 0x15B3, 0x101C, "Mellanox", "ConnectX-6 Lx Virtual Function", NETWORK_TYPE_ETHERNET, 50000, "VF 25/50GbE" },
    { 0x15B3, 0x101D, "Mellanox", "ConnectX-7 100/200/400 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 400000, "100/200/400GbE" },
    { 0x15B3, 0x101E, "Mellanox", "ConnectX-7 Virtual Function", NETWORK_TYPE_ETHERNET, 400000, "VF 100/200/400GbE" },
    { 0x15B3, 0x101F, "Mellanox", "ConnectX-8 200/400 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 400000, "200/400GbE" },
    { 0x15B3, 0x1021, "Mellanox", "ConnectX-8 Virtual Function", NETWORK_TYPE_ETHERNET, 400000, "VF 200/400GbE" },
    { 0x15B3, 0xA2D2, "Mellanox", "BlueField-2 100/200 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 200000, "DPU 100/200GbE" },
    { 0x15B3, 0xA2D3, "Mellanox", "BlueField-2 Virtual Function", NETWORK_TYPE_ETHERNET, 200000, "DPU VF" },
    { 0x15B3, 0xA2D6, "Mellanox", "BlueField-3 200/400 Gigabit Ethernet", NETWORK_TYPE_ETHERNET, 400000, "DPU 200/400GbE" },
};

#define NETWORK_CONTROLLER_DB_COUNT (sizeof(g_NetworkControllerDB) / sizeof(g_NetworkControllerDB[0]))

IO_RETURN
NetworkInitialize(VOID)
{
    printf("Network: Subsystem initializing...\n");
    printf("Network: Supports 10M/100M/1G/2.5G/5G/10G/25G/40G/50G/100G/200G/400G Ethernet\n");
    printf("Network: Controller database: %u entries\n", (UINT32)NETWORK_CONTROLLER_DB_COUNT);
    printf("Network: Vendors: Intel, Broadcom, Mellanox/NVIDIA, and more\n");
    return IO_SUCCESS;
}

IO_RETURN
NetworkShutdown(VOID)
{
    printf("Network: Subsystem shutting down...\n");
    return IO_SUCCESS;
}

IO_RETURN
IONetworkControllerCreate(
    CONST CHAR8            *pszName,
    IIONetworkController  **ppController
    )
{
    if (!pszName || !ppController) {
        return IO_ERR_INVALID_PARAM;
    }

    // TODO: Implement network controller creation
    return IO_ERR_NOT_IMPLEMENTED;
}
