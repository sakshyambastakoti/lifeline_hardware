import { LifeLineDevice } from '../constants/ble';

export class MockBleService {
  private isScanning: boolean = false;
  private isConnected: boolean = false;
  private telemetryInterval: any = null;
  private onPacketCallback: ((packet: string) => void) | null = null;
  private onDisconnectCallback: (() => void) | null = null;
  private batteryLevel: number = 94;

  public startScan(
    onDeviceFound: (device: LifeLineDevice) => void,
    _onError: (error: any) => void
  ): void {
    this.isScanning = true;
    setTimeout(() => {
      if (this.isScanning) {
        onDeviceFound({
          id: 'SIM-TX-003',
          name: 'LifeLine-TX-003 (Simulated Field Node)',
          rssi: -62,
          isSimulated: true,
        });
      }
    }, 800);

    setTimeout(() => {
      if (this.isScanning) {
        onDeviceFound({
          id: 'SIM-RX-BASE',
          name: 'LifeLine-RX-Base (Simulated HQ)',
          rssi: -74,
          isSimulated: true,
        });
      }
    }, 1500);
  }

  public stopScan(): void {
    this.isScanning = false;
  }

  public async connect(
    deviceId: string,
    onPacketReceived: (packet: string) => void,
    onDisconnected: () => void
  ): Promise<LifeLineDevice> {
    this.stopScan();
    this.isConnected = true;
    this.onPacketCallback = onPacketReceived;
    this.onDisconnectCallback = onDisconnected;

    const deviceName = deviceId.includes('RX')
      ? 'LifeLine-RX-Base'
      : 'LifeLine-TX-003';

    // Push initial status after 500ms
    setTimeout(() => {
      if (this.isConnected && this.onPacketCallback) {
        this.onPacketCallback(`STATUS:DEV=003,BAT=${this.batteryLevel},LORA=OK,VER=v3.1.0 PRO`);
      }
    }, 500);

    // Push periodic telemetry every 4 seconds
    this.telemetryInterval = setInterval(() => {
      if (this.isConnected && this.onPacketCallback) {
        const lat = (27.7172 + (Math.random() - 0.5) * 0.001).toFixed(6);
        const lon = (85.3240 + (Math.random() - 0.5) * 0.001).toFixed(6);
        const temp = (16.4 + (Math.random() - 0.5) * 0.8).toFixed(1);
        const hum = (68.0 + (Math.random() - 0.5) * 2.0).toFixed(1);
        const alt = Math.floor(1350 + (Math.random() - 0.5) * 5);
        this.batteryLevel = Math.max(15, this.batteryLevel - (Math.random() > 0.8 ? 1 : 0));

        this.onPacketCallback(
          `TELEMETRY:DEV=003,TEMP=${temp},HUM=${hum},LAT=${lat},LON=${lon},ALT=${alt},BAT=${this.batteryLevel},GAS=18,RSSI=-66`
        );
      }
    }, 4000);

    return {
      id: deviceId,
      name: deviceName,
      rssi: -59,
      isSimulated: true,
    };
  }

  public async sendCommand(command: string): Promise<boolean> {
    if (!this.isConnected || !this.onPacketCallback) {
      throw new Error('Device not connected in simulator.');
    }

    const trimmed = command.trim();

    // 1. SOS Alert Trigger
    if (trimmed.startsWith('ALERT:')) {
      const code = trimmed.split(':')[1] || 'A';
      setTimeout(() => {
        this.onPacketCallback?.(`ALERT_SENT:CODE=${code},NAME=EMERGENCY,TIME=${Date.now()}`);
      }, 400);

      // Simulate closed-loop Base Station ACK via LoRa mesh after 2.5 seconds!
      setTimeout(() => {
        this.onPacketCallback?.(
          `ACK_RECV:STATUS=DISPATCHED,BASE=BASE01,NOTE=Rescue team deployed ETA 25min,RSSI=-64,SNR=10`
        );
      }, 2500);
    }
    // 2. Tactical Text Chat
    else if (trimmed.startsWith('MSG:')) {
      const text = trimmed.substring(4);
      setTimeout(() => {
        this.onPacketCallback?.(`ACK_RECV:STATUS=RELAYED,NOTE=LoRa packet broadcast,RSSI=-65,SNR=9`);
      }, 600);

      // Base Station replies to victim after 3.5s
      setTimeout(() => {
        this.onPacketCallback?.(
          `CHAT:DEV=BASE01,TEXT=Message received: "${text.substring(0, 20)}...". Stay in place, rescue team moving in.,RSSI=-67`
        );
      }, 3500);
    }
    // 3. Status query
    else if (trimmed === 'STATUS') {
      setTimeout(() => {
        this.onPacketCallback?.(`STATUS:DEV=003,BAT=${this.batteryLevel},LORA=OK,VER=v3.1.0 PRO`);
      }, 300);
    }

    return true;
  }

  public async disconnect(): Promise<void> {
    if (this.telemetryInterval) {
      clearInterval(this.telemetryInterval);
      this.telemetryInterval = null;
    }
    this.isConnected = false;
    this.onPacketCallback = null;
    if (this.onDisconnectCallback) {
      this.onDisconnectCallback();
      this.onDisconnectCallback = null;
    }
  }
}

export const mockBleService = new MockBleService();
