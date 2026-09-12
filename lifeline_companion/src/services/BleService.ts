import { NUS_SERVICE_UUID, NUS_RX_UUID, NUS_TX_UUID, LifeLineDevice } from '../constants/ble';
import { stringToBase64, base64ToString } from '../utils/base64';

// Dynamic import safeguard to prevent crashes in web environments
let BleManager: any = null;
let bleManagerInstance: any = null;

try {
  const blePlx = require('react-native-ble-plx');
  BleManager = blePlx.BleManager;
  bleManagerInstance = new BleManager();
} catch (err) {
  // Running on Web / Bluefy
}

export class BleService {
  // Native BLE state
  private activeDevice: any = null;
  private notificationSubscription: any = null;
  private packetBuffer: string = '';

  // Web Bluetooth state (for Bluefy on iOS and Chrome on Web)
  private webDevice: any = null;
  private webRxChar: any = null;
  private webTxChar: any = null;
  private isWebBleMode: boolean = false;

  public isNativeBleSupported(): boolean {
    return bleManagerInstance !== null;
  }

  public isWebBleSupported(): boolean {
    return typeof navigator !== 'undefined' && 'bluetooth' in navigator;
  }

  public async requestPermissions(): Promise<boolean> {
    return true;
  }

  public async startScan(
    onDeviceFound: (device: LifeLineDevice) => void,
    onError: (error: any) => void
  ): Promise<void> {
    // 1. Web Bluetooth Mode (iPhone Bluefy / Desktop Chrome)
    if (this.isWebBleSupported()) {
      try {
        const nav = navigator as any;
        const device = await nav.bluetooth.requestDevice({
          filters: [
            { namePrefix: 'LifeLine' },
            { namePrefix: 'TX' },
            { namePrefix: 'RX' },
            { services: [NUS_SERVICE_UUID] },
          ],
          optionalServices: [NUS_SERVICE_UUID],
        });

        if (device) {
          this.webDevice = device;
          this.isWebBleMode = true;
          onDeviceFound({
            id: device.id || 'WEB-BLE-DEVICE',
            name: device.name || 'LifeLine Field Node',
            rssi: -65,
            isSimulated: false,
          });
        }
      } catch (err: any) {
        if (err.name !== 'NotFoundError') {
          onError(err);
        }
      }
      return;
    }

    // 2. Native BLE Mode (React Native / Android APK)
    if (bleManagerInstance) {
      try {
        this.isWebBleMode = false;
        bleManagerInstance.startDeviceScan(
          null,
          { allowDuplicates: false },
          (error: any, scannedDevice: any) => {
            if (error) {
              onError(error);
              return;
            }

            if (scannedDevice && (scannedDevice.name || scannedDevice.localName)) {
              const name = scannedDevice.name || scannedDevice.localName || 'Unknown LifeLine';
              if (name.includes('LifeLine') || name.includes('TX') || name.includes('RX')) {
                onDeviceFound({
                  id: scannedDevice.id,
                  name: name,
                  rssi: scannedDevice.rssi || -70,
                  isSimulated: false,
                });
              }
            }
          }
        );
      } catch (e) {
        onError(e);
      }
      return;
    }

    // Neither supported (Standard Expo Go)
    onError(
      new Error(
        'Bluetooth is not supported in Expo Go. Open this page in the Bluefy app on your iPhone or build an Android APK!'
      )
    );
  }

  public stopScan(): void {
    if (bleManagerInstance) {
      try {
        bleManagerInstance.stopDeviceScan();
      } catch (e) {
        console.warn('Error stopping BLE scan:', e);
      }
    }
  }

