import React from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  FlatList,
  ActivityIndicator,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';
import { LifeLineDevice } from '../constants/ble';

export const RadarScreen: React.FC = () => {
  const {
    connectionState,
    availableDevices,
    connectedDevice,
    statusMessage,
    startScan,
    stopScan,
    connectDevice,
    disconnectDevice,
    isSimulator,
  } = useLifeLine();

  const renderDevice = ({ item }: { item: LifeLineDevice }) => {
    const isConnected = connectedDevice?.id === item.id;
    const isBase = item.name.includes('RX') || item.name.includes('Base');

    return (
      <View style={styles.deviceCard}>
        <View style={styles.deviceIconBox}>
          <Ionicons
            name={isBase ? 'business' : 'walk'}
            size={22}
            color={isBase ? '#06b6d4' : '#10b981'}
          />
        </View>

        <View style={styles.deviceInfo}>
          <Text style={styles.deviceName}>{item.name}</Text>
          <View style={styles.deviceSubRow}>
            <Text style={styles.deviceId}>{item.id}</Text>
            {item.rssi !== null && (
              <View style={styles.rssiBadge}>
                <Ionicons name="cellular" size={12} color="#94a3b8" />
                <Text style={styles.rssiText}>{item.rssi} dBm</Text>
              </View>
            )}
          </View>
        </View>

        <TouchableOpacity
          style={[styles.connectBtn, isConnected ? styles.connectedBtn : styles.actionBtn]}
          onPress={() => (isConnected ? disconnectDevice() : connectDevice(item))}
          disabled={connectionState === 'CONNECTING'}
        >
          <Text style={styles.connectBtnText}>
            {isConnected ? 'DISCONNECT' : 'CONNECT'}
          </Text>
        </TouchableOpacity>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      {/* Radar Status Banner */}
      <View style={styles.radarCard}>
        <View style={styles.radarVisual}>
          <Ionicons
            name="radio-outline"
            size={48}
            color={connectionState === 'SCANNING' ? '#06b6d4' : '#64748b'}
          />
          {connectionState === 'SCANNING' && (
            <ActivityIndicator
              size="small"
              color="#06b6d4"
              style={styles.radarSpinner}
            />
          )}
        </View>

        <Text style={styles.radarTitle}>
          {connectionState === 'SCANNING'
            ? 'SCANNING 2.4 GHz RF...'
            : connectionState === 'CONNECTED'
            ? 'DEVICE PAIRED'
            : 'BLE RADAR STANDBY'}
        </Text>

        <Text style={styles.statusSub}>{statusMessage || 'Ready to discover field devices'}</Text>

        {isSimulator && (
          <View style={styles.simNotice}>
            <Ionicons name="information-circle" size={14} color="#f59e0b" />
            <Text style={styles.simNoticeText}>
              Simulator Active: Generates realistic LifeLine nodes for testing.
            </Text>
          </View>
        )}

        <View style={styles.buttonRow}>
          {connectionState === 'SCANNING' ? (
            <TouchableOpacity style={styles.stopBtn} onPress={stopScan}>
              <Ionicons name="stop-circle" size={18} color="#ef4444" />
              <Text style={styles.stopBtnText}>STOP SCAN</Text>
            </TouchableOpacity>
          ) : (
            <TouchableOpacity
              style={styles.scanBtn}
              onPress={startScan}
              disabled={connectionState === 'CONNECTING'}
            >
              <Ionicons name="search" size={18} color="#0f172a" />
              <Text style={styles.scanBtnText}>SCAN FOR LIFELINE</Text>
            </TouchableOpacity>
          )}
        </View>
      </View>

      {/* Discovered Devices List */}
      <Text style={styles.sectionHeader}>
        NEARBY FIELD UNITS ({availableDevices.length})
      </Text>

      {availableDevices.length === 0 ? (
        <View style={styles.emptyState}>
          <Ionicons name="bluetooth" size={36} color="#334155" />
          <Text style={styles.emptyText}>No LifeLine units detected yet.</Text>
          <Text style={styles.emptySubText}>
            Tap "SCAN FOR LIFELINE" to search for TX Pro or RX Base nodes.
          </Text>
        </View>
      ) : (
        <FlatList
          data={availableDevices}
          renderItem={renderDevice}
          keyExtractor={item => item.id}
          contentContainerStyle={styles.listContainer}
        />
      )}
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#090d16',
    padding: 16,
  },
  radarCard: {
    backgroundColor: '#0f172a',
    borderRadius: 12,
    padding: 20,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#1e293b',
    marginBottom: 20,
  },
  radarVisual: {
    width: 72,
    height: 72,
    borderRadius: 36,
    backgroundColor: 'rgba(6, 182, 212, 0.08)',
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: 12,
    position: 'relative',
  },
  radarSpinner: {
    position: 'absolute',
  },
  radarTitle: {
    color: '#f8fafc',
    fontSize: 16,
    fontWeight: '800',
    letterSpacing: 1.5,
    marginBottom: 4,
  },
  statusSub: {
    color: '#94a3b8',
    fontSize: 12,
    textAlign: 'center',
    marginBottom: 14,
  },
  simNotice: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    backgroundColor: 'rgba(245, 158, 11, 0.1)',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: 'rgba(245, 158, 11, 0.3)',
    marginBottom: 14,
  },
  simNoticeText: {
    color: '#f59e0b',
    fontSize: 11,
    fontWeight: '600',
  },
  buttonRow: {
    width: '100%',
  },
  scanBtn: {
    backgroundColor: '#06b6d4',
    paddingVertical: 12,
    borderRadius: 8,
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    gap: 8,
  },
  scanBtnText: {
    color: '#090d16',
    fontWeight: '900',
    fontSize: 13,
    letterSpacing: 1,
  },
  stopBtn: {
    backgroundColor: 'rgba(239, 68, 68, 0.15)',
    borderWidth: 1,
    borderColor: '#ef4444',
    paddingVertical: 12,
    borderRadius: 8,
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    gap: 8,
  },
  stopBtnText: {
    color: '#ef4444',
    fontWeight: '800',
    fontSize: 13,
    letterSpacing: 1,
  },
  sectionHeader: {
    color: '#64748b',
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 1.5,
    marginBottom: 12,
  },
  listContainer: {
    paddingBottom: 24,
  },
  deviceCard: {
    backgroundColor: '#0f172a',
    borderRadius: 10,
    padding: 14,
    flexDirection: 'row',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#1e293b',
    marginBottom: 10,
  },
  deviceIconBox: {
    width: 42,
    height: 42,
    borderRadius: 8,
    backgroundColor: '#1e293b',
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 12,
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
    color: '#f1f5f9',
    fontSize: 14,
    fontWeight: '700',
    marginBottom: 3,
  },
  deviceSubRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 10,
  },
  deviceId: {
    color: '#64748b',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  rssiBadge: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 3,
  },
  rssiText: {
    color: '#94a3b8',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  connectBtn: {
    paddingHorizontal: 14,
    paddingVertical: 8,
    borderRadius: 6,
  },
  actionBtn: {
    backgroundColor: '#3b82f6',
  },
  connectedBtn: {
    backgroundColor: '#dc2626',
  },
  connectBtnText: {
    color: '#ffffff',
    fontSize: 11,
    fontWeight: '800',
    letterSpacing: 0.5,
  },
  emptyState: {
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 40,
  },
  emptyText: {
    color: '#94a3b8',
    fontSize: 14,
    fontWeight: '700',
    marginTop: 12,
  },
  emptySubText: {
    color: '#475569',
    fontSize: 12,
    textAlign: 'center',
    marginTop: 4,
    paddingHorizontal: 20,
  },
});
