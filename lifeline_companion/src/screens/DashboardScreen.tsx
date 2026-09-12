import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';
import { THEME } from '../constants/theme';

export const DashboardScreen: React.FC = () => {
  const { connectionState, connectedDevice, telemetry } = useLifeLine();

  if (connectionState !== 'CONNECTED') {
    return (
      <View style={styles.disconnectedContainer}>
        <Ionicons name="link-outline" size={40} color={THEME.colors.text3} />
        <Text style={styles.discTitle}>RF TELEMETRY DISCONNECTED</Text>
        <Text style={styles.discSub}>
          Establish a Bluetooth Low Energy link with a field node via the Radar console to stream real-time environmental metrics.
        </Text>
      </View>
    );
  }

  const isGasHazard = (telemetry?.gasPpm || 0) > 40;

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Node Info Banner */}
      <View style={styles.nodeBanner}>
        <View>
          <Text style={styles.nodeTitle}>{connectedDevice?.name?.toUpperCase() || 'LIFELINE FIELD NODE'}</Text>
          <Text style={styles.nodeSub}>
            DEVICE // {telemetry?.deviceId || '003'} • FW // {telemetry?.version || 'v3.1.0 PRO'}
          </Text>
        </View>
        <View style={styles.loraPill}>
          <View style={styles.loraDot} />
          <Text style={styles.loraPillText}>LORA {telemetry?.loraStatus || 'ACTIVE'}</Text>
        </View>
      </View>

      {/* Grid of Telemetry Cards */}
      <View style={styles.grid}>
        {/* Battery Telemetry */}
        <View style={styles.card}>
          <View style={styles.cardHeader}>
            <Text style={styles.cardLabel}>POWER RESERVES</Text>
            <Ionicons
              name={
                (telemetry?.batteryPct || 0) > 40
                  ? 'battery-charging-outline'
                  : 'battery-dead-outline'
              }
              size={16}
              color={(telemetry?.batteryPct || 0) > 20 ? THEME.colors.success : THEME.colors.danger}
            />
          </View>
          <Text style={styles.cardValue}>
            {telemetry?.batteryPct !== undefined ? `${telemetry.batteryPct}%` : '--'}
          </Text>
          <Text style={styles.cardHint}>Sub-Zero Thermal Regulated LiPo</Text>
        </View>

        {/* Barometric Altitude */}
        <View style={styles.card}>
          <View style={styles.cardHeader}>
            <Text style={styles.cardLabel}>ELEVATION MSL</Text>
            <Ionicons name="trending-up-outline" size={16} color={THEME.colors.cyanStream} />
          </View>
          <Text style={styles.cardValue}>
            {telemetry?.altitude ? `${telemetry.altitude}M` : '1,350M'}
          </Text>
          <Text style={styles.cardHint}>Barometric & GNSS Fused</Text>
        </View>

        {/* Ambient Temperature */}
        <View style={styles.card}>
          <View style={styles.cardHeader}>
            <Text style={styles.cardLabel}>TEMPERATURE</Text>
            <Ionicons name="thermometer-outline" size={16} color={THEME.colors.solarAmber} />
          </View>
          <Text style={styles.cardValue}>
            {telemetry?.temperature !== undefined ? `${telemetry.temperature}°C` : '16.4°C'}
          </Text>
          <Text style={styles.cardHint}>Alpine Ambient Sensor</Text>
        </View>

        {/* Toxic Gas / Carbon Monoxide */}
        <View style={[styles.card, isGasHazard && styles.cardDanger]}>
          <View style={styles.cardHeader}>
            <Text style={[styles.cardLabel, isGasHazard && styles.cardLabelDanger]}>AIR INTEGRITY</Text>
            <Ionicons
              name="warning-outline"
              size={16}
              color={isGasHazard ? THEME.colors.danger : THEME.colors.success}
            />
          </View>
          <Text style={[styles.cardValue, isGasHazard && styles.cardValueDanger]}>
            {telemetry?.gasPpm !== undefined ? `${telemetry.gasPpm} PPM` : '18 PPM'}
          </Text>
          <Text style={[styles.cardHint, isGasHazard && styles.cardHintDanger]}>
            {isGasHazard ? 'HAZARDOUS ATMOSPHERE' : 'Nominal Safe Quality'}
          </Text>
        </View>
      </View>

      {/* GPS Geo-Positioning Card */}
      <View style={styles.gpsCard}>
        <View style={styles.gpsHeader}>
          <View style={styles.gpsTitleGroup}>
            <Ionicons name="navigate-outline" size={16} color={THEME.colors.text0} />
            <Text style={styles.gpsTitle}>SATELLITE POSITIONING [GNSS]</Text>
          </View>
          <View style={styles.gpsLockPill}>
            <Text style={styles.gpsLockText}>3D FIX ACTIVE</Text>
          </View>
        </View>

        <View style={styles.gpsRow}>
          <View style={styles.gpsCoordBox}>
            <Text style={styles.coordLabel}>LATITUDE</Text>
            <Text style={styles.coordValue}>
              {telemetry?.latitude ? telemetry.latitude.toFixed(6) : '27.717200'}° N
            </Text>
          </View>
          <View style={styles.coordDivider} />
          <View style={styles.gpsCoordBox}>
            <Text style={styles.coordLabel}>LONGITUDE</Text>
            <Text style={styles.coordValue}>
              {telemetry?.longitude ? telemetry.longitude.toFixed(6) : '085.324000'}° E
            </Text>
          </View>
        </View>

        <View style={styles.gpsFooter}>
          <Text style={styles.gpsMeta}>NEO-8M HIGH-PRECISION DUAL-CONSTELLATION RECEIVER</Text>
        </View>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: THEME.colors.bg0,
  },
  content: {
    padding: 16,
    paddingBottom: 30,
  },
  nodeBanner: {
    backgroundColor: THEME.colors.bg2,
    borderRadius: THEME.geometry.sharp,
    padding: 16,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: THEME.colors.border,
    marginBottom: 16,
  },
  nodeTitle: {
    color: THEME.colors.text0,
    fontSize: 14,
    fontWeight: '900',
    letterSpacing: 1,
  },
  nodeSub: {
    color: THEME.colors.text2,
    fontSize: 11,
    marginTop: 3,
    fontFamily: THEME.fonts.mono,
  },
  loraPill: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    backgroundColor: THEME.colors.successBg,
    paddingHorizontal: 10,
    paddingVertical: 4,
    borderRadius: THEME.geometry.pill,
    borderWidth: 1,
    borderColor: THEME.colors.success,
  },
  loraDot: {
    width: 6,
    height: 6,
    borderRadius: 3,
    backgroundColor: THEME.colors.success,
  },
  loraPillText: {
    color: THEME.colors.text0,
    fontSize: 10,
    fontWeight: '800',
    fontFamily: THEME.fonts.mono,
    letterSpacing: 1,
  },
  grid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 12,
    marginBottom: 16,
  },
  card: {
    backgroundColor: THEME.colors.bg2,
    width: '48%',
    borderRadius: THEME.geometry.sharp,
    padding: 16,
    borderWidth: 1,
    borderColor: THEME.colors.border,
  },
  cardDanger: {
    borderColor: THEME.colors.danger,
    backgroundColor: THEME.colors.dangerBg,
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  cardLabel: {
    color: THEME.colors.text2,
    fontSize: 10,
    fontWeight: '900',
    letterSpacing: 1,
  },
  cardLabelDanger: {
    color: THEME.colors.danger,
  },
  cardValue: {
    color: THEME.colors.text0,
    fontSize: 22,
    fontWeight: '900',
    fontFamily: THEME.fonts.mono,
    marginBottom: 4,
  },
  cardValueDanger: {
    color: THEME.colors.danger,
  },
  cardHint: {
    color: THEME.colors.text3,
    fontSize: 10,
    fontWeight: '500',
    lineHeight: 14,
  },
  cardHintDanger: {
    color: THEME.colors.danger,
  },
  gpsCard: {
    backgroundColor: THEME.colors.bg2,
    borderRadius: THEME.geometry.sharp,
    padding: 18,
    borderWidth: 1,
    borderColor: THEME.colors.border,
  },
  gpsHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 16,
  },
  gpsTitleGroup: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  gpsTitle: {
    color: THEME.colors.text0,
    fontSize: 12,
    fontWeight: '900',
    letterSpacing: 1.5,
  },
  gpsLockPill: {
    backgroundColor: THEME.colors.bg1,
    borderWidth: 1,
    borderColor: THEME.colors.borderStrong,
    paddingHorizontal: 8,
    paddingVertical: 3,
    borderRadius: THEME.geometry.pill,
  },
  gpsLockText: {
    color: THEME.colors.text2,
    fontSize: 9,
    fontWeight: '800',
    letterSpacing: 1,
    fontFamily: THEME.fonts.mono,
  },
  gpsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 16,
  },
  gpsCoordBox: {
    alignItems: 'center',
  },
  coordLabel: {
    color: THEME.colors.text3,
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 1,
    marginBottom: 4,
  },
  coordValue: {
    color: THEME.colors.text0,
    fontSize: 16,
    fontWeight: '900',
    fontFamily: THEME.fonts.mono,
  },
  coordDivider: {
    width: 1,
    height: '100%',
    backgroundColor: THEME.colors.border,
  },
  gpsFooter: {
    borderTopWidth: 1,
    borderTopColor: THEME.colors.border,
    paddingTop: 12,
    alignItems: 'center',
  },
  gpsMeta: {
    color: THEME.colors.text3,
    fontSize: 9,
    fontWeight: '700',
    letterSpacing: 1,
    fontFamily: THEME.fonts.mono,
  },
  disconnectedContainer: {
    flex: 1,
    backgroundColor: THEME.colors.bg0,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 28,
  },
  discTitle: {
    color: THEME.colors.text1,
    fontSize: 14,
    fontWeight: '900',
    letterSpacing: 2,
    marginTop: 16,
    marginBottom: 8,
  },
  discSub: {
    color: THEME.colors.text3,
    fontSize: 12,
    textAlign: 'center',
    lineHeight: 18,
  },
});
