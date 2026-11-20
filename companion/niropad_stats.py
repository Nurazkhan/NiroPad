"""
NiroPad PC Companion Daemon
Pulls CPU, RAM, Volume, Wi-Fi, and Windows Media (Spotify, YouTube, etc.)
and sends real-time telemetry to the NiroPad OLED via QMK Raw HID.
"""

import sys
import time
import re
import subprocess
import asyncio
import hid
import psutil

try:
    from winsdk.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager
    WINSDK_AVAILABLE = True
except ImportError:
    WINSDK_AVAILABLE = False

try:
    from pycaw.pycaw import AudioUtilities
    PYCAW_AVAILABLE = True
except ImportError:
    PYCAW_AVAILABLE = False

VENDOR_ID = 0x4E49
PRODUCT_ID = 0x6060
RAW_EPSIZE = 32

def find_niropad():
    """Locate the NiroPad Raw HID endpoint."""
    devices = hid.enumerate(VENDOR_ID, PRODUCT_ID)
    if not devices:
        return None

    # Specifically find the Raw HID interface (usage_page 0xFF60)
    for d in devices:
        if d.get("usage_page") == 0xFF60:
            try:
                device = hid.device()
                device.open_path(d["path"])
                device.set_nonblocking(True)
                return device
            except Exception:
                pass

    # Secondary check: find interface with raw usage
    for d in devices:
        if d.get("usage") == 0x61:
            try:
                device = hid.device()
                device.open_path(d["path"])
                device.set_nonblocking(True)
                return device
            except Exception:
                pass

    return None

def send_packet(dev, cmd_id, payload_bytes):
    """Send a 32-byte Raw HID report (prefixed by 0x00 report ID for Windows)."""
    buf = bytearray(RAW_EPSIZE)
    buf[0] = cmd_id
    length = min(len(payload_bytes), RAW_EPSIZE - 1)
    buf[1:1 + length] = payload_bytes[:length]

    # Windows HID report write requires report ID (0x00) prefix
    packet = bytes([0x00]) + bytes(buf)
    try:
        dev.write(packet)
        return True
    except Exception:
        return False

def clean_ascii(text, max_len=18):
    """Filter string to only printable standard ASCII characters."""
    if not text:
        return ""
    # Keep only ASCII 32 (space) through 126 (~)
    cleaned = re.sub(r"[^\x20-\x7E]", "", text).strip()
    return cleaned[:max_len]

def get_master_volume():
    """Retrieve current Windows Master Volume (0-100%)."""
    if not PYCAW_AVAILABLE:
        return 50
    try:
        sp = AudioUtilities.GetSpeakers()
        if sp and sp.EndpointVolume:
            # GetMasterVolumeLevelScalar returns float 0.0 to 1.0
            scalar = sp.EndpointVolume.GetMasterVolumeLevelScalar()
            return max(0, min(100, int(round(scalar * 100))))
    except Exception:
        pass
    return 50

def get_wifi_info():
    """Retrieve current Wi-Fi SSID and Signal strength % via netsh."""
    try:
        out = subprocess.check_output(
            ["netsh", "wlan", "show", "interfaces"],
            stderr=subprocess.DEVNULL,
            text=True,
            encoding="utf-8",
            errors="ignore"
        )
        ssid_match = re.search(r"^\s*SSID\s*:\s*(.+)$", out, re.MULTILINE)
        sig_match = re.search(r"^\s*Signal\s*:\s*(\d+)%", out, re.MULTILINE)

        ssid = ssid_match.group(1).strip() if ssid_match else "Online"
        sig = int(sig_match.group(1)) if sig_match else 80
        return clean_ascii(ssid, 16), max(0, min(100, sig))
    except Exception:
        return "Online", 80

async def get_now_playing():
    """Get active media track title and artist via Windows Media Session."""
    if not WINSDK_AVAILABLE:
        return "No Media", "Idle"

    try:
        mgr = await MediaManager.request_async()
        session = mgr.get_current_session()
        if not session:
            return "No Media", "Idle"

        props = await session.try_get_media_properties_async()
        if not props:
            return "No Media", "Idle"

        title = clean_ascii(props.title, 18) if props.title else "No Title"
        artist = clean_ascii(props.artist, 18) if props.artist else "Unknown"
        return (title if title else "No Title"), (artist if artist else "Unknown")
    except Exception:
        return "No Media", "Idle"

def main():
    print("=" * 55)
    print("         NiroPad OLED Companion Telemetry       ")
    print("=" * 55)
    print(f"Searching for NiroPad (VID: 0x{VENDOR_ID:04X}, PID: 0x{PRODUCT_ID:04X})...")

    # Prime psutil CPU calculation
    psutil.cpu_percent(interval=None)

    dev = None
    last_media_check = 0
    cached_title = "No Media"
    cached_artist = "Idle"
    last_wifi_check = 0
    cached_ssid = "Online"
    cached_sig = 80

    print("Searching for NiroPad on USB (VID: 0x4E49, PID: 0x6060)...")

    while True:
        try:
            if dev is None:
                dev = find_niropad()
                if dev is None:
                    time.sleep(1.5)
                    continue
                print("\n[+] NiroPad connected! Streaming system telemetry...")

            # 1. Gather System Stats
            cpu = max(0, min(100, int(psutil.cpu_percent(interval=None))))
            ram = max(0, min(100, int(psutil.virtual_memory().percent)))
            vol = get_master_volume()
            gpu_temp = 45

            # 2. Update Wi-Fi stats periodically (every 5 seconds)
            now = time.time()
            if now - last_wifi_check > 5:
                cached_ssid, cached_sig = get_wifi_info()
                last_wifi_check = now

            # 3. Update Now Playing periodically (every 2 seconds)
            if now - last_media_check > 2:
                try:
                    cached_title, cached_artist = asyncio.run(get_now_playing())
                except Exception:
                    pass
                last_media_check = now

            # 4. Transmit Telemetry (with tiny delays to prevent USB buffer collisions)
            # Packet 0x51: System Stats [0x51, cpu, ram, gpu, wifi, vol]
            stats_payload = bytes([cpu, ram, gpu_temp, cached_sig, vol])
            if not send_packet(dev, 0x51, stats_payload):
                raise IOError("Device disconnected")

            time.sleep(0.01)

            # Packet 0x52: Media Title
            title_payload = cached_title.encode("ascii", errors="replace")
            send_packet(dev, 0x52, title_payload)

            time.sleep(0.01)

            # Packet 0x53: Media Artist
            artist_payload = cached_artist.encode("ascii", errors="replace")
            send_packet(dev, 0x53, artist_payload)

            time.sleep(0.01)

            # Packet 0x54: Network Info
            net_payload = cached_ssid.encode("ascii", errors="replace")
            send_packet(dev, 0x54, net_payload)

            # Console status
            print(f"\r[Telemetry] CPU:{cpu:2d}% | RAM:{ram:2d}% | VOL:{vol:2d}% | WiFi:{cached_sig:2d}% | {cached_title}", end="", flush=True)

            time.sleep(1.0)

        except (IOError, OSError):
            print(f"\n[-] NiroPad connection lost. Retrying...")
            if dev:
                try:
                    dev.close()
                except Exception:
                    pass
            dev = None
            time.sleep(2)
        except KeyboardInterrupt:
            print("\nExiting NiroPad Companion...")
            if dev:
                dev.close()
            break

if __name__ == "__main__":
    main()
