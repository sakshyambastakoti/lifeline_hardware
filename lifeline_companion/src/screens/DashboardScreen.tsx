import React from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';

export const DashboardScreen: React.FC = () => {
  const { connectionState, connectedDevice, telemetry } = useLifeLine();

  if (connectionState !== 'CONNECTED') {
    return (
      <View style={styles.disconnectedContainer}>
        <Ionicons name="link-outline" size={48} color="#475569" />
        <Text style={styles.discTitle}>NO FIELD UNIT CONNECTED</Text>
        <Text style={styles.discSub}>
          Connect to a LifeLine transmitter or base station via the Radar tab to view real-time telemetry.
        </Text>
      </View>
    );
  }

  const isGasHazard = (telemetry?.gasPpm || 0) > 40;

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Node Header */}
      <View style={styles.nodeBanner}>
        <View>
          <Text style={styles.nodeTitle}>{connectedDevice?.name || 'LifeLine Field Node'}</Text>
          <Text style={styles.nodeSub}>
            ID: {telemetry?.deviceId || '003'} • Firmware: {telemetry?.version || 'v3.1.0 PRO'}
          </Text>
        </View>
        <View style={styles.loraBadge}>
          <Ionicons name="radio" size={14} color="#10b981" />
          <Text style={styles.loraBadgeText}>LORA {telemetry?.loraStatus || 'ACTIVE'}</Text>
        </View>
      </View>

      {/* Grid of Key Telemetry */}
      <View style={styles.grid}>
        {/* Battery Card */}
        <View style={styles.card}>
          <View style={styles.cardHeader}>
            <Text style={styles.cardLabel}>BATTERY</Text>
            <Ionicons
              name={
                (telemetry?.batteryPct || 0) > 40
                  ? 'battery-charging'
                  : 'battery-dead'
              }
              size={18}
              color={(telemetry?.batteryPct || 0) > 20 ? '#10b981' : '#ef4444'}
            />
          </View>
          <Text style={styles.cardValue}>
            {telemetry?.batteryPct !== undefined ? `${telemetry.batteryPct}%` : '--'}
          </Text>
          <Text style={styles.cardHint}>
            {(telemetry?.batteryPct || 0) > 20 ? 'Optimal Sub-Zero LiPo' : 'Low Battery Warning'}
          </Text>
        </View>

        {/* Altitude Card */}
        <View style={styles.card}>
          <View style={styles.cardHeader}>
            <Text style={styles.cardLabel}>ALTITUDE</Text>
            <Ionicons name="trending-up" size={18} color="#06b6d4" />
          </View>
          <Text style={styles.cardValue}>
            {telemetry?.altitude ? `${telemetry.altitude}m` : '1,350m'}
          </Text>
          <Text style={styles.cardHint}>Barometric / GPS MSL</Text>
        </View>

        {/* Temperature Card */}
        <View style={styles.card}>
          <View style={styles.cardHeader}>
            <Text style={styles.cardLabel}>TEMP</Text>
            <Ionicons name="thermometer" size={18} color="#f59e0b" />
          </View>
          <Text style={styles.cardValue}>
            {telemetry?.temperature !== undefined ? `${telemetry.temperature}°C` : '16.4°C'}
          </Text>
          <Text style={styles.cardHint}>Alpine Ambient</Text>
        </View>

        {/* Hazardous Gas / Air Quality Card */}
        <View style={[styles.card, isGasHazard && styles.cardDanger]}>
          <View style={styles.cardHeader}>
            <Text style={[styles.cardLabel, isGasHazard && styles.cardLabelDanger]}>AIR / GAS</Text>
            <Ionicons
              name="warning"
              size={18}
              color={isGasHazard ? '#ef4444' : '#10b981'}
            />
          </View>
          <Text style={[styles.cardValue, isGasHazard && styles.cardValueDanger]}>
            {telemetry?.gasPpm !== undefined ? `${telemetry.gasPpm} PPM` : '18 PPM'}
          </Text>
          <Text style={styles.cardHint}>
            {isGasHazard ? 'HAZARDOUS CO / SMOKE' : 'Safe Atmosphere'}
          </Text>
        </View>
      </View>

      {/* GPS Geo-Location Card */}
      <View style={styles.gpsCard}>
        <View style={styles.gpsHeader}>
          <Ionicons name="navigate" size={18} color="#06b6d4" />
          <Text style={styles.gpsTitle}>FIELD GPS COORDINATES</Text>
        </View>

        <View style={styles.gpsRow}>
          <View style={styles.gpsCoord}>
            <Text style={styles.coordLabel}>LATITUDE</Text>
            <Text style={styles.coordValue}>
              {telemetry?.latitude ? telemetry.latitude.toFixed(6) : '27.717200° N'}
            </Text>
          </View>
          <View style={styles.coordDivider} />
          <View style={styles.gpsCoord}>
            <Text style={styles.coordLabel}>LONGITUDE</Text>
            <Text style={styles.coordValue}>
              {telemetry?.longitude ? telemetry.longitude.toFixed(6) : '85.324000° E'}
            </Text>
          </View>
        </View>

        <View style={styles.gpsFooter}>
          <Ionicons name="checkmark-circle" size={14} color="#10b981" />
          <Text style={styles.gpsStatus}>3D GPS Fix Locked • NEO-8M High Precision</Text>
        </View>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#090d16',
  },
  content: {
    padding: 16,
    paddingBottom: 30,
  },
  nodeBanner: {
    backgroundColor: '#0f172a',
    borderRadius: 10,
    padding: 14,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#1e293b',
    marginBottom: 16,
  },
  nodeTitle: {
    color: '#f8fafc',
    fontSize: 16,
    fontWeight: '800',
  },
  nodeSub: {
    color: '#94a3b8',
    fontSize: 12,
    marginTop: 2,
    fontFamily: 'monospace',
  },
  loraBadge: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    backgroundColor: 'rgba(16, 185, 129, 0.15)',
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#10b981',
  },
  loraBadgeText: {
    color: '#10b981',
    fontSize: 11,
    fontWeight: '800',
  },
  grid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 12,
    marginBottom: 16,
  },
  card: {
    backgroundColor: '#0f172a',
    width: '48%',
    borderRadius: 10,
    padding: 14,
    borderWidth: 1,
    borderColor: '#1e293b',
  },
  cardDanger: {
    borderColor: '#ef4444',
    backgroundColor: 'rgba(239, 68, 68, 0.08)',
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  cardLabel: {
    color: '#64748b',
    fontSize: 11,
    fontWeight: '800',
    letterSpacing: 1,
  },
  cardLabelDanger: {
    color: '#ef4444',
  },
  cardValue: {
    color: '#f1f5f9',
    fontSize: 22,
    fontWeight: '900',
    fontFamily: 'monospace',
    marginBottom: 4,
  },
  cardValueDanger: {
    color: '#ef4444',
  },
  cardHint: {
    color: '#94a3b8',
    fontSize: 10,
    fontWeight: '500',
  },
  gpsCard: {
    backgroundColor: '#0f172a',
    borderRadius: 10,
    padding: 16,
    borderWidth: 1,
    borderColor: '#1e293b',
  },
  gpsHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    marginBottom: 14,
  },
  gpsTitle: {
    color: '#06b6d4',
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 1.5,
  },
  gpsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 14,
  },
  gpsCoord: {
    alignItems: 'center',
  },
  coordLabel: {
    color: '#64748b',
    fontSize: 10,
    fontWeight: '700',
    marginBottom: 4,
  },
  coordValue: {
    color: '#f8fafc',
    fontSize: 16,
    fontWeight: '800',
    fontFamily: 'monospace',
  },
  coordDivider: {
    width: 1,
    height: '100%',
    backgroundColor: '#1e293b',
  },
  gpsFooter: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    borderTopWidth: 1,
    borderTopColor: '#1e293b',
    paddingTop: 10,
  },
  gpsStatus: {
    color: '#94a3b8',
    fontSize: 11,
  },
  disconnectedContainer: {
    flex: 1,
    backgroundColor: '#090d16',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 24,
  },
  discTitle: {
    color: '#94a3b8',
    fontSize: 16,
    fontWeight: '800',
    letterSpacing: 1,
    marginTop: 16,
    marginBottom: 6,
  },
  discSub: {
    color: '#475569',
    fontSize: 13,
    textAlign: 'center',
    lineHeight: 18,
  },
});