  public async connect(
    deviceId: string,
    onPacketReceived: (rawPacket: string) => void,
    onDisconnected: () => void
  ): Promise<LifeLineDevice> {
    this.stopScan();

    // 1. Connect via Web Bluetooth (Bluefy / Chrome)
    if (this.isWebBleMode && this.webDevice) {
      try {
        this.webDevice.addEventListener('gattserverdisconnected', () => {
          this.cleanup();
          onDisconnected();
        });

        const server = await this.webDevice.gatt.connect();
        const service = await server.getPrimaryService(NUS_SERVICE_UUID);
        this.webRxChar = await service.getCharacteristic(NUS_RX_UUID);
        this.webTxChar = await service.getCharacteristic(NUS_TX_UUID);

        await this.webTxChar.startNotifications();
        this.webTxChar.addEventListener('characteristicvaluechanged', (event: any) => {
          const decoder = new TextDecoder();
          const chunk = decoder.decode(event.target.value);
          this.handleIncomingChunk(chunk, onPacketReceived);
        });

        return {
          id: this.webDevice.id || deviceId,
          name: this.webDevice.name || 'LifeLine Field Node',
          rssi: -65,
          isSimulated: false,
        };
      } catch (e: any) {
        console.error('[Web BLE] Connection failed:', e);
        throw e;
      }
    }

    // 2. Connect via Native BLE
    if (bleManagerInstance) {
      const device = await bleManagerInstance.connectToDevice(deviceId);
      this.activeDevice = device;

      await device.discoverAllServicesAndCharacteristics();

      device.onDisconnected(() => {
        this.cleanup();
        onDisconnected();
      });

      this.notificationSubscription = device.monitorCharacteristicForService(
        NUS_SERVICE_UUID,
        NUS_TX_UUID,
        (error: any, characteristic: any) => {
          if (error) {
            console.warn('[BLE] Monitor error:', error);
            return;
          }

          if (characteristic?.value) {
            try {
              const decoded = base64ToString(characteristic.value);
              this.handleIncomingChunk(decoded, onPacketReceived);
            } catch (err) {
              console.error('[BLE] Base64 decode error:', err);
            }
          }
        }
      );

      return {
        id: device.id,
        name: device.name || 'LifeLine Field Node',
        rssi: device.rssi || -65,
        isSimulated: false,
      };
    }

    throw new Error('Bluetooth LE not supported in this runtime.');
  }

  private handleIncomingChunk(chunk: string, onPacketReceived: (packet: string) => void): void {
    this.packetBuffer += chunk;
    const lines = this.packetBuffer.split('\n');
    while (lines.length > 1) {
      const line = lines.shift()?.trim();
      if (line && line.length > 0) {
        onPacketReceived(line);
      }
    }
    this.packetBuffer = lines[0] || '';
  }

  public async sendCommand(command: string): Promise<boolean> {
    const payload = command.endsWith('\n') ? command : `${command}\n`;

    // 1. Web Bluetooth Transmit
    if (this.isWebBleMode && this.webRxChar) {
      const encoder = new TextEncoder();
      const data = encoder.encode(payload);
      await this.webRxChar.writeValue(data);
      return true;
    }

    // 2. Native BLE Transmit
    if (this.activeDevice) {
      const base64Data = stringToBase64(payload);
      await this.activeDevice.writeCharacteristicWithResponseForService(
        NUS_SERVICE_UUID,
        NUS_RX_UUID,
        base64Data
      );
      return true;
    }

    throw new Error('No LifeLine device currently connected.');
  }

  public async disconnect(): Promise<void> {
    if (this.isWebBleMode && this.webDevice?.gatt?.connected) {
      try {
        await this.webDevice.gatt.disconnect();
      } catch (e) {}
    }

    if (this.activeDevice) {
      try {
        await this.activeDevice.cancelConnection();
      } catch (e) {}
    }

    this.cleanup();
  }

  private cleanup(): void {
    if (this.notificationSubscription) {
      try {
        this.notificationSubscription.remove();
      } catch (e) {}
      this.notificationSubscription = null;
    }
    this.activeDevice = null;
    this.webDevice = null;
    this.webRxChar = null;
    this.webTxChar = null;
    this.packetBuffer = '';
  }
}

export const bleService = new BleService();
