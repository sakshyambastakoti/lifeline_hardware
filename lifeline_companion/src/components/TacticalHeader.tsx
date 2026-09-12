import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';

export const TacticalHeader: React.FC = () => {
  const { connectionState, connectedDevice, telemetry, isSimulator, setIsSimulator, disconnectDevice } = useLifeLine();

  return (
    <View style={styles.headerContainer}>
      <View style={styles.topRow}>
        <View style={styles.brandGroup}>
          <Ionicons name="radio" size={22} color="#ef4444" style={styles.radioIcon} />
          <Text style={styles.brandTitle}>LIFELINE</Text>
          <Text style={styles.brandSub}>COMPANION</Text>
        </View>

        <TouchableOpacity
          style={[styles.simBadge, isSimulator ? styles.simActive : styles.simInactive]}
          onPress={() => setIsSimulator(!isSimulator)}
          activeOpacity={0.7}
        >
          <Text style={[styles.simText, isSimulator ? styles.simTextActive : styles.simTextInactive]}>
            {isSimulator ? 'SIMULATOR ON' : 'BLE HARDWARE'}
          </Text>
        </TouchableOpacity>
      </View>

      <View style={styles.statusRow}>
        <View style={styles.statusGroup}>
          <View
            style={[
              styles.statusDot,
              connectionState === 'CONNECTED'
                ? styles.dotConnected
                : connectionState === 'CONNECTING'
                ? styles.dotConnecting
                : styles.dotDisconnected,
            ]}
          />
          <Text style={styles.statusText}>
            {connectionState === 'CONNECTED'
              ? connectedDevice?.name || 'LINK ACTIVE'
              : connectionState === 'CONNECTING'
              ? 'LINKING...'
              : connectionState === 'SCANNING'
              ? 'SCANNING...'
              : 'STANDBY'}
          </Text>
        </View>

        {connectionState === 'CONNECTED' && (
          <View style={styles.telemetryMini}>
            {telemetry?.batteryPct !== undefined && (
              <View style={styles.miniItem}>
                <Ionicons
                  name={
                    telemetry.batteryPct > 50
                      ? 'battery-charging'
                      : telemetry.batteryPct > 20
                      ? 'battery-half'
                      : 'battery-dead'
                  }
                  size={16}
                  color={telemetry.batteryPct > 20 ? '#10b981' : '#ef4444'}
                />
                <Text style={styles.miniVal}>{telemetry.batteryPct}%</Text>
              </View>
            )}

            <TouchableOpacity onPress={disconnectDevice} style={styles.disconnectBtn}>
              <Ionicons name="power" size={14} color="#f87171" />
            </TouchableOpacity>
          </View>
        )}
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  headerContainer: {
    backgroundColor: '#0c1322',
    paddingTop: 48,
    paddingBottom: 14,
    paddingHorizontal: 16,
    borderBottomWidth: 1,
    borderBottomColor: '#1e293b',
  },
  topRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  brandGroup: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
  },
  radioIcon: {
    marginRight: 2,
  },
  brandTitle: {
    color: '#f8fafc',
    fontSize: 18,
    fontWeight: '900',
    letterSpacing: 2,
  },
  brandSub: {
    color: '#06b6d4',
    fontSize: 12,
    fontWeight: '700',
    letterSpacing: 1.5,
    backgroundColor: 'rgba(6, 182, 212, 0.15)',
    paddingHorizontal: 6,
    paddingVertical: 2,
    borderRadius: 4,
  },
  simBadge: {
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 6,
    borderWidth: 1,
  },
  simActive: {
    backgroundColor: 'rgba(245, 158, 11, 0.15)',
    borderColor: '#f59e0b',
  },
  simInactive: {
    backgroundColor: 'rgba(16, 185, 129, 0.15)',
    borderColor: '#10b981',
  },
  simText: {
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 0.5,
  },
  simTextActive: {
    color: '#f59e0b',
  },
  simTextInactive: {
    color: '#10b981',
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#070b13',
    paddingVertical: 6,
    paddingHorizontal: 10,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#1e293b',
  },
  statusGroup: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
  },
  dotConnected: {
    backgroundColor: '#10b981',
  },
  dotConnecting: {
    backgroundColor: '#f59e0b',
  },
  dotDisconnected: {
    backgroundColor: '#64748b',
  },
  statusText: {
    color: '#cbd5e1',
    fontSize: 12,
    fontWeight: '700',
    fontFamily: 'monospace',
  },
  telemetryMini: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
  },
  miniItem: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
  },
  miniVal: {
    color: '#e2e8f0',
    fontSize: 12,
    fontWeight: '700',
    fontFamily: 'monospace',
  },
  disconnectBtn: {
    padding: 4,
    backgroundColor: 'rgba(239, 68, 68, 0.1)',
    borderRadius: 4,
  },
});
